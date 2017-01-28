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
#include <stddef.h>
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

#define TOGGLE(x) cm3_gpio_toggle(_GPIO(x))
#define OFF(x...) cm3_gpio_clear(_GPIO(x))
#define ON(x...)  cm3_gpio_set(_GPIO(x))
#define GET(x...) cm3_gpio_get(_GPIO(x))

#define EN_VDD      P5_0,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN9,  clear        // RF Power
#define EN_1V8      P6_10, SCU_CONF_FUNCTION0, GPIO3, GPIOPIN6,  clear        // CPLD Power
#define MIC_AMP_DIS P9_1,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN13, set          // MIC Power

#define RAD1O_LED1        P4_1,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN1,  clear
#define RAD1O_LED2        P4_2,  SCU_CONF_FUNCTION0, GPIO2, GPIOPIN2,  clear
#define RAD1O_LED3        P6_12, SCU_CONF_FUNCTION0, GPIO2, GPIOPIN8,  clear
#define RAD1O_LED4        PB_6,  SCU_CONF_FUNCTION4, GPIO5, GPIOPIN26, clear
#define RAD1O_RGB_LED     P8_0,  SCU_CONF_FUNCTION0, GPIO4, GPIOPIN0,  clear


/* M0 core operation mode */
static volatile int mode;

/* frequency
 *
 * the really tuned frequency is derived from this, based on operation mode
 */
static int64_t frequency;

static int16_t rxlna_enable = 1;
static int16_t txlna_enable = 1;

static int16_t lna_gain_db = 16;
static int16_t vga_gain_db = 20;
static int16_t txvga_gain_db = 47;

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
    max2837_set_lna_gain(&max2837, lna_gain_db);     /* 8dB increments */
    max2837_set_vga_gain(&max2837, vga_gain_db);     /* 2dB increments, up to 62dB */
    max2837_set_txvga_gain(&max2837, txvga_gain_db); /* 1dB increments, up to 47dB */
}

/* send an interrupt to the other core(s) */
static void send_interrupt(uint32_t ack) {
    *m0_ack = ack;
    __asm volatile("dsb \n sev" : : : "memory");
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

	si5351c_disable_all_outputs(&clock_gen);
	si5351c_disable_oeb_pin_control(&clock_gen);
	si5351c_power_down_all_clocks(&clock_gen);
	si5351c_set_crystal_configuration(&clock_gen);
	si5351c_enable_xo_and_ms_fanout(&clock_gen);
	si5351c_configure_pll_sources(&clock_gen);
	si5351c_configure_pll_multisynth(&clock_gen);

	/* MS3/CLK3 is the source for the external clock output. */
	si5351c_configure_multisynth(&clock_gen, 3, 80*128-512, 0, 1, 0); /* 800/80 = 10MHz */

	/* MS5/CLK5 is the source for the RFFC5071 mixer. */
	si5351c_configure_multisynth(&clock_gen, 5, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS4/CLK4 is the source for the MAX2837 clock input. */
	si5351c_configure_multisynth(&clock_gen, 4, 20*128-512, 0, 1, 0); /* 800/20 = 40MHz */

	/* MS6/CLK6 is unused. */
	/* MS7/CLK7 is the source for the LPC43xx microcontroller. */
	uint8_t ms7data[] = { 90, 255, 20, 0 };
	si5351c_write(&clock_gen, ms7data, sizeof(ms7data));

	si5351c_set_clock_source(&clock_gen, PLL_SOURCE_XTAL);
	// soft reset
	uint8_t resetdata[] = { 177, 0xac };
	si5351c_write(&clock_gen, resetdata, sizeof(resetdata));
	si5351c_enable_clock_outputs(&clock_gen);
};

/* portapack_init plus a bunch of stuff from here and there, cleaned up */
static void rf_init() {
    /* Release CPLD JTAG pins */
    scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
    scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);

    gpio_input(jtag_cpld.gpio->gpio_tdo);
    gpio_input(jtag_cpld.gpio->gpio_tck);
    gpio_input(jtag_cpld.gpio->gpio_tms);
    gpio_input(jtag_cpld.gpio->gpio_tdi);

    hackrf_clock_init();
    rf_path_pin_setup(&rf_path);

    /* Configure external clock in */
    scu_pinmux(SCU_PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

    /* Disable unused clock outputs. They generate noise. */
    scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
    scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

    sgpio_configure_pin_functions(&sgpio_config);

    ON(EN_VDD);
    ON(EN_1V8);
    // doesn't work without:
    for(int i=250000; i>0; i--) {
        __asm__ volatile ("nop");
    }

    si5351_init();

    cpu_clock_pll1_max_speed();

    //ssp1_init();

    rf_path_init(&rf_path);
}

static void rf_off() {
    OFF(EN_VDD);
    OFF(EN_1V8);
}

static void switch_to_mode(uint32_t new_mode) {
    switch(new_mode) {
        case MODE_STANDBY:
            sgpio_cpld_stream_disable(&sgpio_config);
            rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_OFF);
            break;
        case MODE_OFF:
            sgpio_cpld_stream_disable(&sgpio_config);
            rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_OFF);
            rf_off();
            break;
        default:
            if(mode == MODE_OFF) rf_init();
    }
    mode = new_mode;
}

