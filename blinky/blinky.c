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

#define PORT_LED1 GPIO1
#define PIN_LED1 (1<<11)

int main(void)
{
	int i;
	cpu_clock_init();
//	cpu_clock_pll1_max_speed();
    // Config LED as out
	scu_pinmux(P2_11,SCU_GPIO_NOPULL|SCU_CONF_FUNCTION0);
	GPIO1_DIR |= PIN_LED1;

    lcdInit();
    lcdFill(0xff);
	lcdDisplay();

	/* Blink LED1 on the board. */
	while (1) 
	{
		gpio_set(PORT_LED1, PIN_LED1); /* LED off */
		delay(2000000);
		gpio_clear(PORT_LED1, PIN_LED1); /* LED on */
		delay(2000000);
	}

	return 0;
}


