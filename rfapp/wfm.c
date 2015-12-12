/* FM Radio RX/TX
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
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <rad1olib/pins.h>

#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/tuning.h>
#include <common/max2837.h>
#include <common/streaming.h>
#include <libopencm3/lpc43xx/dac.h>
#include <libopencm3/lpc43xx/adc.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/cm3/vector.h>

#include <portalib/arm_intrinsics.h>
#include <portalib/complex.h>
#include <portalib/fxpt_atan2.h>
#include <stddef.h>

#include "cossin1024.h"

// define this to enable transmit functionality
#define TRANSMIT

// default to 100 MHz
#define FREQSTART 100000000
// by default, frequency steps are 100 kHz
#define FREQSTEP 100000

// receive options
#define BANDWIDTH  1750000
#define SAMPLERATE 12288000
#define DECIMATION 4
#define FREQOFFSET -(SAMPLERATE/DECIMATION/4)

// transmit options
#define TX_BANDWIDTH  1750000
#define TX_SAMPLERATE 8000000
#define TX_FREQOFFSET 0
#define TX_DECIMATION 1

// default audio output level
#define AUDIOVOLUME 20
#define TXVOLUME 4

static volatile int audiovolume = AUDIOVOLUME;
static volatile int txvolume = TXVOLUME;

static int freq_offset = FREQOFFSET;

static int64_t frequency_step = FREQSTEP;
static int64_t frequency = FREQSTART;

static void my_set_frequency(const int64_t new_frequency) {
    const int64_t tuned_frequency = new_frequency + freq_offset;
    ssp1_set_mode_max2837();
    if(set_freq(tuned_frequency)) {
        frequency = new_frequency;
    }
}

/* filter functions from portapack C code, slightly modified */

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

static void my_fir_cic4_decim_2_real_s16_s16(
	int16_t* src,
	int16_t* dst,
	const size_t sample_count
) {
	static const int16_t tap[] = { 1, 4, 6, 4, 1 };
	static int16_t z[7];

	int32_t n = sample_count;
	for(; n>0; n-=2) {
		z[5] = *(src++);
		z[5+1] = *(src++);

		int32_t t = 0;
		for(int_fast8_t j=0; j<5; j++) {
			t += z[j] * tap[j];
			z[j] = z[j+2];
		}
		*(dst++) = t / 16;
	}
}

static void my_fm_demodulate_s16_s16(
	const complex_s16_t* const src,
	int16_t* dst,
	int32_t n
) {
	static complex_s16_t z1;

	const complex_s16_t* p = src;
	for(; n>0; n-=1) {
		const complex_s16_t s = *(p++);
		const complex_s32_t t = multiply_conjugate_s16_s32(s, z1);
		z1 = s;
		*(dst++) = fxpt_atan2(t.q >> 12, t.i >> 12) >> 1;
	}
}

/*
 * the following follows the example of hackrf's sgpio_isr
 * which triggers read from SGPIO on 8 planes
 *
 * using the DMA approach from portapack code (old C code)
 * failed miserably, samples are distorted.
 *
 * as this happens even for lower sample rates, it is probably not
 * a bandwidth issue with memory, but some problem between LPC4330
 * and CPLD. In DMA mode, the CPLD should switch to single-plane
 * mode - I've yet to see this setup work properly on a rad1o.
 *
 * we read 32 bytes, i.e. 16 8bit complex samples (8bit q, 8bit i)
 * and we will immediately send it through the filter chain
 * and then write out the resulting audio sample, which is one
 * real 16bit sample for the 16 samples input.
 *
 * At 3.072 MHz input sampling rate, this results in a sampling
 * rate of 192 kHz on the DAC.
 **/
