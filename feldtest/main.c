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
#include "feldtest.h"

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
	cpu_clock_init();
//	cpu_clock_pll1_max_speed();
	scu_pinmux(RF_EN_PIN,SCU_GPIO_NOPULL|RF_EN_FUNC);
	GPIO_DIR(RF_EN_GPORT) |= RF_EN_GPIN;
//	gpio_clear(RF_EN_GPORT, RF_EN_GPIN); /* RF off */
	gpio_set(RF_EN_GPORT, RF_EN_GPIN); /* RF on */

    // Config LED as out
	scu_pinmux(LED1_PIN,SCU_GPIO_NOPULL|LED1_FUNC);
	GPIO_DIR(LED1_GPORT) |= LED1_GPIN;

	inputInit();
	feldInit();

    lcdInit();
    lcdFill(0xff);
	setSystemFont();
	char tu=0,td=0,tl=0,tr=0;
	char page=0;
	char tu2=0,td2=0,tl2=0,tr2=0;

	int ctr=0;
	int k=0;
	/* Blink LED1 on the board. */
	while (1) 
	{
		lcdClear(0xff);
		lcdPrintln("Feld-Test v5");
		lcdPrintln("");
		lcdPrint("Page ");lcdPrintln(IntToStr(page,2,0));
		if (page==0){
			lcdPrint(IntToStr(tu,2,F_HEX)); lcdPrintln(" Up   BY_MIX");
			lcdPrint(IntToStr(td,2,F_HEX)); lcdPrintln(" Down BY_AMP");
			lcdPrint(IntToStr(tl,2,F_HEX)); lcdPrintln(" Left TX_RX");
			lcdPrint(IntToStr(tr,2,F_HEX)); lcdPrintln(" Right LOW_HIGH_FILT");
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					tu=1-tu;
					if (tu){
						gpio_set(BY_MIX_GPORT,BY_MIX_GPIN);
					}else{
						gpio_clear(BY_MIX_GPORT,BY_MIX_GPIN);
					};
					break;
				case BTN_DOWN:
					td=1-td;
					if (td){
						gpio_set(BY_AMP_GPORT,BY_AMP_GPIN);
						gpio_clear(BY_AMP_N_GPORT,BY_AMP_N_GPIN);
					}else{
						gpio_clear(BY_AMP_GPORT,BY_AMP_GPIN);
						gpio_set(BY_AMP_N_GPORT,BY_AMP_N_GPIN);
					};
					break;
				case BTN_LEFT:
					tl=1-tl;
					if (tl){
						gpio_set(TX_RX_GPORT,TX_RX_GPIN);
						gpio_clear(TX_RX_N_GPORT,TX_RX_N_GPIN);
					}else{
						gpio_clear(TX_RX_GPORT,TX_RX_GPIN);
						gpio_set(TX_RX_N_GPORT,TX_RX_N_GPIN);
					};
					break;
				case BTN_RIGHT:
					tr=1-tr;
					if (tr){
						gpio_set(LOW_HIGH_FILT_GPORT,LOW_HIGH_FILT_GPIN);
						gpio_clear(LOW_HIGH_FILT_N_GPORT,LOW_HIGH_FILT_N_GPIN);
					}else{
						gpio_clear(LOW_HIGH_FILT_GPORT,LOW_HIGH_FILT_GPIN);
						gpio_set(LOW_HIGH_FILT_N_GPORT,LOW_HIGH_FILT_N_GPIN);
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};
		}else if (page==1){
			lcdPrint(IntToStr(tu2,2,F_HEX)); lcdPrintln(" Up   ");
			lcdPrint(IntToStr(td2,2,F_HEX)); lcdPrintln(" Down TX_AMP|~RX_LNA");
			lcdPrint(IntToStr(tl2,2,F_HEX)); lcdPrintln(" Left CS_VCO");
			lcdPrint(IntToStr(tr2,2,F_HEX)); lcdPrintln(" Right ");
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					tu2=1-tu2;
					if (tu2){
						;
					}else{
						;
					};
					break;
				case BTN_DOWN:
					td2=1-td2;
					if (td2){
						gpio_set(TX_AMP_GPORT,TX_AMP_GPIN);
						gpio_clear(RX_LNA_GPORT,RX_LNA_GPIN);
					}else{
						gpio_clear(TX_AMP_GPORT,TX_AMP_GPIN);
						gpio_set(RX_LNA_GPORT,RX_LNA_GPIN);
					};
					break;
				case BTN_LEFT:
					tl2=1-tl2;
					if (tl2){
						gpio_set(CS_VCO_GPORT,CS_VCO_GPIN);
					}else{
						gpio_clear(CS_VCO_GPORT,CS_VCO_GPIN);
					};
					break;
				case BTN_RIGHT:
					tr2=1-tr2;
					if (tr2){
						;
					}else{
						;
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};

		};
		if (page>1){page=0;}

		gpio_set(LED1_GPORT, LED1_GPIN); /* LED off */
		delay(200000);
		gpio_clear(LED1_GPORT, LED1_GPIN); /* LED on */
		delay(200000);

		ctr++;
		lcdNl();
		lcdPrint(IntToStrX(ctr,4));
	}

	return 0;
}


