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
#include <r0ketlib/itoa.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>

#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/select.h>

unsigned char * ram=(uint8_t *)0x20000000;
unsigned int ram_len=64*1024;

//# MENU dac
void dac_menu(){
	int i=0;
	int mode=0;
	UINT readbytes=0;
	cpu_clock_set(204);
	getInputWaitRelease();
	lcdClear();
	lcdPrintln("DAC test");
	lcdNl();
	lcdPrintln("Up Load file");
	lcdPrintln("Lt File output");
	lcdPrintln("Rt Echo test");
	lcdDisplay();

	SETUPgout(MIC_AMP_DIS);
	OFF(MIC_AMP_DIS); // Enable AMP
	dac_init(false); 

	while(1){
	    while (getInputRaw()==BTN_NONE) {
		if(mode==1){ // FILE
		    if(++i>=readbytes)
			i=0;
		    dac_set(ram[i]);
		    delayNop(765);
		}else if (mode==0){ // ECHO
		    dac_set(adc_get_single(ADC0,ADC_CR_CH7)>>1);
		};
	    };
	    if(getInputRaw()==BTN_ENTER)
		return;
	    if(getInputRaw()==BTN_LEFT)
		mode=1;
	    if(getInputRaw()==BTN_RIGHT)
		mode=0;

	    if(getInputRaw()==BTN_UP){
		FATFS FatFs;
		FRESULT res;
		FIL file;
		char filename[FLEN];

		getInputWaitRelease();
		if(selectFile(filename,"DAC")<0){
		    lcdPrintln("Select ERROR");
		    lcdDisplay();
		    continue;
		};

		res=f_open(&file, filename, FA_OPEN_EXISTING|FA_READ);
		if(res!=FR_OK){
		    lcdPrintln("FOPEN ERROR");
		    lcdPrintln(f_get_rc_string(res));
		    lcdDisplay();
		    continue;
		};

		res=f_read(&file, ram, ram_len, &readbytes);

		if(res!=FR_OK || readbytes<1){
		    lcdPrint("Read Error:");
		    lcdPrintln(f_get_rc_string(res));
		    lcdDisplay();
		    continue;
		};
		lcdPrint(IntToStr(readbytes,8,0));
		lcdPrintln(" bytes");
		lcdDisplay();
		mode=1;
	    };
	};
}