static void sgpio_isr_rx() {
    static uint32_t buffer[8];

    SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

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

    my_translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16((complex_s8_t*) buffer, 16);
    complex_s16_t* const in_cs16 = (complex_s16_t*) buffer;

    /* 1.544MHz complex<int16>[N/2]
     * -> 3rd order CIC decimation by 2, gain of 8
     * -> 768kHz complex<int16>[N/4] */
    my_fir_cic3_decim_2_s16_s16(in_cs16, in_cs16, 8);

    /* 768kHz complex<int32>[N/4]
     * -> FIR LPF, 90kHz cut-off, max attenuation by 192kHz.
     * -> 768kHz complex<int32>[N/4] */
    /* TODO: To improve adjacent channel rejection, implement complex channel filter:
     *        pass < +/- 100kHz, stop > +/- 200kHz
     */

    /* 768kHz complex<int16>[N/4]
     * -> FM demodulation
     * -> 768kHz int16[N/4] */
    int16_t* const work_int16 = (int16_t*)in_cs16;
    my_fm_demodulate_s16_s16(in_cs16, work_int16, 4);

    /* 768kHz int16[N/4]
     * -> 4th order CIC decimation by 2, gain of 1
     * -> 384kHz int16[N/8] */
    my_fir_cic4_decim_2_real_s16_s16(work_int16, work_int16, 4);

    /* 384kHz int16[N/8]
     * -> 4th order CIC decimation by 2, gain of 1
     * -> 192kHz int16[N/16] */
    my_fir_cic4_decim_2_real_s16_s16(work_int16, work_int16, 2);

    int32_t v = work_int16[0] * audiovolume + 0x8000;
    // clip:
    uint16_t u = v < 0 ? 0 : ( v > 0xFFFF ? 0x3FF : (v >> 6));
    dac_set(u);
}

#ifdef TRANSMIT

inline void my_adc_start(uint32_t adc, uint32_t flags)
{
	ADC_CR(adc)=flags | ADC_CR_CLKDIV((uint8_t)(208/4.5))|ADC_CR_10BITS|ADC_CR_POWER|ADC_CR_START;
}

/* from libopencm3, we copy it here to have it inlined */
inline uint16_t my_adc_get_single(uint32_t adc, uint32_t flags)
{
	uint32_t result;

	do {
		result=ADC_GDR(adc);
	} while( (!ADC_DR_DONE(result)) );

    uint16_t adc_value = ADC_DR_VREF(result);

    my_adc_start(adc, flags);

	return adc_value;
};

/*
 * the following follows the example of hackrf's sgpio_isr
 * which triggers write to SGPIO on 8 planes
 * will be triggered with FREQ/16
 **/
void sgpio_isr_tx() {
    static complex_s8_t samplebuf[16];
    static uint32_t audiosamplebuf = 0; // we use this to rotate 3x 10bit
    static uint32_t audiosample = 0;
    static int16_t audiosample_diff = 0;
    static uint8_t audioctr = 0;

	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

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
    /* ADC 10 bit conversion time is 2.45 us minimum (see UM10503, ch. 47)
     * so we fetch a new audio sample only every 4th run.
     * This means we get 1 new audio sample per 64 generated I/Q samples.
     **/
    if(0 == (audioctr++ & 3)) {
        // f = 125kHz, -> 8us

        // moving average over 4 samples:
        uint16_t sample = my_adc_get_single(ADC0,ADC_CR_CH7);

        int32_t audiosample_new =
            sample +
            (audiosamplebuf & 0x3FF) +
            ((audiosamplebuf & (0x3FF<<11)) >> 11) +
            (audiosamplebuf >> 22);

        audiosamplebuf = (audiosamplebuf << 11) + sample;

        /* Remove the ADC offset (roughly).
         * Some filter would better.
         * 4 times, as the sample from the ADC gets
         * accumulated above */
        audiosample_new -= 512 * 4;
        
        audiosample_new *= txvolume;
        /* we will linearly go from previous sample to this over 4x 16 steps.
         * we start with 64x the previous sample (already accumulated in the
         * variable), 0x the current one, and will on each step add the
         * difference, which we need to calculate here:
         **/
        audiosample_diff = audiosample_new - (audiosample >> 6);
    }
    /* prepare 16 I/Q samples.
     * "audiosample" contains the sum of 64 audio samples and thus
     * needs to be shifted/divided.
     **/
    static uint32_t j = 0;
    for(int i=0; i<16; i++) {
        audiosample += audiosample_diff;
        j = j + audiosample;
        samplebuf[i] = cos_sin[(j >> 16) % 1024];
    }
}

static bool transmitting = false;

