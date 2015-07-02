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
#include <libopencm3/lpc43xx/creg.h>

#include <unistd.h>

#include "setup.h"
#include "display.h"
#include "print.h"
#include "itoa.h"
#include "keyin.h"
#include "menu.h"
#include "mixer.h"
#include "si5351c.h"

#include "spi-flash.h"

#include <select.h>
#include "../fatfs/ff.h"
#include <pins.h>
#include <keyin.h>

static uint16_t counter=0;

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

static inline uint32_t get_sp(void){
	register uint32_t result;
	__asm volatile ("MRS %0, msp\n" : "=r" (result) );
	return result;
};


static inline void boot(const void * vtable){
	// Set new Vtable
	SCB_VTOR = (uintptr_t) vtable;  

	// Reset stack pointer & branch to the new reset vector.  
	__asm(  "mov r0, %0\n"  
			"ldr sp, [r0]\n"  
			"ldr r0, [r0, #4]\n"  
			"bx r0\n"  
			:  
			: "r"(vtable)
			: "%sp", "r0");  
};

static inline uint32_t get_pc(void){
	register uint32_t result;
	__asm volatile ("mov %0, pc\n" : "=r" (result) );
	return result;
};

extern void * end;
extern uint32_t * _text_start;
volatile uint32_t global=42;

extern unsigned _data_loadaddr, _data, _edata, _bss, _ebss, _stack;


void doCopy(){
	lcdClear(0xff);
	lcdNl();
	lcdPrint("PC: "); lcdPrint(IntToStr(get_pc(),8,F_HEX));lcdNl();
	lcdPrint("SP: "); lcdPrint(IntToStr(get_sp(),8,F_HEX));lcdNl();
	lcdPrint("SR: "); lcdPrint(IntToStr(CREG_M4MEMMAP,8,F_HEX));lcdNl();
/*	lcdPrint("end: "); lcdPrint(IntToStr((uintptr_t)&end,8,F_HEX));lcdNl();
	lcdPrint("ts: "); lcdPrint(IntToStr((uintptr_t)&_text_start,8,F_HEX));lcdNl();
	lcdPrint("dl: "); lcdPrint(IntToStr((uintptr_t)&_data_loadaddr,8,F_HEX));lcdNl();
	lcdPrint("d: "); lcdPrint(IntToStr((uintptr_t)&_data,8,F_HEX));lcdNl();
	lcdPrint("ed: "); lcdPrint(IntToStr((uintptr_t)&_edata,8,F_HEX));lcdNl();
	lcdPrint("dr: "); lcdPrint(IntToStr((uintptr_t)&_data_rom,8,F_HEX));lcdNl();
	lcdPrint("edr: "); lcdPrint(IntToStr((uintptr_t)&_edata_rom,8,F_HEX));lcdNl();
	*/
	lcdPrint("global: "); lcdPrint(IntToStr(global,8,F_HEX));lcdNl();
	lcdDisplay();

	uint8_t key;
	while((key=getInputRaw())==BTN_NONE)
		delay(100);

	if(key==BTN_UP){

	};

	boot((void*)0);

	while(1){
		delay(1000*1000);
		TOGGLE(LED1);
	};
};

int main(void) {
	cpu_clock_init();

//	cpu_clock_pll1_max_speed();

	SETUPgout(EN_VDD);
	SETUPgout(MIXER_EN);

	SETUPgout(LED1);
	SETUPgout(LED2);
	SETUPgout(LED3);
	SETUPgout(LED4);
	inputInit();

    lcdInit();
    lcdFill(0xff); /* Display BL Image here */
	setSystemFont();

	static const struct MENU main={ "main 1", {
		{ "Copy", &doCopy},
		{NULL,NULL}
	}};
	handleMenu(&main);
	return 0;
}

extern uint32_t _size;
extern uint32_t * _reloc_ep;

void __attribute__ ((naked)) reset_handler(void){
	volatile unsigned *dest;
	volatile uint32_t *src;
	volatile uint32_t idx;

	if ((void *)CREG_M4MEMMAP != &_reloc_ep){
		src=(uint32_t *)CREG_M4MEMMAP;
		for (idx=0; idx < ((uintptr_t)& _size)/sizeof(uint32_t); idx++){
			((uint32_t*)&_reloc_ep)[idx]= src[idx];
		};
		CREG_M4MEMMAP = (uintptr_t)&_reloc_ep;
		boot(&_reloc_ep);
	}else{
		for (dest = &_bss; dest < &_ebss; ) {
			*dest++ = 0;
		}
	};

	/* Call the application's entry point. */
	main();
}

