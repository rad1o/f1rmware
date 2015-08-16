/*
 * Copyright 2015 team rad1o
 *
 */

#include <unistd.h>
#include <string.h>

#include <rad1olib/setup.h>
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
#include <rad1olib/systick.h>

#include <portalib/portapack.h>
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <libopencm3/lpc43xx/dac.h>

#include <portalib/complex.h>

#include "display.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))


volatile int64_t freq = 2450000000;
uint8_t spectrum_y = 0;
uint16_t spectrum[RESY][128];
int acc = 0;

#define ACC_SAMPLES 8

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

                spectrum[spectrum_y][i] += v;
	}

        acc++;
        if (acc >= ACC_SAMPLES) {
          acc = 0;
          spectrum_y++;
          if (spectrum_y >= RESY)
            spectrum_y = 0;
          memset(spectrum[spectrum_y], 0, 128 * sizeof(spectrum[0][0]));
        }
}

void sys_tick_handler(void){
	incTimer();
};

uint8_t v_to_rgb[255][3];

void init_v_to_rgb() {
  for(int v = 0; v < 255; v++) {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      if (v < 0x40) {
        r = 0;
        g = 0;
        b = v << 2;
      } else if (v < 0x80) {
        r = (v - 0x40) << 2;
        g = 0;
        b = 255 - ((v - 0x40) << 2);
      } else if (v < 0xC0) {
        r = 255;
        g = (v - 0x80) << 2;
        b = 0;
      } else {
        r = 255;
        g = 255;
        b = (v - 0xC0) << 2;
      }
      v_to_rgb[v][0] = r;
      v_to_rgb[v][1] = g;
      v_to_rgb[v][2] = b;
  }
}

void draw(void) {
  int x, y;
  /* uint16_t max = 0; */
  /* for(y = 0; y < RESY; y++) { */
  /*   for(x = 0; x < 128; x++) { */
  /*     uint8_t v = spectrum[y][x]; */
  /*     if (v > max) */
  /*       max = v; */
  /*   } */
  /* } */
  
  int sy = spectrum_y;
  startPixels();
  for(y = 0; y < RESY; y++) {
    for(x = 0; x < RESX; x++) {
      uint16_t v = (x >= 1 && x <= 128) ? spectrum[sy][x - 1] : 0;
      uint8_t *vs = v_to_rgb[MIN(255, v) /*255 * v / max*/];
      emitPixel(vs[0], vs[1], vs[2]);
      /* lcdSetPixel(x + 1, y, (r & 0b11100000) | ((g >> 3) & 0b11100) | ((b >> 6) & 0b11)); */
    }
    sy--;
    if (sy < 0)
      sy = RESY - 1;
  }
  stopPixels();
}

int main(void) {
	cpu_clock_init_(); /* CPU Clock is now 104 MHz */
	ssp_clock_init();
	systickInit();
	dac_init(false);

	SETUPgout(EN_VDD);
	SETUPgout(EN_1V8);
	SETUPgout(MIXER_EN);
	SETUPgout(MIC_AMP_DIS);

	SETUPgout(LED1);
	SETUPgout(LED2);
	SETUPgout(LED3);
	SETUPgout(LED4);

	inputInit();
	/* lcdInit(); */
	/* fsInit();  */
	/* lcdFill(0); */
	/* readConfig(); */
        memset(spectrum, RESY * 128, 0);

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

        init_v_to_rgb();
	while(1){
		TOGGLE(LED1);
		switch(getInputRaw())
		{
			case BTN_LEFT:
				freq -= 1000000;
				ssp1_set_mode_max2837();
				set_freq(freq);
				break;
			case BTN_RIGHT:
				freq += 1000000;
				ssp1_set_mode_max2837();
				set_freq(freq);
				break;
		}

                draw();
	};
};
