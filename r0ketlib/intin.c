#include <string.h>

#include <r0ketlib/keyin.h>
#include <r0ketlib/render.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

#define CHARWIDTH 10

struct intin{
    char *prompt;
    int the_number, digits, min, max, pos;
    bool done;
} s_int;

int power(int base, unsigned int exp) {
    int i, result = 1;
    for (i = 0; i < exp; i++)
        result *= base;
    return result;
 }


static void intinInit(char p[], int initial, int min, int max, int digits) {
	s_int.prompt = p;
	s_int.the_number = initial;
  s_int.digits = digits;
	s_int.min = min;
  s_int.max = max;
  s_int.pos = 0;
	s_int.done = false;
}


static void intinMove() {
	switch(getInputWaitRepeat()){
        case BTN_LEFT:
            if (s_int.pos > 0) {
                s_int.pos --;
            }
            break;
        case BTN_RIGHT:
            if (s_int.pos < s_int.digits-1) {
                s_int.pos++;
            }
            break;
        case BTN_UP:
            s_int.the_number += power(10,s_int.digits-s_int.pos-1);
            break;
        case BTN_DOWN:
          s_int.the_number -= power(10,s_int.digits-s_int.pos-1);
            break;
        case BTN_ENTER:
            s_int.done = true;
//            getInputWaitRelease();
            break;
    }
}

static void intinDraw() {
  lcdClear();
	lcdPrint(s_int.prompt);
	for (int dx = 0; dx< s_int.digits; dx++){
		DoChar(dx*CHARWIDTH,20,IntToStr(s_int.the_number,s_int.digits,F_LONG|F_ZEROS)[dx]);
	}
  DoChar(s_int.pos * CHARWIDTH, 30, '^');

}

static void intinClean() {
  if (s_int.the_number > s_int.max)
    s_int.the_number = s_int.max;
  if (s_int.the_number < s_int.min)
    s_int.the_number = s_int.min;
}

int input_int(char prompt[], int initial, int min, int max, int digits) {
	setSystemFont();
  setTextColor(0xFF,0x00);
	intinInit(prompt, initial, min, max, digits);
	while (!s_int.done) {
		intinDraw();
		lcdDisplay();
		intinMove();
    intinClean();
	}
  lcdPrintln("");
  lcdPrint("number: ");
  lcdPrintln(IntToStr(s_int.the_number,s_int.digits,F_LONG|F_ZEROS));
	return s_int.the_number;
}
