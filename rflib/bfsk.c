/* BPSK RX/TX
 *
 * Copyright (C) 2015 Hans-Werner Hilse <hwhilse@gmail.com>
 *
 * some parts (receive/filters) are
 *   Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>

#include <rad1olib/pins.h>

#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/tuning.h>
#include <common/max2837.h>
#include <common/streaming.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/cm3/vector.h>

#include <stddef.h>
#include <portalib/arm_intrinsics.h>
#include <portalib/complex.h>
#include <portalib/fxpt_atan2.h>

#include <lpcapi/cdc/cdc_main.h>
#include <lpcapi/cdc/cdc_vcom.h>
#include <string.h>

#include <libopencm3/lpc43xx/ipc.h>

#include "m0_bin.h"
#include "m0/m0rxtx.h"

// default to 2496 MHz
#define FREQSTART 2496000000

static int64_t frequency = FREQSTART;

static void my_set_frequency(const int64_t new_frequency, const int32_t offset) {
    const int64_t tuned_frequency = new_frequency + offset;
    ssp1_set_mode_max2837();
    if(set_freq(tuned_frequency)) {
        frequency = new_frequency;
    }
}

static bool lna_enable = true;
static bool txlna_enable = true;
static int32_t lna_gain_db = 8;
static int32_t vga_gain_db = 20;
static int32_t txvga_gain_db = 30;

/* set amps */
static void set_rf_params() {
    ssp1_set_mode_max2837(); // need to reset this since display driver will hassle with SSP1
    rf_path_set_lna(lna_enable ? 1 : 0);
    max2837_set_lna_gain(lna_gain_db);     /* 8dB increments */
    max2837_set_vga_gain(vga_gain_db);     /* 2dB increments, up to 62dB */
    max2837_set_txvga_gain(txvga_gain_db); /* 1dB increments, up to 47dB */
}

