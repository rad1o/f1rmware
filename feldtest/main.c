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
#include <libopencm3/lpc43xx/spifi.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/ccu.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>

#include <unistd.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include "feldtest.h"
#include <r0ketlib/menu.h>
#include <common/mixer.h>
#include <common/si5351c.h>

#include <rad1olib/spi-flash.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>
#include <common/w25q80bv.h>

#include <r0ketlib/select.h>
#include <r0ketlib/fs_util.h>
#include <fatfs/ff.h>

void doChrg();
void doADC();
void doFeld();
void doFlash();
void doSpeed();
void doLCD();
void doMSC();
void doFS();

void sys_tick_handler(void){
	incTimer();
};

void si_en(){
	//	i2c0_init(255); // is default here
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
};

int main(void) {
	cpu_clock_init_();
	ssp_clock_init();

	systickInit();

//	cpu_clock_pll1_max_speed();

	SETUPgout(EN_VDD);
	SETUPgout(MIXER_EN);

	SETUPgout(LED1);
	SETUPgout(LED2);
	SETUPgout(LED3);
	SETUPgout(LED4);

	inputInit();
	feldInit();

    lcdInit();
	fsInit(); 
    lcdFill(0xff);

	static const struct MENU main={ "main 1", {
		{ "FS", &doFS},
		{ "MSC", &doMSC},
		{ "flash", &doFlash},
		{ "LCD", &doLCD},
		{ "speed", &doSpeed},
		{ "ADC", &doADC},
		{ "feld", &doFeld},
		{ "chrg", &doChrg},
		{NULL,NULL}
	}};
	handleMenu(&main);
	return 0;
}

void turnoff ( volatile uint32_t *foo){
	(*foo)|= (1<<1); // AUTO = 1
	(*foo)&= ~(1<<0); // RUN=0
};
void clkoff (volatile uint32_t * foo){
	(*foo) |= (1<<0);
};

void doFS(){
	char filename[FLEN]; // ???
	FATFS FatFs;

	int res;
	lcdPrint("Mount:");
	res=f_mount(&FatFs,"/",0);
	lcdPrintln(IntToStr(res,3,0));

	lcdDisplay();

	selectFile(filename,"TXT");
};

void doMSC(){
//	cpu_clock_set(204);
	dwim();
};

