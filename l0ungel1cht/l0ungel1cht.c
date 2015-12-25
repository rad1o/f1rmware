/* L0ungeL1cht
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
#include <r0ketlib/config.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/pins.h>

#include <libopencm3/lpc43xx/uart.h>
/*
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/tuning.h>
#include <common/max2837.h>
#include <common/streaming.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>

#include <libopencm3/cm3/vector.h>

#include <stddef.h>
#include <portalib/arm_intrinsics.h>
#include <portalib/complex.h>
#include <portalib/fxpt_atan2.h>

#include <lpcapi/cdc/cdc_main.h>
#include <lpcapi/cdc/cdc_vcom.h>

#include "cossin1024.h"
*/
#include "l0ungel1cht.h"

#include "../rflib/rflib_m0.h"

#include <string.h>
// default to 2395 MHz
#define FREQSTART 2395000000

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

/**************************************************************************/

void senddata(uint8_t *data, uint16_t len) {
    rflib_bfsk_transmit(data, len, true);
}


void init_rgbLeds(void) {
	readTextFile("ledfile.cfg",GLOBAL(ledfile),FLEN);
	readRgbLedFile();
}

void tx_rgbLeds(void) {
	if(GLOBAL(rgbleds)) {
		if(frames > 0) {
			if(ctr == 0) {
                senddata(&leds[framectr*3*8+2], 3*8);
				framectr++;
				if(framectr >= frames)
					framectr = 0;
			}

			ctr++;
			// LED delay is in leds[0:1]
			if(ctr >= ((leds[0]<<8) + leds[1]))
				ctr = 0;
		}
	}
	return;
}

const uint8_t nleds = 8;

void set_led(uint8_t *pattern, int index, uint8_t r, uint8_t g, uint8_t b)
{
    pattern[index*3+0] = r;
    pattern[index*3+1] = g;
    pattern[index*3+2] = b;
}

void animation_tx(void)
{
    tx_rgbLeds();
}

