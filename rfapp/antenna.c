#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/intin.h>


// default to 100 MHz
#define FREQSTART 100000000

double frequency = FREQSTART;

//speed of light
const double SPEEDOFLIGHT = 299792458;


//# MENU antenna
void antenna_main()
{
	frequency=(int64_t)input_int("Frequency:",(int)(frequency/1000000),50,4000,4)*1000000;

	lcdClear();
	lcdSetCrsr(0,0);
	lcdPrintln("Antenna Frequency");

	lcdNl();
	lcdPrint(" ");
	lcdPrint(IntToStr(frequency/1000000,4,F_LONG));
	lcdPrintln(" MHz ");

	lcdNl();
	lcdPrintln("Antenna Length");

	float meter = SPEEDOFLIGHT/frequency;

	lcdNl();
	lcdPrint(" ");
	lcdPrint(IntToStr(meter * 1000, 4, F_LONG ));
	lcdPrint(" mm");
	lcdNl();
	lcdPrint(" ");
	lcdPrint(IntToStr(meter * 1000 / 2, 4, F_LONG ));
	lcdPrint(" mm");
	lcdNl();
	lcdPrint(" ");
	lcdPrint(IntToStr(meter * 1000 / 4, 4, F_LONG ));
	lcdPrint(" mm");

	lcdDisplay();

	getInputWaitRepeat();

}
