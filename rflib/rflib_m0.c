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

#include <r0ketlib/display.h>
#include <rad1olib/pins.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>

#include "m0_bin.h"
#include "m0/m0rxtx.h"

/* this flag will get set back to false when a transfer is finished */
static volatile bool transmitting = false;
/* this flag indicates which mode should be started when a transfer is finished */
static volatile uint32_t mode_after_transmit = MODE_STANDBY;
/* handler function to call for rx data */
static void (*rx_handler)() = NULL;
/* this flag will indicate that RF setup is due, using the SPI bus
 * (amongst others), so this can be used to avoid with M4 code using
 * the same chipset devices.
 */
static volatile bool in_rf_setup = false;
/* flag that indicates whether we await M0 core acknowledgement */
static volatile bool m0_wait_for_ack = false;

/* send a command to the M0 core
 *
 * do not wait for acknowledgement when this is called from ISR context, since
 * the acknowledgement will be delivered using the same interrupt mechanism
 * that is still handling the previous interrupt request, so it would
 * deadlock.
 */
static void send_cmd(const uint32_t cmd, const uint32_t arg, const uint32_t arg2, const uint32_t arg3, const bool wait_for_ack) {
    *m0_command = cmd;
    *m0_arg = arg;
    *m0_arg2 = arg2;
    *m0_arg3 = arg3;
    m0_wait_for_ack = wait_for_ack;
    /* send an interrupt to the other core(s) */
    __asm volatile("dsb \n sev" : : : "memory");
    while(m0_wait_for_ack == true) {
        /* wait until command has been handled */
        __asm__ volatile ("nop");
    }
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
            if(mode_after_transmit != MODE_STANDBY) {
                in_rf_setup = true;
            }
            send_cmd(CMD_SET_MODE, mode_after_transmit, 0, 0, false);
            break;
        case ACK_TX_ABORTED:
            /* this will happen when switching to another mode
             * during TX, so don't set the after-transmit mode
             * here
             */
            transmitting = false;
            break;
        case ACK_RF_SETUP:
            in_rf_setup = false;
            break;
        case ACK_COMMAND_DONE:
            m0_wait_for_ack = false;
            break;
    }
}

void rflib_wait_for_rfsetup() {
    while(in_rf_setup) { __asm__ volatile("nop"); }
}

void rflib_lcdDisplay() {
    while(in_rf_setup) { __asm__ volatile("nop"); }
    lcdDisplay();
}

void rflib_set_freq(const int64_t freq) {
    const uint32_t f2 = freq & 0xFFFFFFFF;
    const uint32_t f1 = freq >> 32;
    send_cmd(CMD_SET_FREQ, f1, f2, 0, true);
}

void rflib_set_bbamp(const int lna_gain_db, const int vga_gain_db, const int txvga_gain_db) {
    send_cmd(CMD_SET_BBAMP, lna_gain_db, vga_gain_db, txvga_gain_db, true);
}

void rflib_set_rxlna(const int enable) {
    send_cmd(CMD_SET_RXLNA, enable, 0, 0, true);
}

void rflib_set_txlna(const int enable) {
    send_cmd(CMD_SET_TXLNA, enable, 0, 0, true);
}

void rflib_set_rxsamplerate(const int samplerate) {
    send_cmd(CMD_SET_RXSAMPLERATE, samplerate, 0, 0, true);
}

void rflib_set_rxdecimation(const int decimation) {
    send_cmd(CMD_SET_RXDECIMATION, decimation, 0, 0, true);
}

void rflib_set_rxbandwidth(const int bandwidth) {
    send_cmd(CMD_SET_RXBANDWIDTH, bandwidth, 0, 0, true);
}

void rflib_set_txsamplerate(const int samplerate) {
    send_cmd(CMD_SET_TXSAMPLERATE, samplerate, 0, 0, true);
}

void rflib_set_txbandwidth(const int bandwidth) {
    send_cmd(CMD_SET_TXBANDWIDTH, bandwidth, 0, 0, true);
}

