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
	int i;
	cpu_clock_init();
//	cpu_clock_pll1_max_speed();
    // Config LED as out
	scu_pinmux(RF_EN_PIN,SCU_GPIO_NOPULL|RF_EN_FUNC);
	GPIO_DIR(RF_EN_GPORT) |= RF_EN_GPIN;
	gpio_clear(RF_EN_GPORT, RF_EN_GPIN); /* RF off */

	scu_pinmux(LED1_PIN,SCU_GPIO_NOPULL|LED1_FUNC);
	GPIO_DIR(LED1_GPORT) |= LED1_GPIN;

    lcdInit();
    lcdFill(0xff);
	lcdSetPixel(5,5,3);
	lcdSetPixel(5,15,3);
	lcdSetPixel(5,25,3);
	setSystemFont();
	lcdPrintln("Hallo Welt");
	lcdDisplay(); 

	int ctr=0;
	/* Blink LED1 on the board. */
	while (1) 
	{
		ctr++;
		lcdPrint(IntToStrX(ctr,4));
		lcdSetCrsrX(0);
		lcdDisplay(); 
		gpio_set(LED1_GPORT, LED1_GPIN); /* LED off */
		delay(2000000);
		gpio_clear(LED1_GPORT, LED1_GPIN); /* LED on */
		delay(2000000);
	}

	return 0;
}


