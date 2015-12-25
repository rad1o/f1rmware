/* L0ungeL1cht
 *
 * Copyright (C) 2015 Hans-Werner Hilse <hwhilse@gmail.com>
 *
 * some parts (receive/filters) are
 *   Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
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

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <r0ketlib/config.h>

#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/pins.h>

#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/tuning.h>
#include <common/max2837.h>
#include <common/streaming.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/uart.h>
#include <libopencm3/cm3/vector.h>

#include <stddef.h>
#include <portalib/arm_intrinsics.h>
#include <portalib/complex.h>
#include <portalib/fxpt_atan2.h>

#include <lpcapi/cdc/cdc_main.h>
#include <lpcapi/cdc/cdc_vcom.h>

#include "cossin1024.h"
#include "l0ungel1cht.h"

#include <string.h>
// default to 2496 MHz
//#define FREQSTART 2496000000
#define FREQSTART 2395000000
//#define FREQSTART 2100000000

static int64_t frequency = FREQSTART;

static void my_set_frequency(const int64_t new_frequency, const int32_t offset) {
    const int64_t tuned_frequency = new_frequency + offset;
    ssp1_set_mode_max2837();
    if(set_freq(tuned_frequency)) {
        frequency = new_frequency;
    }
}

static bool lna_enable = true;
static bool txlna_enable = false;
static int32_t lna_gain_db = 10;
static int32_t vga_gain_db = 10;
static int32_t txvga_gain_db = 10;

/* set amps */
static void set_rf_params(bool rx) {
    ssp1_set_mode_max2837(); // need to reset this since display driver will hassle with SSP1
    if(rx) {
        rf_path_set_lna(lna_enable ? 1 : 0);
    } else {
        rf_path_set_lna(txlna_enable ? 1 : 0);
    }
    max2837_set_lna_gain(lna_gain_db);     /* 8dB increments */
    max2837_set_vga_gain(vga_gain_db);     /* 2dB increments, up to 62dB */
    max2837_set_txvga_gain(txvga_gain_db); /* 1dB increments, up to 47dB */
}

/* portapack_init plus a bunch of stuff from here and there, cleaned up */
static void rfinit() {
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
    delayNop(250000); // doesn't work without

    cpu_clock_set(204); // WARP SPEED! :-)
    si5351_init();

    cpu_clock_pll1_max_speed();

    ssp1_init();

    rf_path_init();
}

/* ---------------------------- TRANSMIT: ------------------------------- */

/* transmit options */
//#define TX_BANDWIDTH  3000000
#define TX_BANDWIDTH  1750000
#define TX_SAMPLERATE 4000000
#define TX_FREQOFFSET (-62500)

static volatile uint8_t *tx_pkg;
static volatile uint16_t tx_pkg_len;
static volatile bool transmitting = false;

static void stop_transmit() {
    baseband_streaming_disable();
    /* switch everything off */
    rf_path_set_direction(RF_PATH_DIRECTION_OFF);
    /* clear flag */
    transmitting = false;
}

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
        data = tx_pkg_len;
        state = STATE_DATA + 9;
    } else if(state == STATE_DATA) {
        if((tx_pkg_len--) == 0) {
            state = STATE_FINISH+2;
            return 0;
        }
        data = *(tx_pkg++);
        state = STATE_DATA + 9;
    } else if(state > STATE_FINISH) {
        state--;
        return 0;
    } else if(state == STATE_FINISH) {
        state = STATE_NULL;
        stop_transmit();
        return 0;
    }
    state--;
    data <<= 1;
    return (data & 0x200) ? FSK_FREQ : -FSK_FREQ;
}

/*
 * the following follows the example of hackrf's sgpio_isr
 * which triggers write to SGPIO on 8 planes
 * will be triggered with FREQ/16
 **/
