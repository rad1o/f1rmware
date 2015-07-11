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
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencmsis/core_cm3.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>

#include <rad1olib/spi-flash.h>

#include <r0ketlib/select.h>
#include <rad1olib/systick.h>
#include <fatfs/ff.h>
#include <rad1olib/pins.h>
#include "intrinsics.h"

#include <lpcapi/msc/msc_main.h>

#define BOOTCFG "BOOT.CFG"

extern uint8_t  _app_start;
void bootFile(const char * filename, uint8_t write);

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
	if(selectFile(filename,"BIN")){
		lcdPrintln("Select ERROR");
		lcdDisplay();
		getInputWait();
		return;
	};
	lcdPrintln("Loading:");
	lcdPrintln(filename);
	lcdDisplay();
	bootFile(filename,1);

};

void doMSC(){
	MSCenable();
	lcdPrintln("MSC enabled.");
	while(getInputRaw()!=BTN_ENTER){
		if(getInputRaw()==BTN_RIGHT)
			lcdPrintln(".");
		lcdDisplay();
		__WFI();
	};
	lcdPrintln("disconnect");
	lcdDisplay();
	MSCdisable();
	getInputWaitRelease();
};

extern void * _text_end;
extern void * _end;

volatile uint32_t global=42;
uint32_t sli;

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
	lcdPrint("startloc:"); lcdPrint(IntToStr(sli,8,F_HEX));lcdNl();
	lcdDisplay();

	getInputWait();
};

void sys_tick_handler(void){ /* every SYSTICKSPEED us */
	incTimer();
};

void bootFile(const char * filename, uint8_t write){
	FIL file;
	UINT readbytes;
	FRESULT res;

	res=f_open(&file, filename, FA_OPEN_EXISTING|FA_READ);
	if(res!=F_OK){
		lcdPrintln("FOPEN ERROR");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
	};
	uint8_t *destination=&_app_start;
#define BLOCK 1024 * 128
	do {
		res=f_read(&file, destination, BLOCK, &readbytes); 
		destination+=readbytes;
	}while(res==F_OK && readbytes==BLOCK);

	lcdDisplay();
	if(res!=F_OK){
		lcdPrint("Read Error:");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
	};
	if(write){
		res=writeFile(BOOTCFG, filename, strlen(filename)+1);
		if(res<0){
			lcdPrint("write Error:");
			lcdPrintln(f_get_rc_string(-res));
			lcdDisplay();
			getInputWait();
		};
		lcdPrint("write Done:");
		lcdPrintln(IntToStr(res,3,0));
		lcdPrintln(IntToStr(strlen(filename),3,0));
		lcdDisplay();
		getInputWait();
	};
	boot((void*)&_app_start);
};

int main(uint32_t startloc) {
	cpu_clock_init();
	ssp_clock_init();
	systickInit();

	cpu_clock_set(204);

	SETUPgout(EN_VDD);
	SETUPgout(MIXER_EN);

	SETUPgout(LED1);
	SETUPgout(LED2);
	SETUPgout(LED3);
	SETUPgout(LED4);
	inputInit();
	flashInit();

    lcdInit();
	fsInit();
    lcdFill(0xff); /* Display BL Image here */

	sli=startloc;

	if (startloc != (uintptr_t)&_app_start){ /* not booted via DFU, do autoboot */ 
		if (getInputRaw()!=BTN_LEFT){
			char filename[FLEN];
			readTextFile(BOOTCFG, filename, FLEN);
			lcdPrintln("Fname");
			lcdPrintln(filename);
			bootFile(filename,0);
		};
	};
	static const struct MENU main={ "Bootloader", {
		{ "Info", &doInfo},
		{ "Exec", &doExec},
		{ "Flash", &doFlash},
		{ "MSC", &doMSC},
		{NULL,NULL}
	}};
	handleMenu(&main);
	return 0;
}
