#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include <rad1olib/pins.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include <rad1olib/systick.h>
#include <rad1olib/sdmmc.h>
#include <lpcapi/sdif_18xx_43xx.h>
#include <lpcapi/sdmmc_18xx_43xx.h>
#include <lpcapi/chip_lpc43xx.h>

#include <r0ketlib/fs_util.h>

static uint8_t buf[4096];
//# MENU rawperf
void menu_rawperf(void){
    lcdClear();

    sdmmc_setup();
    uint32_t r = sdmmc_acquire();

    if(!r) {
        lcdPrintln("No SD card found.");
        lcdDisplay();
        getInputWait();
        return;
    }
    lcdPrintln("SD card raw perf:");
    lcdPrintln("expect to lose the");
    lcdPrintln("first 100 MB and");
    lcdPrintln("press btn to cont.");
    lcdDisplay();
    getInputWait();

    lcdPrintln("ms to write 100MB:");
    lcdDisplay();
    uint32_t write_t = _timectr;
    for(int i=100*1024*1024/4096; i>0; i--) {
        if(i%1024 == 0) TOGGLE(LED2);
		Chip_SDMMC_WriteBlocks(LPC_SDMMC, buf, 4+(i*8), 8);
    }
    write_t = _timectr - write_t;

    lcdPrintln(IntToStr(write_t, 10, 0));
    lcdPrintln("ms to read 100MB:");
    lcdDisplay();
    uint32_t read_t = _timectr;
    for(int i=100*1024*1024/4096; i>0; i--) {
        if(i%1024 == 0) TOGGLE(LED2);
		Chip_SDMMC_ReadBlocks(LPC_SDMMC, buf, 4+(i*8), 8);
    }
    read_t = _timectr - read_t;

    lcdPrintln(IntToStr(read_t, 10, 0));
    lcdPrintln("press button");
    lcdDisplay();
    getInputWait();

    lcdPrintln("goodbye!");
    lcdDisplay();
}
