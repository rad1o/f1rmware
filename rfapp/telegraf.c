/*
 * This file is part of rad1o
 *
 * It implements a basic embedded Morse transmitter able to
 * handle both TX and RX. Enjoy !
 *
 * This transmitter only works between 2590MHz - 2595MHz (based on max2837),
 * with 10 pre-defined channels.
 *
 * Authors:
 *  - Damien Cauquil <d.cauquil@sysdream.com>
 *  - Julien Boulet  <j.boulet@sysdream.com>
 */

#include <unistd.h>
#include <string.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/config.h>
#include <r0ketlib/print.h>
#include <r0ketlib/stringin.h>
#include <r0ketlib/night.h>
#include <r0ketlib/render.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/fonts/orbitron14.h>
#include <r0ketlib/fonts/smallfonts.h>

#include <rad1olib/pins.h>
#include <rad1olib/systick.h>
#include <rad1olib/battery.h>

#include <hackrf_core.h>
#include <max2837.h>
#include <tuning.h>

#include "si5351c.h"
#include "sgpio.h"
#include "rf_path.h"
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/dac.h>
#include <libopencm3/lpc43xx/adc.h>
#include <math.h>

/* Defines ... */
#define WAIT_CPU_CLOCK_INIT_DELAY   (10000)
#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */
#define MEASURES 20
#define PI 3.14159265358979323846
#define NB_SAMPLES 50000

/* Typedefs */

typedef enum {
  TELEGRAPH_RX_MODE,
  TELEGRAPH_TX_MODE
} telegraph_mode_t;

typedef struct {
	uint32_t bandwidth_hz;
} max2837_ft_t;

/* Globals */
int g_bpressed = BTN_NONE;
int g_channel = 0;
uint32_t g_freq = 2590000000U;
const uint64_t g_freq64 = (const uint64_t)2590000000U;
uint32_t baseband_filter_bw_hz = 0;
uint32_t sample_rate_hz;
telegraph_mode_t g_current_mode = TELEGRAPH_TX_MODE;
double g_volume = 0.5;

/* Required for baseband bandwidth cal. */
static const max2837_ft_t max2837_ft[] = {
	{ 1750000  },
	{ 2500000  },
	{ 3500000  },
	{ 5000000  },
	{ 5500000  },
	{ 6000000  },
	{ 7000000  },
	{ 8000000  },
	{ 9000000  },
	{ 10000000 },
	{ 12000000 },
	{ 14000000 },
	{ 15000000 },
	{ 20000000 },
	{ 24000000 },
	{ 28000000 },
	{ 0        }
};


/*
 * Return final bw round down and less than expected bw.
 *
 * (this one was originally implemented in the libhackrf source)
 */

uint32_t hackrf_compute_baseband_filter_bw_round_down_lt(const uint32_t bandwidth_hz)
{
	const max2837_ft_t* p = max2837_ft;
	while( p->bandwidth_hz != 0 )
	{
		if( p->bandwidth_hz >= bandwidth_hz )
		{
			break;
		}
		p++;
	}
	/* Round down (if no equal to first entry) */
	if(p != max2837_ft)
	{
		p--;
	}
	return p->bandwidth_hz;
}

/*
 * Well, math.h provides a pow() function, but it was funny to set our
 * own.
 */

int _pow(int b, int e) {
    int v = 1,i;
    if (e==0)
        return 1;
    for (i=0; i<e; i++)
        v *= b;
    return v;
}


/*
 * Display render.
 *
 * This routine render the frequency and the volume on the display.
 */

void render_display(void) {
  int dx, dy, dx2;
  char num_charset[] = "0123456789";
  char sz_channel[11] = "Channel: 0";
  char sz_vol[5];
  int digits = 0,i;
  int vol = g_volume*100;

  /* Set channel string. */
  sz_channel[9] = 0x30 + g_channel;

  /* Convert volume to string. */
  for (i=3; i>0; i--) {
      sz_vol[3-i] = num_charset[vol/_pow(10, i-1)];
      vol -= (vol/_pow(10, i-1))*(_pow(10,i-1));
  }
  sz_vol[3] = '%';
  sz_vol[4] = '\0';

  setTextColor(0xFF,0x00);
  lcdClear();
  setIntFont(&Font_Orbitron14pt);


  /* Draw name. */
  dx = DoString(0, 0, "Telegraph");
  dx = (RESX - dx)/2;
  dx2 = DoString(0, 0, sz_channel);
  dx2 = (RESX - dx2)/2;

  lcdFill(0xFF);
  DoString(dx, 10, "Telegraph");
  DoString(dx, 62, sz_channel);
  setIntFont(&Font_8x8);
  dx = DoString(12, 90, "Volume:");
  DoString(12+dx+4, 90, sz_vol);


  lcdDisplay();
}

