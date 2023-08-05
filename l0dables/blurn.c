/* blurn -- underdamped blur animation for the rad1o

   This is the little sister of https://github.com/neeels/burnscope

   (c) Neels Hofmeyr <neels@hofmeyr.de> (2015)
   Published under the GNU General Public License v2
 */

#include <stdlib.h>

#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/render.h>

#include "usetable.h"

typedef unsigned int uint;
typedef float pixel_t;

static void add_copy(uint8_t* from, pixel_t* to, uint N, int ofs)
{
  uint8_t* from_pos = from;
  uint8_t* from_end = from + N;
  pixel_t* to_pos = to;
  uint8_t* s;
  pixel_t* d;
  if (ofs < 0) {
    from_pos += -ofs;
    for (s = from, d = to + N - (-ofs);
         s < from_pos;
         s ++, d ++) {
      *d += *s;
    }
  }
  else
  if (ofs > 0) {
    to_pos += ofs;
    from_end -= ofs;
    for (s = from_end, d = to;
         d < to_pos;
         s ++, d ++) {
      *d += *s;
    }
  }

  for (s = from_pos, d = to_pos;
       s < from_end;
       s ++, d ++) {
    *d += *s;
  }
}

void ram(void)
{


// Note: N has to be low enough for memory reasons.
#define W RESX
#define H 55
#define Y ((RESY - 2*H)/2)
#define N (W*H)
#define RESN (RESX*RESY)

  pixel_t pixels[N];
  uint8_t* lcd = lcdGetBuffer();

  // the blur algorithm reads back from the screen buffer. So if I'm
  // displaying the plain image Y pixels lower, I also need to read back Y
  // pixels lower.
  uint8_t* lcd_from = lcd + Y * RESX;

  lcdFill(0);

  setIntFont(&Font_7x8);
  setTextColor(0, 0b11111100);
  lcdSetCrsr(0, 0);
  lcdPrintln("blurn");
  lcdPrintln("underdamped blur");
  lcdNl();
  lcdPrintln("DOWN/UP cool down");
  lcdPrintln("LEFT/RIGHT heat up");
  lcdPrintln("ENTER exit");
  lcdNl();
  lcdPrintln("any key to start");
  lcdDisplay();

  uint random_seed; // hopefully some random value...
  uint debounce;

  // wait for initial key release after program started.
  debounce = 0;
  while (debounce < 100) {
    if (getInputRaw() == BTN_NONE)
      debounce ++;
    else
      debounce = 0;
    random_seed ++;
  }

  // wait for user to press a button to acknowledge greeting.
  debounce = 0;
  while (debounce < 100) {
    if (getInputRaw() != BTN_NONE)
      debounce ++;
    else
      debounce = 0;
    random_seed ++;
  }

  // and wait for another release to start the loop.
  debounce = 0;
  while (debounce < 100) {
    if (getInputRaw() == BTN_NONE)
      debounce ++;
    else
      debounce = 0;
    random_seed ++;
  }


  lcdFill(0);
  srand(random_seed);
  for (uint i = 0; i < N; i++) {
    pixels[i] = rand();
  }

  while (1) {
    {
      // draw in screen resolution and repeat the high res image
      uint lcd_pos = 0;
      while (lcd_pos < RESN) {
        // a mirrored strip up to position Y
        for (int pixels_pos = N-(W * (H - Y))-W; (pixels_pos >= 0) && (lcd_pos < RESN);) {
          uint row_end = pixels_pos + W;
          for (; pixels_pos < row_end; pixels_pos ++, lcd_pos ++) {
            lcd[lcd_pos] = (uint8_t)pixels[pixels_pos];
          }
          pixels_pos -= 2 * W;
        }

        // this is the plain image at position Y
        for (uint pixels_pos = 0; (pixels_pos < N) && (lcd_pos < RESN); pixels_pos ++, lcd_pos ++) {
          lcd[lcd_pos] = (uint8_t)pixels[pixels_pos];
        }

        // another mirrored strip below plain image at Y
        for (int pixels_pos = N-W; (pixels_pos >= 0) && (lcd_pos < RESN);) {
          uint row_end = pixels_pos + W;
          for (; pixels_pos < row_end; pixels_pos ++, lcd_pos ++) {
            lcd[lcd_pos] = (uint8_t)pixels[pixels_pos];
          }
          pixels_pos -= 2 * W;
        }

        // and another forward strip if there's any more space.
        for (uint pixels_pos = 0; (pixels_pos < N) && (lcd_pos < RESN); pixels_pos ++, lcd_pos ++) {
          lcd[lcd_pos] = (uint8_t)pixels[pixels_pos];
        }

      }
    }
    lcdDisplay();

    add_copy(lcd_from, pixels, N, 1);
    add_copy(lcd_from, pixels, N, -1);
    add_copy(lcd_from, pixels, N, W);
    add_copy(lcd_from, pixels, N, - W);
    add_copy(lcd_from, pixels, N, W + 1);
    add_copy(lcd_from, pixels, N, W - 1);
    add_copy(lcd_from, pixels, N, - W + 1);
    add_copy(lcd_from, pixels, N, - W - 1);

    switch (getInputRaw()) {
      default:
      case BTN_NONE:
        // for some reason I can't divide by a floar variable.
        // So I have to have this loop twice with separate variables.
        for (uint i = 0; i < N; i++) {
          pixels[i] /= 8.92;
        }
        break;

      case BTN_DOWN:
        for (uint i = 0; i < N; i++) {
          pixels[i] /= 9.04;
        }
        break;

      case BTN_UP:
        for (uint i = 0; i < N; i++) {
          pixels[i] /= 9.15;
        }
        break;

      case BTN_RIGHT:
        for (uint i = 0; i < N; i++) {
          pixels[i] /= 8.58;
        }
        break;

      case BTN_LEFT:
        for (uint i = 0; i < N; i++) {
          pixels[i] /= 8.67;
        }
        break;

      case BTN_ENTER:
        return;
    }
  }
}
