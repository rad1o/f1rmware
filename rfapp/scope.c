/*
 * Copyright 2015 team rad1o
 *
 */

#include <unistd.h>
#include <string.h>
#include <math.h>

#include <rad1olib/setup.h>
#include <r0ketlib/print.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/display.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <portalib/portapack.h>
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <libopencm3/lpc43xx/dac.h>

#include <portalib/complex.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define USE_SAMPLES 128
#define KEEP_SAMPLES 119

volatile int64_t freq = 2450000000;
uint8_t spectrum_y = 0;
uint8_t spectrum[KEEP_SAMPLES][USE_SAMPLES];
int acc = 0;
int acc_bits = 3;
int acc_max = 8;


void scope_callback(uint8_t* buf, int bufLen)
{
	OFF(LED2);

	for(int i = 0; i < USE_SAMPLES; i++) // display 128 FFT magnitude points
	{
		// FFT unwrap:
		uint8_t v;
		if(i < 64) // negative frequencies
			v = buf[(bufLen/2)+64+i];
		else // positive frequencies
			v = buf[i-63];

                spectrum[spectrum_y][i] += v >> acc_bits;
	}

        acc++;
        if (acc >= acc_max) {
          acc = 0;
          spectrum_y++;
          if (spectrum_y >= KEEP_SAMPLES)
            spectrum_y = 0;
          memset(spectrum[spectrum_y], 0, USE_SAMPLES * sizeof(spectrum[0][0]));
        }

        ON(LED2);
}


void draw_rect(int x1, int y1, int x2, int y2, uint8_t c) {
  for(int y = y1; y <= y2; y++) {
    for(int x = x1; x <= x2; x++) {
      lcdSetPixel(x, y, c);
    }
  }
}

void draw_opt_bar(int content_y);

uint8_t v_to_rgb[255];

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
      v_to_rgb[v] = RGB_TO_8BIT(r, g, b);
  }
}



void opt_freq_input(int key, int step_size) {
  switch(key)
  {
  case BTN_UP:
    freq += step_size * 1000000;
    ssp1_set_mode_max2837();
    set_freq(freq);
    break;
  case BTN_DOWN:
    freq -= step_size * 1000000;
    ssp1_set_mode_max2837();
    set_freq(freq);
    break;
  }
}

void opt_freq_input1(int key) {
  opt_freq_input(key, 1);
}
void opt_freq_input10(int key) {
  opt_freq_input(key, 10);
}
void opt_freq_input100(int key) {
  opt_freq_input(key, 100);
}
void opt_freq_draw(int step_size) {
  draw_opt_bar(122);
  lcdSetCrsr(0, 123);
  setTextColor(0xff, RGB_TO_8BIT(0x7f, 0, 0));
  lcdPrint(IntToStr(freq/1000000,4,F_LONG));
  lcdPrint(" MHz");
  setTextColor(0xff, RGB_TO_8BIT(0, 0x7f, 0));
  lcdPrint("   +-");
  lcdPrint(IntToStr(step_size, 3, 0));
}

void opt_freq_draw1() {
  opt_freq_draw(1);
}
void opt_freq_draw10() {
  opt_freq_draw(10);
}
void opt_freq_draw100() {
  opt_freq_draw(100);
}

void opt_acc_input(int key) {
  if (key == BTN_UP && acc_bits > 0) {
    acc_bits--;
    acc_max = powl(2, acc_bits);
  } else if (key == BTN_DOWN && acc_bits < 7) {
    acc_bits++;
    acc_max = powl(2, acc_bits);
  }
}

void opt_acc_draw() {
  draw_opt_bar(122);
  lcdSetCrsr(0, 123);
  setTextColor(0xff, RGB_TO_8BIT(0, 0x7f, 0));
  lcdPrint("Timescale:");

  lcdSetCrsr(RESX - 24, 123);
  setTextColor(0xff, RGB_TO_8BIT(0x7f, 0, 0));
  lcdPrint("1:");
  lcdPrint(IntToStr(acc_bits + 1, 1, 0));
}

struct option_entry {
  void (*input)(int);
  void (*draw)();
};

struct option_entry options[] = {
  { opt_freq_input1, opt_freq_draw1 },
  { opt_freq_input10, opt_freq_draw10 },
  { opt_freq_input100, opt_freq_draw100 },
  { opt_acc_input, opt_acc_draw }
};
int current_option = 0;
int options_len = sizeof(options) / sizeof(options[0]);

void draw_opt_bar(int content_y) {
  draw_rect(0, content_y, RESX - 1, RESY - 1, 0xff);

  int tab_x1 = RESX * current_option / options_len;
  int tab_x2 = RESX * (current_option + 1) / options_len;
  draw_rect(0, content_y - 2, tab_x1 - 1, content_y - 2, RGB_TO_8BIT(0, 0, 0xFF));
  draw_rect(tab_x1, content_y - 2, tab_x2, content_y - 2, RGB_TO_8BIT(0xFF, 0xFF, 0));
  draw_rect(tab_x2 + 1, content_y - 2, RESX, content_y - 2, RGB_TO_8BIT(0, 0, 0xFF));
  draw_rect(0, content_y - 1, tab_x1 - 1, content_y - 1, RGB_TO_8BIT(0, 0, 0xBF));
  draw_rect(tab_x1, content_y - 1, tab_x2, content_y - 1, RGB_TO_8BIT(0xBF, 0xBF, 0));
  draw_rect(tab_x2 + 1, content_y - 1, RESX, content_y - 1, RGB_TO_8BIT(0, 0, 0xBF));
}

void draw_spectrum(void) {
  int x, y;
  draw_rect(0, 0, (RESX - USE_SAMPLES) / 2 - 1, KEEP_SAMPLES, 0xFF);
  draw_rect(RESX - (RESX - USE_SAMPLES) / 2, 0, RESX - 1, KEEP_SAMPLES, 0xFF);

  int spectrum_y_old = spectrum_y;
  int sy = spectrum_y;
  for(y = 0; y < KEEP_SAMPLES; y++) {
    for(x = 0; x < USE_SAMPLES; x++) {
      uint8_t v = spectrum[sy][x];
      uint8_t c = v_to_rgb[v];
      lcdSetPixel(x + 1, y, c);
    }
    sy--;
    if (sy < 0)
      sy = KEEP_SAMPLES - 1;
  }
}

//# MENU scope
void scope_main(void) {
	dac_init(false);
	setIntFont(&Font_8x8Thin);
  memset(spectrum, KEEP_SAMPLES * USE_SAMPLES, 0);

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
	specan_register_callback(scope_callback);
	init_v_to_rgb();


	while(1){
		OFF(LED1);
    int key = getInputRaw();
    if (key == BTN_LEFT) {
      current_option--;
      if (current_option < 0) {
        current_option = options_len - 1;
      }
    } else if (key == BTN_RIGHT) {
      current_option++;
      if (current_option >= options_len)
        current_option = 0;
    } else {
      options[current_option].input(key);
    }

    draw_spectrum();
    options[current_option].draw();
    lcdDisplay();

    ON(LED1);
	}
}
