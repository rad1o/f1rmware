
#include <rad1olib/setup.h>
#include <rad1olib/systick.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/cm3/systick.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/intin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <portalib/portapack.h>
#include <portalib/specan.h>
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/sgpio_dma.h>
#include <common/tuning.h>
#include <libopencm3/lpc43xx/dac.h>

#include <portalib/complex.h>

#define DEFAULT_FREQ 2450000000
#define DEFAULT_MODE MODE_SPECTRUM

static volatile int64_t freq = DEFAULT_FREQ;

#define MODE_SPECTRUM 10
#define MODE_WATERFALL 20

static volatile int displayMode = DEFAULT_MODE;

// How long to wait before starting fast scroll
#define FAST_CHANGE_DELAY 100

// How much to change per 10 ms when scrolling fast
#define FAST_CHANGE_CHANGE 2000000

void spectrum_callback(uint8_t* buf, int bufLen)
{
	TOGGLE(LED2);
	if (displayMode == MODE_SPECTRUM)
		lcdClear();
	else if (displayMode == MODE_WATERFALL)
		lcdShift(0,1,0);

	for(int i = 0; i < 128; i++) // display 128 FFT magnitude points
	{
		// FFT unwrap:
		uint8_t v;
		if(i < 64) // negative frequencies
			v = buf[(bufLen/2)+64+i];
		else // positive frequencies
			v = buf[i-64];

		// fill display
		if (displayMode == MODE_SPECTRUM)
		{
			for(int j = 0; j < (v/2); j++)
				lcdBuffer[i+RESX*(RESY-j)] = 0x00;
		}
		else if (displayMode == MODE_WATERFALL)
		{
			lcdSetPixel(i,RESY-1,v);
		}
	}

	// text info
	lcdSetCrsr(0,0);
	lcdPrint("f=");
	lcdPrint(IntToStr(freq/1000000,4,F_LONG));
	lcdPrintln("MHz                 ");
	lcdPrintln("-5MHz    0    +5MHz");
	lcdDisplay();
}

void spectrum_init()
{
	// RF initialization from ppack.c:
	dac_init(false);
	cpu_clock_set(204); // WARP SPEED! :-)
	hackrf_clock_init();
	rf_path_pin_setup();
	/* Configure external clock in */
	scu_pinmux(SCU_PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);
	sgpio_configure_pin_functions();
	ON(EN_VDD);
	ON(EN_1V8);
	OFF(MIC_AMP_DIS);
	delayms(500); // doesn't work without
	cpu_clock_set(204); // WARP SPEED! :-)
	si5351_init();
	portapack_init();

	set_rx_mode(RECEIVER_CONFIGURATION_SPEC);
	specan_register_callback(spectrum_callback);

	// defaults:

	// the frequency is either initialized to DEFAULT_FREQ or set by the frequency menu
	// resetting it here would break the menu.
	//freq = DEFAULT_FREQ;

	displayMode = DEFAULT_MODE;
}

void spectrum_stop()
{
//	nvic_disable_irq(NVIC_DMA_IRQ);
	sgpio_dma_stop();
	sgpio_cpld_stream_disable();
	OFF(EN_VDD);
	OFF(EN_1V8);
	ON(MIC_AMP_DIS);
	systick_set_clocksource(0);
	systick_set_reload(12e6/SYSTICKSPEED/1000);

	specan_register_callback(0);
}

//# MENU spectrum frequency
void spectrum_frequency()
{
	freq=(int64_t)input_int("freq:",(int)(freq/1000000),50,4000,4)*1000000;
}

//# MENU spectrum show
void spectrum_show()
{
	int buttonPressTime;
	spectrum_init();
	ssp1_set_mode_max2837();
	set_freq(freq);
	while(1)
	{
		//getInputWaitRepeat does not seem to work?
		switch(getInputRaw())
		{
			case BTN_UP:
				displayMode=MODE_WATERFALL;
                while(getInputRaw()==BTN_UP)
                    ;
				break;
			case BTN_DOWN:
				displayMode=MODE_SPECTRUM;
                while(getInputRaw()==BTN_DOWN)
                    ;
				break;
			case BTN_LEFT:
        buttonPressTime = _timectr;
				freq -= 2000000;
				ssp1_set_mode_max2837();
				set_freq(freq);
                while(getInputRaw()==BTN_LEFT){
                    if (_timectr > buttonPressTime + FAST_CHANGE_DELAY/SYSTICKSPEED)
                    {
                        freq -= FAST_CHANGE_CHANGE;
                        ssp1_set_mode_max2837();
                        set_freq(freq);
                        delayms(10);
                    }
                }
				break;
			case BTN_RIGHT:
        buttonPressTime = _timectr;
				freq += 2000000;
				ssp1_set_mode_max2837();
				set_freq(freq);
                while(getInputRaw()==BTN_RIGHT){
                    if (_timectr > buttonPressTime + FAST_CHANGE_DELAY/SYSTICKSPEED)
                    {
                        freq += FAST_CHANGE_CHANGE;
                        ssp1_set_mode_max2837();
                        set_freq(freq);
                        delayms(10);
                    }
                }
				break;
			case BTN_ENTER:
				spectrum_stop();
				return;
		}
	}
}
