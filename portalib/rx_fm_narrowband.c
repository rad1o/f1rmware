/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
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

#include "rx_fm_narrowband.h"

#include "decimate.h"
#include "demodulate.h"
#include "filters.h"

typedef struct rx_fm_narrowband_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t bb_dec_1;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_2;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_3;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_4;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_5;
	// TODO: Channel filter here.
	fm_demodulate_s16_s16_state_t fm_demodulate;
	fir_64_decim_2_real_s16_s16_state_t audio_dec;
} rx_fm_narrowband_to_audio_state_t;

void rx_fm_narrowband_to_audio_init(void* const _state) {
	rx_fm_narrowband_to_audio_state_t* const state = (rx_fm_narrowband_to_audio_state_t*)_state;

	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->bb_dec_1);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_2);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_3);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_4);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_5);
	// TODO: Channel filter here.
	fm_demodulate_s16_s16_init(&state->fm_demodulate, 96000, 2500);
	fir_64_decim_2_real_s16_s16_init(&state->audio_dec, taps_64_lp_031_063, 64);
}

void rx_fm_narrowband_to_audio_baseband_handler(void* const _state, complex_s8_t* const in, const size_t sample_count_in, baseband_timestamps_t* const timestamps) {
	rx_fm_narrowband_to_audio_state_t* const state = (rx_fm_narrowband_to_audio_state_t*)_state;

	size_t sample_count = sample_count_in;

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	sample_count = translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->bb_dec_1, in, sample_count);
	complex_s16_t* const in_cs16 = (complex_s16_t*)in;

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	complex_s16_t work[512];
	complex_s16_t* const work_cs16 = work;
	int16_t* const work_int16 = (int16_t*)work;
	sample_count = fir_cic3_decim_2_s16_s16(&state->bb_dec_2, in_cs16, work_cs16, sample_count);

	/* TODO: Gain through five CICs will be 32768 (8 ^ 5). Incorporate gain adjustment
	 * somewhere in the chain.
	 */

	/* TODO: Create a decimate_by_2_s16_s16 with gain adjustment and rounding, maybe use
	 * SSAT (no rounding) or use SMMULR/SMMLAR/SMMLSR (provides rounding)?
	 */

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	complex_s16_t* p = work_cs16;
	for(uint_fast16_t n=sample_count; n>0; n-=1) {
		p->i /= 2;
		p->q /= 2;
		p++;
	}

	/* 768kHz complex<int16>[N/4]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 384kHz complex<int16>[N/8] */
	sample_count = fir_cic3_decim_2_s16_s16(&state->bb_dec_3, work_cs16, work_cs16, sample_count);

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	p = work_cs16;
	for(uint_fast16_t n=sample_count; n>0; n-=1) {
		p->i /= 8;
		p->q /= 8;
		p++;
	}

	/* 384kHz complex<int16>[N/8]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 192kHz complex<int16>[N/16] */
	sample_count = fir_cic3_decim_2_s16_s16(&state->bb_dec_4, work_cs16, work_cs16, sample_count);

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	p = work_cs16;
	for(uint_fast16_t n=sample_count; n>0; n-=1) {
		p->i /= 8;
		p->q /= 8;
		p++;
	}

	/* 192kHz complex<int16>[N/16]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 96kHz complex<int16>[N/32] */
	sample_count = fir_cic3_decim_2_s16_s16(&state->bb_dec_5, work_cs16, work_cs16, sample_count);

	timestamps->decimate_end = baseband_timestamp();

	// TODO: Design a proper channel filter.

	timestamps->channel_filter_end = baseband_timestamp();

	/* 96kHz complex<int16>[N/32]
	 * -> FM demodulation
	 * -> 96kHz int16[N/32] */
	fm_demodulate_s16_s16(&state->fm_demodulate, work_cs16, work_int16, sample_count);

	timestamps->demodulate_end = baseband_timestamp();

	/* 96kHz int16[N/32]
	 * -> FIR filter, <3kHz (0.031fs) pass, >6kHz (0.063fs) stop, gain of 1
	 * -> 48kHz int16[N/64] */
	sample_count = fir_64_decim_2_real_s16_s16(&state->audio_dec, work_int16, work_int16, sample_count);

	copy_to_audio_output(work_int16, sample_count);
}
