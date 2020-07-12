#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

static FATFS SDFatFs;          /* File system object for logical drive */

//# MENU wrfile
void menu_wrfile(void){
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

    lcdPrintln("Writing test file");
    lcdDisplay();

    writeFile("1:test.txt", "Hello World!", 12);

    lcdPrintln("Done, press button");
    lcdDisplay();

    while(getInputRaw()==BTN_NONE){
        /* wait */
        __asm__ volatile ("nop");
    };

    lcdPrintln("goodbye!");
    lcdDisplay();
}
