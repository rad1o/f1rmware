/* vim: set ts=4 sw=4 expandtab: */
/*
 * This file is part of rad1o
 *
 */

#include <rad1olib/setup.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

#include <hackrf/firmware/hackrf_usb/usb_api_cpld.h>

#include <hackrf/firmware/common/cpld_jtag.h>

#include <lpcapi/msc/msc_main.h>
#include "msc/msc_disk.h"
#include <libopencmsis/core_cm3.h>
#include <libopencm3/lpc43xx/gpio.h>

#include <string.h>

void sys_tick_handler(void){
    incTimer();
    static int ctr=0;
    if (++ctr%50==1)
	TOGGLE(LED2);
};

const unsigned char default_xsvf[] = {
#include "xsvf.inc"
};

#define BLOCK 1000
uint8_t cpld_xsvf_buffer[BLOCK];
int bytes;

static void refill_cpld_buffer_fs(void) {
    memcpy(cpld_xsvf_buffer,default_xsvf+bytes,BLOCK);

    lcdMoveCrsr(0,-8);
    bytes+=BLOCK;
    lcdPrint(IntToStr(bytes,5,F_LONG)); lcdPrint(" bytes..."); lcdNl();
    lcdDisplay();
}

//# MENU cpld
void cpld_flash(){
	SETUPgout(EN_1V8);
	ON(EN_1V8);
	delayNop(1000000); /* wait until cpld boot */
	cpu_clock_set(204);

	lcdPrintln("Program CPLD");
	bytes=0;
	lcdPrint(IntToStr(bytes,5,F_LONG)); lcdPrint(" bytes..."); lcdNl();

	#define WAIT_LOOP_DELAY (6000000)
	#define ALL_LEDS  (PIN_LED1|PIN_LED2|PIN_LED3)
	int i;
	int error;

	refill_cpld_buffer_fs();

	error = cpld_jtag_program(sizeof(cpld_xsvf_buffer),
				  cpld_xsvf_buffer,
				  refill_cpld_buffer_fs);
	if(error){
	    lcdPrintln("Programming failed!");
	    lcdPrintln(IntToStr(error,5,0));
	    lcdDisplay();
	    /* LED3 (Red) steady on error */
	    ON(LED4);
	    while (1);
	};

	lcdPrintln("Success.");
	lcdDisplay();
	OFF(EN_1V8);
};


void full_msc(){
	MSCenable();
	lcdPrintln("FLASHMSC enabled.");
	lcdNl();
	lcdNl();
	lcdDisplay();
	while(getInputRaw()!=BTN_ENTER){
        uint32_t min = mscDisk_minAddressWR();
        uint32_t max = mscDisk_maxAddressWR();
        lcdMoveCrsr(0,-16);
        lcdPrint("MIN:");
        lcdPrintln(IntToStr(min,8,F_SSPACE));
        lcdPrint("MAX:");
        lcdPrintln(IntToStr(max,8,F_SSPACE));
        lcdDisplay();
        if(min == 0 && max == 2097151) {
            break;
        }
	};
	lcdPrintln("FLASHMSC disabled");
	lcdDisplay();
	MSCdisable();
};

int main(void) {
    cpuClockInit(); /* CPU Clock is now 104 MHz */
    ssp_clock_init();
    systickInit();

    SETUPgout(EN_VDD);
    SETUPgout(MIXER_EN);
    SETUPgout(MIC_AMP_DIS);

    SETUPgout(LED1);
    SETUPgout(LED2);
    SETUPgout(LED3);
    SETUPgout(LED4);

    inputInit();
    lcdInit();
    lcdFill(0xff);
    lcdPrintln("Flash-App");
    lcdNl();
    lcdDisplay();

    cpld_flash();
    cpu_clock_set(50);
    full_msc();
    while(1) {
	    delayNop(2000000);
	    ON(LED4);
	    delayNop(2000000);
	    OFF(LED4);
    }
    return 0;
}