/* portapack_init plus a bunch of stuff from here and there, cleaned up */
static void rfinit() {
    /* Release CPLD JTAG pins */
    scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
    scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    GPIO_DIR(PORT_CPLD_TDO) &= ~PIN_CPLD_TDO;
    GPIO_DIR(PORT_CPLD_TCK) &= ~PIN_CPLD_TCK;
    GPIO_DIR(PORT_CPLD_TMS) &= ~PIN_CPLD_TMS;
    GPIO_DIR(PORT_CPLD_TDI) &= ~PIN_CPLD_TDI;

    hackrf_clock_init();
    rf_path_pin_setup();

    /* Configure external clock in */
    scu_pinmux(SCU_PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

    /* Disable unused clock outputs. They generate noise. */
    scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
    scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

    sgpio_configure_pin_functions();

    ON(EN_VDD);
    ON(EN_1V8);
    delayNop(250000); // doesn't work without

    cpu_clock_set(204); // WARP SPEED! :-)
    si5351_init();

    cpu_clock_pll1_max_speed();
    
    ssp1_init();

    rf_path_init();
}

/* ---------------------------- RECEIVE: -------------------------------- */

/* frequency offset is -375kHz because of the shift done by the first filter
 * (to get the DC peak out of the way)
 */
#define RX_FREQOFFSET (-500000)
#define RX_BANDWIDTH  1750000
#define RX_SAMPLERATE 2000000
#define RX_DECIMATION 1 /* effective sample rate is 2000000 */


#define MAX_PACKET_LEN 255
/* packet buffer */
static volatile uint8_t rx_pkg[MAX_PACKET_LEN+1];
/* length of received data */
static volatile uint32_t rx_pkg_len;
static volatile bool rx_pkg_flag = false;

/* we use a peamble of 0xFFF0 - so mostly 1 bits - in order to keep the
 * phase constant as long as possible during the time the PLL is still about
 * to lock. The PLL should lock when a few other 1-bits (0xFFFF) are sent
 * before sending this peamble. So when sending, just send 0xFFFFFFF0.
 *
 * the row of 1 bits will put a running decoding out of sync if
 * encountered for whatever reason during data decode
 */
#define PREAMBLE 0xFFF0
static void decoder(uint8_t bit) {
    /* shift register for received data */
    static uint16_t shift = 0;
    /* flag to determine whether we have seen a header and are in sync
     * also, counter for received data bits (+1)
     */
    static uint8_t sync = 0;
    /* number of bytes in packet to receive */
    static int16_t pkglen;

    shift <<= 1;
    shift |= bit;
    if(sync == 0) {
        /* wait for sync */
        if(shift == PREAMBLE) {
            pkglen = -1;
            sync = 1;
        }
    } else if(sync == 9) {
        /* the first of every 9 bits must be 0, otherwise, lose sync.*/
        if(shift & 0x100)
            goto desync;

        /* start new byte */
        sync = 1;

        /* check if we're at the start of a new packet */
        if(pkglen == -1) {
            /* we have already received one byte */
            pkglen = shift & 0xFF;
            if(pkglen > MAX_PACKET_LEN)
                goto desync;

            rx_pkg_len = 0;
            return;
        }
        if(rx_pkg_len < pkglen) {
            rx_pkg[rx_pkg_len++] = shift & 0xFF;
            if(rx_pkg_len == pkglen) {
                rx_pkg_flag = true;
            }
        }
        return;
    } else {
        sync++;
    }
    return;

desync:
    sync = 0;
    pkglen = -1;
    return;
}

#define MAX(x,y) (x>y?x:y)
#define MIN(x,y) (x<y?x:y)

/* for now, this allows a range of -8192 to 8192, allowing for
 * a 62.5kHz difference in TX/RX center frequency
 */
#define CLAMP_OFFSET 8192
/* try to lock the offset into a window of this size:
 * (a bit more than the mathematically exact window size to
 * avoid being too jittery)
 */
#define WINDOW_SIZE 8600
static void freq_detect(int16_t sample_i, int16_t sample_q) {
    /* phase difference offset for signal detection
     */
    static int16_t offset;

    /* previous sample */
    static int16_t prev_i, prev_q;

    /* at first, we calculate the phase difference between the last
     * sample and the current one, corresponding to the signal's
     * frequency
     */
    int32_t diff_i = sample_q * (-prev_i) + sample_i * prev_q;
    int32_t diff_q = sample_q * prev_q - sample_i * (-prev_i);

    /* keep as much precision as we can */
    while(diff_q > 0x7FFF || diff_q < -0x8000 || diff_i > 0x7FFF || diff_i < -0x8000) {
        diff_q >>= 1;
        diff_i >>= 1;
    }

    const int16_t p_sample = fxpt_atan2(diff_i, diff_q);

    prev_i = sample_i;
    prev_q = sample_q;

    /* we adapt offset in a way that a maximum phase difference of
     * WINDOW_SIZE/2 is kept.
     */
    const int16_t p_diff = p_sample - offset;
    const int16_t p_upper_diff = p_diff - (WINDOW_SIZE/2);
    const int16_t p_lower_diff = p_diff + (WINDOW_SIZE/2);
    if(p_upper_diff > 0) {
        offset = MIN(
            CLAMP_OFFSET, /* upper bound for offset */
            offset + MIN(
                /* offset correction by
                 * 1/8 of effective phase difference */
                p_upper_diff >> 3,
                /* but no more than 512 */
                512
            )
        );
    } else if(p_lower_diff < 0) {
        offset = MAX(
            -CLAMP_OFFSET, /* lower bound for offset */
            offset + MAX(
                /* offset correction by
                 * 1/8 of effective phase difference */
                p_lower_diff >> 3,
                /* but no more than -512 */
                -512
            )
        );
    }

    /* symbol recognition: */

    /* we store the p_diff value here so we have the value for
     * the previous sample
     */
    static int16_t prev_p_diff = 0;

    /* samples since we've last seen a symbol switch */
    static uint16_t last_switch = 0;

    /* at the current baud rate, we have 4 samples per symbol.
     * we require two consecutive samples to have a frequency either
     * higher (=1) or lower (=0) than the offset which has been fit
     * to the signal in order to recognize a symbol.
     *
     * when this condition is not met, we interpret this as a symbol
     * switch.
     *
     * a symbol switch will reset a sample counter.
     * The sample counter is used to send a new symbol to the
     * decoder only every 4 samples.
     *
     * so this synchronizes on symbol switches.
     *
     *       |__          __          ______
     * f:  0_|..\......../..\......../......
     *       |   \______/    \______/
     * 
     * p_diff: ++--------+++---------+++++++  (+: >0, -: <0)
     * pp_dif:  ++--------+++---------++++++
     * recogn: 11s0000000s11S00000000s111111  (S/s: switch early/late)
     * l_sw  : ++R+++++++R++R++++++++R++++++  (R: last switch ctr reset)
     * output:    0   0   1  0   0    1   1
     */
    if((p_diff > 0) && (prev_p_diff > 0)) {
        if(1 == (last_switch % 4)) {
            decoder(0);
        }
    } else if((p_diff < 0) && (prev_p_diff < 0)) {
        if(1 == (last_switch % 4)) {
            decoder(1);
        }
    } else {
        last_switch = 0;
    }
    last_switch++;
    prev_p_diff = p_diff;
}

/* interrupt handler:
 *
 * M0 signals that there's RX data ready to be read
 */
void bfsk_isr_rx() {
    CREG_M0TXEVENT = 0; /* clear interrupt flag */
    nvic_clear_pending_irq(NVIC_M0CORE_IRQ);

    for(int i=0; i<(RX_BANK_SIZE/2); i+=2) {
        freq_detect((*rx_bank_ready)[i], (*rx_bank_ready)[i+1]);
    }
}

static void receive() {
    /* wait for completion of last transfer */
    //while(transmitting) {};

    //sgpio_cpld_stream_disable();
    /* set up RX mode */
    vector_table.irq[NVIC_M0CORE_IRQ] = bfsk_isr_rx;
    /* set M0 operation mode */
    m0_set_mode(MODE_RECEIVE);
    /* set up RF path */
    rf_path_set_direction(RF_PATH_DIRECTION_RX);
    /* set RX frequency */
    my_set_frequency(frequency, RX_FREQOFFSET);
    /* set TX sample rate */
    sample_rate_frac_set(RX_SAMPLERATE * 2, 1);
    /* and LPF */
    baseband_filter_bandwidth_set(RX_BANDWIDTH);
    /* set up decimation in CPLD */
    sgpio_cpld_stream_rx_set_decimation(RX_DECIMATION);

    sgpio_cpld_stream_enable();
}

/* ---------------------------- TRANSMIT: ------------------------------- */

/* transmit options */
#define TX_BANDWIDTH  3000000
#define TX_SAMPLERATE 4000000
#define TX_FREQOFFSET (-62500)

/* this flag will get set back to false when a transfer is finished */
static volatile bool transmitting = false;
/* this flag indicates if receive mode should be started when a transfer is finished */
static volatile bool receive_after_transmit = true;
/* interrupt handler:
 *
 * M0 signals that transfer is completed
 */
void bfsk_isr_tx() {
    CREG_M0TXEVENT = 0; /* clear interrupt flag */
    nvic_clear_pending_irq(NVIC_M0CORE_IRQ);

    sgpio_cpld_stream_disable();
    rf_path_set_direction(RF_PATH_DIRECTION_OFF);

    transmitting = false;
    if(receive_after_transmit) receive();
}

static void transmit(uint8_t *data, uint16_t length) {
    /* wait for completion of last transfer */
    while(transmitting) {};
    transmitting = true;

    /* if in receive mode, stop generating samples */
    sgpio_cpld_stream_disable();

    /* set M0 operation mode */
    m0_set_mode(MODE_OFF);

    /* set up TX mode */
    vector_table.irq[NVIC_M0CORE_IRQ] = bfsk_isr_tx;
    /* set up RF path */
    rf_path_set_direction(RF_PATH_DIRECTION_TX);
    /* set TX frequency */
    my_set_frequency(frequency, TX_FREQOFFSET);
    /* set TX sample rate */
    sample_rate_frac_set(TX_SAMPLERATE * 2, 1);
    /* and LPF */
    baseband_filter_bandwidth_set(TX_BANDWIDTH);

    /* copy packet data to M0 handled buffer */
    memcpy((uint8_t*) tx_data, data, length);
    tx_len[0] = length;
    m0_set_mode(MODE_TRANSMIT_BFSK);

    sgpio_cpld_stream_enable();
}
/* ------------------------------------------------------------------- */

void m0_init() {
    vector_table.irq[NVIC_M0CORE_IRQ] = bfsk_isr_rx;
    nvic_set_priority(NVIC_M0CORE_IRQ, 1);
    nvic_enable_irq(NVIC_M0CORE_IRQ);

    ipc_halt_m0();
    unsigned char *cm0 = (unsigned char*) cm0_exec_baseaddr;
    for(int i=0; i<m0_bin_size; i++) cm0[i] = m0_bin[i];
    ipc_start_m0(cm0_exec_baseaddr);
}

//# MENU BPSK
void bfsk_menu() {
    lcdClear();
    lcdPrintln("ENTER to go back");
    lcdPrintln("L/R/U/D to xmit");
    lcdDisplay();
    getInputWaitRelease();

    cpu_clock_set(204);
    CDCenable();

    m0_init();

    delayms(10);

    rfinit();
    set_rf_params();
    receive();

    while(1) {
        switch (getInputRaw()) {
            case BTN_UP:
                transmit((uint8_t*)"up", 2);
                getInputWaitRelease();
                break;
            case BTN_DOWN:
                transmit((uint8_t*)"down", 4);
                getInputWaitRelease();
                break;
            case BTN_RIGHT:
                transmit((uint8_t*)"right", 5);
                getInputWaitRelease();
                break;
            case BTN_LEFT:
                transmit((uint8_t*)"left", 4);
                getInputWaitRelease();
                break;
            case BTN_ENTER:
                goto stop;
        }

        if(rx_pkg_flag) {
            rx_pkg_flag = false;
            rx_pkg[rx_pkg_len] = 0; /* ensure string termination */
            lcdPrintln(IntToStr(rx_pkg_len, 3, F_LONG));
            lcdPrintln((char*)rx_pkg);
            lcdDisplay();

            /* also: write to USB-CDC */
            if(vcom_connected()) vcom_write((uint8_t*)rx_pkg, rx_pkg_len);
        }
        if(vcom_connected()) {
            /* check if we got data from USB-CDC, transmit it, if so. */
            uint8_t sendbuf[MAX_PACKET_LEN];
            uint32_t read = vcom_bread(sendbuf, MAX_PACKET_LEN);
            if(read > 0) transmit(sendbuf, read);
        }
    }
stop:
    sgpio_cpld_stream_disable();
    OFF(EN_1V8);
    OFF(EN_VDD);
    return;
}
