#ifndef _RFLIB_M0_H
#define _RFLIB_M0_H
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

void rflib_init();
void rflib_shutdown();

void rflib_set_freq(int64_t freq);
void rflib_set_bbamp(int lna_gain_db, int vga_gain_db, int txvga_gain_db);
void rflib_set_rxlna(int enable);
void rflib_set_txlna(int enable);
void rflib_set_rxsamplerate(int samplerate);
void rflib_set_rxdecimation(int decimation);
void rflib_set_rxbandwidth(int bandwidth);
void rflib_set_txsamplerate(int samplerate);
void rflib_set_txbandwidth(int bandwidth);

void rflib_bfsk_init();
void rflib_bfsk_receive();
void rflib_bfsk_transmit(uint8_t *data, uint16_t length, bool continue_receive);
int rflib_bfsk_get_packet(uint8_t* data, int max_len);

#endif // _RFLIB_M0_H
