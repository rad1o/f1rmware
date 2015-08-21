/*
 * Copyright 2012 Michael Ossmann <mike@ossmann.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
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

#include <rad1olib/setup.h>
#include <rad1olib/systick.h>
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>
#include <stdint.h>
#include <common/si5351c.h>

#define WAIT_CPU_CLOCK_INIT_DELAY   (10000)

uint8_t _cpu_speed=0;

void delayNop(uint32_t duration)
{
	uint32_t i;

	for (i = 0; i < duration; i++)
		__asm__("nop");
}

void cpuClockInit(void) {
	/* initialisation similar to UM10503 v1.9 sec. 13.2.1.1 */
	CGU_BASE_M4_CLK = (CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_IRC) | CGU_BASE_M4_CLK_AUTOBLOCK(1));

	/* Enable XTAL */
	CGU_XTAL_OSC_CTRL &= ~(CGU_XTAL_OSC_CTRL_HF_MASK|CGU_XTAL_OSC_CTRL_ENABLE_MASK);
	delayNop(WAIT_CPU_CLOCK_INIT_DELAY); /* should be 250us / 3000 cycles @ 12MhZ*/

	/* Set PLL1 up for 204 MHz */
	CGU_PLL1_CTRL= CGU_PLL1_CTRL_CLK_SEL(CGU_SRC_XTAL)
				| CGU_PLL1_CTRL_MSEL(17-1)
				| CGU_PLL1_CTRL_NSEL(0)
				| CGU_PLL1_CTRL_AUTOBLOCK(1)
				| CGU_PLL1_CTRL_PSEL(0)
				| CGU_PLL1_CTRL_DIRECT(1)
				| CGU_PLL1_CTRL_FBSEL(1)
				| CGU_PLL1_CTRL_BYPASS(0)
				| CGU_PLL1_CTRL_PD(0)
				;
	/* Wait for PLL Lock */
	while (!(CGU_PLL1_STAT & CGU_PLL1_STAT_LOCK_MASK));

	/* set DIV B to 102 MHz */
	CGU_IDIVB_CTRL= CGU_IDIVB_CTRL_CLK_SEL(CGU_SRC_PLL1)
		| CGU_IDIVB_CTRL_AUTOBLOCK(1) 
		| CGU_IDIVB_CTRL_IDIV(2-1)
		| CGU_IDIVB_CTRL_PD(0)
		;
	_cpu_speed=102;

	/* use DIV B as main clock */
	/* This means, that possible speeds in MHz are:
	 * 204 102 68 51 40.8 34 29.14 25.5 22.66 20.4 18.54 17 15.69 14.57 13.6 12.75
	 */

	CGU_BASE_M4_CLK = (CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_IDIVB) | CGU_BASE_M4_CLK_AUTOBLOCK(1));

	delayNop(WAIT_CPU_CLOCK_INIT_DELAY); /* should be 50us / 5100 @ 102MhZ */
};

void ssp_clock_init(void) {
	/* set DIV C to 40.8 MHz */
	CGU_IDIVC_CTRL= CGU_IDIVC_CTRL_CLK_SEL(CGU_SRC_PLL1)
		| CGU_IDIVC_CTRL_AUTOBLOCK(1) 
		| CGU_IDIVC_CTRL_IDIV(5-1)
		| CGU_IDIVC_CTRL_PD(0)
		;

	/* use DIV C as SSP1 base clock */
	CGU_BASE_SSP1_CLK = (CGU_BASE_SSP1_CLK_CLK_SEL(CGU_SRC_IDIVC) | CGU_BASE_SSP1_CLK_AUTOBLOCK(1));
};

/* Warning: changing from < 102MHz to >102 MHz in one step hangs */
#define MAX_MHZ 204
void cpu_clock_set(uint32_t target_mhz){ // rounds up
	uint8_t divider= MAX_MHZ / target_mhz;

	if (divider==0)
		divider=1;

	if (divider>16) /* max value of DIVB */
		divider=16;

	if(divider==1 && _cpu_speed<102){ // Do not go to 204 in one step
		cpu_clock_set(102);
		delayNop(WAIT_CPU_CLOCK_INIT_DELAY);
	};

	CGU_IDIVB_CTRL= CGU_IDIVB_CTRL_CLK_SEL(CGU_SRC_PLL1)
		| CGU_IDIVB_CTRL_AUTOBLOCK(1) 
		| CGU_IDIVB_CTRL_IDIV(divider-1)
		| CGU_IDIVB_CTRL_PD(0)
		;
	_cpu_speed=MAX_MHZ/divider; /* target_mhz might've been rounded down */
//	systickAdjustFreq(MAX_MHZ*1000000/divider);
}

/* setup USB clock */ 
void usb_clock_init(void)
{
	/* use XTAL_OSC as clock source for PLL0USB */
	CGU_PLL0USB_CTRL = CGU_PLL0USB_CTRL_PD(1)
			| CGU_PLL0USB_CTRL_AUTOBLOCK(1)
			| CGU_PLL0USB_CTRL_CLK_SEL(CGU_SRC_XTAL);
	while (CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_MASK);

	/* configure PLL0USB to produce 480 MHz clock from 12 MHz XTAL_OSC */
	/* Values from User Manual v1.4 Table 94, for 12MHz oscillator. */
	CGU_PLL0USB_MDIV = 0x06167FFA;
	CGU_PLL0USB_NP_DIV = 0x00302062;
	CGU_PLL0USB_CTRL |= (CGU_PLL0USB_CTRL_PD(1)
			| CGU_PLL0USB_CTRL_DIRECTI(1)
			| CGU_PLL0USB_CTRL_DIRECTO(1)
			| CGU_PLL0USB_CTRL_CLKEN(1));

	/* power on PLL0USB and wait until stable */
	CGU_PLL0USB_CTRL &= ~CGU_PLL0USB_CTRL_PD_MASK;
	while (!(CGU_PLL0USB_STAT & CGU_PLL0USB_STAT_LOCK_MASK));

	/* use PLL0USB as clock source for USB0 */
	CGU_BASE_USB0_CLK = CGU_BASE_USB0_CLK_AUTOBLOCK(1)
			| CGU_BASE_USB0_CLK_CLK_SEL(CGU_SRC_PLL0USB);
}

/* rest of hackrf clock startup
Configure PLL1 to max speed (204MHz).
Note: PLL1 clock is used by M4/M0 core, Peripheral, APB1. */ 
void hackrf_clock_init(void)
{
	/* Switch peripheral clock over to use PLL1 (204MHz) */
	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK(1)
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB1 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB3 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_PLL1);
}

void si5351_init(void){
	i2c0_init(255); 

	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources();
	si5351c_configure_pll_multisynth();

//	/* MS3/CLK3 is the source for the external clock output. */
//	si5351c_configure_multisynth(3, 80*128-512, 0, 1, 0); /* 800/80 = 10MHz */

	/* MS5/CLK5 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(5, 16*128-512, 0, 1, 0); /* 800/16 = 50MHz */

	/* MS4/CLK4 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(4, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS6/CLK6 is unused. */
	/* MS7/CLK7 is the source for the LPC43xx microcontroller. */
	uint8_t ms7data[] = { 90, 255, 20, 0 };
	si5351c_write(ms7data, sizeof(ms7data));

	si5351c_set_clock_source(PLL_SOURCE_XTAL);
	// soft reset
	uint8_t resetdata[] = { 177, 0xac };
	si5351c_write(resetdata, sizeof(resetdata));
	si5351c_enable_clock_outputs();
};
