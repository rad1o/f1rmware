/* l0ungel1cht
 *
 * Copyright muCCC 2015
 * schneider, andz, chris007
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
#include <r0ketlib/config.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/pins.h>

#include <libopencm3/lpc43xx/uart.h>

#include "../rflib/rflib_m0.h"

#include "l0ungel1cht.h"

#include <string.h>

// default to 2395 MHz
#define FREQSTART 2395000000

const uint8_t NUM_LEDS = 8;
uint8_t led_divider = 0x2;

/* ------------------------------------------------------------------- */

#define MAX_LED_FRAMES 50
#define BUF_SIZE 3*8*MAX_LED_FRAMES+2

unsigned char leds[BUF_SIZE];
unsigned int frames = 0;
unsigned int ctr = 0;
unsigned int framectr = 0;

void readRgbLedFile(void) {
    int size = getFileSize(GLOBAL(ledfile));
    frames = 0;
    ctr = 0;
    framectr = 0;
    if(size > 0) {
        if(size >= BUF_SIZE)
            size = BUF_SIZE;
        readFile(GLOBAL(ledfile), (char*)leds, size);
        frames = (size-2)/(3*8);
    }
}

void init_rgbLeds(void) {
    readTextFile("ledfile.cfg",GLOBAL(ledfile),FLEN);
    readRgbLedFile();
}

/**************************************************************************/

// Wrapper for Tx Method, switching back to Rx after Tx automatically
void transmit(uint8_t *data, uint16_t len) {
    rflib_bfsk_transmit(data, len, true);
}

void set_single_led(uint8_t *pattern, int index, uint8_t r, uint8_t g, uint8_t b)
{
    pattern[index*3+0] = r >> led_divider;
    pattern[index*3+1] = g >> led_divider;
    pattern[index*3+2] = b >> led_divider;
}

void set_all_leds(uint8_t *pattern, uint8_t r, uint8_t g, uint8_t b)
{
    int i;
    for(i = 0; i < NUM_LEDS; i++) {
        set_single_led(pattern, i, r, g, b);
    }
}

void clear_leds(void)
{
    uint8_t pattern[NUM_LEDS * 3];    
    memset(pattern, 0, sizeof(pattern));
    ws2812_sendarray(pattern, sizeof(pattern));
}

void display_nick(uint8_t nickCol, uint8_t lcdCol)
{
    uint8_t dx;
    uint8_t dy;

    dx=DoString(0,0,GLOBAL(nickname));
    dx=(RESX-dx)/2;
    if(dx<0)
        dx=0;
    dy=(RESY-getFontHeight())/2;
    setExtFont(GLOBAL(nickfont));
    setTextColor(lcdCol,nickCol);
    lcdFill(lcdCol);
    lcdSetCrsr(dx,dy);
    lcdPrint(GLOBAL(nickname));
    rflib_lcdDisplay();
}

void decode(uint8_t* const rx_pkg, uint8_t const len)
{    
    uint8_t pattern[NUM_LEDS * 3];
    uint8_t lcdCol      = 0x00;
    uint8_t nickCol     = 0xFF;

    uint8_t type_id     = rx_pkg[0];
    uint8_t payload_len = rx_pkg[1];
    uint8_t r, g, b, led_no;

    switch (type_id) {
      case 0x10: //All LEDs off
            clear_leds();
            break;
      case 0x11: //All LEDs same Color
            r      = rx_pkg[2];
            g      = rx_pkg[3];
            b      = rx_pkg[4];
            set_all_leds(pattern,
                        g,
                        r,
                        b);
            lcdCol = (RGB_TO_8BIT(r,
                                  g,
                                  b));
            nickCol = (RGB_TO_8BIT((0xFF-r),
                                   (0xFF-g),
                                   (0xFF-b)));
            display_nick(nickCol, lcdCol);
            ws2812_sendarray(pattern, sizeof(pattern));
        break;
      case 0x12: //Set Display Color
            r      = rx_pkg[2];
            g      = rx_pkg[3];
            b      = rx_pkg[4];
            lcdCol = (RGB_TO_8BIT(r,
                                  g,
                                  b));
            nickCol = (RGB_TO_8BIT((0xFF-r),
                                   (0xFF-g),
                                   (0xFF-b)));
            display_nick(nickCol, lcdCol);
        break;
      case 0x13: //Set Animation No
            //Not implemented yet
        break;
      case 0x14: //Set Display Animation No
            //Not implemented yet
        break;
      case 0x15: //Set one LED
            led_no = rx_pkg[2];
            r = rx_pkg[3];
            g = rx_pkg[4];
            b = rx_pkg[5];
            set_single_led(pattern,
                           led_no,
                           g,
                           r,
                           b);
            ws2812_sendarray(pattern, sizeof(pattern));
        break;
      case 0x1D: //set All LEDs diffrent
            set_all_leds(pattern,0,0,0); //all Off
            for(uint8_t i = 0; i < payload_len/3; i++){
                set_single_led(pattern,
                               i,
                               rx_pkg[3+(3*i)],
                               rx_pkg[2+(3*i)],
                               rx_pkg[4+(3*i)]);
            }
            ws2812_sendarray(pattern, sizeof(pattern));
        break;
  }
}