void bfsk_sgpio_isr_tx() {
    static complex_s8_t samplebuf[16];
	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

    /* send buffered samples */
	__asm__(
		"ldr r0, [%[p], #0]\n\t"
		"str r0, [%[SGPIO_REG_SS], #44]\n\t"
		"ldr r0, [%[p], #4]\n\t"
		"str r0, [%[SGPIO_REG_SS], #20]\n\t"
		"ldr r0, [%[p], #8]\n\t"
		"str r0, [%[SGPIO_REG_SS], #40]\n\t"
		"ldr r0, [%[p], #12]\n\t"
		"str r0, [%[SGPIO_REG_SS], #8]\n\t"
		"ldr r0, [%[p], #16]\n\t"
		"str r0, [%[SGPIO_REG_SS], #36]\n\t"
		"ldr r0, [%[p], #20]\n\t"
		"str r0, [%[SGPIO_REG_SS], #16]\n\t"
		"ldr r0, [%[p], #24]\n\t"
		"str r0, [%[SGPIO_REG_SS], #32]\n\t"
		"ldr r0, [%[p], #28]\n\t"
		"str r0, [%[SGPIO_REG_SS], #0]\n\t"
		:
		: [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
		  [p] "l" ((uint32_t*)samplebuf)
		: "r0"
	);

    /* now prepare samples for next round */
    static uint16_t phase = 0;

    /* the current frequency offset is stored here
     * so we can ramp from one offset to the other one
     */
    static int8_t freq = 0;

    /* either +8 or -8 (rather than 0/1, what you would
     * probably expect)
     *
     * we start in the middle...
     */
    static int8_t target = 0;

    /* every 2nd set of 16 samples, we send a new symbol
     * so our baud rate is
     * 4000000 / (2*16) = 125000
     */
    static uint8_t round = 0;
    if(round == 0) {
        target = get_bit_freq();
    }
    round = round ^ 1;

    for(int n = 0; n<16; n++) {
        /* offset: 16/1024 of sample rate
         * = 4 MHz * 16 / 1024 = 62.5 kHz
         */
        phase += 16;

        /* ramp for the frequency shift:
         */
        if(freq < target) freq++;
        if(freq > target) freq--;

        /* frequency shift:
         * +/- 8/1024 of sample rate
         * = 4 MHz * 8 / 1024 = +/- 31.25 kHz
         */
        phase += freq;

        samplebuf[n] = cos_sin[phase % 1024];
    }
}

static void transmit(uint8_t *data, uint16_t length) {
    /* wait for completion of last transfer */
    while(transmitting) {};
    transmitting = true;

    /* if in receive mode, stop generating samples */
    baseband_streaming_disable();
    /* set up TX mode */
    vector_table.irq[NVIC_SGPIO_IRQ] = bfsk_sgpio_isr_tx;
    /* set up RF path */
    rf_path_set_direction(RF_PATH_DIRECTION_TX);
    set_rf_params(false);
    /* set TX frequency */
    my_set_frequency(frequency, TX_FREQOFFSET);
    /* set TX sample rate */
    sample_rate_frac_set(TX_SAMPLERATE * 2, 1);
    /* and LPF */
    baseband_filter_bandwidth_set(TX_BANDWIDTH);

    tx_pkg = data;
    tx_pkg_len = length;

    baseband_streaming_enable();
}
/* ---------------------------- RECEIVE: -------------------------------- */

/* frequency offset is -500kHz because of the shift done by the first filter
 * (to get the DC peak out of the way)
 */
#define RX_FREQOFFSET (-500000)
#define RX_BANDWIDTH  1750000
#define RX_SAMPLERATE 2000000
#define RX_DECIMATION 1 /* effective sample rate is 2000000 */

/* packets are assembled by the ISR and its subroutines, then a flag is
 * set to handle the packet data outside of the ISR.
 */
#define MAX_PACKET_LEN 255
/* packet buffer */
static volatile uint8_t rx_pkg[MAX_PACKET_LEN+1];
/* length of received data */
static volatile uint32_t rx_pkg_len;
static volatile bool rx_pkg_flag = false;

/* filter functions from portapack C code, slightly modified */
static void my_fir_cic3_decim_2_s16_s16(
	complex_s16_t* const src,
	complex_s16_t* const dst,
	const size_t sample_count
) {
	/* Complex non-recursive 3rd-order CIC filter (taps 1,3,3,1).
	 * Gain of 8.
	 * Consumes 16 bytes (4 s16:s16 samples) per loop iteration,
	 * Produces  8 bytes (2 s16:s16 samples) per loop iteration.
	 */
	int32_t n = sample_count;
	static uint32_t t1 = 0;
	static uint32_t t2 = 0;
	uint32_t t3, t4;
	uint32_t taps = 0x00000003;
	uint32_t* s = (uint32_t*)src;
	uint32_t* d = (uint32_t*)dst;
	uint32_t i, q;
	for(; n>0; n-=4) {
		i = __SXTH(t1, 0);			/* 1: I0 */
		q = __SXTH(t1, 16);			/* 1: Q0 */
		i = __SMLABB(t2, taps, i);	/* 1: I1*3 + I0 */
		q = __SMLATB(t2, taps, q);	/* 1: Q1*3 + Q0 */

		t3 = *(s++);				/* 3: Q2:I2 */
		t4 = *(s++);				/*    Q3:I3 */

		i = __SMLABB(t3, taps, i);	/* 1: I2*3 + I1*3 + I0 */
		q = __SMLATB(t3, taps, q);	/* 1: Q2*3 + Q1*3 + Q0 */
		i = __SXTAH(i, t4, 0);		/* 1: I3 + Q2*3 + Q1*3 + Q0 */
		q = __SXTAH(q, t4, 16);		/* 1: Q3 + Q2*3 + Q1*3 + Q0 */
		i = __BFI(i, q, 16, 16);	/* 1: D2_Q0:D2_I0 */
		*(d++) = i;					/* D2_Q0:D2_I0 */

		i = __SXTH(t3, 0);			/* 1: I2 */
		q = __SXTH(t3, 16);			/* 1: Q2 */
		i = __SMLABB(t4, taps, i);	/* 1: I3*3 + I2 */
		q = __SMLATB(t4, taps, q);	/* 1: Q3*3 + Q2 */

		t1 = *(s++);				/* 3: Q4:I4 */
		t2 = *(s++);				/*    Q5:I5 */

		i = __SMLABB(t1, taps, i);	/* 1: I4*3 + I3*3 + I2 */
		q = __SMLATB(t1, taps, q);	/* 1: Q4*3 + Q3*3 + Q2 */
		i = __SXTAH(i, t2, 0);		/* 1: I5 + Q4*3 + Q3*3 + Q2 */
		q = __SXTAH(q, t2, 16);		/* 1: Q5 + Q4*3 + Q3*3 + Q2 */
		i = __BFI(i, q, 16, 16);	/* 1: D2_Q1:D2_I1 */
		*(d++) = i;					/* D2_Q1:D2_I1 */
	}
}

static void my_translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(
	complex_s8_t* const src_and_dst,
	const size_t sample_count
) {
	/* Translates incoming complex<int8_t> samples by -fs/4,
	 * decimates by two using a non-recursive third-order CIC filter.
	 */
	int32_t n = sample_count;
	static uint32_t q1_i0 = 0;
	static uint32_t q0_i1 = 0;
	uint32_t k_3_1 = 0x00030001;
	uint32_t* p = (uint32_t*)src_and_dst;
	for(; n>0; n-=4) {
		const uint32_t q3_i3_q2_i2 = p[0];							// 3
		const uint32_t q5_i5_q4_i4 = p[1];

		const uint32_t i2_i3 = __SXTB16(q3_i3_q2_i2, 16);			// 1: (q3_i3_q2_i2 ror 16)[23:16]:(q3_i3_q2_i2 ror 16)[7:0]
		const uint32_t q3_q2 = __SXTB16(q3_i3_q2_i2,  8);			// 1: (q3_i3_q2_i2 ror  8)[23:16]:(q3_i3_q2_i2 ror  8)[7:0]
		const uint32_t i2_q3 = __PKHTB(i2_i3, q3_q2, 16);			// 1: Rn[31:16]:(Rm>>16)[15:0]
		const uint32_t i3_q2 = __PKHBT(q3_q2, i2_i3, 16);			// 1:(Rm<<16)[31:16]:Rn[15:0]

		// D_I0 = 3 * (i2 - q1) + (q3 - i0)
		const uint32_t i2_m_q1_q3_m_i0 = __QSUB16(i2_q3, q1_i0);	// 1: Rn[31:16]-Rm[31:16]:Rn[15:0]-Rm[15:0]
		const uint32_t d_i0 = __SMUAD(k_3_1, i2_m_q1_q3_m_i0);		// 1: Rm[15:0]*Rs[15:0]+Rm[31:16]*Rs[31:16]

		// D_Q0 = 3 * (q2 + i1) - (i3 + q0)
		const uint32_t i3_p_q0_q2_p_i1 = __QADD16(i3_q2, q0_i1);	// 1: Rn[31:16]+Rm[31:16]:Rn[15:0]+Rm[15:0]
		const uint32_t d_q0 = __SMUSDX(i3_p_q0_q2_p_i1, k_3_1);		// 1: Rm[15:0]*Rs[31:16]–Rm[31:16]*RsX[15:0]
		const uint32_t d_q0_i0 = __PKHBT(d_i0, d_q0, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		const uint32_t i5_i4 = __SXTB16(q5_i5_q4_i4,  0);			// 1: (q5_i5_q4_i4 ror  0)[23:16]:(q5_i5_q4_i4 ror  0)[7:0]
		const uint32_t q4_q5 = __SXTB16(q5_i5_q4_i4, 24);			// 1: (q5_i5_q4_i4 ror 24)[23:16]:(q5_i5_q4_i4 ror 24)[7:0]
		const uint32_t q4_i5 = __PKHTB(q4_q5, i5_i4, 16);			// 1: Rn[31:16]:(Rm>>16)[15:0]
		const uint32_t q5_i4 = __PKHBT(i5_i4, q4_q5, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		// D_I1 = (i2 - q5) + 3 * (q3 - i4)
		const uint32_t i2_m_q5_q3_m_i4 = __QSUB16(i2_q3, q5_i4);	// 1: Rn[31:16]-Rm[31:16]:Rn[15:0]-Rm[15:0]
		const uint32_t d_i1 = __SMUADX(i2_m_q5_q3_m_i4, k_3_1);		// 1: Rm[15:0]*Rs[31:16]+Rm[31:16]*Rs[15:0]

		// D_Q1 = (i5 + q2) - 3 * (q4 + i3)
		const uint32_t q4_p_i3_i5_p_q2 = __QADD16(q4_i5, i3_q2);	// 1: Rn[31:16]+Rm[31:16]:Rn[15:0]+Rm[15:0]
		const uint32_t d_q1 = __SMUSD(k_3_1, q4_p_i3_i5_p_q2);		// 1: Rm[15:0]*Rs[15:0]–Rm[31:16]*Rs[31:16]
		const uint32_t d_q1_i1 = __PKHBT(d_i1, d_q1, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		q1_i0 = q5_i4;
		q0_i1 = q4_i5;

		*(p++) = d_q0_i0;											// 3
		*(p++) = d_q1_i1;
	}
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

    shift <<= 1;
    shift |= bit;
    if(sync == 0) {
        /* wait for sync */
        if(shift == PREAMBLE) {
            pkglen = -1;
            sync = 1;
            TOGGLE(LED4);
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

            rx_pkg_len = 0;
            return;
        }
        if(rx_pkg_len < pkglen) {
            rx_pkg[rx_pkg_len++] = shift & 0xFF;
            if(rx_pkg_len == pkglen)
                rx_pkg_flag = true;
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
static void freq_detect(complex_s16_t sample) {
    /* phase difference offset for signal detection
     */
    static int16_t offset;

    /* previous sample */
    static complex_s16_t prev;

    /* at first, we calculate the phase difference between the last
     * sample and the current one, corresponding to the signal's
     * frequency
     */
    int32_t diff_q = sample.q * prev.q - (-sample.i) * prev.i;
    int32_t diff_i = sample.q * prev.i + (-sample.i) * prev.q;

    /* keep as much precision as we can */
    while(diff_q > 0x7FFF || diff_q < -0x8000 || diff_i > 0x7FFF || diff_i < -0x8000) {
        diff_q >>= 1;
        diff_i >>= 1;
    }

    const int16_t p_sample = fxpt_atan2(diff_i, diff_q);

    prev = sample;

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
            decoder(1);
        }
    } else if((p_diff < 0) && (prev_p_diff < 0)) {
        if(1 == (last_switch % 4)) {
            decoder(0);
        }
    } else {
        last_switch = 0;
    }
    last_switch++;
    prev_p_diff = p_diff;
}

/*
 * the following follows the example of hackrf's sgpio_isr
 * which triggers read from SGPIO on 8 planes
 *
 * we read 32 bytes, i.e. 16 8bit complex samples (8bit q, 8bit i)
 * and we will immediately send it through the filter chain
 **/
void bfsk_sgpio_isr_rx() {
    SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);
    static uint32_t buffer[8];
    __asm__(
        "ldr r0, [%[SGPIO_REG_SS], #44]\n\t"
        "str r0, [%[buffer], #0]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #20]\n\t"
        "str r0, [%[buffer], #4]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #40]\n\t"
        "str r0, [%[buffer], #8]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #8]\n\t"
        "str r0, [%[buffer], #12]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #36]\n\t"
        "str r0, [%[buffer], #16]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #16]\n\t"
        "str r0, [%[buffer], #20]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #32]\n\t"
        "str r0, [%[buffer], #24]\n\t"
        "ldr r0, [%[SGPIO_REG_SS], #0]\n\t"
        "str r0, [%[buffer], #28]\n\t"
        :
        : [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
          [buffer] "l" (buffer)
        : "r0"
    );

    /* 2 MHz complex<int8> */
    my_translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16((complex_s8_t*) buffer, 16);
    /* 1 MHz complex<int16>[N/2] */
    complex_s16_t* const buf16 = (complex_s16_t*) buffer;
    my_fir_cic3_decim_2_s16_s16(buf16, buf16, 8);
    /* 500 kHz complex<int16>[N/4] */
    for(int n=0; n < 4; n++) {
        freq_detect(buf16[n]);
    }
}

static void receive() {
    /* wait for completion of last transfer */
    while(transmitting) {};

    /* set up RX mode */
    vector_table.irq[NVIC_SGPIO_IRQ] = bfsk_sgpio_isr_rx;
    /* set up RF path */
    rf_path_set_direction(RF_PATH_DIRECTION_RX);
    set_rf_params(true);
    /* set RX frequency */
    my_set_frequency(frequency, RX_FREQOFFSET);
    /* set TX sample rate */
    sample_rate_frac_set(RX_SAMPLERATE * 2, 1);
    /* and LPF */
    baseband_filter_bandwidth_set(RX_BANDWIDTH);
    /* set up decimation in CPLD */
    sgpio_cpld_stream_rx_set_decimation(RX_DECIMATION);

    rx_pkg_flag = false;

    baseband_streaming_enable();
}

/* ------------------------------------------------------------------- */

#define MAX_LED_FRAMES 50
#define BUF_SIZE 3*8*MAX_LED_FRAMES+2

unsigned char leds[BUF_SIZE];
unsigned int frames = 0;
unsigned int ctr = 0;
unsigned int framectr = 0;

void readRgbLedFile(void) {
	int size = getFileSize(GLOBAL(ledfile));
	frames = 0;
	ctr = 0;
	framectr = 0;
	if(size > 0) {
		if(size >= BUF_SIZE)
			size = BUF_SIZE;
		readFile(GLOBAL(ledfile), (char*)leds, size);
		frames = (size-2)/(3*8);
	}
}

/**************************************************************************/

void senddata(uint8_t *data, uint16_t len) {
    transmit(data, len);
    receive();
}


void init_rgbLeds(void) {
	readTextFile("ledfile.cfg",GLOBAL(ledfile),FLEN);
	readRgbLedFile();
}

void tx_rgbLeds(void) {
	if(GLOBAL(rgbleds)) {
		if(frames > 0) {
			if(ctr == 0) {
                senddata(&leds[framectr*3*8+2], 3*8);
				framectr++;
				if(framectr >= frames)
					framectr = 0;
			}

			ctr++;
			// LED delay is in leds[0:1]
			if(ctr >= ((leds[0]<<8) + leds[1]))
				ctr = 0;
		}
	}
	return;
}

const uint8_t nleds = 8;

void set_led(uint8_t *pattern, int index, uint8_t r, uint8_t g, uint8_t b)
{
    pattern[index*3+0] = r;
    pattern[index*3+1] = g;
    pattern[index*3+2] = b;
}

void animation_tx(void)
{
    tx_rgbLeds();
}

void set_all(uint8_t *pattern, uint8_t r, uint8_t g, uint8_t b)
{
    int i;
    for(i = 0; i < nleds; i++) {
        set_led(pattern, i, r, g, b);
    }
}

void display_nick(uint8_t nickCol, uint8_t lcdCol)
{
  uint8_t dx;
  uint8_t dy;

  dx=DoString(0,0,GLOBAL(nickname));
  dx=(RESX-dx)/2;
  if(dx<0)
    dx=0;
  dy=(RESY-getFontHeight())/2;
  setExtFont(GLOBAL(nickfont));
  setTextColor(lcdCol,nickCol);
  lcdFill(lcdCol);
  lcdSetCrsr(dx,dy);
  lcdPrint(GLOBAL(nickname));
  lcdDisplay();
}

void serial_handler(uint8_t data)
{
    static uint8_t buf[128];
    static int index = 0;
    static bool sync = false;
    static uint8_t lcdCol = 0x00;
    static uint8_t nickCol = 0xFF;
    static uint8_t lcdBrightness = 0x00;
    uint8_t pattern[nleds * 3];
    uint8_t led_bright = 4;


    buf[index] = data;

    if(index == 1) {
        if(buf[0] == '!' && buf[1] == '!') {
            sync = true;
        } else {
            sync = false;
            index = -1;
        }
    }

    if(sync && index == 3) {
        if((buf[2] >> 4) != 1 ||  buf[3] > 24) {
            sync = false;
            index = -1;
        }
    }

    if(sync && index > 3) {

        if(index == buf[3] + 5) { //All data received
            if(buf[index] == '\r' || buf[index] == '\n') { //CRLF at the End

                senddata(buf + 2, buf[3] + 2);  //SendData

                //Show the Pattern on the master
                switch (buf[2]) {
                    case 0x10: //All LEDs off
                      set_all(pattern, 0, 0, 0);
                      baseband_streaming_disable();
                      ws2812_sendarray(pattern, sizeof(pattern));
                      baseband_streaming_enable();
                      break;
                    case 0x11: //All LEDs same Color
                      set_all(pattern, buf[5]/led_bright, buf[4]/led_bright, buf[6]/led_bright);
                      baseband_streaming_disable();
                      lcdCol = (RGB_TO_8BIT(buf[4],buf[5],buf[6]));
                      nickCol = (RGB_TO_8BIT((0xFF-buf[4]),(0xFF-buf[5]),(0xFF-buf[6])));
                      display_nick(nickCol, lcdCol);
                      ws2812_sendarray(pattern, sizeof(pattern));
                      baseband_streaming_enable();
                      break;
                    case 0x12: //Set Display Color
                      baseband_streaming_disable();
                      lcdCol = (RGB_TO_8BIT(buf[4],buf[5],buf[6]));
                      nickCol = (RGB_TO_8BIT((0xFF-buf[4]),(0xFF-buf[5]),(0xFF-buf[6])));
                      display_nick(nickCol, lcdCol);
                      lcdDisplay();
                      baseband_streaming_enable();
                      break;
                    case 0x13: //Set Animation No
                      //Not implemented yet
                      break;
                    case 0x14: //Set Display Animation No
                        //Not implemented yet
                      break;
                    case 0x15: //Set one LED
                      set_led(pattern, buf[4], buf[6]/led_bright, buf[5]/led_bright, buf[7]/led_bright);
                      baseband_streaming_disable();
                      ws2812_sendarray(pattern, sizeof(pattern));
                      baseband_streaming_enable();
                      break;
                    case 0x1D: //set All LEDs diffrent
                      set_all(pattern,0,0,0); //all Off
                      for(uint8_t i = 0; i < buf[3]/3; i++){
                        set_led(pattern, i, buf[5+(3*i)]/led_bright,buf[4+(3*i)]/led_bright,buf[6+(3*i)]/led_bright);
                      }
                      baseband_streaming_disable();
                      ws2812_sendarray(pattern, sizeof(pattern));
                      baseband_streaming_enable();
                      break;
                }
            }
            sync = false;
            index = -1;
        }
    }

    index += 1;
    if(index == sizeof(buf)) {
        index = 0;
    }

}

//# MENU L0ungeL1cht
void l0ungel1cht() {
    uint8_t pattern[nleds * 3];
    int i = 0;
    int j = 0;
    int dx=0;
    int dy=0;
    static uint8_t lcdCol = 0x00;
    static uint8_t nickCol = 0xFF;
    uint8_t led_bright = 4;

    getInputWaitRelease();

    memset(pattern, 0, sizeof(pattern));
    ws2812_sendarray(pattern, sizeof(pattern));

    init_rgbLeds();

    cpu_clock_set(204);

    rfinit();

    uart_init(UART0_NUM, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE, 111, 0, 1);

    receive();

    while(1) {
       //MENU
        switch (getInputRaw()) {
            case BTN_ENTER:
                goto stop;
        }

        if(rx_pkg_flag) {
            rx_pkg_flag = false;
            if(rx_pkg_len > 2 && rx_pkg[1] == rx_pkg_len - 2) {
                if((rx_pkg[0] == 0x10 || 1) && rx_pkg[1] <= 24) {
                  switch (rx_pkg[0]) {
                      case 0x10: //All LEDs off
                        set_all(pattern, 0, 0, 0);
                        baseband_streaming_disable();
                        ws2812_sendarray(pattern, sizeof(pattern));
                        baseband_streaming_enable();
                        break;
                      case 0x11: //All LEDs same Color
                        set_all(pattern, rx_pkg[3]/led_bright, rx_pkg[2]/led_bright, rx_pkg[4]/led_bright);
                        baseband_streaming_disable();
                        lcdCol = (RGB_TO_8BIT(rx_pkg[2],rx_pkg[3],rx_pkg[4]));
                        nickCol = (RGB_TO_8BIT((0xFF-rx_pkg[2]),(0xFF-rx_pkg[3]),(0xFF-rx_pkg[4])));
                        display_nick(nickCol, lcdCol);
                        ws2812_sendarray(pattern, sizeof(pattern));
                        baseband_streaming_enable();
                        break;
                      case 0x12: //Set Display Color
                        baseband_streaming_disable();
                        lcdCol = (RGB_TO_8BIT(rx_pkg[2],rx_pkg[3],rx_pkg[4]));
                        nickCol = (RGB_TO_8BIT((0xFF-rx_pkg[2]),(0xFF-rx_pkg[3]),(0xFF-rx_pkg[4])));
                        display_nick(nickCol, lcdCol);
                        baseband_streaming_enable();
                        break;
                      case 0x13: //Set Animation No
                        //Not implemented yet
                        break;
                      case 0x14: //Set Display Animation No
                          //Not implemented yet
                        break;
                      case 0x15: //Set one LED
                        set_led(pattern, rx_pkg[2], rx_pkg[4]/led_bright, rx_pkg[3]/led_bright, rx_pkg[5]/led_bright);
                        baseband_streaming_disable();
                        ws2812_sendarray(pattern, sizeof(pattern));
                        baseband_streaming_enable();
                        break;
                      case 0x1D: //set All LEDs diffrent
                        set_all(pattern,0,0,0); //all Off
                        for(uint8_t i = 0; i < rx_pkg[1]/3; i++){
                          set_led(pattern, i, rx_pkg[3+(3*i)]/led_bright,rx_pkg[2+(3*i)]/led_bright,rx_pkg[4+(3*i)]/led_bright);
                        }
                        baseband_streaming_disable();
                        ws2812_sendarray(pattern, sizeof(pattern));
                        baseband_streaming_enable();
                        break;
                  }
                }
            }
        }
        if(uart_rx_data_ready(UART0_NUM) == UART_RX_DATA_READY) {
            uint8_t data = uart_read(UART0_NUM);
            serial_handler(data);
        }
    }
stop:
    baseband_streaming_disable();
    OFF(EN_1V8);
    OFF(EN_VDD);
    return;
}
