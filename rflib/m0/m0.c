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
#include <libopencm3/lpc43xx/m0/nvic.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/i2c.h>

#include "hackrf_core.h"
#include "rf_path.h"
#include "sgpio.h"
#include "tuning.h"
#include "max2837.h"
#include "streaming.h"
#include "si5351c.h"

#include "m0rxtx.h"
#include "cossin1024.h"

/* PIN definitions */
#define _PIN(pin, func, ...) pin
#define _FUNC(pin, func, ...) func
#define _GPORT(pin, func, gport, gpin, ...) gport
#define _GPIN(pin, func, gport, gpin, ...) gpin
#define _GPIO(pin, func, gport, gpin, ...) gport,gpin
#define _VAL(pin, func, gport, gpin, val, ...) val

#define PASTER(x) gpio_ ## x
#define WRAP(x) PASTER(x)

#define SETUPadc(args...)  scu_pinmux(_PIN(args),SCU_CONF_EPUN_DIS_PULLUP|_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args); SCU_ENAIO0|=SCU_ENAIO_ADCx_6;
#define SETUPgin(args...)  scu_pinmux(_PIN(args),_FUNC(args)); GPIO_DIR(_GPORT(args)) &= ~ _GPIN(args);
#define SETUPgout(args...) scu_pinmux(_PIN(args),SCU_CONF_EPUN_DIS_PULLUP|SCU_CONF_EZI_EN_IN_BUFFER|_FUNC(args)); GPIO_DIR(_GPORT(args)) |= _GPIN(args); WRAP( _VAL(args) ) (_GPIO(args));
#define SETUPpin(args...)  scu_pinmux(_PIN(args),_FUNC(args))

#define TOGGLE(x) gpio_toggle(_GPIO(x))
#define OFF(x...) gpio_clear(_GPIO(x))
#define ON(x...)  gpio_set(_GPIO(x))
#define GET(x...) gpio_get(_GPIO(x))

#define EN_VDD      P5_0,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN9,  clear        // RF Power
#define EN_1V8      P6_10, SCU_CONF_FUNCTION0, GPIO3, GPIOPIN6,  clear        // CPLD Power
#define MIC_AMP_DIS P9_1,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN13, set          // MIC Power

#define LED1        P4_1,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN1,  clear
#define LED2        P4_2,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN2,  clear
#define LED3        P6_12, SCU_CONF_FUNCTION0, GPIO2, GPIOPIN8,  clear
#define LED4        PB_6,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN26, clear
#define RGB_LED     P8_0,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN0,  clear


/* M0 core operation mode */
static volatile int mode;

/* frequency
 *
 * the really tuned frequency is derived from this, based on operation mode
 */
static int64_t frequency;

static int16_t rxlna_enable = 1;
static int16_t txlna_enable = 1;

static int16_t lna_gain_db = 8;
static int16_t vga_gain_db = 20;
static int16_t txvga_gain_db = 30;

static uint32_t rxsamplerate = 2000000;
static uint32_t txsamplerate = 2000000;
static uint16_t rxdecimation = 1;
static uint32_t rxbandwidth = 1750000;
static uint32_t txbandwidth = 1750000;

static void rflib_set_frequency(const int64_t new_frequency, const int32_t offset) {
    const int64_t tuned_frequency = new_frequency + offset;
    ssp1_set_mode_max2837();
    if(set_freq(tuned_frequency)) {
        frequency = new_frequency;
    }
}

/* set amps */
static void set_rf_params() {
    ssp1_set_mode_max2837(); // need to reset this since display driver will hassle with SSP1
    max2837_set_lna_gain(lna_gain_db);     /* 8dB increments */
    max2837_set_vga_gain(vga_gain_db);     /* 2dB increments, up to 62dB */
    max2837_set_txvga_gain(txvga_gain_db); /* 1dB increments, up to 47dB */
}

/* rest of hackrf clock startup
Configure PLL1 to max speed (204MHz).
Note: PLL1 clock is used by M4/M0 core, Peripheral, APB1. */ 
static void hackrf_clock_init(void)
{
	/* Switch peripheral clock over to use PLL1 (204MHz) */
	CGU_BASE_PERIPH_CLK = CGU_BASE_PERIPH_CLK_AUTOBLOCK(1)
			| CGU_BASE_PERIPH_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB1 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB1_CLK = CGU_BASE_APB1_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB1_CLK_CLK_SEL(CGU_SRC_PLL1);

	/* Switch APB3 clock over to use PLL1 (204MHz) */
	CGU_BASE_APB3_CLK = CGU_BASE_APB3_CLK_AUTOBLOCK(1)
			| CGU_BASE_APB3_CLK_CLK_SEL(CGU_SRC_PLL1);
}

