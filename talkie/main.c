/*
 * This file is part of rad1o
 *
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

#include <rad1olib/pins.h>
#include <rad1olib/systick.h>
#include <rad1olib/battery.h>

#include <hackrf_core.h>
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

#include "main.gen"

#define EVERY(x,y) if((ctr+y)%(x/SYSTICKSPEED)==0)
#define WAIT_CPU_CLOCK_INIT_DELAY   (10000)

#define DEFAULT_SAMPLE_RATE_HZ (10000000) /* 10MHz default sample rate */
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (5000000) /* 5MHz default */

#define PI 3.14159265358979323846
#define NB_SAMPLES 50000
#define MEASURES 20

typedef enum {
  TALKIE_RX_MODE,
  TALKIE_TX_MODE
} talkie_mode_t;

typedef struct {
	uint32_t bandwidth_hz;
} max2837_ft_t;

uint32_t g_freq = 2537000000U;
const uint64_t g_freq64 = (const uint64_t)2537000000U;
uint32_t baseband_filter_bw_hz = 0;
uint32_t sample_rate_hz;
talkie_mode_t g_current_mode = TALKIE_RX_MODE;

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



void night_tick(void){
    static int ctr;
    ctr++;

    /*
    EVERY(1024,0){
        //if(!adcMutex){
            batteryVoltageCheck();
            LightCheck();
        //}
    };

    static char night=0;
    static char posleds = 0;
    EVERY(128,2){
        if(night!=isNight()){
            night=isNight();
            if(night){
                ON(LCD_BL_EN);
//                push_queue(queue_unsetinvert);
            }else{
                OFF(LCD_BL_EN);
//                push_queue(queue_setinvert);
           };
        };
    };
    */
    EVERY(50,0){
        if(GLOBAL(chargeled)){
            //char iodir= (GPIO_GPIO1DIR & (1 << (11) ))?1:0;
            if(batteryCharging()) {
                ON(LED4);
#if 0
                if (iodir == gpioDirection_Input){
                    IOCON_PIO1_11 = 0x0;
                    gpioSetDir(RB_LED3, gpioDirection_Output);
                    gpioSetValue (RB_LED3, 1);
                    LightCheck();
                }
#endif
            } else {
                OFF(LED4);
#if 0
                if (iodir != gpioDirection_Input){
                    gpioSetValue (RB_LED3, 0);
                    gpioSetDir(RB_LED3, gpioDirection_Input);
                    IOCON_PIO1_11 = 0x41;
                    LightCheck();
                }
#endif
            }
        };

        if(batteryGetVoltage()<3600){
            if( (ctr/(50/SYSTICKSPEED))%10 == 1 ) {
                ON(LED4);
            } else {
                OFF(LED4);
            }
        };
    };

    return;
}

void sys_tick_handler(void){
	incTimer();
    night_tick();
	generated_tick();
};

/*
 * Talkie init
 *
 * This routine was hard to define since there is no doc, but
 * obviously mixing both hackrf init stuff with rad1olib init stuff
 * seems to work pretty good.
 *
 * May have some drawbacks we did not identified yet =).
 */

void talkie_init(void)
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

void talkie_init_rx(void)
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

}

void talkie_init_tx(void)
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
  rf_path_set_direction(RF_PATH_DIRECTION_TX);
  max2837_stop();
  si5351c_activate_best_clock_source();
  /*
  baseband_streaming_enable();
  */
  // Enable amplification (TX)
  rf_path_set_lna(1);
  // Enable antenna
  rf_path_set_antenna(1);
}

/*
 * TX function.
 */