void set_all(uint8_t *pattern, uint8_t r, uint8_t g, uint8_t b)
{
    int i;
    for(i = 0; i < nleds; i++) {
        set_led(pattern, i, r, g, b);
    }
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

void serial_handler(uint8_t data)
{
    static uint8_t buf[128];
    static int index = 0;
    static bool sync = false;
    static uint8_t lcdCol = 0x00;
    static uint8_t nickCol = 0xFF;
    static uint8_t lcdBrightness = 0x00;
    uint8_t pattern[nleds * 3];
    uint8_t led_bright = 4;


    buf[index] = data;

    if(index == 1) {
        if(buf[0] == '!' && buf[1] == '!') {
            sync = true;
        } else {
            sync = false;
            index = -1;
        }
    }

    if(sync && index == 3) {
        if((buf[2] >> 4) != 1 ||  buf[3] > 24) {
            sync = false;
            index = -1;
        }
    }

    if(sync && index > 3) {

        if(index == buf[3] + 5) { //All data received
            if(buf[index] == '\r' || buf[index] == '\n') { //CRLF at the End

                senddata(buf + 2, buf[3] + 2);  //SendData

                //Show the Pattern on the master
                switch (buf[2]) {
                    case 0x10: //All LEDs off
                      set_all(pattern, 0, 0, 0);
                      ws2812_sendarray(pattern, sizeof(pattern));
                      break;
                    case 0x11: //All LEDs same Color
                      set_all(pattern, buf[5]/led_bright, buf[4]/led_bright, buf[6]/led_bright);
                      lcdCol = (RGB_TO_8BIT(buf[4],buf[5],buf[6]));
                      nickCol = (RGB_TO_8BIT((0xFF-buf[4]),(0xFF-buf[5]),(0xFF-buf[6])));
                      display_nick(nickCol, lcdCol);
                      ws2812_sendarray(pattern, sizeof(pattern));
                      break;
                    case 0x12: //Set Display Color
                      lcdCol = (RGB_TO_8BIT(buf[4],buf[5],buf[6]));
                      nickCol = (RGB_TO_8BIT((0xFF-buf[4]),(0xFF-buf[5]),(0xFF-buf[6])));
                      display_nick(nickCol, lcdCol);
                      break;
                    case 0x13: //Set Animation No
                      //Not implemented yet
                      break;
                    case 0x14: //Set Display Animation No
                        //Not implemented yet
                      break;
                    case 0x15: //Set one LED
                      set_led(pattern, buf[4], buf[6]/led_bright, buf[5]/led_bright, buf[7]/led_bright);
                      ws2812_sendarray(pattern, sizeof(pattern));
                      break;
                    case 0x1D: //set All LEDs diffrent
                      set_all(pattern,0,0,0); //all Off
                      for(uint8_t i = 0; i < buf[3]/3; i++){
                        set_led(pattern, i, buf[5+(3*i)]/led_bright,buf[4+(3*i)]/led_bright,buf[6+(3*i)]/led_bright);
                      }
                      ws2812_sendarray(pattern, sizeof(pattern));
                      break;
                }
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
    uint8_t pattern[nleds * 3];
    int i = 0;
    int j = 0;
    int dx=0;
    int dy=0;
    static uint8_t lcdCol = 0x00;
    static uint8_t nickCol = 0xFF;
    uint8_t led_bright = 4;

    //Testmode
    static uint8_t test[128];

    getInputWaitRelease();

    memset(pattern, 0, sizeof(pattern));
    ws2812_sendarray(pattern, sizeof(pattern));

    init_rgbLeds();

    cpu_clock_set(204);

    rflib_init();
    rflib_bfsk_init();
    rflib_set_freq(FREQSTART);
    rflib_bfsk_receive();

    uart_init(UART0_NUM, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE, 111, 0, 1);

    while(1) {
       //MENU
       switch (getInputRaw()) {
           case BTN_UP:
               if(led_bright > 1) led_bright--;
               else if (led_bright == 0) led_bright = 16;
               break;
           case BTN_DOWN:
               if(led_bright < 16) led_bright++;
               else if(led_bright == 16) led_bright = 0;
               break;
           case BTN_RIGHT:
           ///TEST
               test[0]=0x11;
               test[1]=0x3;
               test[2]=0xFF;
               test[3]=0x00;
               test[4]=0x80;
               senddata(test, 5);
               break;
           case BTN_LEFT:
           ///TEST
               test[0]=0x11;
               test[1]=0x3;
               test[2]=0x00;
               test[3]=0xFF;
               test[4]=0x80;
               senddata(test, 5);
               break;
           case BTN_ENTER:
               break;
       }
       getInputWaitRelease();
       //END MENU

       uint8_t rx_pkg[256];
       int rx_pkg_len = rflib_bfsk_get_packet(rx_pkg, 255);

        if(rx_pkg_len > 0) {
            if(rx_pkg_len > 2 && rx_pkg[1] == rx_pkg_len - 2) {
                if((rx_pkg[0] == 0x10 || 1) && rx_pkg[1] <= 24) {
                  switch (rx_pkg[0]) {
                      case 0x10: //All LEDs off
                        set_all(pattern, 0, 0, 0);
                        ws2812_sendarray(pattern, sizeof(pattern));
                        break;
                      case 0x11: //All LEDs same Color
                        set_all(pattern, rx_pkg[3]/led_bright, rx_pkg[2]/led_bright, rx_pkg[4]/led_bright);
                        lcdCol = (RGB_TO_8BIT(rx_pkg[2],rx_pkg[3],rx_pkg[4]));
                        nickCol = (RGB_TO_8BIT((0xFF-rx_pkg[2]),(0xFF-rx_pkg[3]),(0xFF-rx_pkg[4])));
                        display_nick(nickCol, lcdCol);
                        ws2812_sendarray(pattern, sizeof(pattern));
                        break;
                      case 0x12: //Set Display Color
                        lcdCol = (RGB_TO_8BIT(rx_pkg[2],rx_pkg[3],rx_pkg[4]));
                        nickCol = (RGB_TO_8BIT((0xFF-rx_pkg[2]),(0xFF-rx_pkg[3]),(0xFF-rx_pkg[4])));
                        display_nick(nickCol, lcdCol);
                        break;
                      case 0x13: //Set Animation No
                        //Not implemented yet
                        break;
                      case 0x14: //Set Display Animation No
                          //Not implemented yet
                        break;
                      case 0x15: //Set one LED
                        set_led(pattern, rx_pkg[2], rx_pkg[4]/led_bright, rx_pkg[3]/led_bright, rx_pkg[5]/led_bright);
                        ws2812_sendarray(pattern, sizeof(pattern));
                        break;
                      case 0x1D: //set All LEDs diffrent
                        set_all(pattern,0,0,0); //all Off
                        for(uint8_t i = 0; i < rx_pkg[1]/3; i++){
                          set_led(pattern, i, rx_pkg[3+(3*i)]/led_bright,rx_pkg[2+(3*i)]/led_bright,rx_pkg[4+(3*i)]/led_bright);
                        }
                        ws2812_sendarray(pattern, sizeof(pattern));
                        break;
                  }
                }
            }
        }
        if(uart_rx_data_ready(UART0_NUM) == UART_RX_DATA_READY) {
            uint8_t data = uart_read(UART0_NUM);
            serial_handler(data);
        }
    }
stop:
    rflib_shutdown();
    OFF(EN_1V8);
    OFF(EN_VDD);
    return;
}
