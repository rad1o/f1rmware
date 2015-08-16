#include <stddef.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/print.h>
#include <r0ketlib/display.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>
#include <rad1olib/setup.h>
#include <hackrf/firmware/common/mixer.h>
#include <hackrf/firmware/common/si5351c.h>

//# MENU feldtest
void feld_menu(){
	getInputWaitRelease();

	SETUPgout(BY_AMP);
	SETUPgout(BY_AMP_N);
	SETUPgout(TX_RX);
	SETUPgout(TX_RX_N);
	SETUPgout(BY_MIX);
	SETUPgout(BY_MIX_N);
	SETUPgout(LOW_HIGH_FILT);
	SETUPgout(LOW_HIGH_FILT_N);
	SETUPgout(TX_AMP);
	SETUPgout(RX_LNA);
	SETUPgout(CE_VCO);
	SETUPgout(MIXER_EN);
	SETUPgout(EN_VDD);

	char tu=0,td=0,tl=0,tr=0;
	char tuN=0,tdN=0,tlN=0,trN=0;
	char page=0;
	char tu2=0,td2=0,td2N=0,tl2=0,tr2=0;
	uint16_t sf=3500;
	char tu3=0,td3=0,tl3=0,tr3=0;
	char tl4=0,tr4=0;

	int ctr=0;
	while (1) {
		lcdClear(0xff);
		lcdPrintln("Feld-Test v13");
		lcdPrintln("");
		lcdPrint("Page ");lcdPrintln(IntToStr(page,2,0));
		if (page==1){
			tu=GET(BY_MIX); tuN=GET(BY_MIX_N);
			lcdPrint(IntToStr(tu,1,F_HEX)); lcdPrint(IntToStr(tuN,1,F_HEX)); lcdPrintln(" Up BY_MIX/_N");
			td=GET(BY_AMP); tdN=GET(BY_AMP_N);
			lcdPrint(IntToStr(td,1,F_HEX)); lcdPrint(IntToStr(tdN,1,F_HEX)); lcdPrintln(" Dn BY_AMP/_N");
			tl=GET(TX_RX);tlN=GET(TX_RX_N);
			lcdPrint(IntToStr(tl,1,F_HEX)); lcdPrint(IntToStr(tlN,1,F_HEX)); lcdPrintln(" Lt TX_RX/_N");
			tr=GET(LOW_HIGH_FILT); trN=GET(LOW_HIGH_FILT_N);
			lcdPrint(IntToStr(tr,1,F_HEX)); lcdPrint(IntToStr(trN,1,F_HEX)); lcdPrintln(" Rt LOW_HIGH_FILT/_N");
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					tu=1-tu;
					if (tu){
						OFF(BY_MIX_N);
						ON(BY_MIX);
					}else{
						OFF(BY_MIX);
						ON(BY_MIX_N);
					};
					break;
				case BTN_DOWN:
					td=1-td;
					if (td){
						OFF(BY_AMP_N);
						ON(BY_AMP);
					}else{
						OFF(BY_AMP);
						ON(BY_AMP_N);
					};
					break;
				case BTN_LEFT:
					tl=1-tl;
					if (tl){
						OFF(TX_RX_N);
						ON(TX_RX);
					}else{
						OFF(TX_RX);
						ON(TX_RX_N);
					};
					break;
				case BTN_RIGHT:
					tr=1-tr;
					if (tr){
						OFF(LOW_HIGH_FILT_N);
						ON(LOW_HIGH_FILT);
					}else{
						OFF(LOW_HIGH_FILT);
						ON(LOW_HIGH_FILT_N);
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};
		}else if (page==0){
			tu2=GET(EN_VDD);
			lcdPrint(IntToStr(tu2,1,F_HEX)); lcdPrint(" "); lcdPrintln(" Up EN_VDD");
			td2=GET(RX_LNA);td2N=GET(TX_AMP);
			lcdPrint(IntToStr(td2,1,F_HEX)); lcdPrint(IntToStr(td2N,1,F_HEX)); lcdPrintln(" Dn RX/TX");
			lcdPrint(IntToStr(tl2,1,F_HEX)); lcdPrint(" ");                    lcdPrintln(" Lt MIXER_EN");
			lcdPrint(IntToStr(tr2,1,F_HEX)); lcdPrint(" ");
			if(tr2){
				lcdPrintln(" Rt SI-clk_dis");
			}else{
				lcdPrintln(" Rt SI-clk_en");
			};
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					tu2=1-tu2;
					if (tu2){
					    ON(EN_VDD);
					}else{
					    OFF(EN_VDD);
					};
					break;
				case BTN_DOWN:
					td2=td2+2*td2N;
					td2++;
					td2=td2%3;
					switch (td2){
						case(0):
							OFF(RX_LNA);
							OFF(TX_AMP);
							break;
						case(1):
							ON(RX_LNA);
							OFF(TX_AMP);
							break;
						case(2):
							OFF(RX_LNA);
							ON(TX_AMP);
							break;
					};
					break;
				case BTN_LEFT:
					tl2=1-tl2;
					if (tl2){
						ON(MIXER_EN);
					}else{
						OFF(MIXER_EN);
					};
					break;
				case BTN_RIGHT:
					tr2=1-tr2;
					if (tr2){
						si5351c_power_down_all_clocks();
					}else{
						si5351c_set_clock_source(PLL_SOURCE_XTAL);
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};

		}else if (page==2){
			lcdPrint("SF: ");lcdPrint(IntToStr(sf,4,F_LONG)); lcdPrintln(" MHz");
			lcdPrintln("   Up SF +100");
			lcdPrintln("   Dn SF -100");
			tl3=GET(CE_VCO);
			lcdPrint(IntToStr(tl3,1,F_HEX)); lcdPrint(" "); lcdPrintln(" Lt CE_VCO");
			// lcdPrint("  "); lcdPrintln(" Rt ");
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_UP:
					sf+=100;
					mixer_set_frequency(sf);
					break;
				case BTN_DOWN:
					sf-=100;
					mixer_set_frequency(sf);
					break;
				case BTN_LEFT:
					tl3=1-tl3;
					if (tl3){
						ON(CE_VCO);
						mixer_setup();
						mixer_set_frequency(sf);
					}else{
						OFF(CE_VCO);
					};
					break;
				case BTN_RIGHT:
					tr3=1-tr3;
					if (tr3){
						;
					}else{
						;
					};
					break;
				case BTN_ENTER:
					page++;
					break;
			};

		}else if (page==3){
			if (tl4==0) {
				lcdPrintln("    Lt Exit");
				lcdPrintln("   ");
			} else {
				lcdPrintln("*_* ");
				lcdPrintln("    Rt Really?");
			}
			lcdPrintln("  "); 
			lcdPrintln("Enter for next page");
			lcdDisplay(); 
			switch(getInput()){
				case BTN_LEFT:
					tl4=1-tl4;
					break;
				case BTN_RIGHT:
					if (tl4) {
						return;
					};
					break;
				case BTN_ENTER:
					page++;
				case BTN_UP:
				case BTN_DOWN:
					tl4=0;
					break;
			};
		};
		if (page>3){page=0;}

		ON(LED1);
		delayNop(200000);
		OFF(LED1);
		delayNop(200000);

//		ctr++; lcdNl(); lcdPrint(IntToStr(ctr,4,F_HEX)); lcdDisplay();
	}
};
