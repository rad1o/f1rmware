#include <display.h>
#include <render.h>
#include <fonts.h>
#include <print.h>
#include <fonts/smallfonts.h>

int x=0;
int y=0;

static void checkScroll(void){
  if(y+font->u8Height>RESY){
      lcdShift(0,y+font->u8Height-RESY,false);
      y=RESY-font->u8Height;
  };
}

void lcdPrint(const char *string){
  checkScroll();
  x=DoString(x,y,string);
}

void lcdNl(void){
  x=0;y+=font->u8Height;
}

void lcdCheckNl(void){
    if(x>RESX)
         lcdNl();
}

void lcdPrintln(const char *string){
  lcdPrint(string);
  lcdNl();
}
  
void lcdClear(){
  x=0;y=0;
  lcdFill(0);
}


void lcdMoveCrsr(signed int dx,signed int dy){
    x+=dx;
    y+=dy;
}

void lcdSetCrsr(int dx,int dy){
    x=dx;
    y=dy;
}

void lcdSetCrsrX(int dx){
    x=dx;
}

void setSystemFont(void){
    setIntFont(&Font_7x8);
}


int lcdGetVisibleLines(void){
    return (RESY/getFontHeight()); // subtract title line
}

