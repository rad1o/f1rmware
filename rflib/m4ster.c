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

static void hex2(uint8_t* buf, uint32_t v, int pos) {
    if(pos == 8) return;
    const int d = (v & 0xF0000000) >> 28;
    *buf = (d > 9) ? 'A'-10+d : '0'+d;
    hex2(buf+1, v<<4, pos+1);
}

static void cdc_send(uint8_t* msg, int len) {
    if(vcom_connected()) vcom_write(msg, len);
}

#define MAX_CLIENTS 10
#define CLIENT_LIFETIME 50
//# MENU m4ster
void m4ster() {
    uint16_t x = 0;
    uint16_t y = 0;
    uint32_t regcount = 0;
    uint32_t btntime = 0;
    uint32_t id = 0;
    uint16_t free_slot = 0;
    static dp4d_pkg pkg;
    static dp4d_client clients[MAX_CLIENTS];

    memset(clients, 0, MAX_CLIENTS*sizeof(dp4d_client));

    cpu_clock_set(204);

    rflib_init();
    rflib_set_freq(FREQSTART);

    lcdPrintln("send USB/CDC");
    lcdPrintln("initialization");
    lcdPrintln("now...");
    rflib_lcdDisplay();

    getInputWaitRelease();
    systick_interrupt_disable();

    CDCenable();

    bool have_init = false;
    while(!have_init) {
        if(vcom_connected()) {
            /* check if we got data from USB-CDC, check sequence, if so. */
            uint8_t buf[2];
            uint32_t read = vcom_bread(buf, 2);
            if(read == 2 && buf[0] == 'G' && buf[1] == 'O') have_init = true;
        }
    }
    
    rflib_bfsk_init();
    rflib_bfsk_receive();

    while(1) {
        for(int i=0; i<MAX_CLIENTS; i++) {
            /* Poll notice: */
            pkg.version = 0x49;
            pkg.id = clients[i].id;
            pkg.x = 0xFFFF/100 * 10; // 10% probability of client answer (0x28F5C28=0xFFFFFFFF/100)
            TOGGLE(LED3);
            rflib_bfsk_transmit((uint8_t*)&pkg, sizeof(pkg), true);
            /* wait for client reply: */
            delayNop(6*40000); // about 6ms?
            int pkg_len = rflib_bfsk_get_packet((uint8_t*)&pkg, sizeof(pkg));
            if(pkg_len == sizeof(pkg)) {
                /* handle client reply */
                if(pkg.version != 0x49) continue;
                pkg.version = 0;
                if(clients[i].id == 0) {
                    clients[i].id = pkg.id;
                    pkg.version = 1;
                }
                clients[i].lifetime = CLIENT_LIFETIME;
                cdc_send((uint8_t*)&pkg, sizeof(pkg));
            } else {
                /* otherwise check for client kickout timer */
                if(clients[i].id != 0) {
                    if(clients[i].lifetime == 0) {
                        pkg.version = 2;
                        pkg.id = clients[i].id;
                        cdc_send((uint8_t*)&pkg, sizeof(pkg));
                        clients[i].id = 0;
                    } else {
                        clients[i].lifetime--;
                    }
                }
            }
        }
    }
}
