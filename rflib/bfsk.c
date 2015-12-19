/* BPSK RX/TX
 *
 * Copyright (C) 2015 Hans-Werner Hilse <hwhilse@gmail.com>
 *
 * some parts (receive/filters) are
 *   Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>

#include <rad1olib/pins.h>

#include <lpcapi/cdc/cdc_main.h>
#include <lpcapi/cdc/cdc_vcom.h>
#include <string.h>

#include "rflib_m0.h"

// default to 2496 MHz
#define FREQSTART 2496000000

static void transmit(char *str) {
    rflib_bfsk_transmit((uint8_t*)str, strlen(str), true);
    getInputWaitRelease();
}

static void receive() {
    static uint8_t rxbuf[256];
    int rx = rflib_bfsk_get_packet(rxbuf, 255);
    if(rx > 0) {
        rxbuf[rx] = '\0';
        lcdPrintln((char*)rxbuf);
        lcdDisplay();
        /* also: write to USB-CDC */
        if(vcom_connected()) vcom_write((uint8_t*)rxbuf, rx);
    }
}

//# MENU BPSK
void bfsk_menu() {
    lcdClear();
    lcdPrintln("ENTER to go back");
    lcdPrintln("L/R/U/D to xmit");
    lcdDisplay();
    getInputWaitRelease();

    cpu_clock_set(204);
    CDCenable();

    rflib_init();
    rflib_bfsk_init();
    rflib_set_freq(FREQSTART);
    rflib_bfsk_receive();

    while(1) {
        switch (getInputRaw()) {
            case BTN_UP:
                transmit("up");
                break;
            case BTN_DOWN:
                transmit("down");
                break;
            case BTN_RIGHT:
                transmit("right");
                break;
            case BTN_LEFT:
                transmit("left");
                break;
            case BTN_ENTER:
                goto stop;
        }
        if(vcom_connected()) {
            /* check if we got data from USB-CDC, transmit it, if so. */
            uint8_t sendbuf[255];
            uint32_t read = vcom_bread(sendbuf, 255);
            if(read > 0) rflib_bfsk_transmit(sendbuf, read, true);
        }
        receive();
    }
stop:
    rflib_shutdown();
    return;
}
