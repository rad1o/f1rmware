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

#include "setup.h"
#include "display.h"
#include "print.h"
#include "itoa.h"
#include "keyin.h"

#define LED1_PIN (P4_1)
#define LED1_FUNC (SCU_CONF_FUNCTION0)
#define LED1_GPORT GPIO2
#define LED1_GPIN (GPIOPIN1)

#define RF_EN_PIN (P5_0)
#define RF_EN_FUNC (SCU_CONF_FUNCTION0)
#define RF_EN_GPORT GPIO2
#define RF_EN_GPIN GPIOPIN9

int main(void)
{
	cpu_clock_init_();
//	cpu_clock_pll1_max_speed();
	scu_pinmux(RF_EN_PIN,SCU_GPIO_NOPULL|RF_EN_FUNC);
	GPIO_DIR(RF_EN_GPORT) |= RF_EN_GPIN;
	gpio_clear(RF_EN_GPORT, RF_EN_GPIN); /* RF off */

    // Config LED as out
	scu_pinmux(LED1_PIN,SCU_GPIO_NOPULL|LED1_FUNC);
	GPIO_DIR(LED1_GPORT) |= LED1_GPIN;

	inputInit();

    lcdInit();
    lcdFill(0xff);
	setSystemFont();
	char tu=0,td=0,tl=0,tr=0,tm=0;
	char led=0;
	lcdPrintln("Sec-Test v2");
	lcdPrintln("");

	int ctr=0;
	int k=0;
	/* Blink LED1 on the board. */
	while (1) 
	{
		lcdSetCrsr(0,16);
		lcdPrint(IntToStr(tu,2,F_HEX)); lcdPrintln(" Up");
		lcdPrint(IntToStr(td,2,F_HEX)); lcdPrintln(" Down");
		lcdPrint(IntToStr(tl,2,F_HEX)); lcdPrintln(" Left");
		lcdPrint(IntToStr(tr,2,F_HEX)); lcdPrintln(" Right");
		lcdPrint(IntToStr(tm,2,F_HEX)); lcdPrintln(" Enter");
		lcdDisplay(); 
		switch(getInput()){
			case BTN_UP:
				tu=1-tu;
				if (tu){
				}else{
				};
				break;
			case BTN_DOWN:
				td=1-td;
				if (td){
				}else{
				};
				break;
			case BTN_LEFT:
				tl=1-tl;
				if (tl){
				}else{
				};
				break;
			case BTN_RIGHT:
				tr=1-tr;
				if (tr){
				}else{
				};
				break;
			case BTN_ENTER:
				tm=1-tm;
				if (tm){
				}else{
				};
				break;
		};

		led=1-led;
		if (led){
			gpio_set(LED1_GPORT, LED1_GPIN); /* LED on */
		}else{
			gpio_clear(LED1_GPORT, LED1_GPIN); /* LED off */
//			delayNop(200000);
		};

		ctr++;
		lcdNl();
		lcdPrint(IntToStrX(ctr,4));
	}

	return 0;
}


