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
#include "intrinsics.h"

extern uint8_t  _app_start;

void doFlash(){
	uint32_t addr = 0;
	uint16_t len   = (uintptr_t)&_text_size;
	uint8_t * data = (uint8_t *)&_reloc_ep;
	lcdPrintln("Flashing");
	lcdPrint(IntToStr(len,6,0));
	lcdPrintln(" bytes");
	lcdDisplay();
	flash_random_write(addr, len, data);
	lcdPrintln("done.");
	lcdDisplay();
	flash_read(addr, len, &_app_start);
	uint16_t idx;
	int err=0;
	for (idx=0;idx<len;idx++){
		if ((&_app_start)[idx] != data[idx]){
			lcdPrint(IntToStr(idx,4,F_HEX));
			lcdPrint(": ");
			lcdPrint(IntToStr((&_app_start)[idx],2,F_HEX));
			lcdPrint("!=");
			lcdPrint(IntToStr(data[idx],2,F_HEX));
			lcdNl();
			err++;
			lcdDisplay();
		};
		if(err%4==1){
			getInputWait();
		};
	};

	getInputWait();
};

void doExec(){
	char filename[FLEN];
	FATFS FatFs;

	FRESULT res;
	lcdPrint("Mount:");
	res=f_mount(&FatFs,"/",0);
	if(res){
		lcdPrintln("MOUNT ERROR");
		lcdPrintln(IntToStr(res,3,0));
		lcdDisplay();
		getInputWait();
		return;
	};
	if(selectFile(filename,"BIN")){
		lcdPrintln("Select ERROR");
		lcdDisplay();
		getInputWait();
		return;
	};
	lcdPrintln("Loading:");
	lcdPrintln(filename);
	lcdDisplay();

	FIL file;
	UINT readbytes;

	res=f_open(&file, filename, FA_OPEN_EXISTING|FA_READ);
	if(res!=F_OK){
		lcdPrintln("FOPEN ERROR");
		lcdPrintln(IntToStr(res,3,0));
		lcdDisplay();
		getInputWait();
	};
	uint8_t * destination=&_app_start;
#define BLOCK 1024 * 128
	do {
		res=f_read(&file, destination, BLOCK, &readbytes); 
		destination+=readbytes;
/*		lcdPrint("Read:");
		lcdPrint(IntToStr(res,3,0));
		lcdPrint(" ");
		lcdPrint(IntToStr(readbytes,6,0));
		lcdNl(); */
	}while(res==F_OK && readbytes==BLOCK);

	lcdDisplay();
	if(res!=F_OK){
		lcdPrint("Read Error:");
		lcdPrintln(IntToStr(res,3,0));
		lcdDisplay();
		getInputWait();
		return;
	};
	lcdPrintln("Length:");
	lcdPrintln(IntToStr(destination-&_app_start,8,0));
	lcdDisplay();
	getInputWait();
	boot((void*)&_app_start);
};

void doMSC(){
//	cpu_clock_set(204);
	dwim();
};

extern void * _text_end;
extern void * _end;

volatile uint32_t global=42;

void doInfo(){
	lcdClear(0xff);
	lcdNl();
	lcdPrint("PC:      "); lcdPrint(IntToStr(get_pc(),8,F_HEX));lcdNl();
	lcdPrint("StackP:  "); lcdPrint(IntToStr(get_sp(),8,F_HEX));lcdNl();
	lcdPrint("ShadowR: "); lcdPrint(IntToStr(CREG_M4MEMMAP,8,F_HEX));lcdNl();
	lcdPrint("text_s:  "); lcdPrint(IntToStr((uintptr_t)&_text_start,8,F_HEX));lcdNl();
	lcdPrint("text_e:  "); lcdPrint(IntToStr((uintptr_t)&_text_end,8,F_HEX));lcdNl();
	lcdPrint("size:    "); lcdPrint(IntToStr((uintptr_t)&_text_size,8,F_HEX));lcdNl();
	lcdPrint("reloc_ep:"); lcdPrint(IntToStr((uintptr_t)&_reloc_ep,8,F_HEX));lcdNl();
	lcdPrint("end:     "); lcdPrint(IntToStr((uintptr_t)&_end,8,F_HEX));lcdNl();
	lcdDisplay();

	getInputWait();

	boot((void*)0);

	while(1){
		delay(1000*1000);
		TOGGLE(LED1);
	};
};

extern uint32_t _timectr;

void sys_tick_handler(void){
	_timectr++;
	TOGGLE(LED1);
};

int main(void) {
	cpu_clock_init();
	systick_set_reload(208000);
	systick_set_clocksource(0);
	systick_interrupt_enable();
	systick_counter_enable();

//	cpu_clock_pll1_max_speed();

	SETUPgout(EN_VDD);
	SETUPgout(MIXER_EN);

	SETUPgout(LED1);
	SETUPgout(LED2);
	SETUPgout(LED3);
	SETUPgout(LED4);
	inputInit();
	flash_init();

    lcdInit();
    lcdFill(0xff); /* Display BL Image here */
	setSystemFont();

	static const struct MENU main={ "main 1", {
		{ "Info", &doInfo},
		{ "Exec", &doExec},
		{ "Flash", &doFlash},
		{NULL,NULL}
	}};
	handleMenu(&main);
	return 0;
}