/* interrupt handler for interrupts triggered by the M4 core */
void m4core_ipc_isr() {
    CREG_M4TXEVENT = 0; /* clear interrupt flag */
    nvic_clear_pending_irq(NVIC_M4CORE_IRQ);
    /* handle command */
    switch(*m0_command) {
        case CMD_SET_MODE:
            switch_to_mode(*m0_arg);
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
    send_interrupt(ACK_COMMAND_DONE);
}

/* CRC16 implementation */
#define CRC16_POLYNOMIAL 0x1021
/* There seems to be a major dispute over this one: */
#define CRC16_START 0x1D0F
static void crc16(const uint8_t input, uint16_t* const crc16buf) {
    for(int i=7; i>=0; i--) {
        if(((*crc16buf >> 15) & 1) != ((input >> i) & 1)) {
            *crc16buf = (*crc16buf << 1) ^ CRC16_POLYNOMIAL;
        } else {
            *crc16buf = (*crc16buf << 1);
        }
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

/* packet header, randomly chosen:
 * 0b1010000100101100
 */
#define BFSK_MAGIC 0xA12C

#define DECODER_STATE_STANDBY 0
#define DECODER_STATE_PKGLEN1 (DECODER_STATE_STANDBY + 8)
#define DECODER_STATE_PKGLEN2 (DECODER_STATE_PKGLEN1 + 8)
#define DECODER_STATE_CRC 0x1000
#define DECODER_STATE_STOP (DECODER_STATE_CRC + 16)
static void decoder(const int8_t bit) {
    /* state for state machine */
    static uint16_t state = DECODER_STATE_STANDBY;
    /* shift register for received data */
    static uint16_t shift;
    /* CRC for currently received data */
    static uint16_t crc;
    /* byte counter */
    static uint16_t c;
    /* Pointer to buffer for current packet
     * We manage buffers & banks on our own here.
     */
    static uint8_t *rxbuf = (uint8_t*) rx_data;

    /* put bit into shift register */
    shift <<= 1;
    shift |= bit;

#ifdef BFSK_CAN_ABORT
    /* abort condition */
    if(bit == -1) state = DECODER_STATE_STANDBY;
#endif

    if(state == DECODER_STATE_STANDBY) {
        if(shift != BFSK_MAGIC) {
            /* this is the most common case:
             * we have not seen the beginning of a packet.
             */
            return;
        }
    } else if(state == DECODER_STATE_PKGLEN1) {
        const uint8_t pkglen = shift & 0xFF;
        rxbuf[0] = pkglen;
    } else if(state == DECODER_STATE_PKGLEN2) {
        const uint8_t pkglen = shift & 0xFF;
        if(rxbuf[0] != pkglen) {
            state = DECODER_STATE_STANDBY;
            return;
        }
        state = DECODER_STATE_CRC - (pkglen*8);
        crc = CRC16_START;
        c = 1;
    } else if(state == DECODER_STATE_STOP) {
        /* we have the supposed CRC16 in the shift register. */
        if(shift == crc) {
            /* got full packet, notify M4
             * we do this here since it might not correspond with the
             * sample interval of the main receive() routine.
             */
            *rx_bank_ready = (int16_t*) rxbuf;
            send_interrupt(ACK_NOTIFY_RX);
            /* bank switching for RX data */
            rxbuf = (uint8_t*)((uintptr_t)rxbuf ^ RX_BANK_SIZE);
        }
        state = DECODER_STATE_STANDBY;
        /* don't increment state */
        return;
    } else if(state > DECODER_STATE_PKGLEN2 &&
              state <= DECODER_STATE_CRC &&
              state % 8 == 0) {
        /* got a full data byte */
        const uint8_t input = shift & 0xFF;
        rxbuf[c++] = input;
        crc16(input, &crc);
    }
    state++;
}

/* try to lock the offset into a window of this size:
 *
 * This allows for about 62.5kHz frequency error between
 * sender and receiver and it will still lock.
 */
#define WINDOW_SIZE 16384

/* even further process an RX sample: decode BFSK */
static void rxbfsk(__attribute__ ((unused)) int16_t** const buf, int16_t i, int16_t q) {
    static int16_t offset = 0;
    static uint16_t old_w = 0;
    static uint16_t c = 0;
    const uint16_t w = atan2Cordic(i, q);
    const int16_t f = (int16_t)(w - old_w) - offset;
    old_w = w;
    if(f > (WINDOW_SIZE/2)) {
        offset += (f - (WINDOW_SIZE/2)) >> 2;
    } else if(f < (-WINDOW_SIZE/2)) {
        offset += (f + (WINDOW_SIZE/2)) >> 2;
    }
    if(c++ % 2) {
        /* we should get one symbol per two samples.
         * however, which is the right sample to decide on?
         * in the case (b) below, every second sample is about 0:
         * that is the case when we're sampling in the exact
         * middle of the frequency shift.
         * Then, however, the sample in between would be the right one
         * to use for the decision.
         *
         *   |         ____            __
         *   |        /    \          /
         * 0_|......./......\......../...
         *   |      /        \      /
         *   |\____/          \____/
         *   |
         *     |   |   |   |   |   |   |
         *(a)  S1  S2  S1  S2  S1  S2  S1
         *     -   -   +   +   -   -   +
         *
         *       |   |   |   |   |   |
         *(b)    S1  S2  S1  S2  S1  S2
         *       -   0   +   0   -   0
         *
         * so how do we go best to sync on good samples?
         * for now, the approach is to skip a sample if we decide
         * that it is definitely a bad sample. We decide so if it
         * is too much near 0.
         */
        if(f < 2000 && f > -2000) {
            c++;
        } else {
            decoder((f > 0) ? 1 : 0);
        }
    }
}

/* write frequency and signal strength (^2) to ring buffer */
static void rxfreq(int16_t** const buf, int16_t i, int16_t q) {
    int8_t sig;
    const int32_t sig_quad = i*i + q*q;
    /* a rough estimate of signal strength is therefore the log2, which we can
     * approximate like this (could also do interval halving, but this is more
     * compact):
     */
    for(sig=0; sig<28; sig++) {
        if(sig_quad < (1L<<sig)) break;
    }
    static uint16_t old_w = 0;
    const uint16_t w = atan2Cordic(i, q);
    const int16_t f = (int16_t)(w - old_w);
    old_w = w;
    **buf = f;
    (*buf)++;
    **buf = sig;
    (*buf)++;
}

/* write an RX sample to ring buffer */
static void rxtobuf(int16_t** const buf, int16_t i, int16_t q) {
    **buf = i * 4;
    (*buf)++;
    **buf = q * 4;
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
    sgpio_cpld_stream_disable(&sgpio_config);

    rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_RX);
    rf_path_set_lna(&rf_path, rxlna_enable);

    rflib_set_frequency(frequency, -(rxsamplerate>>2));
    sample_rate_frac_set(rxsamplerate * rxdecimation * 2, 1);
    baseband_filter_bandwidth_set(rxbandwidth);
    sgpio_cpld_stream_rx_set_decimation(&sgpio_config, rxdecimation);
    set_rf_params();
    /* send interrupt now in order to allow the M4 code that has
     * put us into receive mode to wait until the RF setup is done
     * - which is needed to manage access to the SPI bus, so the
     * cores don't do conflicting access.
     */
    send_interrupt(ACK_RF_SETUP);

    /* ring buffer pointer */
    int16_t* buf = (int16_t*)rx_data;
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

    sgpio_cpld_stream_enable(&sgpio_config);
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

        if(buf == (int16_t*)((uintptr_t)oldbuf + RX_BANK_SIZE)) {
            /* filled a bank of the ring buffer */
            *rx_bank_ready = oldbuf;
            /* switch banks */
            buf = (int16_t*)((uintptr_t)oldbuf ^ RX_BANK_SIZE);
            /* store pointer to start of new bank */
            oldbuf = buf;
            /* notify M4 */
            send_interrupt(ACK_NOTIFY_RX);
        }
    }
}

/*************************************************************************
 * TRANSMIT
 */

#define FSK_FREQ 8
/* Function to encode one bit that is to be sent.
 * It should return -FSK_FREQ or +FSK_FREQ
 */
/* when sending the preamble, in a first step we change
 * frequencies to set the right offset. Keep one frequency for a longer
 * time in order to have it "settle".
 * on the receiving side.
 * We do this for 16 bits.
 */
#define ENCODER_STATE_NULL 0
/* then we send the value that indicates a package start */
#define ENCODER_STATE_PREAMBLE_MAGIC 16
/* then the length of the packet is sent (1 octet), twice */
#define ENCODER_STATE_PKGLEN1 32
#define ENCODER_STATE_PKGLEN2 40
/* then all the packet payload follows */
#define ENCODER_STATE_DATA 100
/* then a CRC16 checksum, no own state */
/* and that's it. */
#define ENCODER_STATE_FINISH 200

/* return value that flags end of data */
#define TRANSMIT_STOP 255

static volatile uint8_t *tx_data_ptr;

static int16_t get_bit_freq() {
    static uint16_t data;
    static uint8_t state = ENCODER_STATE_NULL;
    static uint16_t crc;

    switch(state) {
        case ENCODER_STATE_NULL:
            data = 0xF0F0;
            break;
        case ENCODER_STATE_PREAMBLE_MAGIC:
            data = BFSK_MAGIC;
            break;
        case ENCODER_STATE_PKGLEN1:
            data = tx_len[0] << 8;
            break;
        case ENCODER_STATE_PKGLEN2:
            data = tx_len[0] << 8;
            state = ENCODER_STATE_DATA - 8;
            crc = CRC16_START;
            break;
        case ENCODER_STATE_DATA:
            if((tx_len[0]--) > 0) {
                data = *(tx_data_ptr++);
                crc16(data, &crc);
                data <<= 8;
                state = ENCODER_STATE_DATA - 8;
            } else {
                data = crc;
                state = ENCODER_STATE_FINISH - 18;
            }
            break;
        case ENCODER_STATE_FINISH:
            state = ENCODER_STATE_NULL;
            return TRANSMIT_STOP;
    }
    int16_t freq = (data & 0x8000) ? FSK_FREQ : -FSK_FREQ;
    state++;
    data <<= 1;
    return freq;
}

static const uint8_t sgpio_planes[] = { 11, 5, 10, 2, 9, 4, 8, 0 };
static void transmit_bfsk() {
    sgpio_cpld_stream_disable(&sgpio_config);

    rf_path_set_direction(&rf_path, RF_PATH_DIRECTION_TX);
    rf_path_set_lna(&rf_path, txlna_enable);

    rflib_set_frequency(frequency, -(txsamplerate>>6));
    sample_rate_frac_set(txsamplerate * 2, 1);
    baseband_filter_bandwidth_set(txbandwidth);
    set_rf_params();
    send_interrupt(ACK_RF_SETUP);

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

    sgpio_cpld_stream_enable(&sgpio_config);

    while(1) {
        /* wait for flag that data is to be put into registers */
        while(!(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A))) {
            /* check for exit condition here so we don't lock up when
             * the SGPIO stops asking for data
             */
            if(mode != MODE_TRANSMIT_BFSK) {
                return send_interrupt(ACK_TX_ABORTED);
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
    switch_to_mode(MODE_STANDBY);
    send_interrupt(ACK_TX_DONE);
}

int main(void) {
    mode = MODE_OFF;

    vector_table.irq[NVIC_M4CORE_IRQ] = m4core_ipc_isr;
    nvic_set_priority(NVIC_M4CORE_IRQ, 0);
    nvic_enable_irq(NVIC_M4CORE_IRQ);

    send_interrupt(ACK_COMMAND_DONE); /* we use this to signal that we're up and running */

    while(1) {
        if(mode & MODE_RECEIVE) {
            receive();
        } else if(mode == MODE_TRANSMIT_BFSK) {
            transmit_bfsk();
        } else if(mode == MODE_OFF || mode == MODE_STANDBY) {
            // TODO: race condition here: interrupt might happen in just this moment.
            __asm__("dsb \n wfe");
        }
    }
}
