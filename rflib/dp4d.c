#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <r0ketlib/config.h>

#include <rad1olib/pins.h>

#include <rad1olib/systick.h>

#include <libopencm3/cm3/systick.h>

#include "../rflib/rflib_m0.h"

#include <lpcapi/cdc/cdc_main.h>
#include <lpcapi/cdc/cdc_vcom.h>

#include <string.h>
#include <stdlib.h>

#include "dp4d.h"

// default to 2399 MHz
#define FREQSTART 2399000000

//# MENU dp4d
void dp4d() {
    dp4d_pkg pkg;

    uint16_t x = 0;
    uint16_t y = 0;
    uint32_t regcount = 0;
    uint16_t btn = 0;
    uint32_t id = 0;

    cpu_clock_set(204);

    rflib_init();
    rflib_set_freq(FREQSTART);
    // first, we let the frequency detector
    // work on some noise or stuff, and use that to initialize
    // the pseudorandom number generator
    rflib_set_rxsamplerate(1000000);
    rflib_set_rxdecimation(2);
    rflib_set_rxbandwidth(1750000);
    rflib_freq_receive();
    int seed_bits = 0;
    int rand_seed = 0;
    while(seed_bits < 32) {
        int16_t freq = 0;
        if(rflib_get_data(&freq, 2) > 0) {
            rand_seed |= ((freq & 0x100) > 0 ? 1 : 0);
            rand_seed <<= 1;
            seed_bits++;
        }
    }
    rflib_standby();
    srand(rand_seed);
    id = rand();

    lcdPrintln("your dp4d ID:");
    lcdPrintln(IntToStr(id, 8, F_HEX));
    rflib_lcdDisplay();

    systick_interrupt_disable();

    rflib_bfsk_init();
    rflib_bfsk_receive();

    while(1) {
        switch (getInputRaw()) {
            case BTN_UP:
                y++;
                break;
            case BTN_DOWN:
                y--;
                break;
            case BTN_RIGHT:
                x++;
                break;
            case BTN_LEFT:
                x--;
                break;
            case BTN_ENTER:
                btn = 0xFFF;
                break;
        }

        // Poll for BFSK packets
        int pkg_len = rflib_bfsk_get_packet((uint8_t*)&pkg, sizeof(pkg));
        if(pkg_len == sizeof(pkg)) {
            TOGGLE(LED3);
            delayNop(10000); // about 250us?
            if(pkg.version != 0x49) goto cont;
            if(pkg.id == id) {
                // we're queried, answer as fast as possible
                ON(LED4);
                regcount = 3000;
                pkg.x = x;
                pkg.y = y;
                pkg.button = btn>>4;
                rflib_bfsk_transmit((uint8_t*)&pkg, sizeof(pkg), true);
            }
            // if not registered, answer to queries for new dp4d clients
            if((regcount == 0) && (pkg.id == 0) && (pkg.x > (rand()>>16))) {
                pkg.id = id;
                pkg.x = x;
                pkg.y = y;
                pkg.button = 0;
                regcount = 500; // only try once every sec
                rflib_bfsk_transmit((uint8_t*)&pkg, sizeof(pkg), true);
            }
        }
cont:
        delayNop(80000); // about 2ms?
        if(btn > 0) btn--;
        if(regcount > 0) {
            regcount--;
        } else {
            OFF(LED4);
        }
    }

    rflib_shutdown();
    return;
}