static void si5351_init(void){
	i2c0_init(255); 

	si5351c_disable_all_outputs();
	si5351c_disable_oeb_pin_control();
	si5351c_power_down_all_clocks();
	si5351c_set_crystal_configuration();
	si5351c_enable_xo_and_ms_fanout();
	si5351c_configure_pll_sources();
	si5351c_configure_pll_multisynth();

	/* MS3/CLK3 is the source for the external clock output. */
	si5351c_configure_multisynth(3, 80*128-512, 0, 1, 0); /* 800/80 = 10MHz */

	/* MS5/CLK5 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(5, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS4/CLK4 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(4, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS6/CLK6 is unused. */
	/* MS7/CLK7 is the source for the LPC43xx microcontroller. */
	uint8_t ms7data[] = { 90, 255, 20, 0 };
	si5351c_write(ms7data, sizeof(ms7data));

	si5351c_set_clock_source(PLL_SOURCE_XTAL);
	// soft reset
	uint8_t resetdata[] = { 177, 0xac };
	si5351c_write(resetdata, sizeof(resetdata));
	si5351c_enable_clock_outputs();
};

/* portapack_init plus a bunch of stuff from here and there, cleaned up */
static void rf_init() {
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
    // doesn't work without:
    for(int i=250000; i>0; i--) {
        __asm__ volatile ("nop");
    }

    si5351_init();

    cpu_clock_pll1_max_speed();

    ssp1_init();

    rf_path_init();
}

static void rf_off() {
    OFF(EN_VDD);
    OFF(EN_1V8);
}

/* interrupt handler for interrupts triggered by the M4 core */
static void m4core_ipc_isr() {
    CREG_M4TXEVENT = 0; /* clear interrupt flag */
    nvic_clear_pending_irq(NVIC_M4CORE_IRQ);
    uint32_t syn = *m0_syn;
    /* handle command */
    switch(*m0_command) {
        case CMD_SET_MODE:
            if((mode == MODE_OFF) && (*m0_arg != MODE_OFF)) {
                rf_init();
            }
            mode = *m0_arg;
            break;
        case CMD_SET_FREQ:
            frequency = *m0_arg;
            frequency<<=32;
            frequency += *m0_arg2;
            break;
        case CMD_SET_BBAMP:
            lna_gain_db = *m0_arg;
            vga_gain_db = *m0_arg2;
            txvga_gain_db = *m0_arg3;
            set_rf_params();
            break;
        case CMD_SET_TXLNA:
            txlna_enable = *m0_arg;
            break;
        case CMD_SET_RXLNA:
            rxlna_enable = *m0_arg;
            break;
        case CMD_SET_RXSAMPLERATE:
            rxsamplerate = *m0_arg;
            break;
        case CMD_SET_RXDECIMATION:
            rxdecimation = *m0_arg;
            break;
        case CMD_SET_RXBANDWIDTH:
            rxbandwidth = *m0_arg;
            break;
        case CMD_SET_TXSAMPLERATE:
            txsamplerate = *m0_arg;
            break;
        case CMD_SET_TXBANDWIDTH:
            txbandwidth = *m0_arg;
            break;
    }
    *m0_command = 0;
    *m0_ack = syn;
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

#define BRAD_PI_SHIFT 15
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
// Returns [0,2pi), where pi ~ 0x8000.
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
    return phi + (dphi>>1);
}

#define MAX_PACKET_LEN 255
/* we use a peamble of 0xFFF0 - so mostly 1 bits - in order to keep the
 * phase constant as long as possible during the time the PLL is still about
 * to lock. The PLL should lock when a few other 1-bits (0xFFFF) are sent
 * before sending this peamble. So when sending, just send 0xFFFFFFF0.
 *
 * the row of 1 bits will put a running decoding out of sync if
 * encountered for whatever reason during data decode
 */
#define PREAMBLE 0xFFF0
static void decoder(int16_t** const buf, const int8_t bit) {
    /* shift register for received data */
    static uint16_t shift = 0;
    /* flag to determine whether we have seen a header and are in sync
     * also, counter for received data bits (+1)
     */
    static uint8_t sync = 0;
    /* number of bytes in packet to receive */
    static int16_t pkglen;
    static int16_t c = 0;
    static uint8_t *curbuf = 0;

    /* enforce desync? */
    if(bit == -1) goto desync;
    shift <<= 1;
    shift |= bit;
    if(sync == 0) {
        /* wait for sync */
        if(shift == PREAMBLE) {
            pkglen = -1;
            sync = 1;
            if(curbuf == (uint8_t*)buf) {
                curbuf = (uint8_t*)(buf+RX_BANK_SIZE);
            } else {
                curbuf = (uint8_t*)buf;
            }
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

            curbuf[0] = pkglen;

            pkglen++; /* add the length byte itself */
            c = 1;
            return;
        }
        if(c < pkglen) {
            curbuf[c] = shift & 0xFF;
            c++;
            if(c == pkglen) {
                /* got full packet, notify M4
                 * we do this here since it might not correspond with the
                 * sample interval of the main receive() routine.
                 */
                *rx_bank_ready = (int16_t*) curbuf;
                *m0_ack = ACK_NOTIFY_RX;
                send_interrupt();
                /* we also desync when we're done. */
                sync = 0;
            }
        }
    } else {
        sync++;
    }
    return;

desync:
    sync = 0;
    return;
}