void doSpeed(){
	SETUPgout(LCD_BL_EN);
	SETUPgout(EN_1V8);
	SETUPgout(LED4);
	int mhz=102;
	while(1){
		TOGGLE(LED1);
		lcdClear(0xff);
		lcdPrint("speed: "); lcdPrint(IntToStr(mhz,3,0));lcdNl();
		lcdDisplay(); 
		switch(getInput()){
			case BTN_UP:
//				mhz=204;
//				cpu_clock_set(mhz);
#define PD0_SLEEP0_HW_ENA MMIO32(0x40042000)
#define PD0_SLEEP0_MODE   MMIO32(0x4004201C)

		PD0_SLEEP0_HW_ENA = 1; 
		PD0_SLEEP0_MODE = 0x003000AA;
		SCB_SCR|=SCB_SCR_SLEEPDEEP;

		ON(LED1);
		CGU_BASE_M4_CLK = (CGU_BASE_M4_CLK_CLK_SEL(CGU_SRC_IRC) | CGU_BASE_M4_CLK_AUTOBLOCK(1));
		CGU_PLL1_CTRL= CGU_PLL1_CTRL_PD(1);
		CGU_PLL0USB_CTRL= CGU_PLL1_CTRL_PD(1);
		CGU_PLL0AUDIO_CTRL= CGU_PLL1_CTRL_PD(1);

		CGU_XTAL_OSC_CTRL &= ~(CGU_XTAL_OSC_CTRL_ENABLE_MASK);



#define __WFI() __asm__("wfi")
		while(1){
			TOGGLE(LED1);
			__WFI();
		};
				break;
			case BTN_DOWN:
				mhz=12;
				cpu_clock_set(mhz);
				break;
			case BTN_LEFT:
				while(1){
					cpu_clock_set(102);
					TOGGLE(LED1);
					delayNop(1000);
					cpu_clock_set(12);
					TOGGLE(LED1);
					delayNop(1000);
				};
				break;
			case BTN_RIGHT:
				TOGGLE(LCD_BL_EN);
						OFF(BY_MIX_N);
						OFF(BY_MIX);
						OFF(BY_AMP_N);
						OFF(BY_AMP);
						OFF(TX_RX_N);
						OFF(TX_RX);
						OFF(LOW_HIGH_FILT_N);
						OFF(LOW_HIGH_FILT);
							OFF(TX_AMP);
							OFF(RX_LNA);
						OFF(MIXER_EN);
						OFF(CE_VCO);
//				cpu_clock_set(mhz);
				break;
			case BTN_ENTER:

//			turnoff(&CCU1_CLK_APB3_BUS_CFG);
				turnoff(&CCU1_CLK_APB3_I2C1_CFG);
				turnoff(&CCU1_CLK_APB3_DAC_CFG);
				turnoff(&CCU1_CLK_APB3_ADC0_CFG);
				turnoff(&CCU1_CLK_APB3_ADC1_CFG);
				turnoff(&CCU1_CLK_APB3_CAN0_CFG);
//			turnoff(&CCU1_CLK_APB1_BUS_CFG);
				turnoff(&CCU1_CLK_APB1_MOTOCONPWM_CFG);
				turnoff(&CCU1_CLK_APB1_I2C0_CFG);
				turnoff(&CCU1_CLK_APB1_I2S_CFG);
				turnoff(&CCU1_CLK_APB1_CAN1_CFG);
				turnoff(&CCU1_CLK_SPIFI_CFG);
//				turnoff(&CCU1_CLK_M4_BUS_CFG);
				turnoff(&CCU1_CLK_M4_SPIFI_CFG);
//				turnoff(&CCU1_CLK_M4_GPIO_CFG);
				turnoff(&CCU1_CLK_M4_LCD_CFG);
				turnoff(&CCU1_CLK_M4_ETHERNET_CFG);
				turnoff(&CCU1_CLK_M4_USB0_CFG);
				turnoff(&CCU1_CLK_M4_EMC_CFG);
				turnoff(&CCU1_CLK_M4_SDIO_CFG);
				turnoff(&CCU1_CLK_M4_DMA_CFG);
//				turnoff(&CCU1_CLK_M4_M4CORE_CFG);
				turnoff(&CCU1_CLK_M4_SCT_CFG);
				turnoff(&CCU1_CLK_M4_USB1_CFG);
				turnoff(&CCU1_CLK_M4_EMCDIV_CFG);
				turnoff(&CCU1_CLK_M4_M0APP_CFG);
				turnoff(&CCU1_CLK_M4_VADC_CFG);
				turnoff(&CCU1_CLK_M4_WWDT_CFG);
				turnoff(&CCU1_CLK_M4_USART0_CFG);
				turnoff(&CCU1_CLK_M4_UART1_CFG);
				turnoff(&CCU1_CLK_M4_SSP0_CFG);
				turnoff(&CCU1_CLK_M4_TIMER0_CFG);
				turnoff(&CCU1_CLK_M4_TIMER1_CFG);
//				turnoff(&CCU1_CLK_M4_SCU_CFG);
//				turnoff(&CCU1_CLK_M4_CREG_CFG);
				turnoff(&CCU1_CLK_M4_RITIMER_CFG);
				turnoff(&CCU1_CLK_M4_USART2_CFG);
				turnoff(&CCU1_CLK_M4_USART3_CFG);
				turnoff(&CCU1_CLK_M4_TIMER2_CFG);
				turnoff(&CCU1_CLK_M4_TIMER3_CFG);
//			turnoff(&CCU1_CLK_M4_SSP1_CFG);
				turnoff(&CCU1_CLK_M4_QEI_CFG);
//				turnoff(&CCU1_CLK_PERIPH_BUS_CFG);
				turnoff(&CCU1_CLK_PERIPH_CORE_CFG);
				turnoff(&CCU1_CLK_PERIPH_SGPIO_CFG);
				turnoff(&CCU1_CLK_USB0_CFG);
				turnoff(&CCU1_CLK_USB1_CFG);
//			turnoff(&CCU1_CLK_SPI_CFG);
				turnoff(&CCU1_CLK_VADC_CFG);
				turnoff(&CCU2_CLK_APLL_CFG);
				turnoff(&CCU2_CLK_APB2_USART3_CFG);
				turnoff(&CCU2_CLK_APB2_USART2_CFG);
				turnoff(&CCU2_CLK_APB0_UART1_CFG);
				turnoff(&CCU2_CLK_APB0_USART0_CFG);
//			turnoff(&CCU2_CLK_APB2_SSP1_CFG);
				turnoff(&CCU2_CLK_APB0_SSP0_CFG);
				turnoff(&CCU2_CLK_SDIO_CFG);

// clkoff(& CGU_BASE_SAFE_CLK);
clkoff(& CGU_BASE_USB0_CLK);
 clkoff(& CGU_BASE_PERIPH_CLK);
clkoff(& CGU_BASE_USB1_CLK);
// clkoff(& CGU_BASE_M4_CLK);
clkoff(& CGU_BASE_SPIFI_CLK);
clkoff(& CGU_BASE_SPI_CLK);
clkoff(& CGU_BASE_PHY_RX_CLK);
clkoff(& CGU_BASE_PHY_TX_CLK);
 clkoff(& CGU_BASE_APB1_CLK);
 clkoff(& CGU_BASE_APB3_CLK);
clkoff(& CGU_BASE_LCD_CLK);
clkoff(& CGU_BASE_VADC_CLK);
clkoff(& CGU_BASE_SDIO_CLK);
clkoff(& CGU_BASE_SSP0_CLK);
 clkoff(& CGU_BASE_SSP1_CLK);
clkoff(& CGU_BASE_UART0_CLK);
clkoff(& CGU_BASE_UART1_CLK);
clkoff(& CGU_BASE_UART2_CLK);
clkoff(& CGU_BASE_UART3_CLK);
clkoff(& CGU_BASE_AUDIO_CLK);
 clkoff(& CGU_BASE_CGU_OUT0_CLK);
 clkoff(& CGU_BASE_CGU_OUT1_CLK);

//				return;
				break;
		};
	};
};