/*
 * Telegraph init
 *
 * This routine was hard to define since there is no doc, but
 * obviously mixing both hackrf init stuff with rad1olib init stuff
 * seems to work pretty good.
 *
 * May have some drawbacks we did not identified yet =).
 */

void telegraph_init(void)
{
    /* Hack RF cpu_clock_init mix with rad10lib. */

    /* use IRC as clock source for APB1 (including I2C0) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_IRC);

	/* use IRC as clock source for APB3 */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_IRC);

	i2c0_init(15);

	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources();
	si5351c_configure_pll_multisynth();

	/*
	 * rad1o clocks:
	 *   CLK0 -> MAX5864/CPLD
	 *   CLK1 -> CPLD
	 *   CLK2 -> SGPIO
	 *   CLK3 -> External Clock Output
	 *   CLK4 -> MAX2837
	 *   CLK5 -> MAX2871
	 *   CLK6 -> none
	 *   CLK7 -> LPC4330 (but LPC4330 starts up on its own crystal)
	 */

	/* MS3/CLK3 is the source for the external clock output. */
	si5351c_configure_multisynth(3, 80*128-512, 0, 1, 0); /* 800/80 = 10MHz */

	/* MS4/CLK4 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(4, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS5/CLK5 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(5, 16*128-512, 0, 1, 0); /* 800/16 = 50MHz */

	/* MS6/CLK6 is unused. */

	/* MS7/CLK7 is the source for the LPC43xx microcontroller. */

	/* Set to 10 MHz, the common rate between Jellybean and Jawbreaker. */
	sample_rate_set(10000000);

	si5351c_set_clock_source(PLL_SOURCE_XTAL);
	// soft reset
	uint8_t resetdata[] = { 177, 0xac };
	si5351c_write(resetdata, sizeof(resetdata));
	si5351c_enable_clock_outputs();

	//FIXME disable I2C
	/* Kick I2C0 down to 400kHz when we switch over to APB1 clock = 204MHz */
	i2c0_init(255);

	/*
	 * 12MHz clock is entering LPC XTAL1/OSC input now.  On
	 * Jellybean/Lemondrop, this is a signal from the clock generator.  On
	 * Jawbreaker, there is a 12 MHz crystal at the LPC.
	 * Set up PLL1 to run from XTAL1 input.
	 */

	//FIXME a lot of the details here should be in a CGU driver

	/* set xtal oscillator to low frequency mode */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_HF_MASK;

	/* power on the oscillator and wait until stable */
	CGU_XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_ENABLE_MASK;

	/* Wait about 100us after Crystal Power ON */
	delay(WAIT_CPU_CLOCK_INIT_DELAY);

	/* use XTAL_OSC as clock source for BASE_M4_CLK (CPU) */
	CGU_BASE_M4_CLK = (CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_XTAL) | CGU_BASE_M4_CLK_AUTOBLOCK(1));

	/* use XTAL_OSC as clock source for APB1 */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_XTAL);

	/* use XTAL_OSC as clock source for APB3 */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_XTAL);

	cpu_clock_pll1_low_speed();

	/* use PLL1 as clock source for BASE_M4_CLK (CPU) */
	CGU_BASE_M4_CLK = (CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_PLL1) | CGU_BASE_M4_CLK_AUTOBLOCK(1));

	/* Switch peripheral clock over to use PLL1 (204MHz) */
	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK(1)
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB1 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB3 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* set DIV C to 40.8 MHz */
	CGU_IDIVC_CTRL= CGU_IDIVC_CTRL_CLK_SEL(CGU_SRC_PLL1)
		| CGU_IDIVC_CTRL_AUTOBLOCK(1)
		| CGU_IDIVC_CTRL_IDIV(5-1)
		| CGU_IDIVC_CTRL_PD(0)
		;

	/* use DIV C as SSP1 base clock */
	CGU_BASE_SSP1_CLK = (CGU_BASE_SSP1_CLK_CLK_SEL(CGU_SRC_IDIVC) | CGU_BASE_SSP1_CLK_AUTOBLOCK(1));

    /* Set DIV B to 102MHz */
	CGU_IDIVB_CTRL= CGU_IDIVB_CTRL_CLK_SEL(CGU_SRC_PLL1)
		| CGU_IDIVB_CTRL_AUTOBLOCK(1)
		| CGU_IDIVB_CTRL_IDIV(2-1)
		| CGU_IDIVB_CTRL_PD(0)
		;
}

