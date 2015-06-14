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

#include <unistd.h>

#include "setup.h"
#include "display.h"
#include "print.h"
#include "itoa.h"
#include "keyin.h"
#include "feldtest.h"
#include "menu.h"
#include "mixer.h"
#include "si5351c.h"

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
void doFeld();

int main(void)
{
	cpu_clock_init();

	i2c0_init(255);
	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources();
	si5351c_configure_pll_multisynth();

	/* MS3/CLK3 is the source for the external clock output. */
	si5351c_configure_multisynth(3, 80*128-512, 0, 1, 0); /* 800/80 = 10MHz */

	/* MS4/CLK4 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(4, 16*128-512, 0, 1, 0); /* 800/16 = 50MHz */
 
 	/* MS5/CLK5 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(5, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS6/CLK6 is unused. */
	/* MS7/CLK7 is the source for the LPC43xx microcontroller. */
	uint8_t ms7data[] = { 90, 255, 20, 0 };
	si5351c_write(ms7data, sizeof(ms7data));

	si5351c_set_clock_source(PLL_SOURCE_XTAL);
	// soft reset
	uint8_t resetdata[] = { 177, 0xac };
	si5351c_write(resetdata, sizeof(resetdata));
	si5351c_enable_clock_outputs();


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
	OFF(MIXER_EN);
	setSystemFont();

	static const struct MENU main={ "main 1", {
		{ "feld", &doFeld},
		{ "chrg", &doChrg},
		{NULL,NULL}
	}};
	handleMenu(&main);
	return 0;
}