void transmit(void) {
    char sz_freq[11];
    double amp=0.8;
    double r0, d_phi = (2*PI)/NB_SAMPLES, phi_zero=PI/4.0;
    int I0, Q0,i;

    volatile uint32_t buffer[4096];
    double magsq[10];
    int8_t sigi[10], sigq[10];
    int moy;


    /* HackRF setup as found in hackrf_core.c */
    pin_setup();
    enable_1v8_power();
    enable_rf_power();
    delay(1000000);
    cpu_clock_init();

    rf_path_init();

    /* LED setup (debug :) */
    SETUPgout(LED4);
    ON(LED4);

    /* Required by the LCD. */
    cpu_clock_set(204);

    /* Mic amplifier enabled. */
    SETUPgout(MIC_AMP_DIS);
    OFF(MIC_AMP_DIS);

    /* DAC enabled (in order to make some sound). */
    dac_init(false);

    /* Found in hackrf_usb: main. */
    ssp1_init();
    rf_path_init();
    talkie_init_rx();
    max2837_set_vga_gain(3);
    #if 0
    /* Set sample rate and frac.
     * Found in hackrf_transfer.c
     */
     sample_rate_frac_set(2*g_freq, 1);

     /* Compute default value depending on sample rate */
     baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(sample_rate_hz);

     /* Set carrier frequency. */
     set_freq(g_freq64);

    /* Found in hackrf_usb: set_transceiver_mode.
     * Called by hackrf_start_tx().
     */
    /*
    baseband_streaming_disable();
    */
    rf_path_set_direction(RF_PATH_DIRECTION_TX);
    max2837_stop();
    si5351c_activate_best_clock_source();
    /*
    baseband_streaming_enable();
    */
    // Enable amplification (TX)
    rf_path_set_lna(1);
    // Enable antenna
    rf_path_set_antenna(1);

    //max2837_set_frequency(g_freq);
    //max2837_start();
    //max2837_tx();
    #endif

    //baseband_streaming_enable();
    max2837_rx();
    sgpio_cpld_stream_enable();
    sgpio_set_slice_mode(false);

    /* JUSQUE LA, TOUT VA BIEN :) */
    sz_freq[0]='2';
    sz_freq[1]='5';
    sz_freq[2]='4';
    sz_freq[3]='1';
    sz_freq[4]='\0';

    /* Select the lcd display. */

    lcdInit();
    ssp_clock_init();
    lcd_select();
    lcdFill(0xff);
    lcdClear();
    lcdPrintln("=== Transmit RF ===");
    lcdPrintln(IntToStr(g_freq/1000000, 5, F_LONG));
    lcdDisplay();
    OFF(LED4);

    while (1){
#if 0
        /* Handles joystick up and down, inc/dec frequency when pressed. */
        if ((getInputRaw() & BTN_UP) == BTN_UP) {
            delay(4000);
            if ((getInputRaw() & BTN_UP) == BTN_UP) {
                ON(LED4);
                delay(1000000);
                OFF(LED4);
                g_freq += 500000;

                /* Select the lcd display. */
                ssp_clock_init();
                lcdFill(0xff);
                lcdClear();
                lcdPrintln("=== Transmit RF ===");
                lcdPrintln(IntToStr(g_freq/1000000, 5, F_LONG));
                lcdDisplay();

                /* Select the max2837. */
                ssp1_init();
                ssp1_set_mode_max2837();
                max2837_set_frequency(g_freq);
            }
        }
        if ((getInputRaw() & BTN_DOWN) == BTN_DOWN) {
            delay(4000);
            if ((getInputRaw() & BTN_DOWN) == BTN_DOWN) {
                ON(LED4);
                delay(1000000);
                OFF(LED4);
                g_freq -= 500000;

                /* Select the lcd display. */
                ssp_clock_init();
                //lcdInit();
                lcdFill(0xff);
                lcdClear();
                lcdPrintln("=== Transmit RF ===");
                lcdPrintln(IntToStr(g_freq/1000000, 5, F_LONG));
                lcdDisplay();

                /* Select the max2837. */
                ssp1_init();
                ssp1_set_mode_max2837();
                max2837_set_frequency(g_freq);
            }
        }

        if ((adc_get_single(ADC0, ADC_CR_CH7)>>2) > 127)
            ON(LED4);
        else
            OFF(LED4);

        /* We read 8 samples from the ADC. */
        for (i=0; i<8; i++)
            samples[i] = (i%4)?0xff:0x00;

        /* We transfer this to the SGPIO. */
        __asm__(
            "ldr r0, [%[p], #0]\n\t"
            "str r0, [%[SGPIO_REG_SS], #44]\n\t"
            "ldr r0, [%[p], #4]\n\t"
            "str r0, [%[SGPIO_REG_SS], #20]\n\t"
            "ldr r0, [%[p], #8]\n\t"
            "str r0, [%[SGPIO_REG_SS], #40]\n\t"
            "ldr r0, [%[p], #12]\n\t"
            "str r0, [%[SGPIO_REG_SS], #8]\n\t"
            "ldr r0, [%[p], #16]\n\t"
            "str r0, [%[SGPIO_REG_SS], #36]\n\t"
            "ldr r0, [%[p], #20]\n\t"
            "str r0, [%[SGPIO_REG_SS], #16]\n\t"
            "ldr r0, [%[p], #24]\n\t"
            "str r0, [%[SGPIO_REG_SS], #32]\n\t"
            "ldr r0, [%[p], #28]\n\t"
            "str r0, [%[SGPIO_REG_SS], #0]\n\t"
            :
            : [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
              [p] "l" (p)
            : "r0"
        );
	//sgpio_set_slice_mode(false);
    //SGPIO_SET_EN_1 &= (~(1 << SGPIO_SLICE_A));
	//sgpio_configure(TRANSCEIVER_MODE_TX);
#endif

    /* We generate 22727 samples, with a increase of phi of 0.000276 */
    while(true) {
      /* Handles joystick up and down, inc/dec frequency when pressed. */
      if ((getInputRaw() & BTN_UP) == BTN_UP) {
          delay(4000);
          if ((getInputRaw() & BTN_UP) == BTN_UP) {
              g_freq += 500000;

              /* Select the lcd display. */
              ssp_clock_init();
              lcdFill(0xff);
              lcdClear();
              lcdPrintln("=== Transmit RF ===");
              lcdPrintln(IntToStr(g_freq/1000000, 5, F_LONG));
              lcdDisplay();

              /* Select the max2837. */
              if (g_current_mode == TALKIE_RX_MODE) {
                talkie_init_rx();
              } else {
                talkie_init_tx();
              }
              ssp1_init();
              ssp1_set_mode_max2837();
              max2837_set_frequency(g_freq);
          }
      }
      if ((getInputRaw() & BTN_DOWN) == BTN_DOWN) {
          delay(4000);
          if ((getInputRaw() & BTN_DOWN) == BTN_DOWN) {
              ON(LED4);
              delay(1000000);
              OFF(LED4);
              max2837_stop();
              g_freq -= 500000;

              /* Select the lcd display. */
              ssp_clock_init();
              //lcdInit();
              lcdFill(0xff);
              lcdClear();
              lcdPrintln("=== Transmit RF ===");
              lcdPrintln(IntToStr(g_freq/1000000, 5, F_LONG));
              lcdDisplay();

              /* Select the max2837. */
              //ssp1_init();
              //ssp1_set_mode_max2837();
              //max2837_set_frequency(g_freq);

              if (g_current_mode == TALKIE_RX_MODE) {
                talkie_init_rx();
              } else {
                talkie_init_tx();
              }
              ssp1_init();
              ssp1_set_mode_max2837();
              max2837_set_frequency(g_freq);
          }
      }
      if ((getInputRaw() & BTN_ENTER) == BTN_ENTER) {
        if (g_current_mode == TALKIE_RX_MODE) {
          max2837_stop();
          talkie_init_tx();
          sgpio_cpld_stream_disable();
          sgpio_set_slice_mode(false);
      g_current_mode = TALKIE_TX_MODE;
        }
        max2837_start();
        max2837_tx();
        delay(5000);
        max2837_stop();
        delay(5000);
      } else if (g_current_mode == TALKIE_TX_MODE) {
        max2837_stop();
        talkie_init_rx();
        sgpio_cpld_stream_enable();
        sgpio_set_slice_mode(false);
        max2837_start();
        max2837_set_vga_gain(3);
        max2837_rx();
        g_current_mode = TALKIE_RX_MODE;
      }

      while(SGPIO_STATUS_1 == 0);
      SGPIO_CLR_STATUS_1 = 1;
      buffer[0] = SGPIO_REG_SS(SGPIO_SLICE_A);
      sigi[i] = (buffer[i] & 0xff) | ((buffer[i] & 0xff0000)>>8);
      sigq[i] = ((buffer[i] >> 8) & 0xff) | ((buffer[i] & 0xff000000)>>16);
      //magsq[0] = sqrt(sigi[0]*sigi[0] + sigq[0]*sigq[0]);

      dac_set(sqrt(sigi[0]*sigi[0] + sigq[0]*sigq[0]));

#if 0
      for (i=0; i<MEASURES; i++) {
        while(SGPIO_STATUS_1 == 0);
        SGPIO_CLR_STATUS_1 = 1;
        buffer[i] = SGPIO_REG_SS(SGPIO_SLICE_A);

        /* find the magnitude squared */
        sigi[i] = (buffer[i] & 0xff) | ((buffer[i] & 0xff0000)>>8);
        sigq[i] = ((buffer[i] >> 8) & 0xff) | ((buffer[i] & 0xff000000)>>16);
        magsq[i] = sigi[i]*sigi[i] + sigq[i]*sigq[i];
        moy += magsq[i];
        /*
        if ((uint16_t)magsq & 0x8000) {
          magsq[i] ^= 0xffff;
          magsq[i]++;
        }*/
        delay(10000);
      }
      moy = moy/MEASURES;
      if (((magsq[0] > moy) && (magsq[3] < moy)) ||
          ((magsq[0] < moy) && (magsq[3] > moy)))
          ON(LED4);
      else
          OFF(LED4);
#endif
    }
  }
}

int main(void) {
    /* Init CPU clock, and other hackrf/rad1olib related stuff. */
	talkie_init();

    /* Init r0cketlib in order to use the LCD display and the joystick. */
    ssp_clock_init();
	systickInit();

	SETUPgout(LED4);
    //SETUPgout(MIC_AMP_DIS);
    //OFF(MIC_AMP_DIS);
    OFF(LED4);

	inputInit();
	lcdInit();
	lcdFill(0xff);

    /* Required by the tick-based callbacks. */
	generated_init();

    setTextColor(0xFF,0x00);
    lcdClear();
    lcdPrintln("=== Transmit RF ===");
    lcdDisplay();

    /* Transmit. */
    while(1) transmit();

    return 0;
}
