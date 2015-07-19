/*
 * Copyright 2015 team rad1o
 *
 */

#include <unistd.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

void do_image(char * filename){
	FATFS FatFs;
	FRESULT res;
	int i;
	FIL file;
	UINT readbytes;

	res=f_open(&file, filename, FA_OPEN_EXISTING|FA_READ);
	if(res!=F_OK){
		lcd_select(); lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2); lcd_deselect();
		lcdPrintln("FOPEN ERROR");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
	};

#define BLOCK 1024
	uint8_t idata[BLOCK];
	lcd_select();
	lcdWrite(TYPE_CMD,0x2C);
	lcd_deselect();
	do {
		res=f_read(&file, idata, BLOCK, &readbytes); 
		lcd_select();
		for (i=0;i<readbytes;i++)
			lcdWrite(TYPE_DATA,idata[i]);
		lcd_deselect();
	}while(res==F_OK && readbytes==BLOCK);

	if(res!=F_OK){
		lcd_select(); lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2); lcd_deselect();
		lcdPrint("Read Error:");
		lcdPrintln(f_get_rc_string(res));
		lcdDisplay();
		getInputWait();
		return;
	};
};


//# MENU img
void img_menu() {
	char filename[FLEN];
	FATFS FatFs;
	FRESULT res;
	int ct=0x3a;

	lcdClear();
	lcdPrintln("Image");
	lcdPrintln("up:   16bit");
	lcdPrintln("down: 12bit");
	lcdPrintln("l/r:  contrast");
	lcdDisplay();

	getInputWaitRelease();

	while(1){
		switch(getInputWaitRepeat()){
			case BTN_UP:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2);
				lcd_deselect();
				getInputWaitRelease();
				if(selectFile(filename,"L16")){
					lcdPrintln("Select ERROR");
					lcdDisplay();
					getInputWait();
					return;
				};

				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,5);
				lcd_deselect();
				do_image(filename);
				break;
			case BTN_DOWN:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2);
				lcd_deselect();
				getInputWaitRelease();
				if(selectFile(filename,"LCD")){
					lcdPrintln("Select ERROR");
					lcdDisplay();
					getInputWait();
					return;
				};

				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,3);
				lcd_deselect();
				do_image(filename);
				break;
			case BTN_LEFT:
				ct-=1;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, ct);
				lcd_deselect();
				break;
			case BTN_RIGHT:
				ct+=1;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, ct);
				lcd_deselect();
				break;
			case BTN_ENTER:
				lcd_select();
				lcdWrite(TYPE_CMD,0x3a); lcdWrite(TYPE_DATA,2);
				lcd_deselect();
				return;
		};
	};
};
