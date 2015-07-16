#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>
#include <rad1olib/pins.h>

#include <rad1olib/setup.h>
#include <lpcapi/cdc/cdc_main.h>
#include <lpcapi/cdc/cdc_vcom.h>
#include <libopencmsis/core_cm3.h>

//# MENU CDC
void cdc_menu(){
    uint32_t prompt = 0, rdCnt = 0;
    static uint8_t g_rxBuff[256];
    CDCenable();
    lcdPrintln("CDC enabled.");
    getInputWaitRelease();

    while(getInputRaw()!=BTN_ENTER){
	if(getInputRaw()==BTN_RIGHT){
	    lcdPrint("vc:");
	    lcdPrint(IntToStr(g_vCOM.tx_flags,3,F_LONG));
	    lcdPrintln(".");
	    lcdPrint("pt:");
	    lcdPrint(IntToStr(prompt,3,F_LONG));
	    lcdPrintln(".");
	    lcdDisplay();
	    getInputWaitRelease();
	};
	if(getInputRaw()==BTN_LEFT){
	    vcom_write((uint8_t *)"Hello World!!\r\n", 15);
	    prompt=1;
	    getInputWaitRelease();
	};
	if ((vcom_connected() != 0) && (prompt == 0)) {
	    vcom_write((uint8_t *)"Hello World!\r\n", 14);
	    prompt = 1;
	}
	/* If VCOM port is opened echo whatever we receive back to host. */
	if (prompt) {
	    rdCnt = vcom_bread(&g_rxBuff[0], 256);
	    if (rdCnt) {
		vcom_write((uint8_t*)"[", 1);
		while(g_vCOM.tx_flags & VCOM_TX_BUSY) __WFI();
		vcom_write(&g_rxBuff[0], rdCnt);
		while(g_vCOM.tx_flags & VCOM_TX_BUSY) __WFI();
		vcom_write((uint8_t*)"]", 1);
	    }
	}
	/* Sleep until next IRQ happens */
	__WFI();
    };
    lcdPrintln("disconnect");
    lcdDisplay();
    CDCdisable();
    getInputWaitRelease();
}
