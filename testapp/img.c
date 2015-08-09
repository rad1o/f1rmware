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
#include <r0ketlib/image.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

//# MENU img
void img_menu() {
	char filename[FLEN];
	FATFS FatFs;
	FRESULT res;
	int so=58;

	lcdClear();
	lcdPrintln("Image");
	lcdPrintln("up:  view file");
	lcdPrintln("down:view ani");
	lcdPrintln("l/r: contrast");
	lcdDisplay();

	getInputWaitRelease();

	while(1){
		switch(getInputWaitRepeat()){
			case BTN_UP:
				getInputWaitRelease();
				if(selectFile(filename,"LCD")<0){
					lcdPrintln("Select ERROR");
					lcdDisplay();
					getInputWait();
					return;
				};

				lcdShowImageFile(filename);
				break;
			case BTN_DOWN:
				getInputWaitRelease();
				if(selectFile(filename,"ANI")<0){
					lcdPrintln("Select ERROR");
					lcdDisplay();
					getInputWait();
					return;
				};

				lcdShowAnim(filename);
                lcdPrintln("done.");
                lcdDisplay();
                getInputWaitRelease();
				break;
			case BTN_LEFT:
				so-=1;
				if(so<0) so=132;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, so);
				lcd_deselect();
				break;
			case BTN_RIGHT:
				so+=1;
				if(so>132) so=0;
				lcd_select();
				lcdWrite(TYPE_CMD,0x25); lcdWrite(TYPE_DATA, so);
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
