/*
 * Copyright 2015 team rad1o
 *
 */

#include <unistd.h>

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

extern uint32_t sctr;
extern complex_s8_t *s8ram;
extern volatile int64_t freq;
extern uint8_t spectrum_y;
extern uint8_t spectrum[RESY][128];

void sys_tick_handler(void){
	incTimer();
};


void draw(void) {
  int sy = spectrum_y;
  startPixels();
  for(int y = 0; y < RESY; y++) {
    for(int x = 0; x < RESX; x++) {
      uint8_t v = (x >= 1 && x <= 128) ? spectrum[sy][x - 1] : 0;
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
        b = 255 - (v << 2);
      } else if (v < 0xC0) {
        r = 255;
        g = (v - 0x80) << 2;
        b = 0;
      } else {
        r = 255;
        g = 255;
        b = (v - 0xC0) << 2;
      }
      emitPixel(r, g, b);
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
