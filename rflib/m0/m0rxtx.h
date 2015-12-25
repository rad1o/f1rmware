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
 *
 * Range 8-15 is reserved for TX modes
 */
#define MODE_TRANSMIT_BFSK 8

/* simple receive mode:
 * this does a frequency shift by -fs/4, FIR filter [1, 3, 3, 1],
 * decimation by two, FIR filter [1, 3, 3, 1] again and another
 * decimation by two. Resulting sample rate is fs/4.
 *
 * overall gain is 64
 *
 * Maximum sample rate: 2500000
 *
 * Range 16-31 is reserved for RX modes
 */
#define MODE_RECEIVE 16

/* even more processing:
 * will return the phase angle of the samples 0-65535 (+/- 10?), atan2(i, q)
 *
 * Maximum sample rate: 1500000
 */
#define MODE_RECEIVE_ATAN2 17

/* still more processing:
 * will return frequency (delta(atan2(I,Q)))
 */
#define MODE_RECEIVE_FREQ 18

/* still more processing:
 * will return decoded BFSK
 *
 * Only sample rate 1000000 is supported
 */
#define MODE_RECEIVE_BFSK 19

/* values for ACK SHM register: */

/* generic command acknowledgement,
 * also used for M0 ready notification on startup
 */
#define ACK_COMMAND_DONE    1
/* this is set when new RX data is ready */
#define ACK_NOTIFY_RX       2
/* TX is done */
#define ACK_TX_DONE         3
#define ACK_TX_ABORTED      4
/* RF setup has been done after switching to a new operation mode */
#define ACK_RF_SETUP        5

/* size of RX ring buffer (will be split into two banks) */
#define RX_BUF_SIZE 0x200
#define RX_BANK_SIZE (RX_BUF_SIZE/2)

/* size of TX data buffer */
#define TX_BUF_SIZE 0x100

/* we should probably get this from the linker somehow: */
#define M0_SHM_OFFSET 0x20007000
/* memory location for RX ring buffer */
volatile int16_t  *const rx_data = (volatile int16_t *const) (M0_SHM_OFFSET);
volatile uint8_t  *const tx_data = (volatile uint8_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE);

volatile uint32_t *const m0_command = (volatile uint32_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE);
volatile uint32_t *const m0_arg = (volatile uint32_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE + 4);
volatile uint32_t *const m0_arg2 = (volatile uint32_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE + 8);
volatile uint32_t *const m0_arg3 = (volatile uint32_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE + 12);

volatile uint32_t *const m0_ack = (volatile uint32_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE + 16);

volatile uint16_t *const tx_len = (volatile uint16_t *const) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE + 20);
volatile int16_t* *const volatile rx_bank_ready = (volatile int16_t* *const volatile) (M0_SHM_OFFSET + RX_BUF_SIZE + TX_BUF_SIZE + 24);
#endif // _M0RXTX_H
