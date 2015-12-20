#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/fonts.h>
#include <r0ketlib/fonts/smallfonts.h>
#include <r0ketlib/render.h>
#include <r0ketlib/fs_util.h>

#include <rad1olib/systick.h>
#include <rad1olib/draw.h>
#include <rad1olib/setup.h>
#include <rad1olib/pins.h>

#include <rf_path.h>
#include <hackrf_core.h>

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>

uint64_t freq = 0;
uint32_t sample_rate = 0;
uint32_t filter_bw = 0;
rf_path_direction_t direction;
uint32_t bblna_gain = 0;
uint32_t bbvga_gain = 0;
uint32_t bbtxvga_gain = 0;
bool lna_on = false;

#define BLACK       0b00000000
#define RED         0b11100000
#define RED_DARK    0b01100000
#define GREEN       0b00011100
#define GREEN_DARK  0b00001100
#define BLUE        0b00000011
#define WHITE       0b11111111
#define GREY        0b01001101

// Needed for some busy loops in the LCD code
void sys_tick_handler(void){ /* every SYSTICKSPEED us */
    incTimer();
};

void draw_frequency(void)
{
    char tmp[100];
    uint32_t mhz;
    uint32_t khz;

    mhz = freq/1e6;
    khz = (freq - mhz * 1e6) / 1000;

    setTextColor(BLACK, GREEN);
    setExtFont("UBUNTU18.F0N");
    sprintf(tmp, "%4u.%03u", (unsigned int)mhz, (unsigned int)khz); lcdPrint(tmp);

    setIntFont(&Font_7x8);
    lcdMoveCrsr(1, 18-7);
    lcdPrint("MHz");
}

void draw_tx_rx(void)
{
    uint8_t bg, fg;

    setExtFont("UBUNTU18.F0N");

    bg = BLACK;

    fg = GREY;
    if(direction == RF_PATH_DIRECTION_OFF) {
        fg = WHITE;
    }
    setTextColor(bg, fg);
    lcdPrint("OFF ");

    fg = GREY;
    if(direction == RF_PATH_DIRECTION_RX) {
        fg = GREEN;
    }
    setTextColor(bg, fg);
    lcdPrint("RX ");

    fg = GREY;
    if(direction == RF_PATH_DIRECTION_TX) {
        fg = RED;
    }
    setTextColor(bg, fg);
    lcdPrint("TX");

    setIntFont(&Font_7x8);
}

void hackrf_ui_update(void)
{
    char tmp[100];
    uint32_t mhz;
    uint32_t khz;

    lcdClear();
    lcdFill(0x00);

    drawHLine(0, 0, RESX - 1, WHITE);
    drawVLine(0, 0, RESY - 1, WHITE);

    drawHLine(RESY - 1, 0, RESX - 1, WHITE);
    drawVLine(RESX - 1, 0, RESY - 1, WHITE);

    lcdSetCrsr(25, 2);

    setTextColor(BLACK, GREEN);

    lcdPrint("HackRF Mode");lcdNl();

    drawHLine(11, 0, RESX - 1, WHITE);

    lcdSetCrsr(2, 12);
    draw_frequency();

    drawHLine(40, 0, RESX - 1, WHITE);

    lcdSetCrsr(6, 41);
    draw_tx_rx();
    drawHLine(69, 0, RESX - 1, WHITE);

    setTextColor(BLACK, WHITE);
    lcdSetCrsr(2, 71);
    lcdPrint("Rate:   ");
    mhz = sample_rate/1e6;
    khz = (sample_rate - mhz * 1e6) / 1000;
    sprintf(tmp, "%2u.%03u MHz", (unsigned int)mhz, (unsigned int)khz); lcdPrint(tmp); lcdNl();

    lcdMoveCrsr(2, 0);
    lcdPrint("Filter: ");
    mhz = filter_bw/1e6;
    khz = (filter_bw - mhz * 1e6) / 1000;
    sprintf(tmp, "%2u.%03u MHz", (unsigned int)mhz, (unsigned int)khz); lcdPrint(tmp); lcdNl();

    drawHLine(88, 0, RESX - 1, WHITE);

    setTextColor(BLACK, WHITE);
    lcdSetCrsr(2, 90);
    lcdPrint("      Gains"); lcdNl();

    setTextColor(BLACK, GREEN);
    lcdMoveCrsr(2, 2);
    lcdPrint("AMP: ");
    if(lna_on) {
        setTextColor(BLACK, RED);
        lcdPrint("ON ");
    } else {
        lcdPrint("OFF");
    }

    setTextColor(BLACK, RED_DARK);
    if(direction == RF_PATH_DIRECTION_TX) {
        setTextColor(BLACK, RED);
    }
    sprintf(tmp, " TX: %u dB", (unsigned int)bbtxvga_gain); lcdPrint(tmp); lcdNl();

    lcdMoveCrsr(2, 0);
    setTextColor(BLACK, GREEN_DARK);
    if(direction == RF_PATH_DIRECTION_RX) {
        setTextColor(BLACK, GREEN);
    }
    sprintf(tmp, "LNA: %2u dB", (unsigned int)bblna_gain); lcdPrint(tmp); lcdNl();
    lcdMoveCrsr(2, 0);
    sprintf(tmp, "VGA: %2u dB", (unsigned int)bbvga_gain); lcdPrint(tmp); lcdNl();

    lcdDisplay();

    // Don't ask...
    ssp1_set_mode_max2837();
}

void hackrf_ui_init(void)
{
    SETUPgout(RGB_LED);
    systickInit();
    fsInit();
    lcdInit();
    uint8_t pattern[8 * 3];
    memset(pattern, 0, sizeof(pattern));
    ws2812_sendarray(pattern, sizeof(pattern));
    hackrf_ui_update();
}

void hackrf_ui_setFrequency(uint64_t _freq)
{
    freq = _freq;
    hackrf_ui_update();
}

void hackrf_ui_setSampleRate(uint32_t _sample_rate)
{
    sample_rate = _sample_rate;
    hackrf_ui_update();
}

void hackrf_ui_setDirection(const rf_path_direction_t _direction)
{
    direction = _direction;
    hackrf_ui_update();
}

void hackrf_ui_setFilterBW(const uint32_t _filter_bw)
{
    filter_bw = _filter_bw;
    hackrf_ui_update();
}

void hackrf_ui_setLNAPower(bool _lna_on)
{
    lna_on = _lna_on;
    hackrf_ui_update();
}

void hackrf_ui_setBBLNAGain(const uint32_t gain_db)
{
    bblna_gain = gain_db;
    hackrf_ui_update();
}

void hackrf_ui_setBBVGAGain(const uint32_t gain_db)
{
    bbvga_gain = gain_db;
    hackrf_ui_update();
}

void hackrf_ui_setBBTXVGAGain(const uint32_t gain_db)
{
    bbtxvga_gain = gain_db;
    hackrf_ui_update();
}

