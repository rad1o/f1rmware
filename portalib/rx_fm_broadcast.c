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

#include "rx_fm_broadcast.h"

#include "portapack.h"

#include "decimate.h"
#include "demodulate.h"
#include "filters.h"

typedef struct rx_fm_broadcast_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t dec_stage_1_state;
	fir_cic3_decim_2_s16_s16_state_t dec_stage_2_state;
	fm_demodulate_s16_s16_state_t fm_demodulate_state;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_1;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_2;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_3;
	fir_64_decim_2_real_s16_s16_state_t audio_dec_4;
} rx_fm_broadcast_to_audio_state_t;

void rx_fm_broadcast_to_audio_init(void* const _state) {
	rx_fm_broadcast_to_audio_state_t* const state = (rx_fm_broadcast_to_audio_state_t*)_state;

	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->dec_stage_1_state);
	fir_cic3_decim_2_s16_s16_init(&state->dec_stage_2_state);
	fm_demodulate_s16_s16_init(&state->fm_demodulate_state, 768000, 75000);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_1);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_2);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_3);
	fir_64_decim_2_real_s16_s16_init(&state->audio_dec_4, taps_64_lp_156_198, 64);
}

void rx_fm_broadcast_to_audio_baseband_handler(void* const _state, complex_s8_t* const in, const size_t sample_count_in, baseband_timestamps_t* const timestamps) {
	rx_fm_broadcast_to_audio_state_t* const state = (rx_fm_broadcast_to_audio_state_t*)_state;

	size_t sample_count = sample_count_in;

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	sample_count = translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->dec_stage_1_state, in, sample_count);
	complex_s16_t* const in_cs16 = (complex_s16_t*)in;

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	complex_s16_t work[512];
	complex_s16_t* const work_cs16 = work;
	int16_t* const work_int16 = (int16_t*)work;
	sample_count = fir_cic3_decim_2_s16_s16(&state->dec_stage_2_state, in_cs16, work_cs16, sample_count);

	timestamps->decimate_end = baseband_timestamp();

	/* 768kHz complex<int32>[N/4]
	 * -> FIR LPF, 90kHz cut-off, max attenuation by 192kHz.
	 * -> 768kHz complex<int32>[N/4] */
	/* TODO: To improve adjacent channel rejection, implement complex channel filter:
	 *		pass < +/- 100kHz, stop > +/- 200kHz
	 */

	timestamps->channel_filter_end = baseband_timestamp();

	/* 768kHz complex<int16>[N/4]
	 * -> FM demodulation
	 * -> 768kHz int16[N/4] */
	fm_demodulate_s16_s16(&state->fm_demodulate_state, work_cs16, work_int16, sample_count);

	timestamps->demodulate_end = baseband_timestamp();

	/* 768kHz int16[N/4]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 384kHz int16[N/8] */
	sample_count = fir_cic4_decim_2_real_s16_s16(&state->audio_dec_1, work_int16, work_int16, sample_count);

	/* 384kHz int16[N/8]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 192kHz int16[N/16] */
	sample_count = fir_cic4_decim_2_real_s16_s16(&state->audio_dec_2, work_int16, work_int16, sample_count);

	/* 192kHz int16[N/16]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 96kHz int16[N/32] */
	sample_count = fir_cic4_decim_2_real_s16_s16(&state->audio_dec_3, work_int16, work_int16, sample_count);

	/* 96kHz int16[N/32]
	 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop, gain of 1
	 * -> 48kHz int16[N/64] */
	sample_count = fir_64_decim_2_real_s16_s16(&state->audio_dec_4, work_int16, work_int16, sample_count);

	copy_to_audio_output(work_int16, sample_count);
}