void doFlash(){
	uint32_t addr=0x0;
	uint32_t sw=0x1;
	uint8_t data[256];
	uint8_t x=0;

	flashInit();

	lcdClear(0xff);
	lcdPrint("xxd @ "); lcdPrint(IntToStr(addr,8,F_HEX));lcdNl();
	lcdPrint("      "); lcdPrint(IntToStr(sw,8,F_HEX));lcdNl();
	lcdDisplay();

	while(1){
		TOGGLE(LED1);
		switch(getInput()){
			case BTN_UP:
				/* addr-=sw;
				lcdClear(0xff);
				lcdPrint("xxd @ "); lcdPrint(IntToStr((uint32_t)addr,8,F_HEX));lcdNl();
				lcdPrint("      "); lcdPrint(IntToStr(sw,8,F_HEX));lcdNl();
				lcdDisplay(); */
				flash_write_enable();
				lcdPrint("WE done.");
				lcdDisplay();
				break;
			case BTN_DOWN:
				addr+=sw;
				lcdClear(0xff);
				lcdPrint("xxd @ "); lcdPrint(IntToStr((uint32_t)addr,8,F_HEX));lcdNl();
				lcdPrint("      "); lcdPrint(IntToStr(sw,8,F_HEX));lcdNl();
				lcdDisplay();
				break;
			case BTN_LEFT:
				/*sw<<=1;
				lcdClear(0xff);
				lcdPrint("xxd @ "); lcdPrint(IntToStr((uint32_t)addr,8,F_HEX));lcdNl();
				lcdPrint("      "); lcdPrint(IntToStr(sw,8,F_HEX));lcdNl();
				lcdDisplay(); */
				lcdPrint(IntToStr(flash_status1(),2,F_HEX));
				lcdPrint(" ");
				lcdPrint(IntToStr(flash_status2(),2,F_HEX));
				lcdNl();lcdDisplay();
				break;
			case BTN_RIGHT:
				/*sw>>=1;
				lcdClear(0xff);
				lcdPrint("xxd @ "); lcdPrint(IntToStr((uint32_t)addr,8,F_HEX));lcdNl();
				lcdPrint("      "); lcdPrint(IntToStr(sw,8,F_HEX));lcdNl();
				lcdDisplay(); */
				data[0]=0xfe;
				data[1]=0xf8;
				flash_program(addr,0x2,data);
				lcdPrint("done.");
				lcdNl();
				lcdDisplay(); 
				break;
			case BTN_ENTER:
				lcdClear(0xff);
				lcdPrint("xxd @ "); lcdPrint(IntToStr(addr,8,F_HEX));lcdNl();
				lcdPrint("      "); lcdPrint(IntToStr(sw,8,F_HEX));lcdNl();

				flash_read(addr,0x100,data);

				int ctr;
				for (ctr=0x00;ctr<0x024;ctr++){
					if (ctr%4==0){
						lcdNl();
						lcdPrint(IntToStr(ctr,2,F_HEX));
						lcdPrint(":");
					};
					lcdPrint(" ");
					lcdPrint(IntToStr(data[ctr],2,F_HEX));
				};
				lcdNl();
				lcdDisplay(); 
				break;
		};
	};
};

