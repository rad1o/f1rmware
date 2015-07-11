#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>

#include <libopencm3/lpc43xx/adc.h>

//# MENU batt
void batt_menu(){
	getInputWaitRelease();

	SETUPgin(BC_DONE);
	SETUPgout(BC_CEN);
	SETUPgout(BC_PEN2);
	SETUPgout(BC_USUS);
	SETUPgout(BC_THMEN);
	SETUPgin(BC_IND);
	SETUPgin(BC_OT);
	SETUPgin(BC_DOK);
	SETUPgin(BC_UOK);
	SETUPgin(BC_FLT);
	char ct=0;
	int32_t vBat=0;
	int32_t vIn=0;

	while(1){
		lcdClear(0xff);
		lcdPrintln("Charger-Test v1");
		lcdPrintln("");

		lcdPrint("BC_DONE ~");lcdPrint(IntToStr(GET(BC_DONE),1,F_HEX));lcdNl();
		lcdPrint("BC_IND  ~");lcdPrint(IntToStr(GET(BC_IND ),1,F_HEX));lcdNl();
		lcdPrint("BC_OT   ~");lcdPrint(IntToStr(GET(BC_OT  ),1,F_HEX));lcdNl();
		lcdPrint("BC_DOK  ~");lcdPrint(IntToStr(GET(BC_DOK ),1,F_HEX));lcdNl();
		lcdPrint("BC_UOK  ~");lcdPrint(IntToStr(GET(BC_UOK ),1,F_HEX));lcdNl();
		lcdPrint("BC_FLT  ~");lcdPrint(IntToStr(GET(BC_FLT ),1,F_HEX));lcdNl();
		lcdNl();

		lcdPrint("U BC_CEN   ~");lcdPrint(IntToStr(GET(BC_CEN  ),1,F_HEX));lcdNl();
		lcdPrint("D BC_PEN2   ");lcdPrint(IntToStr(GET(BC_PEN2 ),1,F_HEX));lcdNl();
		lcdPrint("L BC_USUS  ~");lcdPrint(IntToStr(GET(BC_USUS ),1,F_HEX));lcdNl();
		lcdPrint("R BC_THMEN  ");lcdPrint(IntToStr(GET(BC_THMEN),1,F_HEX));lcdNl();
		lcdNl();
		vBat=adc_get_single(ADC0,ADC_CR_CH3)*2*330/1023;
		vIn=adc_get_single(ADC0,ADC_CR_CH4)*2*330/1023;
		lcdPrint("vBat: "); lcdPrint(IntToStr(vBat,4,F_LONG));lcdNl();
		lcdPrint("vIn:  "); lcdPrint(IntToStr(vIn ,4,F_LONG));lcdNl();
		lcdDisplay();
		switch(getInput()){
			case BTN_UP:
				TOGGLE(BC_CEN);
				break;
			case BTN_DOWN:
				TOGGLE(BC_PEN2);
				break;
			case BTN_LEFT:
				TOGGLE(BC_USUS);
				break;
			case BTN_RIGHT:
				TOGGLE(BC_THMEN);
				break;
			case BTN_ENTER:
				return;
				break;
		};
	};
	return;


};