/* try to lock the offset into a window of this size:
 * (a bit more than the mathematically exact window size to
 * avoid being too jittery)
 *
 * The window will never be moved outside of the frequency
 * value range, which is -32768..32767.
 *
 * This allows for about 62.5kHz frequency error between
 * sender and receiver and it will still lock.
 */
#define WINDOW_SIZE 18000

/* even further process an RX sample: decode BFSK */
static void rxbfsk(int16_t** const buf, int16_t i, int16_t q) {
    static int32_t old_f = 0;
    static int32_t offset = 0;
    static uint16_t old_w = 0;
    static uint16_t c = 0;
    const uint16_t w = atan2Cordic(i, q);
    const int32_t f = (int16_t)(w - old_w);
    old_w = w;
    if((f > (offset+WINDOW_SIZE/2)) || (f < (offset-WINDOW_SIZE/2))) {
        offset += (f - (offset+WINDOW_SIZE/2)) >> 2;
    }
    if(c++ % 2) {
        /* we just add two samples and look whether they are above
         * or below the offset (x2, since we compare with the sum of
         * two samples).
         */
        if(old_f+f > 2*offset) {
            return decoder(buf, 1);
        } else {
            return decoder(buf, 0);
        }
    }
    old_f = f;
}

/* write frequency and signal strength (^2) to ring buffer */
static void rxfreq(int16_t** const buf, int16_t i, int16_t q) {
    static uint16_t old_w = 0;
    const int32_t sig = i * i + q * q;
    const uint16_t w = atan2Cordic(i, q);
    const int16_t f = (int16_t)(w - old_w);
    old_w = w;
    **buf = f;
    (*buf)++;
    **buf = sig>>12; // TODO: calculate a good value for the shift
    (*buf)++;
}

/* write an RX sample to ring buffer */
static void rxtobuf(int16_t** const buf, int16_t i, int16_t q) {
    **buf = i;
    (*buf)++;
    **buf = q;
    (*buf)++;
}

