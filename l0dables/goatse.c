/*
 * Copyright 2015 team rad1o
 *
 * This is a small sample for a l0dable
 *
 */

#include <libopencm3/lpc43xx/gpio.h>
#include <rad1olib/pins.h>
#include <rad1olib/setup.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fonts/smallfonts.c>
#include "usetable.h"

const char *lines[] = {
    "                                               ",
    " /     \\             \\            /    \\       ",
    "|       |             \\          |      |      ",
    "|       `.             |         |       :     ",
    "`        |             |        \\|       |     ",
    " \\       | /       /  \\\\\\   --__ \\\\       :    ",
    "  \\      \\/   _--~~          ~--__| \\     |    ",
    "   \\      \\_-~                    ~-_\\    |    ",
    "    \\_     \\        _.--------.______\\|   |    ",
    "      \\     \\______// _ ___ _ (_(__>  \\   |    ",
    "       \\   .  C ___)  ______ (_(____>  |  /    ",
    "       /\\ |   C ____)/      \\ (_____>  |_/     ",
    "      / /\\|   C_____)       |  (___>   /  \\    ",
    "     |   (   _C_____)\\______/  // _/ /     \\   ",
    "     |    \\  |__   \\\\_________// (__/       |  ",
    "    | \\    \\____)   `----   --'             |  ",
    "    |  \\_          ___\\       /_          _/ | ",
    "   |              /    |     |  \\            | ",
    "   |             |    /       \\  \\           | ",
    "   |          / /    |         |  \\           |",
    "   |         / /      \\__/\\___/    |          |",
    "  |           /        |    |       |         |",
    "  |          |         |    |       |         |",
    "* G O A T S E X * G O A T S E X * G O A T S E X *",
};

void ram(void){
    setIntFont(&Font_3x6);

    lcdNl();
    lcdDisplay();

    lcdSetCrsr(0,8*8);
    for (int i = 0; i < 24; i++) {
        lcdPrintln(lines[i]);
    }
    lcdDisplay();
    delayms(1000000000);
    return;
}