void doLCD(){

	uint8_t pwm=50;
	lcdClear(0xff);
	lcdPrintln("LCD-Test v1");
	lcdDisplay(); 
// #define LCD_BL_EN   P1_1,  SCU_CONF_FUNCTION1, GPIO0, GPIOPIN8 // LCD Backlight: PWM

	while(1){

		OFF(LCD_BL_EN);
		delayNop(10000+pwm*100);
		ON(LCD_BL_EN);
		delayNop(10000-pwm*100);

		switch(getInput()){
			case BTN_NONE:
				continue;
			case BTN_UP:
				break;
			case BTN_DOWN:
				break;
			case BTN_LEFT:
				pwm--;
				break;
			case BTN_RIGHT:
				pwm++;
				break;
			case BTN_ENTER:
				return;
				break;
		};
		lcdClear(0xff);
		lcdPrintln("LCD-Test v1");
		lcdPrintln("");
		lcdPrint("pwm=");lcdPrint(IntToStr(pwm,3,0));lcdNl();
		lcdDisplay(); 
	};
};
void doADC(){

	int32_t vBat=0;
	int32_t vIn=0;
	int32_t RSSI=0;
	int32_t LED=0;
	int32_t MIC=0;
	int v;
	int df=0;

//#define LED4        PB_6, SCU_CONF_FUNCTION4, GPIO5, GPIOPIN26
	SETUPadc(LED4);

	while(1){
		lcdClear(0xff);
		lcdPrintln("ADC-Test v1");
		lcdPrintln("");



		lcdPrint("vBat: "); lcdPrint(IntToStr(vBat,4,F_ZEROS));lcdNl();
		lcdPrint("vIn:  "); lcdPrint(IntToStr(vIn ,4,F_ZEROS));lcdNl();
		lcdPrint("RSSI: "); lcdPrint(IntToStr(RSSI ,4,F_ZEROS));lcdNl();
		lcdPrint("LED:  "); lcdPrint(IntToStr(LED ,4,F_ZEROS));lcdNl();
		lcdPrint("Mic:  "); lcdPrint(IntToStr(MIC ,4,F_ZEROS));lcdNl();
		df++;
		lcdPrint("df: "); lcdPrint(IntToStr(df,4,F_ZEROS));lcdNl();
		lcdPrint("ctr: "); lcdPrint(IntToStr(_timectr,6,0));lcdNl();
		lcdNl();

/*		lcdPrint("U ADC3/vBat");lcdNl();
		lcdPrint("D ADC4/vIn ");lcdNl();
		lcdPrint("L ADC0/RSSI");lcdNl();
		lcdPrint("R ADC7/Mic ");lcdNl(); */
		lcdDisplay(); 

		vBat=adc_get_single(ADC0,ADC_CR_CH3)*2*330/1023;
		vIn=adc_get_single(ADC0,ADC_CR_CH4)*2*330/1023;
		RSSI=adc_get_single(ADC0,ADC_CR_CH0)*2*330/1023;
		LED=adc_get_single(ADC0,ADC_CR_CH6)*2*330/1023;
		MIC=adc_get_single(ADC0,ADC_CR_CH7)*2*330/1023; 

		switch(getInput()){
			case BTN_UP:
				vBat=adc_get_single(ADC0,ADC_CR_CH3)*2*330/1023;
				cpu_clock_set(204);
				break;
			case BTN_DOWN:
				vIn=adc_get_single(ADC0,ADC_CR_CH4)*2*330/1023;
				cpu_clock_set(102);
				break;
			case BTN_LEFT:
				RSSI=adc_get_single(ADC0,ADC_CR_CH0)*2*330/1023;
				cpu_clock_set(12);
				break;
			case BTN_RIGHT:
				LED=adc_get_single(ADC0,ADC_CR_CH6)*2*330/1023;
				MIC=adc_get_single(ADC0,ADC_CR_CH7)*2*330/1023;
				break;
			case BTN_ENTER:
				return;
				break;
		};
	};
};

