#ifndef _RFLIB_M0_H
#define _RFLIB_M0_H
/*******************************************************************************
 * RFlib_M0
 *
 * interface for RF handling, uses M0 core for offloading lots of operations.
 * RFlib
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

/* initialize M0 core for operation */
void rflib_init();

/* shut down RF parts and put M0 core in halt mode */
void rflib_shutdown();

/* if setup of RF parts is in progress, wait until that is done */
void rflib_wait_for_rfsetup();

/* convenience wrapper around lcdDisplay() so it doesn't conflict
 * with the M0 core using the SPI bus
 */
void rflib_lcdDisplay();

/* set operational frequency */
void rflib_set_freq(const int64_t freq);

/* set amp settings:
 * lna_gain_db: 8dB increments, up to 40dB, default = 8 dB
 * vga_gain_db: 2dB increments, up to 62dB, default = 20 dB
 * txvga_gain_db; 1dB increments, up to 47dB, default = 30 dB
 */
void rflib_set_bbamp(const int lna_gain_db, const int vga_gain_db, const int txvga_gain_db);

/* enable (1) or disable (0) RX low noise amplifier */
void rflib_set_rxlna(const int enable);

/* enable (1) or disable (0) TX low noise amplifier */
void rflib_set_txlna(const int enable);

/* set receive sample rate
 * NOTE: in order to avoid division on calculation on the M0 core,
 * this is to be set to the sample rate *after* decimation.
 * So the RF parts are set to rxsamplerate*rxdecimation.
 */
void rflib_set_rxsamplerate(const int samplerate);

/* set decimation factor that is done in the CPLD.
 * Default is 1, which means that no samples are to be dropped
 * (that is what the CPLD does when we "decimate")
 */
void rflib_set_rxdecimation(const int decimation);

/* set band filter bandwidth for RX operation */
void rflib_set_rxbandwidth(const int bandwidth);

/* set sample rate for TX modes */
void rflib_set_txsamplerate(const int samplerate);

/* set band filter bandwidth for TX operation - no idea if it is really
 * in the RF path then, might be very well a no-op.
 */
void rflib_set_txbandwidth(const int bandwidth);

/* put M0 core and RF parts into raw sample receive mode.
 * will return immediately.
 * Poll for samples using the rflib_get_data() function.
 * Samples will be int16_t I, int16_t Q.
 * Maximum sample rate is about 2500000.
 */
void rflib_raw_receive();

/* put M0 core and RF parts into frequency receive mode.
 * will return immediately.
 * Poll for freq data using the rflib_get_data() function.
 * Will return interleaved frequency (in +-pi/32768 int16_t)
 * and signal strength (^2), also int16_t.
 * Maximum sample rate is probably about 1500000.
 */
void rflib_freq_receive();

/* poll for received samples / freq data.
 * the data is copied to the location pointed to by
 * the "data" pointer.
 * The max_len parameter specifies how big that buffer is.
 * will return the number of bytes copied,
 * -1 if there is no new data
 */
int rflib_get_data(int16_t* const data, const int max_len);

/* initialize sample rate, bandwidth and decimation for BFSK RX/TX */
void rflib_bfsk_init();

/* put M0 core and RF parts into BFSK receive mode.
 * this will return immediately. You can use rflib_bfsk_get_packet() in
 * order to poll for received packets.
 */
void rflib_bfsk_receive();

/* switch to transmit mode and transmit a packet.
 * maximum length at the moment is 255 bytes.
 * if continue_receive is set to true, the M0 and RF parts will switch
 * to BFSK receive mode as soon as the transmit is done.
 */
void rflib_bfsk_transmit(uint8_t* const data, const uint16_t length, const bool continue_receive);

/* poll for a received packet.
 * if a packet was received, the data is copied to the location pointed to by
 * the "data" pointer.
 * The max_len parameter specifies how big that buffer is.
 * will return the number of bytes received in the packet.
 * -1 if there is no new packet (there can be 0-byte packets - which
 *  don't really make sense, though, but will correctly return 0 here).
 */
int rflib_bfsk_get_packet(uint8_t* const data, const int max_len);

#endif // _RFLIB_M0_H
