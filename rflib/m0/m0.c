#include <libopencm3/lpc43xx/m0/nvic.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/creg.h>

#include "m0rxtx.h"
#include "cossin1024.h"

volatile int mode;

/* interrupt handler for interrupts triggered by the M4 core */
void m4core_ipc_isr() {
    CREG_M4TXEVENT = 0; /* clear interrupt flag */
    nvic_clear_pending_irq(NVIC_M4CORE_IRQ);
    /* handle command */
    switch(*m0_command) {
        case CMD_SET_MODE:
            mode = *m0_arg;
            break;
    }
}

/* atan2 CORDIC approximation
 *
 * special feature: no divison!
 * thus it is fit especially well for the M0 core
 *
 * (c) 2009, Jasper Vijn
 * http://www.coranac.com/documents/arctangent/
 *
 * adapted to generate 1 bit less precision in order to be able to
 * work at a sample rate of 1500000
 */

#define BRAD_PI_SHIFT 14
#define BRAD_PI (1<<BRAD_PI_SHIFT)
// Get the octant a coordinate pair is in.
#define OCTANTIFY(_x, _y, _o)   do {                            \
    int _t; _o= 0;                                              \
    if(_y<  0)  {            _x= -_x;   _y= -_y; _o += 4; }     \
    if(_x<= 0)  { _t= _x;    _x=  _y;   _y= -_t; _o += 2; }     \
    if(_x<=_y)  { _t= _y-_x; _x= _x+_y; _y=  _t; _o += 1; }     \
} while(0);
static const uint16_t atan2Cordic_list[] = { 
    0x4000, 0x25C8, 0x13F6, 0x0A22, 0x0516, 0x028C, 0x0146, 0x00A3, 
    0x0051, 0x0029, 0x0014, 0x000A, 0x0005, 0x0003, 0x0001, 0x0001
};
// atan via CORDIC (coordinate rotations).
// Returns [0,2pi), where pi ~ 0x4000.
// atan(2^-i) terms using PI=0x10000 for accuracy
static inline uint16_t atan2Cordic(int x, int y)
{
    if(y==0)    return (x>=0 ? 0 : BRAD_PI);

    int phi;

    OCTANTIFY(x, y, phi);
    phi *= BRAD_PI/4;

    // Scale up a bit for greater accuracy.
    if(x < 0x10000)
    { 
        x *= 0x1000;
        y *= 0x1000;
    }

    int i, tmp, dphi=0;
    for(i=1; i<11; i++)
    {
        if(y>=0)
        {
            tmp= x + (y>>i);
            y  = y - (x>>i);
            x  = tmp;
            dphi += atan2Cordic_list[i];
        }
        else
        {
            tmp= x - (y>>i);
            y  = y + (x>>i);
            x  = tmp;
            dphi -= atan2Cordic_list[i];
        }
    }
    return phi + (dphi>>2);
}

/* write an RX sample to ring buffer */
static int rxtobuf(int16_t* const buf, int16_t i, int16_t q) {
    buf[0] = i;
    buf[1] = q;
    return 2;
}

/* further process an RX sample: atan2(i, q) */
static int rxatan2tobuf(int16_t* const buf, int16_t i, int16_t q) {
    *buf = atan2Cordic(i, q);
    return 1;
}

/* this allows access to the single parts of a sample data word from
 * the SGPIO. This relies on undefined behaviour C code, however.
 * Makes life so much easier that we ignore this fact.
 */
typedef union {
    uint32_t v32;
    struct {
        int8_t i0;
        int8_t q0;
        int8_t i1;
        int8_t q1;
    } __attribute__((packed));
} sgpio_val_t;

/* main handler for receiving data from the SGPIO
 *
 * we don't use an interrupt handler for this to save the overhead
 * of constant entry/exit.
 */