void doChrg(){
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
	int32_t vBat=0;
	int32_t vIn=0;

	while(1){
		lcdClear(0xff);
		lcdPrintln("Charger-Test v1");
		lcdPrintln("");

		lcdPrint("BC_DONE ~");lcdPrint(IntToStr(GETg(BC_DONE),1,F_HEX));lcdNl();
		lcdPrint("BC_IND  ~");lcdPrint(IntToStr(GETg(BC_IND ),1,F_HEX));lcdNl();
		lcdPrint("BC_OT   ~");lcdPrint(IntToStr(GETg(BC_OT  ),1,F_HEX));lcdNl();
		lcdPrint("BC_DOK  ~");lcdPrint(IntToStr(GETg(BC_DOK ),1,F_HEX));lcdNl();
		lcdPrint("BC_UOK  ~");lcdPrint(IntToStr(GETg(BC_UOK ),1,F_HEX));lcdNl();
		lcdPrint("BC_FLT  ~");lcdPrint(IntToStr(GETg(BC_FLT ),1,F_HEX));lcdNl();
		lcdNl();

		lcdPrint("U BC_CEN   ~");lcdPrint(IntToStr(GETg(BC_CEN  ),1,F_HEX));lcdNl();
		lcdPrint("D BC_PEN2  ");lcdPrint(IntToStr(GETg(BC_PEN2 ),1,F_HEX));lcdNl();
		lcdPrint("L BC_USUS  ~");lcdPrint(IntToStr(GETg(BC_USUS ),1,F_HEX));lcdNl();
		lcdPrint("R BC_THMEN ");lcdPrint(IntToStr(GETg(BC_THMEN),1,F_HEX));lcdNl();
		lcdNl();
		vBat=adc_get_single(ADC0,ADC_CR_CH3)*2*330/1023;
		vIn=adc_get_single(ADC0,ADC_CR_CH4)*2*330/1023;
		lcdPrint("vBat: "); lcdPrint(IntToStr(vBat,4,F_ZEROS));lcdNl();
		lcdPrint("vIn:  "); lcdPrint(IntToStr(vIn ,4,F_ZEROS));lcdNl();
		lcdDisplay(); 
		switch(getInput()){
			case BTN_UP:
				TOGGLE(BC_CEN);
				break;
			case BTN_DOWN:
				TOGGLE(BC_PEN2);
				break;
			case BTN_LEFT:
				TOGGLE(BC_USUS);
				break;
			case BTN_RIGHT:
				TOGGLE(BC_THMEN);
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
			tl3=GET(CE_VCO);
			lcdPrint(IntToStr(tl3,1,F_HEX)); lcdPrint(" "); lcdPrintln(" Lt CE_VCO");
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
						ON(CE_VCO);
						mixer_setup();
						mixer_set_frequency(sf);
					}else{
						OFF(CE_VCO);
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

		ON(LED1);
		delayNop(200000);
		OFF(LED1);
		delayNop(200000);

		ctr++;
		lcdNl();
		lcdPrint(IntToStr(ctr,4,F_HEX));
	}
};