void serial_handler(uint8_t data)
{
    static uint8_t buf[128];
    static int index = 0;
    static bool sync = false;

    buf[index] = data;

    if(index == 1) {
        // Test for "!!" sync chars
        if(buf[0] == '!' && buf[1] == '!') {
            sync = true;
        } else {
            sync = false;
            index = -1;
        }
    }
    
    if(sync && index == 3) {
        // Check for protocol validity (typeId and payload length)
        if((buf[2] >> 4) != 1 ||  buf[3] > 24) {
            sync = false;
            index = -1;
        }
    }

    if(sync && index > 3) {
        if(index == buf[3] + 5) { //All data received (!!+Header+PayloadLen+\n\r)
            if(buf[index] == '\r' || buf[index] == '\n') { //CRLF at the End
                // Leave out "!!" sync chars
                transmit(buf+2, buf[3]+2);
                //Show the Pattern on the master
                decode(buf+2, buf[3]+2);                
            }
            sync = false;
            index = -1;
        }
    }

    index += 1;
    if(index == sizeof(buf)) {
        index = 0;
    }
}

//# MENU l0ungel1cht
void l0ungel1cht() {

    // Only needed when started from menu (and not as app)
    //getInputWaitRelease();

    clear_leds();
    init_rgbLeds();

    cpu_clock_set(204);

    rflib_init();
    rflib_bfsk_init();
    rflib_set_freq(FREQSTART);
    rflib_bfsk_receive();

    uart_init(UART0_NUM, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE, 111, 0, 1);

    bool button_action = false;

    bool running = true;
    while(running) {
        //MENU
        switch (getInputRaw()) {
            case BTN_UP:
                if(!button_action)
                {
                    if(led_divider > 0) led_divider--;
                    button_action = true;
                }
                break;
            case BTN_DOWN:
                if(!button_action)
                {
                    if(led_divider < 8) led_divider++;
                    button_action = true;
                }
                break;
            case BTN_RIGHT:
                // Nothing to do here
                break;
            case BTN_LEFT:
                // Nothing to do here
                break;
            case BTN_ENTER:
                // Not needed when running as app
                //running = false;
                break;
            default:
                button_action = false;
                break;
        }
        //END MENU

        // Poll for BFSK packets
        uint8_t rx_pkg[256];
        int rx_pkg_len = rflib_bfsk_get_packet(rx_pkg, 255);
        if(rx_pkg_len > 0) {
            const uint8_t HEADER_LEN = 2;

            uint8_t type_id     = rx_pkg[0];
            uint8_t payload_len = rx_pkg[1];
            // Check validity of Packet Len vs. Payload Len
            if(rx_pkg_len > HEADER_LEN 
                && payload_len == rx_pkg_len - HEADER_LEN) {
                // Check if within valid TypeId Range and Maximum Length
                if((type_id == 0x10 || 1) && payload_len <= NUM_LEDS*3) {
                  decode(rx_pkg, rx_pkg_len);
                }
            }
        }
        if(uart_rx_data_ready(UART0_NUM) == UART_RX_DATA_READY) {
            uint8_t data = uart_read(UART0_NUM);
            serial_handler(data);
        }
    }

    rflib_shutdown();
    // \todo Clear LEDs
    OFF(EN_1V8);
    OFF(EN_VDD);
    return;
}