void receive() {
    /* sample processing function, defaults to write out */
    int (*outfunc)(int16_t *const, int16_t, int16_t) = rxtobuf;
    /* other mode(s): */
    if(mode & MODE_RECEIVE_ATAN2) {
        outfunc = rxatan2tobuf;
    }

    /* ring buffer pointer */
    int16_t* buf = (int16_t*) RX_BUF_BASE;
    /* reference to start of current bank */
    int16_t* oldbuf = buf;

    while(1) {
        sgpio_val_t v7;
        int16_t t6i, t6q, t7i, t7q;

        /* wait for flag that data is ready */
        while(!(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A))) {
            /* check for exit condition here so we don't lock up when
             * the SGPIO stops getting data
             */
            if((mode & (MODE_RECEIVE|MODE_RECEIVE_ATAN2)) == 0) return;
        };
        /* clear data ready flag again
         * (Should we do this after reading? Probably not, so we can "run after" one
         * slightly overlong turn. TODO: think this over carefully.)
         */
        SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

        /* this does a frequency shift by -fs/4,
         * FIR filter [1, 3, 3, 1]
         * decimation by two
         * FIR filter [1, 3, 3, 1] again
         * and another decimation by two.
         *
         * overall gain is 64
         *
         * frequency is a quarter of input frequency
         */
        const sgpio_val_t v0 = (sgpio_val_t) SGPIO_REG_SS(11);

        const int16_t t0i = 3 * (v0.i0 - v7.q1) + (v0.q1 - v7.i0);
        const int16_t t0q = 3 * (v0.q0 + v7.i1) - (v0.i1 + v7.q0);

        const sgpio_val_t v1 = (sgpio_val_t) SGPIO_REG_SS(5);

        const int16_t t1i = (v0.i0 - v1.q1) + 3 * (v0.q1 - v1.i0);
        const int16_t t1q = (v1.i1 + v0.q0) - 3 * (v1.q0 + v0.i1);

        buf += outfunc(buf, t6i + 3* t7i + 3* t0i + t1i, t6q + 3* t7q + 3* t0q + t1q);

        const sgpio_val_t v2 = (sgpio_val_t) SGPIO_REG_SS(10);

        const int16_t t2i = 3 * (v2.i0 - v1.q1) + (v2.q1 - v1.i0);
        const int16_t t2q = 3 * (v2.q0 + v1.i1) - (v2.i1 + v1.q0);

        const sgpio_val_t v3 = (sgpio_val_t) SGPIO_REG_SS(2);

        const int16_t t3i = (v2.i0 - v3.q1) + 3 * (v2.q1 - v3.i0);
        const int16_t t3q = (v3.i1 + v2.q0) - 3 * (v3.q0 + v2.i1);

        buf += outfunc(buf, t0i + 3* t1i + 3* t2i + t3i, t0q + 3* t1q + 3* t2q + t3q);

        const sgpio_val_t v4 = (sgpio_val_t) SGPIO_REG_SS(9);

        const int16_t t4i = 3 * (v4.i0 - v3.q1) + (v4.q1 - v3.i0);
        const int16_t t4q = 3 * (v4.q0 + v3.i1) - (v4.i1 + v3.q0);

        const sgpio_val_t v5 = (sgpio_val_t) SGPIO_REG_SS(4);

        const int16_t t5i = (v4.i0 - v5.q1) + 3 * (v4.q1 - v5.i0);
        const int16_t t5q = (v5.i1 + v4.q0) - 3 * (v5.q0 + v4.i1);

        buf += outfunc(buf, t2i + 3* t3i + 3* t4i + t5i, t2q + 3* t3q + 3* t4q + t5q);

        const sgpio_val_t v6 = (sgpio_val_t) SGPIO_REG_SS(8);

        t6i = 3 * (v6.i0 - v5.q1) + (v6.q1 - v5.i0);
        t6q = 3 * (v6.q0 + v5.i1) - (v6.i1 + v5.q0);

        v7 = (sgpio_val_t) SGPIO_REG_SS(0);

        t7i = (v6.i0 - v7.q1) + 3 * (v6.q1 - v7.i0);
        t7q = (v7.i1 + v6.q0) - 3 * (v7.q0 + v6.i1);

        buf += outfunc(buf, t4i + 3* t5i + 3* t6i + t7i, t4q + 3* t5q + 3* t6q + t7q);

        if(((uintptr_t)buf % (RX_BANK_SIZE)) == 0) {
            /* filled a bank of the ring buffer */
            *rx_bank_ready = oldbuf;
            if(((uintptr_t)buf % RX_BUF_SIZE) == 0) {
                /* it was the last bank, start at head of the ring buffer */
                buf = (int16_t*) RX_BUF_BASE;
            }
            /* store pointer to start of new bank */
            oldbuf = buf;
            /* notify M4 */
            send_interrupt();
        }
    }
}

/*************************************************************************
 * TRANSMIT
 */

#define FSK_FREQ 8
/* Function to encode one bit that is to be sent.
 * It should return -FSK_FREQ or +FSK_FREQ,
 * and it might return 0 to indicate that no bit
 * is to be encoded.
 */
#define STATE_NULL 0
/* when sending the preamble, in a first step we change
 * frequencies as fast as we can to set the right offset
 * on the receiving side.
 */
#define STATE_PREAMBLE_FLATTER 1
/* then we send 12x 1, 4x 0 (0xFFF0), which is what triggers
 * a new packet start with the receiver.
 */
