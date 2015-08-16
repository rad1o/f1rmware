#include <stdint.h>
#include <string.h>

#include <r0ketlib/display.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/config.h>
#include <r0ketlib/print.h>
#include <rad1olib/pins.h>
#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/image.h>


#include "usetable.h"

void dim(uint8_t *target, uint8_t *colours, size_t size, int dimmingfactor){
  for(int i= 0; i < size; i++){
    target[i] = colours[i]/dimmingfactor ;
  };
};

void ram(void) {
  int dimmingfactor = 10;
  static uint32_t ctr=0;
  ctr++;
  uint8_t pattern[] = {
    0, 0, 0,
    0, 0, 0,

    0, 0, 204,
    0, 51, 204,
    0, 102, 204,
    0, 153, 204,
    0, 204, 204,
    0, 255, 204
  };

  uint8_t green[] = {
    255, 0, 0,
    255, 0, 0,

    255, 0, 0,
    255, 0, 0,
    255, 0, 0,
    255, 0, 0,
    255, 0, 0,
    255, 0, 0
  };
  uint8_t red[] = {
    0, 255, 0,
    0, 255, 0,

    0, 255, 0,
    0, 255, 0,
    0, 255, 0,
    0, 255, 0,
    0, 255, 0,
    0, 255, 0
  };
  uint8_t blue[] = {
    0,0,255,
    0,0,255,

    0,0,255,
    0,0,255,
    0,0,255,
    0,0,255,
    0,0,255,
    0,0,255
  };
  uint8_t off[] = {
    0,0,0,
    0,0,0,

    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0
  };
  uint8_t  dimmer[] = {
    0,0,0,
    0,0,0,

    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0
  };
  getInputWaitRelease();
  SETUPgout(RGB_LED);

  if (lcdShowImageFile("nick.lcd") != 0) {
    lcdClear();
    lcdNl();
    lcdPrintln("File nick.lcd");
    lcdPrintln("not present.");
    lcdNl();
    lcdDisplay();
  }

  getInputWait();
  setTextColor(0xFF,0x00);

  while(1){
    char key = getInputRaw();
    switch(key){
      case BTN_UP:
        dim(dimmer, pattern, sizeof(dimmer), dimmingfactor);
            ws2812_sendarray(dimmer, sizeof(dimmer));
        break;
      case BTN_DOWN:
        dim(dimmer, green, sizeof(dimmer), dimmingfactor);
            ws2812_sendarray(dimmer, sizeof(dimmer));
        break;
      case BTN_LEFT:
        dim(dimmer, red, sizeof(dimmer), dimmingfactor);
            ws2812_sendarray(dimmer, sizeof(dimmer));
        break;
      case BTN_RIGHT:
        dim(dimmer, blue, sizeof(dimmer), dimmingfactor);
            ws2812_sendarray(dimmer, sizeof(dimmer));
        break;
      case BTN_ENTER:
        lcd_deselect();
        ws2812_sendarray(off, sizeof(off));
        return;
    };
  };
    
  return;
  
}
