/*
 * Copyright 2015 team rad1o
 *
 * Fancy system information
 *
 */

#include <r0ketlib/keyin.h>
#include <r0ketlib/render.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fs_util.h>
#include <r0ketlib/display.h>
#include <fatfs/ff.h>
#include "usetable.h"

#define MENU_DISK	0
#define MENU_MAX	MENU_DISK

void prettyPrintSize(int size)
{
	unsigned char sizeUnit = 0;
	int decimalPlace = 0;
	static const char *unit[] = {
		"Byte",
		"KiB",
		"MiB",
		"GiB"
	};

	while(size > 1024) {
		decimalPlace = size % 1024;
		size /= 1024;
		sizeUnit++;
	}

	lcdPrint(IntToStr(size,3,F_LONG));
	if(sizeUnit > 0)
	{
		lcdPrint(".");
		lcdPrint(IntToStr(decimalPlace,3,F_LONG | F_ZEROS));
	}
	lcdPrint(" ");
	lcdPrint(unit[sizeUnit]);
}

void drawUsage(double usage)
{
	int lineHeight = getFontHeight();
	int y1 = lcdGetCrsrY();
	int y2 = y1 + lineHeight;
	static const int frameSize = 1;
	BYTE color_frame = 0x00;

	// Colors: green - yellow - red
	static const BYTE colors[] = {
		0x1C, 0x3C, 0x5C, 0x7C, 0x9C, 0xBC, 0xDC, 0xFC,
		0xF8, 0xF4, 0xF0, 0xEC, 0xE8, 0xE4, 0xE0
	};

	drawRectFill(0, y1+frameSize, frameSize, lineHeight-2*frameSize, color_frame);
	drawRectFill(0, y1, 3*frameSize, frameSize, color_frame);
	drawRectFill(0, y2-frameSize, 3*frameSize, frameSize, color_frame);

	drawRectFill(RESX-frameSize, y1+frameSize, frameSize, lineHeight-2*frameSize, color_frame);
	drawRectFill(RESX-3*frameSize, y1, 3*frameSize, frameSize, color_frame);
	drawRectFill(RESX-3*frameSize, y2-frameSize, 3*frameSize, frameSize, color_frame);

	int lines = (RESX - 2*frameSize - 1) * usage + 0.5;
	for(int i = 0; i < lines; ++i)
		drawVLine(frameSize + i, y1 + frameSize, y2 - frameSize - 1, colors[(int)((double)i / (RESX-2*frameSize) * sizeof(colors) + 0.5)]);

	lcdNl();
}

void printHeader()
{
	int lineHeight = getFontHeight();
	BYTE color = 0x00;

	lcdMoveCrsr(3, 2);
	lcdPrintln("System Information");
	lcdMoveCrsr(0, -2);
	drawHLine(lineHeight+3, 0, RESX-1, color);
	lcdNl();
}

void printDisk()
{
	FATFS fs;
	FS_USAGE fs_usage;

	fsInfo(&fs);
	lcdPrintln("[Disk]");
	lcdNl();

	lcdPrint("format: ");
	switch(fs.fs_type)
	{
		case 0:
			lcdPrintln("unmounted");
			break;

		case FS_FAT12:
			lcdPrintln("FAT12");
			break;
		case FS_FAT16:
			lcdPrintln("FAT16");
			break;
		case FS_FAT32:
			lcdPrintln("FAT32");
			break;

		default:
			lcdPrint("Unknown: ");
			lcdPrintln(IntToStr(fs.fs_type,3,0));
			break;
	}

	fsUsage(&fs, &fs_usage);
	lcdPrint("total: ");
	prettyPrintSize(fs_usage.total);
	lcdNl();
	lcdPrint("free:  ");
	prettyPrintSize(fs_usage.free);
	lcdNl();
	lcdNl();

	double usage = 1 - ((double)fs_usage.free / fs_usage.total);
	lcdPrint("usage: ");
	lcdPrint(IntToStr(usage*100,3,0));
	lcdPrintln("%");
	drawUsage(usage);
}

void ram(void){
	int menu = 0;

	while(true) {
		lcdClear();
		printHeader();

		switch(menu)
		{
			case MENU_DISK:
				printDisk();
				break;

			default:
				break;
		}

		lcdDisplay();

		getInputWait();
		return;
		/*
		switch(getInputWaitRelease()){
			case BTN_UP:
			case BTN_DOWN:
				return;

			case BTN_LEFT:
				menu = --menu < 0 ? MENU_MAX : menu;
				break;
			case BTN_RIGHT:
				menu = ++menu > MENU_MAX ? 0 : menu;
				break;

			case BTN_ENTER:
				return;
		}
		*/
	}
}
