#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include "usetable.h"

static void redraw(uint8_t brightness);

//# MENU ws2812b
void ram(void){
    uint8_t brightness = 0;

    getInputWaitRelease();

    SETUPgout(RAD1O_RGB_LED);

    while(1){
        lcdClear(0xff);
        lcdPrintln("WS2812B LEDs");
        lcdPrintln("UP: brighter");
        lcdPrintln("DOWN: darker");
        lcdPrintln("ENTER: exit");
        lcdDisplay();

        switch(getInput()){
            case BTN_UP:
                if(brightness < 10)
                    redraw(++brightness);
                break;
            case BTN_DOWN:
                if(brightness > 0)
                    redraw(--brightness);
                break;
            case BTN_LEFT:
                break;
            case BTN_RIGHT:
                break;
            case BTN_ENTER:
                return;
        };
    };
    return;
};

static void redraw(uint8_t brightness){
    uint8_t pattern[] = {
        255, 255, 0,
        255, 255, 0,

        0,   0,   255,
        0,   0,   255,
        0,   0,   255,
        0,   0,   255,
        0,   0,   255,
        255, 0,   0
    };

    for(uint8_t i=0; i<sizeof(pattern); i++){
        pattern[i] = pattern[i] * brightness / 10;
    }

    ws2812_sendarray(pattern, sizeof(pattern));
}