void doChrg(){
#define _PIN(pin, func, gport, gpin, ...) pin
#define _FUNC(pin, func, gport, gpin, ...) func
#define _GPORT(pin, func, gport, gpin, ...) gport
#define _GPIN(pin, func, gport, gpin, ...) gpin
#define _GPIO(pin, func, gport, gpin, ...) gport,gpin
#define _VAL(pin, func, gport, gpin, val, ...) val

#define PASTER(x) gpio_ ## x
#define WRAP(x) PASTER(x)
#define SETUPgin(args...) scu_pinmux(_PIN(args),_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args);
#define SETUPgout(args...) scu_pinmux(_PIN(args),SCU_GPIO_NOPULL|_FUNC(args)); GPIO_DIR(_GPORT(args)) |= _GPIN(args); WRAP( _VAL(args) ) (_GPIO(args));

// Pull: SCU_GPIO_NOPULL, SCU_GPIO_PDN, SCU_GPIO_PUP

	/* input */
#define BC_DONE      PD_16, SCU_GPIO_PUP|SCU_CONF_FUNCTION4, GPIO6, GPIOPIN30  // Charge Complete Output (active low)
	/*output */
#define BC_CEN       PA_3,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN10, clear        // Active-Low Charger Enable Input
#define BC_PEN2      PA_4,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN19, set          // Input Limit Control 2. (100mA/475mA)
#define BC_USUS      PD_12, SCU_CONF_FUNCTION4, GPIO6, GPIOPIN26, clear        // (active low) USB Suspend Digital Input (disable charging)
#define BC_THMEN     P8_8,  SCU_CONF_FUNCTION4, GPIO6, GPIOPIN26, clear        // Thermistor Enable Input
	/* input */
#define BC_IND       PD_11, SCU_GPIO_PUP|SCU_CONF_FUNCTION4, GPIO6, GPIOPIN25  // (active low) Charger Status Output
#define BC_OT        P8_7,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN7   // (active low) Battery Overtemperature Flag
#define BC_DOK       P8_6,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN6   // (active low) DC Power-OK Output
#define BC_UOK       P8_5,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN5   // (active low) USB Power-OK Output
#define BC_FLT       P8_4,  SCU_GPIO_PUP|SCU_CONF_FUNCTION0, GPIO4, GPIOPIN4   // (active low) Fault Output

SETUPgin(BC_DONE);
SETUPgout(BC_CEN);
SETUPgout(BC_PEN2);
SETUPgout(BC_USUS);
SETUPgout(BC_THMEN);
SETUPgin(BC_IND);
SETUPgin(BC_OT);
SETUPgin(BC_DOK);
SETUPgin(BC_UOK);
SETUPgin(BC_FLT);

#define GETg(x...) gpio_get(_GPIO(x))

char ct=0;

	while(1){
		lcdClear(0xff);
		lcdPrintln("Charger-Test v1");
		lcdPrintln("");

		lcdPrint("BC_DONE ");lcdPrint(IntToStr(GETg(BC_DONE),1,F_HEX));lcdNl();
		lcdPrint("BC_IND  ");lcdPrint(IntToStr(GETg(BC_IND ),1,F_HEX));lcdNl();
		lcdPrint("BC_OT   ");lcdPrint(IntToStr(GETg(BC_OT  ),1,F_HEX));lcdNl();
		lcdPrint("BC_DOK  ");lcdPrint(IntToStr(GETg(BC_DOK ),1,F_HEX));lcdNl();
		lcdPrint("BC_UOK  ");lcdPrint(IntToStr(GETg(BC_UOK ),1,F_HEX));lcdNl();
		lcdPrint("BC_FLT  ");lcdPrint(IntToStr(GETg(BC_FLT ),1,F_HEX));lcdNl();
		lcdNl();

		lcdPrint("U BC_CEN   ");lcdPrint(IntToStr(GETg(BC_CEN  ),1,F_HEX));lcdNl();
		lcdPrint("D BC_PEN2  ");lcdPrint(IntToStr(GETg(BC_PEN2 ),1,F_HEX));lcdNl();
		lcdPrint("L BC_USUS  ");lcdPrint(IntToStr(GETg(BC_USUS ),1,F_HEX));lcdNl();
		lcdPrint("R BC_THMEN ");lcdPrint(IntToStr(GETg(BC_THMEN),1,F_HEX));lcdNl();
		lcdDisplay(); 
#define TOGg(x) gpio_toggle(_GPIO(x))
#define OFFg(x...) gpio_clear(_GPIO(x))
#define ONg(x...) gpio_set(_GPIO(x))
//#define TOGg(x...) if (GETg(x)) {OFFg(x); }else {ONg(x);}
		switch(getInput()){
			case BTN_UP:
				TOGg(BC_CEN);
				break;
			case BTN_DOWN:
				TOGg(BC_PEN2);
				break;
			case BTN_LEFT:
				TOGg(BC_USUS);
				break;
			case BTN_RIGHT:
				TOGg(BC_THMEN);
				break;
			case BTN_ENTER:
				return;
				break;
		};
	};
	return;
};

