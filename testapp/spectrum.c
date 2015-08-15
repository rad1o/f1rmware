
#include <rad1olib/setup.h>
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

void spectrum_callback(uint8_t* buf, int bufLen)
{
	TOGGLE(LED2);

	lcdClear();
	for(int i = 0; i < 128; i++) // display 128 FFT magnitude points
	{
		// FFT unwrap:
		uint8_t v;
		if(i < 64) // negative frequencies
			v = buf[(bufLen/2)+64+i];
		else // positive frequencies
			v = buf[i-64];
		
		// fill display	
		for(int j = 0; j < (v/2); j++)
			lcdBuffer[i+RESX*(RESY-j)] = 0x00;
	}
	
	// text info
	lcdPrint("f=");
	lcdPrint(IntToStr(freq/1000000,4,F_LONG));
	lcdPrintln("MHz");
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
				//lcdPrintln("Hallo, Welt!");
				//lcdDisplay();
				break;
			case BTN_DOWN:
				//lcdPrintln(IntToStr(sctr,7,F_LONG));
				//lcdDisplay();
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
		}
	}
}