#define STATE_PREAMBLE_ONE 20
#define STATE_PREAMBLE_ZERO 32
/* then the length of the packet is sent (1 octet) */
#define STATE_PKGLEN 36
/* then all the packet payload follows */
#define STATE_DATA 100
/* and that's it. */
#define STATE_FINISH 200

/* return value that flags end of data */
#define TRANSMIT_STOP 255

static volatile uint8_t *tx_data_ptr;

static int get_bit_freq() {
    static uint16_t data;
    static uint8_t state = STATE_NULL;

    if(state < STATE_PREAMBLE_FLATTER) {
        /* in STATE_NULL */
        state++;
        return 0;
    } else if(state < STATE_PREAMBLE_ONE) {
        /* in STATE_PREAMBLE_FLATTER */
        state++;
        return (state & 1) ? FSK_FREQ : -FSK_FREQ;
    } else if(state < STATE_PREAMBLE_ZERO) {
        /* in STATE_PREAMBLE_ONE */
        state++;
        return FSK_FREQ;
    } else if(state < STATE_PKGLEN) {
        /* in STATE_PREAMBLE_ZERO */
        state++;
        return -FSK_FREQ;
    } else if(state == STATE_PKGLEN) {
        data = tx_len[0];
        state = STATE_DATA + 9;
    } else if(state == STATE_DATA) {
        if((tx_len[0]--) == 0) {
            state = STATE_FINISH+2;
            return 0;
        }
        data = *(tx_data_ptr++);
        state = STATE_DATA + 9;
    } else if(state > STATE_FINISH) {
        state--;
        return 0;
    } else if(state == STATE_FINISH) {
        state = STATE_NULL;
        return TRANSMIT_STOP;
    }
    state--;
    data <<= 1;
    return (data & 0x200) ? FSK_FREQ : -FSK_FREQ;
}

static const uint8_t sgpio_planes[] = { 11, 5, 10, 2, 9, 4, 8, 0 };
void transmit_bfsk() {
    tx_data_ptr = tx_data;

    uint16_t phase = 0;

    /* the current frequency offset is stored here
     * so we can ramp from one offset to the other one
     */
    int16_t freq = 0;

    /* either +8 or -8 (rather than 0/1, what you would
     * probably expect)
     *
     * we start in the middle...
     *
     * results in (fs/(1024/(8*2))) freq shift
     */
    int16_t target = 0;

    /* every 2nd set of 16 samples, we send a new symbol
     * so our baud rate is
     * fs / (2*16)
     */
    uint16_t round = 0;
    while(1) {
        /* wait for flag that data is to be put into registers */
        while(!(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A))) {
            /* check for exit condition here so we don't lock up when
             * the SGPIO stops asking for data
             */
            if(mode != MODE_TRANSMIT_BFSK) return;
        };
        SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

        if(round == 0) {
            target = get_bit_freq();
            if(target == TRANSMIT_STOP) break;
        }
        round = round ^ 1;
            
        for(int n = 0; n<8; n++) {
            /* offset: 16/1024 of sample rate */
            phase += 16;
            /* ramp for the frequency shift: */
            if(freq < target) freq++;
            if(freq > target) freq--;
            /* frequency shift: */
            phase += freq;

            uint32_t v = *(uint16_t*)&cos_sin[phase % 1024];

            /* offset: 16/1024 of sample rate */
            phase += 16;
            /* ramp for the frequency shift: */
            if(freq < target) freq++;
            if(freq > target) freq--;
            /* frequency shift: */
            phase += freq;

            v |= (*(uint16_t*)&cos_sin[phase % 1024]) << 16;
            SGPIO_REG_SS(sgpio_planes[n]) = v;
        }
    }
    mode = MODE_OFF;
    send_interrupt();
}

int main(void) {
    mode = MODE_OFF;

    vector_table.irq[NVIC_M4CORE_IRQ] = m4core_ipc_isr;
    nvic_set_priority(NVIC_M4CORE_IRQ, 0);
    nvic_enable_irq(NVIC_M4CORE_IRQ);
    while(1) {
        /* set *m0_mode after evaluating mode, so an interrupt is less
         * less likely to happen in between
         */
        if(mode & (MODE_RECEIVE|MODE_RECEIVE_ATAN2)) {
            *m0_mode = mode;
            receive();
        } else if(mode == MODE_TRANSMIT_BFSK) {
            *m0_mode = mode;
            transmit_bfsk();
        } else if(mode == MODE_OFF) {
            *m0_mode = mode;
            /* wait for notification from M4 */
            __asm__("wfe");
        }
    }
}
