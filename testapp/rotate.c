#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>

//# MENU rotate
void rotate_menu(){
	getInputWaitRelease();
	
	lcdClear();
	lcdPrintln("Rotate:");
	lcdPrintln("   ");
	lcdPrintln("Lt Turn Left");   
	lcdPrintln("Rt Turn Right"); 
	lcdPrintln("Up Turn Upright");
	lcdPrintln("Enter to Exit");
	lcdDisplay();
	
	while(1){
	switch(getInputWaitRepeat()){
		case BTN_LEFT:
			lcd_select();
			lcdWrite(TYPE_CMD,0x36); // MDAC-Command 
			lcdWrite(TYPE_DATA,0b01100000); // // my,mx,v,lao,rgb,x,x,x
			lcd_deselect();
			lcdDisplay();
			break;
		case BTN_RIGHT:
			lcd_select();
			lcdWrite(TYPE_CMD,0x36); // MDAC-Command 
			lcdWrite(TYPE_DATA,0b10100000); // // my,mx,v,lao,rgb,x,x,x
			lcd_deselect();	
			lcdDisplay();
			break;
		case BTN_DOWN:
			lcd_select();
			lcdWrite(TYPE_CMD,0x36); // MDAC-Command 
			lcdWrite(TYPE_DATA,0b00000000); // // my,mx,v,lao,rgb,x,x,x
			lcd_deselect();	
			lcdDisplay();
			break;			
		case BTN_UP:	
			lcd_select();
			lcdWrite(TYPE_CMD,0x36); // MDAC-Command 
			lcdWrite(TYPE_DATA,0b11000000); // // my,mx,v,lao,rgb,x,x,x
			lcd_deselect();	
			lcdDisplay();
			break;
		case BTN_ENTER:   // Exit
			return;
			break;
	};
	getInputWaitRelease();
	};
};	