void change_freq(uint32_t freq) {
  uint64_t freq64 = (uint64_t)freq;

  /* Set sample rate and frac.
   * Found in hackrf_transfer.c
   */
   sample_rate_frac_set(2*freq, 1);

   /* Compute default value depending on sample rate */
   baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(sample_rate_hz);

   /* Set carrier frequency. */
   //set_freq(freq64);
   /*
   baseband_streaming_disable();
   rf_path_set_direction(RF_PATH_DIRECTION_TX);
   si5351c_activate_best_clock_source();
   baseband_streaming_enable();
   */
   max2837_set_frequency(g_freq);
   max2837_start();
   max2837_tx();
}

/*
 * Initalize the transceiver for reception.
 *
 * This setup was undocumented, took us HOURS to figure it out.
 */

void telegraph_init_rx(void)
{
  ssp1_init();

  /* Set sample rate and frac.
   * Found in hackrf_transfer.c
   */
   sample_rate_frac_set(2*g_freq, 1);

   /* Compute default value depending on sample rate */
   baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(sample_rate_hz);

   /* Set carrier frequency. */
   set_freq(g_freq);
   //max2837_set_frequency(g_freq);
  /* Found in hackrf_usb: set_transceiver_mode.
   * Called by hackrf_start_tx().
   */
  /*
  baseband_streaming_disable();
  */
  rf_path_set_direction(RF_PATH_DIRECTION_RX);
  max2837_stop();
  si5351c_activate_best_clock_source();
  /*
  baseband_streaming_enable();
  */
  // Enable amplification (TX)
  rf_path_set_lna(1);
  // Enable antenna
  rf_path_set_antenna(1);

  /* Enable streaming. */
  sgpio_cpld_stream_enable();
  sgpio_set_slice_mode(false);
}

/*
 * Same for TX, took us HOURS again (and a lot of Club Mate).
 */

void telegraph_init_tx(void)
{
  ssp1_init();

  /* Set sample rate and frac.
   * Found in hackrf_transfer.c
   */
   sample_rate_frac_set(2*g_freq, 1);

   /* Compute default value depending on sample rate */
   baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(sample_rate_hz);

   /* Set carrier frequency. */
   set_freq(g_freq);
   //max2837_set_frequency(g_freq);

  /* Found in hackrf_usb: set_transceiver_mode.
   * Called by hackrf_start_tx().
   */

  rf_path_set_direction(RF_PATH_DIRECTION_TX);
  max2837_stop();
  si5351c_activate_best_clock_source();

  // Enable amplification (TX)
  rf_path_set_lna(1);
  // Enable antenna
  rf_path_set_antenna(1);

  /* Disable streaming. */
  sgpio_cpld_stream_disable();
  sgpio_set_slice_mode(false);

}

/*
 * Main UI & TX/RX routine.
 */

