#include <string.h>

#include <r0ketlib/keyin.h>
#include <r0ketlib/render.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

#define CHARWIDTH 20

struct colorpicker{
    char *prompt;
    uint8_t color, pos, r,g,b;
	  bool done;
} s_color;

static void colorpickerInit(char p[], uint8_t c) {
	s_color.prompt = p;
	s_color.color = c;
  s_color.r = (c & 0b11100000)>>5;
  s_color.g = (c & 0b00011100)>>2;
  s_color.b = c & 0b00000011;
	s_color.pos = 0;
	s_color.done = false;
}


static void inputMove() {
	switch(getInputWaitRepeat()){
        case BTN_LEFT:
            if (s_color.pos > 0) {
                s_color.pos --;
            }
            break;
        case BTN_RIGHT:
            if (s_color.pos < 2) {
                s_color.pos++;
            }
            break;
        case BTN_UP:
            if (s_color.pos == 0) {
              if (s_color.r < 7) s_color.r++;
            } else if (s_color.pos == 1) {
              if (s_color.g < 7) s_color.g++;
            } else if (s_color.pos == 2) {
              if (s_color.b < 3) s_color.b++;
            }
            break;
        case BTN_DOWN:
            if (s_color.pos == 0) {
              if (s_color.r > 0) s_color.r--;
            } else if (s_color.pos == 1) {
              if (s_color.g > 0) s_color.g--;
            } else if (s_color.pos == 2) {
              if (s_color.b > 0) s_color.b--;
            }
            break;
        case BTN_ENTER:
            s_color.done = true;
//            getInputWaitRelease();
            break;
    }
}

static void inputDraw() {
	s_color.color=(s_color.r<<5)+(s_color.g<<2)+(s_color.b);
  lcdClear();
  lcdFill(s_color.color);
	lcdPrint(s_color.prompt);
	for (int dx = 0; dx<= 3; dx++){
		if (dx == 0) {
      DoString(dx*CHARWIDTH, 30,"R");
      DoString(dx*CHARWIDTH,20,IntToStr(s_color.r,1,0));
    }
    if (dx == 1) {
      DoString(dx*CHARWIDTH, 30,"G");
      DoString(dx*CHARWIDTH,20,IntToStr(s_color.g,1,0));
    }
    if (dx == 2) {
      DoString(dx*CHARWIDTH, 30,"B");
      DoString(dx*CHARWIDTH,20,IntToStr(s_color.b,1,0));
    }

	}
	DoChar(s_color.pos * CHARWIDTH, 40, '-');
}

uint8_t colorpicker(char prompt[], uint8_t color){
	setSystemFont();
  setTextColor(0xFF,0x00);
	colorpickerInit(prompt, color);
	while (!s_color.done) {
		inputDraw();
		lcdDisplay();
		inputMove();
	}
  lcdPrintln("");
  lcdPrint("color: ");
  lcdPrintln(IntToStr(s_color.color,3,0));
	return s_color.color;
}
