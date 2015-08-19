/*
 * Copyright 2015 Team rad1o
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
#include <lpcapi/msc/msc_disk.h>

#define BOOTCFG "BOOT.CFG"

extern uint8_t  _app_start;
void bootFile(const char * filename);

void doFlash(){
	uint32_t addr = 0;
	uint16_t len   = (uintptr_t)&_bin_size;
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

void doRealExec(int silent){
	char filename[FLEN];
	FATFS FatFs;
	FRESULT res;
	int sres;

	sres=selectFile(filename,"B1N");
	if(sres<0){
	    if(!silent){
		lcdPrintln("Select ERROR");
		lcdDisplay();
		getInputWait();
	    };
	    return;
	};
	if(sres==0){
	    lcdPrintln("set as default:");
	    res=writeFile(BOOTCFG, filename, strlen(filename)+1);
        if(res<0){
            lcdPrint("write Error:");
            lcdPrintln(IntToStr(-res,3,0));
            lcdPrintln(f_get_rc_string(res));
            lcdDisplay();
            getInputWait();
        }else{
		lcdPrint("wrote ");
		lcdPrint(IntToStr(res,3,0));
		lcdPrintln(" bytes.");
		lcdDisplay();
	    };
	};
	lcdPrintln("Loading:");
	lcdPrintln(filename);
	lcdDisplay();
	bootFile(filename);
};
static inline void doExec(){
	doRealExec(0);
};

void doMSC(){
	MSCenable();
	lcdPrintln("MSC enabled.");
	lcdDisplay();
	getInputWaitRelease();

	while(getInputRaw()!=BTN_ENTER){
        uint32_t max = mscDisk_maxAddressWR();
        lcdPrint("MAX:");
        lcdPrintln(IntToStr(max,8,F_SSPACE));
        lcdDisplay();
        lcdMoveCrsr(0,-8);
        if(max == 1572863) {
            break;
        }
		//__WFI();
	};

	lcdPrintln("disconnect");
	lcdDisplay();
	MSCdisable();
	fsReInit();
	getInputWaitRelease();
};


extern void * _bin_end;
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
	lcdPrint("bin_end: "); lcdPrint(IntToStr((uintptr_t)&_bin_end,8,F_HEX));lcdNl();
	lcdPrint("bin_size:"); lcdPrint(IntToStr((uintptr_t)&_bin_size,8,F_HEX));lcdNl();
	lcdPrint("reloc_ep:"); lcdPrint(IntToStr((uintptr_t)&_reloc_ep,8,F_HEX));lcdNl();
	lcdPrint("end:     "); lcdPrint(IntToStr((uintptr_t)&_end,8,F_HEX));lcdNl();
	lcdPrint("startloc:"); lcdPrint(IntToStr(sli,8,F_HEX));lcdNl();
	lcdDisplay();

	getInputWait();
};

void sys_tick_handler(void){ /* every SYSTICKSPEED us */
	incTimer();
};

void bootFile(const char * filename){
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

	systick_interrupt_disable(); /* TODO: maybe disable all interrupts? */
	boot((void*)&_app_start);
};

int main(uint32_t startloc) {
	cpuClockInit();
	ssp_clock_init();
	systickInit();

//	cpu_clock_set(204);

	SETUPgout(EN_VDD);
	SETUPgout(MIXER_EN);
	SETUPgout(MIC_AMP_DIS);

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
        char filename[FLEN];
        switch(getInputRaw()){
            case BTN_LEFT:
                getInputWaitRelease();
                doRealExec(1);
                break;
            case BTN_UP:
                doMSC();
            default:
            case BTN_NONE:
                readTextFile(BOOTCFG, filename, FLEN);
                lcdPrintln("Booting:");
                lcdPrintln(filename);
                bootFile(filename);
                break;
		};
	};
	static const struct MENU main={ "Bootloader", {
		{ "Exec Firmware", &doExec},
		{ "MSC", &doMSC},
		{ "Flash Boot Ldr", &doFlash},
		{ "Info", &doInfo},
		{NULL,NULL}
	}};
	do {
		handleMenu(&main);
	}while(1);
}
