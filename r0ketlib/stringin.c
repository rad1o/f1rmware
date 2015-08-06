#include <string.h>

#include <r0ketlib/keyin.h>
#include <r0ketlib/render.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

#define CHARWIDTH 12
#define CHARSPACE 0x20

struct in{
    char *line, *prompt;
    uint8_t pos, dcursor, maxlength, asciistart, asciiend;
	bool done;
} s_input;

static void textinputInit(char p[],char s[], uint8_t l, uint8_t as, uint8_t ae) {
	//TODO: Check length!
	s_input.prompt = p;
	s_input.line = s;
	s_input.maxlength = l;
	s_input.asciistart = as;
	s_input.asciiend = ae;
	s_input.pos = 0;
	s_input.dcursor = 0;
	s_input.done = false;
    s[l-1]=0;
    for(int i=strlen(s);i<(l-1);i++)
        s[i]=0;
}


static void inputMove() {
	char *cur = s_input.line+s_input.pos+s_input.dcursor;
    switch(getInputWaitRepeat()){
        case BTN_LEFT:
            if (s_input.dcursor >0) {
                s_input.dcursor --;
            } else if (s_input.pos > 0) {
                s_input.pos --;
            }
            break;
        case BTN_RIGHT:
            if (s_input.dcursor <RESX/CHARWIDTH-1 && s_input.pos + s_input.dcursor < s_input.maxlength) {
                if (*cur == 0) {
                    *cur = CHARSPACE;
                }
                s_input.dcursor ++;
            } else if (s_input.pos + RESX/CHARWIDTH < s_input.maxlength) {
                s_input.pos++;
                if (*cur == 0) {
                    *cur = CHARSPACE;
                }
            }			
            break;
        case BTN_UP:
            if (*cur <= s_input.asciistart) {
                *cur = s_input.asciiend;
            } else if (*cur > s_input.asciiend) {
                *cur = s_input.asciiend;
            } else  {
                *cur = *cur - 1;
            }
            break;
        case BTN_DOWN:
            if (*cur >= s_input.asciiend) {
                *cur = s_input.asciistart;
            } else if (*cur < s_input.asciistart) {
                *cur = s_input.asciistart;
            } else {
                *cur = *cur + 1;
            }
            break;
        case BTN_ENTER:
            s_input.done = true;
//            getInputWaitRelease();
            break;
    }
}

static void inputDraw() {
	char tmp;
	int pos = 0;	
	lcdClear();
	lcdPrint(s_input.prompt);
	for (int dx = 0; dx<= RESX/CHARWIDTH && s_input.pos+dx<s_input.maxlength; dx++){
		tmp = s_input.line[s_input.pos+dx];
		if(tmp==0)
			tmp=32;
		DoChar(dx*CHARWIDTH, 30,tmp);
	}
	DoChar(s_input.dcursor * CHARWIDTH, 40, '-');
	

	lcdSetCrsr(0,60);
	lcdPrint("[");
	lcdPrint(IntToStr(s_input.pos+s_input.dcursor+1,2,F_LONG));
	lcdPrint("/");
	lcdPrint(IntToStr(s_input.maxlength,2,F_LONG));
	lcdPrint("]");
}

static void inputClean() {
	for (int x=0;x<=s_input.maxlength;x++) {
		if (s_input.line[x] == 0) {
			x--;
			while (s_input.line[x] == CHARSPACE && x>=0) {
				s_input.line[x] = 0;
				x--;
			}
			return;
		}
	}
}

void input(char prompt[], char line[], uint8_t asciistart, uint8_t asciiend, uint8_t maxlength){
	setSystemFont();
	textinputInit(prompt, line, maxlength, asciistart, asciiend);
	while (!s_input.done) {
		inputDraw();
		lcdDisplay();
		inputMove();
	}
	inputClean();
	return;
}

