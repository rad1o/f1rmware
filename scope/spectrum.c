
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

volatile int64_t freq = 2450000000;

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
