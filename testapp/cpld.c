#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <hackrf/firmware/hackrf_usb/usb_api_cpld.h>

#include <libopencm3/lpc43xx/gpio.h>

#include <hackrf/firmware/common/cpld_jtag.h>
#include <hackrf/firmware/common/usb_queue.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BLOCK 512
uint8_t cpld_xsvf_buffer[BLOCK];
FIL file;
int bytes;

static void refill_cpld_buffer_fs(void) {
    FRESULT res;
    UINT readbytes;

    res=f_read(&file, cpld_xsvf_buffer, BLOCK, &readbytes); 
    lcdMoveCrsr(0,-8);
    bytes+=readbytes;
    lcdPrint(IntToStr(bytes,5,F_LONG));
    lcdPrint(" bytes...");
    if (res!=FR_OK){
	    lcdPrintln(f_get_rc_string(res));
    };
    lcdNl();
    lcdDisplay();
}

//# MENU cpld
void cpld_menu(){
	getInputWaitRelease();
	SETUPgout(EN_1V8);
	ON(EN_1V8);
	delayNop(1000000); /* wait until cpld boot */
	cpu_clock_set(204);

	lcdClear();
	lcdPrintln("CPLD");
	lcdNl();
	lcdNl();
	bytes=0;
	lcdPrint(IntToStr(bytes,5,F_LONG));
	lcdPrint(" bytes...");
	lcdNl();

	#define WAIT_LOOP_DELAY (6000000)
	#define ALL_LEDS  (PIN_LED1|PIN_LED2|PIN_LED3)
	int i;
	int error;
	FRESULT res;

	res=f_open(&file, "cpld.xsv", FA_OPEN_EXISTING|FA_READ);
	if(res!=FR_OK){
	    lcdPrintln("FOPEN ERROR");
	    lcdPrintln(f_get_rc_string(res));
	    lcdDisplay();
	    getInputWait();
	    return;
	};
	refill_cpld_buffer_fs();

	error = cpld_jtag_program(sizeof(cpld_xsvf_buffer),
				  cpld_xsvf_buffer,
				  refill_cpld_buffer_fs);
	if(error){
	    lcdPrintln("Programming failed!");
	    lcdPrintln(IntToStr(error,5,0));
	    lcdDisplay();
	    /* LED3 (Red) steady on error */
	    ON(LED3);
	    while (1);
	};


	lcdPrintln("Success.");
	lcdDisplay();

	for (res=0;res<10;res++){
	    /* blink LED1, LED2, and LED3 on success */
	    TOGGLE(LED1);
	    TOGGLE(LED2);
	    TOGGLE(LED3);
	    for (i = 0; i < WAIT_LOOP_DELAY; i++)  /* Wait a bit. */
		__asm__("nop");
	};
	/* XXX: Maybe power toggle needed to start CPLD? */
	OFF(EN_1V8);
};