static void transmit(bool enable) {
    baseband_streaming_disable();
    if(enable) {
        my_adc_start(ADC0,ADC_CR_CH7);
        vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_tx;
        freq_offset = TX_FREQOFFSET;
        my_set_frequency(frequency);
        rf_path_set_direction(RF_PATH_DIRECTION_TX);
        sample_rate_frac_set(TX_SAMPLERATE * 2, 1);
        baseband_filter_bandwidth_set(TX_BANDWIDTH);
        sgpio_cpld_stream_rx_set_decimation(TX_DECIMATION);
        dac_set(0);
        OFF(MIC_AMP_DIS); // enable amp
    } else {
        vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_rx;
        freq_offset = FREQOFFSET;
        my_set_frequency(frequency);
        rf_path_set_direction(RF_PATH_DIRECTION_RX);
        sample_rate_frac_set(SAMPLERATE * 2, 1);
        baseband_filter_bandwidth_set(BANDWIDTH);
        sgpio_cpld_stream_rx_set_decimation(DECIMATION);
        ON(MIC_AMP_DIS); // disable amp
    }
    transmitting = enable;
    baseband_streaming_enable();
}

static int32_t txvga_gain_db = 20;
#endif // TRANSMIT

static bool lna_enable = true;
static int32_t lna_gain_db = 24;
static int32_t vga_gain_db = 20;

/* set amps */
static void set_rf_params() {
    ssp1_set_mode_max2837(); // need to reset this since display driver will hassle with SSP1
    rf_path_set_lna(lna_enable ? 1 : 0);
    max2837_set_lna_gain(lna_gain_db);     /* 8dB increments */
    max2837_set_vga_gain(vga_gain_db);     /* 2dB increments, up to 62dB */
#ifdef TRANSMIT
    max2837_set_txvga_gain(txvga_gain_db); /* 1dB increments, up to 47dB */
#endif
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
    /* Disable unused clock outputs. They generate noise. */
    scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
    scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

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
    
    // set up SGPIO ISR
    vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_rx;

    ssp1_init();

    rf_path_init();
    set_rf_params();
    rf_path_set_direction(RF_PATH_DIRECTION_RX);

    my_set_frequency(frequency);

    sample_rate_frac_set(SAMPLERATE * 2, 1);
    baseband_filter_bandwidth_set(BANDWIDTH);
    sgpio_cpld_stream_rx_set_decimation(DECIMATION);

    baseband_streaming_enable();
}

//static int menuitem = 0;
#ifdef TRANSMIT
static enum {MENU_FREQ, MENU_STEP, MENU_TRANSMIT, MENU_VOLUME, MENU_LNA, MENU_BBLNA, MENU_BBVGA, MENU_BBTXVGA, MENU_EXIT, MENUITEMS} menuitem = MENU_FREQ;
#else // TRANSMIT
static enum {MENU_FREQ, MENU_STEP, MENU_VOLUME, MENU_LNA, MENU_BBLNA, MENU_BBVGA, MENU_EXIT, MENUITEMS} menuitem = MENU_FREQ;
#endif // TRANSMIT

static void status() {
    lcdClear();
    lcdSetCrsr(0,0);
    lcdPrintln("WFM rad1o  [@hilse]");
    lcdPrintln("-o-o-o-o-o-o-o-o-o-");

    if(menuitem == MENU_FREQ) lcdPrint("> "); else lcdPrint("  ");
    lcdPrint(IntToStr(frequency/1000000,4,F_LONG));
    lcdPrint(",");
    lcdPrint(IntToStr((frequency%1000000) / 1000, 3, F_LONG | F_ZEROS));
    lcdPrintln(" MHz ");

    if(menuitem == MENU_STEP) lcdPrint("> Step: "); else lcdPrint("  Step: ");
    lcdPrint(IntToStr(frequency_step/1000000,3,F_LONG));
    lcdPrint(",");
    lcdPrintln(IntToStr((frequency_step%1000000) / 1000, 3, F_LONG | F_ZEROS));

#ifdef TRANSMIT
    if(menuitem == MENU_TRANSMIT) lcdPrint("> RX/TX: "); else lcdPrint("  RX/TX: ");
    if(transmitting) lcdPrintln("SENDING"); else lcdPrintln("listening");
#endif

    lcdNl();

    if(transmitting) {
        if(menuitem == MENU_VOLUME) lcdPrint("> TX Vol: "); else lcdPrint("  TX Vol: ");
        lcdPrintln(IntToStr(txvolume,3,F_LONG));
    } else {
        if(menuitem == MENU_VOLUME) lcdPrint("> RX Vol: "); else lcdPrint("  RX Vol: ");
        lcdPrintln(IntToStr(audiovolume,3,F_LONG));
    }

    lcdNl();
    lcdPrintln("  Settings:");
    lcdNl();

    if(menuitem == MENU_LNA) lcdPrint("> LNA: "); else lcdPrint("  LNA: ");
    if(lna_enable) lcdPrintln("on (+14dB)"); else lcdPrintln("off");

    if(menuitem == MENU_BBLNA) lcdPrint("> BBLNA: +"); else lcdPrint("  BBLNA: +");
    lcdPrint(IntToStr(lna_gain_db,2,F_LONG));
    lcdPrintln(" dB");

    if(menuitem == MENU_BBVGA) lcdPrint("> BBVGA: +"); else lcdPrint("  BBVGA: +");
    lcdPrint(IntToStr(vga_gain_db,2,F_LONG));
    lcdPrintln(" dB");

#ifdef TRANSMIT
    if(menuitem == MENU_BBTXVGA) lcdPrint("> BBTXVGA: +"); else lcdPrint("  BBTXVGA: +");
    lcdPrint(IntToStr(txvga_gain_db,2,F_LONG));
    lcdPrintln(" dB");
#endif

    lcdNl();
    if(menuitem == MENU_EXIT) lcdPrintln("> Exit"); else lcdPrintln("  Exit");

    lcdDisplay();
}

