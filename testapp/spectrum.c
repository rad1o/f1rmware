
#include <rad1olib/setup.h>
#include <rad1olib/systick.h>
#include <libopencm3/lpc43xx/m4/nvic.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/pins.h>

#include <portalib/portapack.h>
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/tuning.h>
#include <libopencm3/lpc43xx/dac.h>

#include <portalib/complex.h>

static volatile int64_t freq = 2450000000;

#define MODE_SPECTRUM 10
#define MODE_WATERFALL 20

static volatile int displayMode = MODE_SPECTRUM;
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
	lcdPrintln("MHz                ");
	lcdPrintln("-5MHz    0    +5MHz");
	lcdDisplay();
}

//# MENU spectrum
void spectrum_menu()
{
	lcdClear();
	lcdDisplay();
	getInputWaitRelease();

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

	while(1)
	{
		switch(getInput())
		{
			case BTN_UP:
				displayMode=MODE_WATERFALL;
				break;
			case BTN_DOWN:
				displayMode=MODE_SPECTRUM;
				break;
			case BTN_LEFT:
				freq -= 2000000;
				ssp1_set_mode_max2837();
				set_freq(freq);
				break;
			case BTN_RIGHT:
				freq += 2000000;
				ssp1_set_mode_max2837();
				set_freq(freq);
				break;
			case BTN_ENTER:
				//FIXME: unset the callback, reset the clockspeed, tidy up
                nvic_disable_irq(NVIC_DMA_IRQ);
                OFF(EN_VDD);
                OFF(EN_1V8);
                ON(MIC_AMP_DIS);
                systick_set_clocksource(0);
                systick_set_reload(12e6/SYSTICKSPEED/1000);
				return;

		}
	}
}
