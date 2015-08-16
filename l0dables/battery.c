#include <rad1olib/setup.h>
#include <rad1olib/battery.h>
#include <rad1olib/draw.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include "usetable.h"

void drawCommonThings(int c);

void ram(void){
    getInputWaitRelease();

    uint8_t old_state=-1;
    uint8_t charging_count=0;

    while(1){
        uint32_t mv = batteryGetVoltage();

        if (batteryCharging()) {
            if(!(charging_count % 50)){
                drawCommonThings(true);
                lcdPrintln("   Charging ...");

                if(charging_count>=50)  drawRectFill(16, 65, 23, 27, 0b11100000);
                if(charging_count>=100) drawRectFill(40, 65, 23, 27, 0b11110000);
                if(charging_count>=150) drawRectFill(64, 65, 23, 27, 0b10011100);
                if(charging_count>=200) drawRectFill(88, 65, 23, 27, 0b00011100);
                old_state = 0;
            }

            if(++charging_count > 250) charging_count=0;
            delayms(5);
        } else if (mv<3550 && old_state != 1){
            drawCommonThings(false);
            lcdPrintln("    Charge NOW!");
            drawRectFill(16, 65, 5, 27, 0b11100000);
            old_state = 1;
        }else if (mv<3650 && mv>=3550 && old_state != 2){
            drawCommonThings(false);
            lcdPrintln("    Charge soon");
            drawRectFill(16, 65, 23, 27, 0b11100000);
            old_state = 2;
        }else if (mv<4000 && mv>=3650 && old_state != 3){
            drawCommonThings(false);
            lcdPrintln("        OK");
            drawRectFill(16, 65, 23, 27, 0b11100000);
            drawRectFill(40, 65, 23, 27, 0b11110000);
            old_state = 3;
        }else if (mv<4120 && mv>=4000 && old_state != 4){
            drawCommonThings(false);
            lcdPrintln("       Good");
            drawRectFill(16, 65, 23, 27, 0b11100000);
            drawRectFill(40, 65, 23, 27, 0b11110000);
            drawRectFill(64, 65, 23, 27, 0b10011100);
            old_state = 4;
        }else if (mv>=4120 && old_state != 5) {
            drawCommonThings(false);
            lcdPrintln("       Full");
            drawRectFill(16, 65, 23, 27, 0b11100000);
            drawRectFill(40, 65, 23, 27, 0b11110000);
            drawRectFill(64, 65, 23, 27, 0b10011100);
            drawRectFill(88, 65, 23, 27, 0b00011100);
            old_state = 5;
        }

        // print voltage
        lcdSetCrsr(0, 100);
        uint8_t v = mv/1000;
        lcdPrint("      ");
        lcdPrint(IntToStr(v,2,0));
        lcdPrint(".");
        lcdPrint(IntToStr(mv%1000, 3, F_ZEROS | F_LONG));
        lcdPrintln("V");

        lcdDisplay();

        switch(getInput()){
            case BTN_LEFT:
                return;
            case BTN_ENTER:
                return;
        };
    };
}

void drawCommonThings(int charging) {
    lcdClear();
    lcdNl();
    lcdPrintln("  Battery status");

    // Draw battery frame.
    drawHLine( 63,  14, 112, 0b00000011);
    drawHLine( 93,  14, 112, 0b00000011);
    drawVLine( 14,  63,  93, 0b00000011);
    drawVLine(112,  63,  73, 0b00000011);
    drawVLine(112,  83,  93, 0b00000011);
    drawHLine( 73, 112, 116, 0b00000011);
    drawHLine( 83, 112, 116, 0b00000011);
    drawVLine(116,  73,  83, 0b00000011);

    // Print if not charging.
    lcdSetCrsr(0, 40);
    if(!charging){
        lcdPrintln("  (not charging)");
    };
    lcdSetCrsr(0, 30);
}
