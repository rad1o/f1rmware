
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

#include "display.h"

volatile int64_t freq = 2450000000;
uint8_t spectrum_y = 0;
uint8_t spectrum[RESY][128];
int acc = 0;

void spectrum_callback(uint8_t* buf, int bufLen)
{
	TOGGLE(LED2);

	for(int i = 0; i < 128; i++) // display 128 FFT magnitude points
	{
		// FFT unwrap:
		uint8_t v;
		if(i < 64) // negative frequencies
			v = buf[(bufLen/2)+64+i];
		else // positive frequencies
			v = buf[i-64];

                spectrum[spectrum_y][i] += v >> 3;
                /* spectrum[spectrum_y][i] = v; */
	}

        acc++;
        if (acc >= 8) {
          acc = 0;
          spectrum_y++;
          if (spectrum_y >= RESY)
            spectrum_y = 0;
          memset(spectrum[spectrum_y], 0, 128);
        }
}
