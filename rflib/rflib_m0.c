/*******************************************************************************
 * RFlib_M0
 *
 * interface for RF handling, uses M0 core for offloading lots of operations.
 *
 * (c) 2015 Hans-Werner Hilse (@hilse)
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
#include <string.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/ipc.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/cm3/vector.h>

/* DEBUGGING:
#include <libopencm3/lpc43xx/gpio.h>
#include <rad1olib/pins.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
*/

#include <portalib/fxpt_atan2.h>

#include "m0_bin.h"
#include "m0/m0rxtx.h"

/* this flag will get set back to false when a transfer is finished */
static volatile bool transmitting = false;
/* this flag indicates which mode should be started when a transfer is finished */
static volatile uint32_t mode_after_transmit = MODE_STANDBY;
static void (*rx_handler)() = NULL;

static uint32_t get_syn() {
    static uint32_t syn = 0;
    syn++;
    if(syn > 0xFFFF0000) syn = 0;
    return syn;
}

static void send_cmd(uint32_t cmd, uint32_t arg, uint32_t arg2, uint32_t arg3) {
    *m0_command = cmd;
    *m0_arg = arg;
    *m0_arg2 = arg2;
    *m0_arg3 = arg3;
    const uint32_t syn = get_syn();
    *m0_syn = syn;
    send_interrupt();
    do {
        /* wait until command has been handled */
        __asm__ volatile ("nop");
    } while(*m0_ack != syn);
}

/* interrupt handler: */
static void rflib_m0_isr() {
    CREG_M0TXEVENT = 0; /* clear interrupt flag */
    nvic_clear_pending_irq(NVIC_M0CORE_IRQ);
    switch(*m0_ack) {
        case ACK_NOTIFY_RX:
            if(rx_handler != NULL) (*rx_handler)();
            break;
        case ACK_TX_DONE:
            transmitting = false;
            send_cmd(CMD_SET_MODE, mode_after_transmit, 0, 0);
            break;
        case ACK_TX_ABORTED:
            /* this will happen when switching to another mode
             * during TX, so don't set the after-transmit mode
             * here
             */
            transmitting = false;
            break;
    }
}

void rflib_set_freq(int64_t freq) {
    const uint32_t f2 = freq & 0xFFFFFFFF;
    const uint32_t f1 = freq >> 32;
    send_cmd(CMD_SET_FREQ, f1, f2, 0);
}

void rflib_set_bbamp(int lna_gain_db, int vga_gain_db, int txvga_gain_db) {
    send_cmd(CMD_SET_BBAMP, lna_gain_db, vga_gain_db, txvga_gain_db);
}

void rflib_set_rxlna(int enable) {
    send_cmd(CMD_SET_RXLNA, enable, 0, 0);
}

void rflib_set_txlna(int enable) {
    send_cmd(CMD_SET_TXLNA, enable, 0, 0);
}

void rflib_set_rxsamplerate(int samplerate) {
    send_cmd(CMD_SET_RXSAMPLERATE, samplerate, 0, 0);
}

void rflib_set_rxdecimation(int decimation) {
    send_cmd(CMD_SET_RXDECIMATION, decimation, 0, 0);
}

void rflib_set_rxbandwidth(int bandwidth) {
    send_cmd(CMD_SET_RXBANDWIDTH, bandwidth, 0, 0);
}

void rflib_set_txsamplerate(int samplerate) {
    send_cmd(CMD_SET_TXSAMPLERATE, samplerate, 0, 0);
}

void rflib_set_txbandwidth(int bandwidth) {
    send_cmd(CMD_SET_TXBANDWIDTH, bandwidth, 0, 0);
}

/********************************* BPSK **********************************/

#define MAX_PACKET_LEN 255
/* packet buffer, 2 banks so a new receive doesn't overwrite the last one */
static volatile uint8_t rx_pkg[2*(MAX_PACKET_LEN+1)];
/* length of received data */
static volatile uint16_t rx_pkg_len;
static volatile uint16_t rx_pkg_ptr = 0;
static volatile bool rx_pkg_flag = false;

int rflib_bfsk_get_packet(uint8_t* data, int max_len) {
    if(!rx_pkg_flag) return -1;
    rx_pkg_flag = false;
    int read_len = 0;
    if(rx_pkg_len < max_len) max_len = rx_pkg_len;
    memcpy(data, (void*) &rx_pkg[rx_pkg_ptr], max_len);
    return max_len;
}

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
    static int16_t c = 0;
    static int16_t pkg_ptr;

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

            c = 0;
            pkg_ptr = (pkg_ptr + MAX_PACKET_LEN) % (2*MAX_PACKET_LEN);
            return;
        }
        if(c < pkglen) {
            rx_pkg[pkg_ptr + (c++)] = shift & 0xFF;
            if(c == pkglen) {
                rx_pkg_ptr = pkg_ptr;
                rx_pkg_len = c;
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
static void rxlib_bfsk_handler() {
    for(int i=0; i<(RX_BANK_SIZE/2); i+=2) {
        freq_detect((*rx_bank_ready)[i], (*rx_bank_ready)[i+1]);
    }
}

void rflib_bfsk_init() {
    rflib_set_rxsamplerate(2000000);
    rflib_set_rxdecimation(1);
    rflib_set_rxbandwidth(1750000);
    rflib_set_txsamplerate(4000000);
    rflib_set_txbandwidth(1750000);
}

void rflib_bfsk_receive() {
    while(transmitting) {
        __asm__ volatile ("nop");
    };
    rx_handler = rxlib_bfsk_handler;
    send_cmd(CMD_SET_MODE, MODE_RECEIVE, 0, 0);
}

void rflib_bfsk_transmit(uint8_t *data, uint16_t length, bool continue_receive) {
    /* wait for completion of last transfer */
    while(transmitting) {
        __asm__ volatile ("nop");
    };
    transmitting = true;
    if(continue_receive) {
        mode_after_transmit = MODE_RECEIVE;
        rx_handler = rxlib_bfsk_handler;
    }

    /* copy packet data to M0 handled buffer */
    memcpy((uint8_t*) tx_data, data, length);
    tx_len[0] = length;

    send_cmd(CMD_SET_MODE, MODE_TRANSMIT_BFSK, 0, 0);
}

/********************************* BPSK **********************************/

void rflib_init() {
    vector_table.irq[NVIC_M0CORE_IRQ] = rflib_m0_isr;
    nvic_set_priority(NVIC_M0CORE_IRQ, 1);
    nvic_enable_irq(NVIC_M0CORE_IRQ);

    ipc_halt_m0();
    unsigned char *cm0 = (unsigned char*) cm0_exec_baseaddr;
    for(int i=0; i<m0_bin_size; i++) cm0[i] = m0_bin[i];
    ipc_start_m0(cm0_exec_baseaddr);
}

void rflib_shutdown() {
    send_cmd(CMD_SET_MODE, MODE_OFF, 0, 0);
    while(*m0_mode != MODE_OFF) { __asm__ volatile ("nop"); }
    ipc_halt_m0();
}
