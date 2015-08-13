#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include "usetable.h"

//# MENU ws2812b
void ram(void){
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

    uint8_t black[] = {
        0, 0, 0,
        0, 0, 0,

        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
    };

    getInputWaitRelease();

    SETUPgout(RGB_LED);

    while(1){
        lcdClear(0xff);
        lcdPrintln("WS2812B LEDs");
        lcdPrintln("UP: pattern");
        lcdPrintln("DOWN: black");
        lcdPrintln("ENTER: exit");
        lcdDisplay();

        switch(getInput()){
            case BTN_UP:
                ws2812_sendarray(pattern, sizeof(pattern));
                break;
            case BTN_DOWN:
                ws2812_sendarray(black, sizeof(black));
                break;
            case BTN_LEFT:
                return;
            case BTN_RIGHT:
                break;
            case BTN_ENTER:
                return;
        };
    };
    return;
};
