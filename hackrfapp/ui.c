#include <r0ketlib/display.h>
#include <rad1olib/systick.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>

#include <string.h>

uint64_t freq = 0;
uint32_t sample_rate = 0;
uint32_t filter_bw = 0;
bool tx = false;

void sys_tick_handler(void){ /* every SYSTICKSPEED us */
	incTimer();
};

void ui_update(void)
{
    char tmp[100];
    uint32_t mhz;
    uint32_t khz;

	lcdClear();
	lcdPrint("HackRF Mode"); lcdNl();

	lcdPrint("Frequency:"); lcdNl();
    mhz = freq/1e6;
    khz = (freq - mhz * 1e6) / 1000;
    sprintf(tmp, " %4u.%03u MHz", mhz, khz); lcdPrint(tmp); lcdNl();

	lcdPrint("Sample Rate:"); lcdNl();
    mhz = sample_rate/1e6;
    khz = (sample_rate - mhz * 1e6) / 1000;
    sprintf(tmp, "   %2u.%03u MHz", mhz, khz); lcdPrint(tmp); lcdNl();

	lcdPrint("Filter Bandwidth:"); lcdNl();
    mhz = filter_bw/1e6;
    khz = (filter_bw - mhz * 1e6) / 1000;
    sprintf(tmp, "   %2u.%03u MHz", mhz, khz); lcdPrint(tmp); lcdNl();

	lcdPrint("Direction:"); lcdNl();
    tx ? lcdPrint(" TX") : lcdPrint(" RX"); lcdNl();

	lcdDisplay();
}

void ui_init(void)
{
    systickInit();
	lcdInit();
    ui_update();
}

void ui_setFrequency(uint64_t _freq)
{
    freq = _freq;
    ui_update();
}

void ui_setSampleRate(uint32_t _sample_rate)
{
    sample_rate = _sample_rate;
}