/* further process an RX sample: atan2(i, q) */
static void rxatan2tobuf(int16_t** const buf, int16_t i, int16_t q) {
    **buf = atan2Cordic(i, q);
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
static void receive() {
    sgpio_cpld_stream_disable();

    rf_path_set_direction(RF_PATH_DIRECTION_RX);
    rf_path_set_lna(rxlna_enable);

    rflib_set_frequency(frequency, -(rxsamplerate>>2));
    sample_rate_frac_set(rxsamplerate * rxdecimation * 2, 1);
    baseband_filter_bandwidth_set(rxbandwidth);
    sgpio_cpld_stream_rx_set_decimation(rxdecimation);
    set_rf_params();
    /* send interrupt now in order to allow the M4 code that has
     * put us into receive mode to wait until the RF setup is done
     * - which is needed to manage access to the SPI bus, so the
     * cores don't do conflicting access.
     */
    *m0_ack = ACK_RF_SETUP;
    send_interrupt();

    /* ring buffer pointer */
    int16_t* buf = (int16_t*) RX_BUF_BASE;
    /* reference to start of current bank */
    int16_t* oldbuf = buf;

    /* sample processing function, defaults to write out */
    void (*outfunc)(int16_t **const, int16_t, int16_t) = rxtobuf;
    /* other mode(s): */
    if(mode == MODE_RECEIVE_ATAN2) {
        outfunc = rxatan2tobuf;
    } else if(mode == MODE_RECEIVE_FREQ) {
        outfunc = rxfreq;
    } else if(mode == MODE_RECEIVE_BFSK) {
        /* BFSK mode will manage the buffer on its own and won't touch
         * the "buf" pointer
         */
        outfunc = rxbfsk;
    }

    sgpio_cpld_stream_enable();
    while(1) {
        sgpio_val_t v7;
        int16_t t6i, t6q, t7i, t7q;

        /* wait for flag that data is ready */
        while(!(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A))) {
            /* check for exit condition here so we don't lock up when
             * the SGPIO stops getting data
             */
            if(!(mode & MODE_RECEIVE)) {
                return;
            }
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

        outfunc(&buf, t6i + 3* t7i + 3* t0i + t1i, t6q + 3* t7q + 3* t0q + t1q);

        const sgpio_val_t v2 = (sgpio_val_t) SGPIO_REG_SS(10);

        const int16_t t2i = 3 * (v2.i0 - v1.q1) + (v2.q1 - v1.i0);
        const int16_t t2q = 3 * (v2.q0 + v1.i1) - (v2.i1 + v1.q0);

        const sgpio_val_t v3 = (sgpio_val_t) SGPIO_REG_SS(2);

        const int16_t t3i = (v2.i0 - v3.q1) + 3 * (v2.q1 - v3.i0);
        const int16_t t3q = (v3.i1 + v2.q0) - 3 * (v3.q0 + v2.i1);

        outfunc(&buf, t0i + 3* t1i + 3* t2i + t3i, t0q + 3* t1q + 3* t2q + t3q);

        const sgpio_val_t v4 = (sgpio_val_t) SGPIO_REG_SS(9);

        const int16_t t4i = 3 * (v4.i0 - v3.q1) + (v4.q1 - v3.i0);
        const int16_t t4q = 3 * (v4.q0 + v3.i1) - (v4.i1 + v3.q0);

        const sgpio_val_t v5 = (sgpio_val_t) SGPIO_REG_SS(4);

        const int16_t t5i = (v4.i0 - v5.q1) + 3 * (v4.q1 - v5.i0);
        const int16_t t5q = (v5.i1 + v4.q0) - 3 * (v5.q0 + v4.i1);

        outfunc(&buf, t2i + 3* t3i + 3* t4i + t5i, t2q + 3* t3q + 3* t4q + t5q);

        const sgpio_val_t v6 = (sgpio_val_t) SGPIO_REG_SS(8);

        t6i = 3 * (v6.i0 - v5.q1) + (v6.q1 - v5.i0);
        t6q = 3 * (v6.q0 + v5.i1) - (v6.i1 + v5.q0);

        v7 = (sgpio_val_t) SGPIO_REG_SS(0);

        t7i = (v6.i0 - v7.q1) + 3 * (v6.q1 - v7.i0);
        t7q = (v7.i1 + v6.q0) - 3 * (v7.q0 + v6.i1);

        outfunc(&buf, t4i + 3* t5i + 3* t6i + t7i, t4q + 3* t5q + 3* t6q + t7q);

        if(buf == (oldbuf + RX_BANK_SIZE)) {
            /* filled a bank of the ring buffer */
            *rx_bank_ready = oldbuf;
            if(buf == ((int16_t*) RX_BUF_BASE + RX_BUF_SIZE)) {
                /* it was the last bank, start at head of the ring buffer */
                buf = (int16_t*) RX_BUF_BASE;
            }
            /* store pointer to start of new bank */
            oldbuf = buf;
            /* notify M4 */
            *m0_ack = ACK_NOTIFY_RX;
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
static void transmit_bfsk() {
    sgpio_cpld_stream_disable();

    rf_path_set_direction(RF_PATH_DIRECTION_TX);
    rf_path_set_lna(txlna_enable);

    rflib_set_frequency(frequency, -(txsamplerate>>6));
    sample_rate_frac_set(txsamplerate * 2, 1);
    baseband_filter_bandwidth_set(txbandwidth);
    set_rf_params();
    *m0_ack = ACK_RF_SETUP;
    send_interrupt();

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

    sgpio_cpld_stream_enable();

    while(1) {
        /* wait for flag that data is to be put into registers */
        while(!(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A))) {
            /* check for exit condition here so we don't lock up when
             * the SGPIO stops asking for data
             */
            if(mode != MODE_TRANSMIT_BFSK) {
                *m0_ack = ACK_TX_ABORTED;
                goto stop;
            }
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
    mode = MODE_STANDBY;
    *m0_ack = ACK_TX_DONE;
stop:
    send_interrupt();
}

int main(void) {
    mode = MODE_OFF;

    vector_table.irq[NVIC_M4CORE_IRQ] = m4core_ipc_isr;
    nvic_set_priority(NVIC_M4CORE_IRQ, 0);
    nvic_enable_irq(NVIC_M4CORE_IRQ);
    while(1) {
        if(mode & MODE_RECEIVE) {
            receive();
        } else if(mode == MODE_TRANSMIT_BFSK) {
            transmit_bfsk();
        } else if(mode == MODE_OFF || mode == MODE_STANDBY) {
            sgpio_cpld_stream_disable();
            rf_path_set_direction(RF_PATH_DIRECTION_OFF);
            if(mode == MODE_OFF) {
                rf_off();
                *m0_ack = ACK_SWITCHED_OFF;
                send_interrupt();
            }
            __asm__("dsb \n wfe");
        }
    }
}
