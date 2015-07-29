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
#include <libopencm3/lpc43xx/dac.h>
#include <libopencm3/lpc43xx/adc.h>

#include <rad1olib/setup.h>
#include <rad1olib/pins.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>


//# MENU dac
void dac_menu(){
	int i;
	uint32_t sample;
	cpu_clock_set(204);
	getInputWaitRelease();
	lcdPrintln("DAC Echo test");
	lcdDisplay();

	SETUPgout(MIC_AMP_DIS);
	OFF(MIC_AMP_DIS); // Enable AMP
	dac_init(false); 

	while (getInputRaw()==BTN_NONE) {
	    dac_set(adc_get_single(ADC0,ADC_CR_CH7)>>1); 
	};
}