/********************************* BFSK **********************************/
/* interrupt handler:
 *
 * M0 signals that there's RX data ready to be read
 */
static volatile bool rx_pkg_flag = false;

int rflib_bfsk_get_packet(uint8_t* const data, const int max_len) {
    if(!rx_pkg_flag) return -1;
    rx_pkg_flag = false;
    int read_len = ((uint8_t*)*rx_bank_ready)[0];
    if(read_len > max_len) read_len = max_len;
    memcpy(data, (uint8_t*)*rx_bank_ready + 1, max_len);
    return read_len;
}

static void rxlib_receive_handler() {
    rx_pkg_flag = true;
}

void rflib_bfsk_init() {
    rflib_set_rxsamplerate(1000000);
    rflib_set_rxdecimation(2);
    rflib_set_rxbandwidth(1750000);
    rflib_set_txsamplerate(4000000);
    rflib_set_txbandwidth(1750000);
}

void rflib_bfsk_receive() {
    while(transmitting) {
        __asm__ volatile ("nop");
    };
    rx_handler = rxlib_receive_handler;
    in_rf_setup = true;
    send_cmd(CMD_SET_MODE, MODE_RECEIVE_BFSK, 0, 0, true);
}

void rflib_bfsk_transmit(uint8_t* const data, const uint16_t length, const bool continue_receive) {
    /* wait for completion of last transfer */
    while(transmitting) {
        __asm__ volatile ("nop");
    };
    transmitting = true;
    if(continue_receive) {
        mode_after_transmit = MODE_RECEIVE_BFSK;
        rx_handler = rxlib_receive_handler;
    }

    /* copy packet data to M0 handled buffer */
    memcpy((uint8_t*) tx_data, data, length);
    tx_len[0] = length;

    in_rf_setup = true;
    send_cmd(CMD_SET_MODE, MODE_TRANSMIT_BFSK, 0, 0, true);
}

/*************************************************************************/

int rflib_get_data(int16_t* const data, const int max_len) {
    if(!rx_pkg_flag) return -1;
    rx_pkg_flag = false;
    int read_len = RX_BANK_SIZE;
    if(RX_BANK_SIZE > max_len) read_len = max_len;
    memcpy((uint8_t*) data, (uint8_t*) *rx_bank_ready, read_len);
    return read_len;
}

void rflib_freq_receive() {
    while(transmitting) {
        __asm__ volatile ("nop");
    };
    rx_handler = rxlib_receive_handler;
    in_rf_setup = true;
    send_cmd(CMD_SET_MODE, MODE_RECEIVE_FREQ, 0, 0, true);
}

void rflib_raw_receive() {
    while(transmitting) {
        __asm__ volatile ("nop");
    };
    rx_handler = rxlib_receive_handler;
    in_rf_setup = true;
    send_cmd(CMD_SET_MODE, MODE_RECEIVE, 0, 0, true);
}

/*************************************************************************/

void rflib_init() {
    vector_table.irq[NVIC_M0CORE_IRQ] = rflib_m0_isr;
    nvic_set_priority(NVIC_M0CORE_IRQ, 1);
    nvic_enable_irq(NVIC_M0CORE_IRQ);

    ipc_halt_m0();

    m0_wait_for_ack = true;
    unsigned char *cm0 = (unsigned char*) cm0_exec_baseaddr;
    for(int i=0; i<m0_bin_size; i++) cm0[i] = m0_bin[i];
    ipc_start_m0((uintptr_t)cm0_exec_baseaddr);

    while(m0_wait_for_ack == true) {
        /* wait until command has been handled */
        __asm__ volatile ("nop");
    }
}

void rflib_shutdown() {
    send_cmd(CMD_SET_MODE, MODE_OFF, 0, 0, true);
    nvic_disable_irq(NVIC_M0CORE_IRQ);
    ipc_halt_m0();
}
