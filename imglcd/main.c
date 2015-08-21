/*
 * Copyright 2010 - 2012 Michael Ossmann
 *
 * This file is part of HackRF.
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

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/adc.h>

#include <unistd.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include "feldtest.h"
#include <r0ketlib/menu.h>

#define TYPE_CMD    0
#define TYPE_DATA   1

#define LED1_PIN (P4_1)
#define LED1_FUNC (SCU_CONF_FUNCTION0)
#define LED1_GPORT GPIO2
#define LED1_GPIN (GPIOPIN1)

#define RF_EN_PIN (P5_0)
#define RF_EN_FUNC (SCU_CONF_FUNCTION0)
#define RF_EN_GPORT GPIO2
#define RF_EN_GPIN GPIOPIN9

#define OFF(foo) gpio_clear(foo ## _GPORT,foo ## _GPIN);
#define ON(foo) gpio_set(foo ## _GPORT,foo ## _GPIN);
#define GET(foo) gpio_get(foo ## _GPORT,foo ## _GPIN);
void doChrg();
void doLCD();

#define _PIN(pin, func, ...) pin
#define _FUNC(pin, func, ...) func
#define _GPORT(pin, func, gport, gpin, ...) gport
#define _GPIN(pin, func, gport, gpin, ...) gpin
#define _GPIO(pin, func, gport, gpin, ...) gport,gpin
#define _VAL(pin, func, gport, gpin, val, ...) val

#define PASTER(x) gpio_ ## x
#define WRAP(x) PASTER(x)
#define SETUPadc(args...) scu_pinmux(_PIN(args),SCU_CONF_EPUN_DIS_PULLUP|_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args); SCU_ENAIO0|=SCU_ENAIO_ADCx_6;
#define SETUPgin(args...) scu_pinmux(_PIN(args),_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args);
#define SETUPgout(args...) scu_pinmux(_PIN(args),SCU_GPIO_NOPULL|_FUNC(args)); GPIO_DIR(_GPORT(args)) |= _GPIN(args); WRAP( _VAL(args) ) (_GPIO(args));


int main(void)
{
	cpu_clock_init_();
	ssp_clock_init();

	inputInit();

    lcdInit();
    lcdFill(0xff);
	OFF(MIXER_EN);

	static const struct MENU main={ "img test", {
		{ "LCD", &doLCD},
		{NULL,NULL}
	}};
	handleMenu(&main);
	return 0;
}

//#include "fairy2-12.lcd"
#include "camp.lcd"
#include "fairy2-16.lcd"
void lcd_select();
void lcd_deselect();
void lcdWrite(uint8_t cd, uint8_t data);

void doLCD(){
	//	lcdWrite(TYPE_CMD,0x36); lcdWrite(TYPE_DATA,0b11000000);
	int i;
	lcd_select();
	lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,3);
	lcdWrite(TYPE_CMD,0x2C);

	for (i=0;i<img12_len;i++){
		lcdWrite(TYPE_DATA,img12_raw[i]);
	};
	lcd_deselect();
	int ct=0x3a;
	while(1){
		switch(getInput()){
			case BTN_UP:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,5);
				lcdWrite(TYPE_CMD,0x2C);
				for (i=0;i<img16_len;i++){
					lcdWrite(TYPE_DATA,img16_raw[i]);
				};
				lcd_deselect();
				break;
			case BTN_DOWN:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,3);
				lcdWrite(TYPE_CMD,0x2C);

				for (i=0;i<img12_len;i++){
					lcdWrite(TYPE_DATA,img12_raw[i]);
				};
				lcd_deselect();
				break;
			case BTN_LEFT:
				ct-=1;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, ct);
//				lcdWrite(TYPE_CMD,0x20);
				lcd_deselect();
				break;
			case BTN_RIGHT:
				ct+=1;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, ct);
//				lcdWrite(TYPE_CMD,0x21);
				lcd_deselect();
				break;
			case BTN_ENTER:
				return;
				break;
		};
	};
};
