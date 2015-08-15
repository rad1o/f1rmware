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
static void blink_shift();

uint8_t pattern[] = {
        255, 255, 0,
        255, 0, 255,
        0,   128,   255,
        128,   128,   128,
        0,   255,  255,
        255,   255,   0,
        0,   0,   255,
        255, 0,   0
    };



//# MENU ws2812b
void ram(void){
    uint8_t brightness = 5;

    getInputWaitRelease();

    SETUPgout(RGB_LED);

    while(1){
        lcdClear(0xff);
        lcdPrintln("WS2812B LEDs");
        lcdPrintln("UP: brighter");
        lcdPrintln("DOWN: darker");
        lcdPrintln("RIGHT: blink");
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
            case BTN_RIGHT:
                    while(getInput()==BTN_NONE) {
                        blink_shift();
                     }
                break;
            case BTN_ENTER:
                return;
        };
    };
    return;
};

static void redraw(uint8_t brightness){

    for(uint8_t i=0; i<sizeof(pattern); i++){
        pattern[i] = pattern[i] * brightness / 10;
    }

    ws2812_sendarray(pattern, sizeof(pattern));
}

static void blink_shift(){

    int pattern_size = sizeof(pattern);
    uint8_t swap[] = { pattern[0], pattern[1], pattern[2], };

    for(uint8_t i=0; i<pattern_size; i = i+3){
            pattern[i] = pattern[i+3];
            pattern[i+1] = pattern[i+4];
            pattern[i+2] = pattern[i+5];
    }
    pattern[pattern_size-3] = swap[0];
    pattern[pattern_size-2] = swap[1];
    pattern[pattern_size-1] = swap[2];

    lcdDisplay();

    ws2812_sendarray(pattern, sizeof(pattern));
}