//# MENU WideFM_Radio
void wfm_menu() {
    lcdClear();
    lcdDisplay();
    getInputWaitRelease();

    cpu_clock_set(204);

    SETUPgout(MIC_AMP_DIS);
    ON(MIC_AMP_DIS); // disable amp

    dac_init(false); 
 
    status();

    rfinit();

    while(1) {
        switch (getInputWaitRepeat()) {
            case BTN_UP:
                if(menuitem == 0) menuitem = MENUITEMS;
                menuitem--;
                break;
            case BTN_DOWN:
                menuitem++;
                if(menuitem == MENUITEMS) menuitem = 0;
                break;
            case BTN_LEFT:
                switch (menuitem) {
                    case MENU_FREQ:
                        my_set_frequency(frequency - frequency_step);
                        break;
                    case MENU_STEP:
                        if(frequency_step > 1000) frequency_step /= 10;
                        break;
                    case MENU_VOLUME:
                        if(transmitting && txvolume > 0) txvolume--;
                        if(!transmitting && audiovolume > 0) audiovolume--;
                        break;
                    case MENU_LNA:
                        if(lna_enable) lna_enable=false; else lna_enable=true;
                        set_rf_params();
                        break;
                    case MENU_BBLNA:
                        if(lna_gain_db > 0) lna_gain_db-=8;
                        set_rf_params();
                        break;
                    case MENU_BBVGA:
                        if(vga_gain_db > 0) vga_gain_db-=2;
                        set_rf_params();
                        break;
#ifdef TRANSMIT
                    case MENU_BBTXVGA:
                        if(txvga_gain_db > 0) txvga_gain_db--;
                        set_rf_params();
                        break;
                    case MENU_TRANSMIT:
                        transmit(!transmitting);
                        break;
#endif // TRANSMIT
                    default: /*nothing*/ break;
                }
                break;
            case BTN_RIGHT:
                switch (menuitem) {
                    case MENU_FREQ:
                        my_set_frequency(frequency + frequency_step);
                        break;
                    case MENU_STEP:
                        if(frequency_step < 100000000) frequency_step *= 10;
                        break;
                    case MENU_VOLUME:
                        if(transmitting && txvolume < 16) txvolume++;
                        if(!transmitting && audiovolume < 80) audiovolume++;
                        break;
                    case MENU_EXIT:
                        goto stop;
                    case MENU_LNA:
                        if(lna_enable) lna_enable=false; else lna_enable=true;
                        set_rf_params();
                        break;
                    case MENU_BBLNA:
                        if(lna_gain_db < 40) lna_gain_db+=8;
                        set_rf_params();
                        break;
                    case MENU_BBVGA:
                        if(vga_gain_db < 62) vga_gain_db+=2;
                        set_rf_params();
                        break;
#ifdef TRANSMIT
                    case MENU_BBTXVGA:
                        if(txvga_gain_db < 47) txvga_gain_db++;
                        set_rf_params();
                        break;
                    case MENU_TRANSMIT:
                        // transmit as long as key is being held
                        transmit(!transmitting);
                        status();
                        getInputWaitRelease();
                        transmit(!transmitting);
                        break;
#endif // TRANSMIT
                    default: /*nothing*/ break;
                }
                break;
        }
        status();
    }
stop:
    baseband_streaming_disable();
    dac_set(0);
    OFF(EN_1V8);
    OFF(EN_VDD);
    return;
}