void main_ui(void) {
    char sz_freq[11];

    volatile uint32_t buffer[4096];
    double magsq[10];
    int8_t sigi[10], sigq[10];
    int moy,i;


    /* HackRF setup as found in hackrf_core.c */
    pin_setup();
    enable_1v8_power();
    enable_rf_power();
    delay(1000000);
    cpu_clock_init();

    rf_path_init();

    /* LED setup (debug :) */
    SETUPgout(LED4);
    OFF(LED4);

    /* Required by the LCD. */
    cpu_clock_set(204);

    /* Mic amplifier enabled. */
    SETUPgout(MIC_AMP_DIS);
    OFF(MIC_AMP_DIS);

    /* DAC enabled (in order to make some sound). */
    dac_init(false);

    /* Found in hackrf_usb: main. */
    ssp1_init();

    lcdInit();
    ssp_clock_init();
    render_display();

    max2837_stop();
    telegraph_init_rx();
    max2837_start();
    max2837_set_vga_gain(3);
    max2837_rx();

    while(true) {

      if ((g_bpressed != BTN_NONE) && (getInputRaw()==BTN_NONE)) {
        /* Quick'n'dirty debounce. */
        delay(30000);
        if ((g_bpressed != BTN_NONE) && (getInputRaw()==BTN_NONE)) {
          /* Handle channel switch. */
          switch(g_bpressed) {
            case BTN_RIGHT:
              {
                max2837_stop();

                if (g_channel < 9)
                    g_channel++;
                else
                    g_channel = 9;

                /* Compute carrier frequency. */
                g_freq = 2590000000U + (500000 * g_channel);

                /* Select the lcd display. */
                ssp_clock_init();
                render_display();

                /* Select the max2837. */
                if (g_current_mode == TELEGRAPH_RX_MODE) {
                  telegraph_init_rx();
                } else {
                  telegraph_init_tx();
                }
                ssp1_init();
                ssp1_set_mode_max2837();
                max2837_set_frequency(g_freq);

                if (g_current_mode == TELEGRAPH_RX_MODE) {
                  max2837_start();
                  max2837_rx();
                } else {
                  max2837_start();
                  max2837_tx();
                }
              }
              break;

            case BTN_LEFT:
              {
                max2837_stop();

                if (g_channel > 0)
                    g_channel--;
                else
                  g_channel = 0;

                /* Compute carrier frequency. */
                g_freq = 2590000000U + (500000 * g_channel);

                /* Update the lcd display. */
                ssp_clock_init();
                render_display();

                if (g_current_mode == TELEGRAPH_RX_MODE) {
                  telegraph_init_rx();
                } else {
                  telegraph_init_tx();
                }
                ssp1_init();
                ssp1_set_mode_max2837();
                max2837_set_frequency(g_freq);

                if (g_current_mode == TELEGRAPH_RX_MODE) {
                  max2837_start();
                  max2837_rx();
                } else {
                  max2837_start();
                  max2837_tx();
                }
              }
              break;
          }
          g_bpressed = BTN_NONE;
        }
      } else {
        g_bpressed = getInputRaw();
      }

      /*
       * Volume and beep button are handled the classic way,
       * because they may be long-pressed.
       */

      switch(getInputRaw()) {
        case BTN_DOWN:
          {
            if (g_volume > 0.11)
              g_volume -= 0.01;
            else
              g_volume = 0.1;

            /* Update the lcd display. */
            ssp_clock_init();
            render_display();
          }
          break;

        case BTN_UP:
          {
            if (g_volume < 0.99)
              g_volume += 0.01;
            else
              g_volume = 1.0;

            /* Update the lcd display. */
            ssp_clock_init();
            render_display();
          }
          break;

        case BTN_ENTER:
          {
            if (g_current_mode == TELEGRAPH_RX_MODE) {
              /* We were in RX mode, switch to TX mode. */
              max2837_stop();
              telegraph_init_tx();
              g_current_mode = TELEGRAPH_TX_MODE;
            }

            /* Send the tone. */
            max2837_start();
            max2837_tx();
            dac_set(8.0*g_volume);
            delay(5000);
            dac_set(0.0*g_volume);
            max2837_stop();
            delay(5000);
          }
          break;

        default:
          {
            if (g_current_mode == TELEGRAPH_TX_MODE) {
              /* We were in TX mode, switch to RX mode. */
              max2837_stop();
              telegraph_init_rx();
              max2837_start();
              max2837_set_vga_gain(3);
              max2837_rx();
              g_current_mode = TELEGRAPH_RX_MODE;
            }

            /* Send audio to the headphones. */
            while(SGPIO_STATUS_1 == 0);
            SGPIO_CLR_STATUS_1 = 1;
            buffer[0] = SGPIO_REG_SS(SGPIO_SLICE_A);
            sigi[i] = (buffer[0] & 0xff) | ((buffer[0] & 0xff0000)>>8);
            sigq[i] = ((buffer[0] >> 8) & 0xff) | ((buffer[0] & 0xff000000)>>16);
            dac_set(sqrt(sigi[0]*sigi[0] + sigq[0]*sigq[0])*g_volume);
          }
          break;
      }
    }
}

//# MENU telegraf
void telegraph_main(void) {
    /* Init CPU clock, and other hackrf/rad1olib related stuff. */
	telegraph_init();

    /* Init r0cketlib in order to use the LCD display and the joystick. */
    ssp_clock_init();
	systickInit();

	SETUPgout(LED4);
  OFF(LED4);

	inputInit();
	lcdInit();
	lcdFill(0xff);

  /* Required by the tick-based callbacks. */
  setTextColor(0xFF,0x00);
  render_display();

  /* Display UI and switch on radio stuff. */
  main_ui();

  return;
}
