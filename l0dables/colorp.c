/*
 * Copyright 2015 team rad1o
 *
 * This is a small sample for a l0dable
 *
 */

#include <r0ketlib/keyin.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include "usetable.h"

void ram(void){
	int lineHeight = getFontHeight();

	lcdClear();
	for(int row = -1; row < 16; row++)
	{
		for(int col = 0; col < 16; col++)
		{
			if(col == -1)
			{
				lcdPrint(IntToStr(row, 1, F_HEX));
				lcdMoveCrsr(1, 0);
			}
			else if(row == -1)
			{
				lcdPrint(IntToStr(col, 1, F_HEX));
				lcdMoveCrsr(1, 0);
			}
			else
			{
				drawRectFill(1 + 8 * col, 7 * (row+1) + 1, 5, 5, (row << 4) + col);
			}
		}

		lcdNl();
	}

	lcdDisplay();
	getInputWait();
	getInputWaitRelease();
	return;
}
