/* send samples over USB/CDC ACMi
 *
 * An example application using RFlib
 * (c) 2015 Hans-Werner Hilse (@hilse)
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

static void receive() {
    static int16_t rxbuf[256];
    int rx = rflib_get_data(rxbuf, 256);
    if(rx > 0) {
        if(vcom_connected()) vcom_write((uint8_t*)rxbuf, rx);
    }
}

//# MENU USBraw
void usbraw_menu() {
    lcdClear();
    lcdPrintln("ENTER to go back");
    rflib_lcdDisplay();
    getInputWaitRelease();

    cpu_clock_set(204);
    CDCenable();

    rflib_init();
    rflib_set_freq(FREQSTART);
    rflib_set_rxsamplerate(1000000);
    rflib_set_rxdecimation(2);
    rflib_set_rxbandwidth(1750000);
    rflib_raw_receive();

    while(1) {
        switch (getInputRaw()) {
            case BTN_ENTER:
                goto stop;
        }
#if 0
        if(vcom_connected()) {
            /* check if we got data from USB-CDC, transmit it, if so. */
            uint8_t sendbuf[255];
            uint32_t read = vcom_bread(sendbuf, 255);
            if(read > 0) rflib_bfsk_transmit(sendbuf, read, true);
        }
#endif
        receive();
    }
stop:
    rflib_shutdown();
    return;
}
