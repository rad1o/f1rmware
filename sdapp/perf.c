#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include <rad1olib/systick.h>

#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

static FATFS SDFatFs;          /* File system object for logical drive */
static uint8_t buf[1024];
//# MENU perf
void menu_perf(void){
    lcdClear();

    lcdPrintln("Mounting FAT FS");
    lcdDisplay();

	FRESULT res;
	res=f_mount(&SDFatFs,"1:",1);
	if(res != FR_OK){
		lcdPrintln("FS init error");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
	};

    FIL file;
    UINT bytes;

	res=f_open(&file, "1:perf.tst", FA_CREATE_ALWAYS|FA_WRITE);
    if(res){
		lcdPrintln("f_open error");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
    };

    lcdPrintln("testing write perf");
    lcdDisplay();
    uint32_t write_t = _timectr;

    for(int i=100*1024; i>0; i--) {
        res = f_write(&file, buf, 1024, &bytes);
        if(res){
            lcdPrintln("f_write error");
            lcdPrintln(f_get_rc_string(res));
            lcdDisplay();
            getInputWait();
            return;
        };
    }
    write_t = _timectr - write_t;
    f_close(&file);

    lcdPrintln("ms to write 100MB:");
    lcdPrintln(IntToStr(write_t, 10, 0));
    lcdDisplay();

	res=f_open(&file, "1:perf.tst", FA_OPEN_EXISTING|FA_READ);
    if(res){
		lcdPrintln("f_open error");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
    };

    lcdPrintln("testing read perf");
    lcdDisplay();
    uint32_t read_t = _timectr;

    for(int i=100*1024; i>0; i--) {
        res = f_read(&file, buf, 1024, &bytes);
        if(res){
            lcdPrintln("f_read error");
            lcdPrintln(f_get_rc_string(res));
            lcdDisplay();
            getInputWait();
            return;
        };
    }
    write_t = _timectr - write_t;
    f_close(&file);

    lcdPrintln("ms to read 100MB:");
    lcdPrintln(IntToStr(write_t, 10, 0));
    lcdPrintln("Done, press button");
    lcdDisplay();

    while(getInputRaw()==BTN_NONE){
        /* wait */
        __asm__ volatile ("nop");
    };

    lcdPrintln("goodbye!");
    lcdDisplay();
}