void doFeld(){
	char tu=0,td=0,tl=0,tr=0;
	char tuN=0,tdN=0,tlN=0,trN=0;
	char page=0;
	char tu2=0,td2=0,td2N=0,tl2=0,tr2=0;
	uint16_t sf=3500;
	char tu3=0,td3=0,tl3=0,tr3=0;

	int ctr=0;
	/* Blink LED1 on the board. */

	while (1) 
	{


		lcdClear(0xff);
		lcdPrintln("Feld-Test v11");
		lcdPrintln("");
		lcdPrint("Page ");lcdPrintln(IntToStr(page,2,0));
		if (page==0){
			tu=GET(BY_MIX); tuN=GET(BY_MIX_N);
			lcdPrint(IntToStr(tu,1,F_HEX)); lcdPrint(IntToStr(tuN,1,F_HEX)); lcdPrintln(" Up BY_MIX/_N");
			td=GET(BY_AMP); tdN=GET(BY_AMP_N);
			lcdPrint(IntToStr(td,1,F_HEX)); lcdPrint(IntToStr(tdN,1,F_HEX)); lcdPrintln(" Dn BY_AMP/_N");
			tl=GET(TX_RX);tlN=GET(TX_RX_N);
			lcdPrint(IntToStr(tl,1,F_HEX)); lcdPrint(IntToStr(tlN,1,F_HEX)); lcdPrintln(" Lt TX_RX/_N");
			tr=GET(LOW_HIGH_FILT); trN=GET(LOW_HIGH_FILT_N);
			lcdPrint(IntToStr(tr,1,F_HEX)); lcdPrint(IntToStr(trN,1,F_HEX)); lcdPrintln(" Rt LOW_HIGH_FILT/_N");
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					tu=1-tu;
					if (tu){
						OFF(BY_MIX_N);
						ON(BY_MIX);
					}else{
						OFF(BY_MIX);
						ON(BY_MIX_N);
					};
					break;
				case BTN_DOWN:
					td=1-td;
					if (td){
						OFF(BY_AMP_N);
						ON(BY_AMP);
					}else{
						OFF(BY_AMP);
						ON(BY_AMP_N);
					};
					break;
				case BTN_LEFT:
					tl=1-tl;
					if (tl){
						OFF(TX_RX_N);
						ON(TX_RX);
					}else{
						OFF(TX_RX);
						ON(TX_RX_N);
					};
					break;
				case BTN_RIGHT:
					tr=1-tr;
					if (tr){
						OFF(LOW_HIGH_FILT_N);
						ON(LOW_HIGH_FILT);
					}else{
						OFF(LOW_HIGH_FILT);
						ON(LOW_HIGH_FILT_N);
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};
		}else if (page==1){
			lcdPrint("  "); lcdPrintln(" Up   ");
			td2=GET(RX_LNA);td2N=GET(TX_AMP);
			lcdPrint(IntToStr(td2,1,F_HEX)); lcdPrint(IntToStr(td2N,1,F_HEX)); lcdPrintln(" Dn RX/TX");
			lcdPrint(IntToStr(tl2,1,F_HEX)); lcdPrint(" ");                    lcdPrintln(" Lt MIXER_EN");
			lcdPrint(IntToStr(tr2,2,F_HEX)); lcdPrintln(" Rt SI-clkdis");
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
					td2=td2+2*td2N;
					td2++;
					td2=td2%3;
					switch (td2){
						case(0):
							OFF(RX_LNA);
							OFF(TX_AMP);
							break;
						case(1):
							ON(RX_LNA);
							OFF(TX_AMP);
							break;
						case(2):
							OFF(RX_LNA);
							ON(TX_AMP);
							break;
					};
					break;
				case BTN_LEFT:
					tl2=1-tl2;
					if (tl2){
						ON(MIXER_EN);
					}else{
						OFF(MIXER_EN);
					};
					break;
				case BTN_RIGHT:
					tr2=1-tr2;
					if (tr2){
						si5351c_power_down_all_clocks();
					}else{
						si5351c_set_clock_source(PLL_SOURCE_XTAL);
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};

		}else if (page==2){
			lcdPrint("SF: ");lcdPrint(IntToStr(sf,4,F_ZEROS)); lcdPrintln(" MHz");
			tl3=GET(CS_VCO);
			lcdPrint(IntToStr(tl3,1,F_HEX)); lcdPrint(" "); lcdPrintln(" Lt CS_VCO");
			lcdPrint("  "); lcdPrintln(" Rt ");
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					sf+=100;
					mixer_set_frequency(sf);
					break;
				case BTN_DOWN:
					sf-=100;
					mixer_set_frequency(sf);
					break;
				case BTN_LEFT:
					tl3=1-tl3;
					if (tl3){
						ON(CS_VCO);
						mixer_setup();
						mixer_set_frequency(sf);
					}else{
						OFF(CS_VCO);
					};
					break;
				case BTN_RIGHT:
					tr3=1-tr3;
					if (tr3){
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
		if (page>2){page=0;}

		gpio_set(LED1_GPORT, LED1_GPIN); /* LED off */
		delay(200000);
		gpio_clear(LED1_GPORT, LED1_GPIN); /* LED on */
		delay(200000);

		ctr++;
		lcdNl();
		lcdPrint(IntToStrX(ctr,4));
	}
};

