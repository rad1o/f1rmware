#ifndef _M0RXTX_H
#define _M0RXTX_H 1

/* set mode, argument in m0_arg is operation mode to switch to */
#define CMD_SET_MODE 1


/* no operation mode: core will linger in wait-for-interrupt state */
#define MODE_OFF 0

/* simple receive mode:
 * this does a frequency shift by -fs/4, FIR filter [1, 3, 3, 1],
 * decimation by two, FIR filter [1, 3, 3, 1] again and another
 * decimation by two. Resulting sample rate is fs/4.
 *
 * overall gain is 64
 *
 * Maximum sample rate: 2500000
 */
#define MODE_RECEIVE 1

/* even more processing:
 * will return the phase angle of the samples 0-32767 (+/- 10?), atan2(i, q)
 *
 * Maximum sample rate: 1500000
 */
#define MODE_RECEIVE_ATAN2 2

/* BFSK transmit mode:
 * will transmit *tx_len bytes, stored at tx_data
 */
#define MODE_TRANSMIT_BFSK 256

/* memory location for RX ring buffer */
#define RX_BUF_BASE 0x20002400
/* size of RX ring buffer (will be split into two banks, allowed sizes 0x200, 0x100, 0x80, 0x40, 0x20, 0x10) */
#define RX_BUF_SIZE 0x200
#define RX_BANK_SIZE (RX_BUF_SIZE/2)

volatile uint32_t *const m0_command = (volatile uint32_t *const) 0x20002000;
volatile uint32_t *const m0_arg = (volatile uint32_t *const) 0x20002004;
volatile uint32_t *const m0_mode = (volatile uint32_t *const) 0x20002010;
volatile uint16_t *const tx_len = (volatile uint16_t *const) 0x20002040;
volatile uint8_t *const tx_data = (volatile uint8_t *const) 0x20002600;
volatile int16_t* *const rx_bank_ready = (volatile int16_t* *const) 0x20002080;

/* send an interrupt to the other core(s) */
inline void send_interrupt() {
    __asm volatile("dsb \n sev" : : : "memory");
}

inline void m0_set_mode(uint32_t mode) {
    *m0_command = CMD_SET_MODE;
    *m0_arg = mode;
    *m0_mode = 0;
    send_interrupt();
    while(*m0_mode != mode) { __asm volatile("nop" : : : "memory"); }
}

#endif // _M0RXTX_H
