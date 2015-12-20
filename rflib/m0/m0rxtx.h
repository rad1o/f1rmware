#ifndef _M0RXTX_H
#define _M0RXTX_H 1

/* set mode, argument in m0_arg is operation mode to switch to */
#define CMD_SET_MODE 1
#define CMD_SET_FREQ 2
#define CMD_SET_BBAMP 3
#define CMD_SET_TXLNA 4
#define CMD_SET_RXLNA 5
#define CMD_SET_RXSAMPLERATE 6
#define CMD_SET_RXDECIMATION 7
#define CMD_SET_RXBANDWIDTH 8
#define CMD_SET_TXSAMPLERATE 9
#define CMD_SET_TXBANDWIDTH 10

/* switch off RF path, power down CPLD and RF components */
#define MODE_OFF 0

/* standby: switch off RF, but keep everything running, wait for RX/TX. */
#define MODE_STANDBY 2

/* BFSK transmit mode:
 * will transmit *tx_len bytes, stored at tx_data
 */
#define MODE_TRANSMIT_BFSK 10

/* simple receive mode:
 * this does a frequency shift by -fs/4, FIR filter [1, 3, 3, 1],
 * decimation by two, FIR filter [1, 3, 3, 1] again and another
 * decimation by two. Resulting sample rate is fs/4.
 *
 * overall gain is 64
 *
 * Maximum sample rate: 2500000
 */
#define MODE_RECEIVE 256

/* even more processing:
 * will return the phase angle of the samples 0-65535 (+/- 10?), atan2(i, q)
 *
 * Maximum sample rate: 1500000
 */
#define MODE_RECEIVE_ATAN2 257

/* still more processing:
 * will return frequency (delta(atan2(I,Q)))
 */
#define MODE_RECEIVE_FREQ 258

/* still more processing:
 * will return decoded BFSK
 *
 * Only sample rate 1000000 is supported
 */
#define MODE_RECEIVE_BFSK 259

/* special value for ACK SHM register:
 * this is set when new RX data is ready */
#define ACK_NOTIFY_RX       0xFFFFFFFF
/* TX is done */
#define ACK_TX_DONE         0xFFFFFFFE
#define ACK_TX_ABORTED      0xFFFFFFFD
/* RF setup has been done after switching to a new operation mode */
#define ACK_RF_SETUP        0xFFFFFFFC
/* RF parts switched off, ready for halting the core */
#define ACK_SWITCHED_OFF    0xFFFFFF00

/* memory location for RX ring buffer */
#define RX_BUF_BASE 0x20002400
/* size of RX ring buffer (will be split into two banks, allowed sizes 0x200, 0x100, 0x80, 0x40, 0x20, 0x10) */
#define RX_BUF_SIZE 0x200
#define RX_BANK_SIZE (RX_BUF_SIZE/2)

volatile uint32_t *const m0_command = (volatile uint32_t *const) 0x20002000;
volatile uint32_t *const m0_arg = (volatile uint32_t *const) 0x20002004;
volatile uint32_t *const m0_arg2 = (volatile uint32_t *const) 0x20002008;
volatile uint32_t *const m0_arg3 = (volatile uint32_t *const) 0x2000200C;

volatile uint32_t *const m0_syn = (volatile uint32_t *const) 0x20002020;
volatile uint32_t *const m0_ack = (volatile uint32_t *const) 0x20002024;

volatile uint16_t *const tx_len = (volatile uint16_t *const) 0x20002040;
volatile uint8_t  *const tx_data = (volatile uint8_t *const) 0x20002600;
volatile int16_t* *const rx_bank_ready = (volatile int16_t* *const) 0x20002080;

/* send an interrupt to the other core(s) */
inline void send_interrupt() {
    __asm volatile("dsb \n sev" : : : "memory");
}

#endif // _M0RXTX_H
