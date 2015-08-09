#include <stdint.h>

#include <r0ketlib/keyin.h>
#include <r0ketlib/idle.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/image.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#define BLOCK 1024
int lcdShowImage(FIL* file) {
    uint8_t idata[BLOCK];
    FRESULT res;
    UINT readbytes;

    image_t type=IMG_NONE;
    int len;
    res=f_read(file, &type, 1, &readbytes); 

    switch(type){
        case IMG_RAW_8:
            lcd_select();
            lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2);
            lcd_deselect();
            len=RESX*RESY;
            break;
        case IMG_RAW_12:
            lcd_select();
            lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,3);
            lcd_deselect();
            len=RESX*RESY*3/2;
            break;
        case IMG_RAW_16:
            lcd_select();
            lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,5);
            lcd_deselect();
            len=RESX*RESY*2;
            break;
        default:
            lcdPrint("ERR: Image Type=");
            lcdPrintln(IntToStr(type,3,0));
            lcdDisplay();
            getInputWait();
            return 1;
    };

    lcd_select();
    lcdWrite(TYPE_CMD,0x2C);
    lcd_deselect();
    int rb;
    do {
        rb=BLOCK;
        if (len<BLOCK)
            rb=len;
        res=f_read(file, idata, rb, &readbytes); 
        lcd_select();
        for (int i=0;i<readbytes;i++)
            lcdWrite(TYPE_DATA,idata[i]);
        lcd_deselect();
        len-=readbytes;
    }while(res==FR_OK && len>0 && rb==readbytes);

    if(res!=FR_OK){
        lcdPrint("Read Error:");
        lcdPrintln(f_get_rc_string(res));
        lcdDisplay();
        getInputWait();
        return 1;
    };
    return 0;
}

uint8_t lcdShowImageFile(char *fname){
    FIL file;            /* File object */
	int res;

	res=f_open(&file, fname, FA_OPEN_EXISTING|FA_READ);
	if(res)
		return 1;
    return lcdShowImage(&file);
};


uint8_t lcdShowAnim(char *fname) {
    FIL file;            /* File object */
	int res;
    UINT readbytes;
	uint8_t state=0;
    uint16_t framems=0;

	res=f_open(&file, fname, FA_OPEN_EXISTING|FA_READ);
	if(res != FR_OK)
		return 1;

	getInputWaitRelease();
	while(!getInputRaw()){
		res = f_read(&file, &framems, sizeof(framems), &readbytes);
        /*
        lcdPrint("ms=");lcdPrintln(IntToStr(framems,4,0));
        lcdPrint("off=");lcdPrintln(IntToStr(f_tell(&file),4,0));
        */
        if(res != FR_OK)
            return 1;
		if(readbytes<sizeof(framems)){
			f_lseek(&file,0);
            continue;
        };
        lcdShowImage(&file);
        getInputWaitTimeout(framems);
	}

    return 0;
}


