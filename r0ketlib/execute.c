
#include <r0ketlib/print.h>
#include <r0ketlib/select.h>
#include <r0ketlib/print.h>
#include <r0ketlib/display.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fs_util.h>

#include <fatfs/ff.h>

extern uintptr_t _l0dable_start;
extern uintptr_t _l0dable_len;
extern uintptr_t _jumptable_len;

#define l0dable_start ((uintptr_t)&_l0dable_start)
#define l0dable_len ((uintptr_t)&_l0dable_len)
#define jumptable_len ((uintptr_t)&_jumptable_len)

/**************************************************************************/

uint8_t execute_file (const char * fname){
    FRESULT res;
    FIL file;
    UINT readbytes;
    void (*dst)(void);
    uint32_t version=0;

    res=f_open(&file, fname, FA_OPEN_EXISTING|FA_READ);
    if (res!=FR_OK){
	lcdPrintln(f_get_rc_string(res));
	lcdDisplay();
	return -1;
    };

    res = f_read(&file, &version, sizeof(uint32_t), &readbytes);
    if(res!=FR_OK){
	lcdPrintln(f_get_rc_string(res));
	lcdDisplay();
        return -1;
    };

    if (version>jumptable_len){
	lcdPrintln("l0dable incompat.");
	lcdPrint(IntToStr(jumptable_len,4,F_HEX));
	lcdPrint(" < ");
	lcdPrintln(IntToStr(version,4,F_HEX));
	lcdDisplay();
        return -1;
    };

    res = f_read(&file, &dst, sizeof(uint32_t), &readbytes);
    if(res!=FR_OK){
	lcdPrintln(f_get_rc_string(res));
	lcdDisplay();
        return -1;
    };

    if ((uintptr_t)dst<l0dable_start || (uintptr_t)dst>(l0dable_start+l0dable_len)){
	lcdPrintln("l0daddr illegal");
	lcdPrint(IntToStr((uintptr_t)dst,8,F_HEX));
	lcdDisplay();
        return -1;
    };


    res = f_read(&file, (uint8_t *)l0dable_start, l0dable_len, &readbytes);
    if(res!=FR_OK){
	lcdPrintln(f_get_rc_string(res));
	lcdDisplay();
        return -1;
    };

    if(readbytes>=l0dable_len){
	lcdPrintln("l0dable too long.");
	lcdDisplay();
	return -1;
    };

    lcdPrint(IntToStr(readbytes,5,F_LONG));
    lcdPrintln(" bytes...");

    dst=(void (*)(void)) ((uintptr_t)dst|1); // Enable Thumb mode!

#if 0
    lcdPrint("dst= "); lcdPrint(IntToStr((uintptr_t)dst,8,F_HEX)); lcdNl();
    lcdPrint("len= "); lcdPrint(IntToStr((uintptr_t)&_l0dable_len,8,F_HEX)); lcdNl();
    lcdPrint("jt=  "); lcdPrint(IntToStr(jumptable_len,8,F_HEX)); lcdNl();
    lcdPrint("ver= "); lcdPrint(IntToStr(version,8,F_HEX)); lcdNl();
    lcdDisplay();
#endif

    dst();
    return 0;
}

/**************************************************************************/

void executeSelect(const char *ext){
    char filename[FLEN];

    if( selectFile(filename,ext) >= 0){
        if(execute_file(filename)!=0){
            getInputWait();
        };
    };
}

