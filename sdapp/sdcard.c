#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/itoa.h>

#include <rad1olib/sdmmc.h>
#include <lpcapi/sdif_18xx_43xx.h>
#include <lpcapi/sdmmc_18xx_43xx.h>
#include <lpcapi/chip_lpc43xx.h>

#include "msc_main.h"
#include <lpcapi/msc/msc_main.h>

//# MENU SD_USB
void menu_sdcard(void){
    lcdClear();

    sdmmc_setup();
    uint32_t r = sdmmc_acquire();

    if(r) {
        lcdPrintln("SD card found.");
        lcdPrintln("No. of blocks:");
        lcdPrintln(IntToStr(Chip_SDMMC_GetDeviceBlocks(LPC_SDMMC), 10, 0));
        lcdPrintln("in USB MSC mode");
        // SD I/O needs higher interrupt prio than USB so it works within
        // the USB callbacks
        NVIC_SetPriority(USB0_IRQn, 2);
        NVIC_SetPriority(SDIO_IRQn, 0);
        SD_MSCenable();
    } else {
        lcdPrintln("No SD card found.");
    }
    lcdDisplay();
    while(getInputRaw()==BTN_NONE){
        /* wait */
        __asm__ volatile ("nop");
    };

    lcdPrintln("goodbye!");
    lcdDisplay();
    MSCdisable();
    Chip_SDIF_DeInit(LPC_SDMMC);
}
