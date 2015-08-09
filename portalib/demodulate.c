/*
 * Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
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

#include "demodulate.h"

#include "portapack.h"

#include <stdint.h>
#include <stddef.h>
#include <math.h>

#include "complex.h"
#include "fxpt_atan2.h"
 
void am_demodulate_s16_s16(complex_s16_t* src, uint16_t* dst, int32_t n) {
	/* Maximum output: 46340 (when input is -32768,-32768) */
	for(; n>0; n-=1) {
		const complex_s16_t s = *(src++);
		const uint32_t t = s.i * s.i + s.q * s.q;
		*(dst++) = sqrtf((float)t);
	}
}
 
void am_demodulate_s16_f32(complex_s16_t* src, float* dst, int32_t n) {
	/* Maximum output: 46340.951 (when input is -32768,-32768) */
	for(; n>0; n-=1) {
		const complex_s16_t s = *(src++);
		const uint32_t t = s.i * s.i + s.q * s.q;
		*(dst++) = sqrtf((float)t);
	}
}

void fm_demodulate_s32_s32_init(fm_demodulate_s32_s32_state_t* const state, const float sampling_rate, const float deviation_hz) {
	state->z1.i = state->z1.q = 0;
	state->k = sampling_rate / (2.0f * M_PI * deviation_hz);
}

void fm_demodulate_s32_s32(
	fm_demodulate_s32_s32_state_t* const state,
	const complex_s32_t* const src,
	int32_t* dst,
	int32_t n
) {
	complex_s32_t z1 = state->z1;
	// TODO: Gain compensation based on ratio of sampling rate and deviation?
	//const int32_t decimation_rate = 1;
	//const float k = state->k * 4096.0f / decimation_rate;
	const complex_s32_t* p = src;
	for(; n>0; n-=1) {
		const complex_s32_t s = *(p++);
		const complex_s32_t t = multiply_conjugate_s32_s32(s, z1);
		z1 = s;
		*(dst++) = fxpt_atan2(t.q >> 12, t.i >> 12) >> 1;
	}
	state->z1 = z1;
}

void fm_demodulate_s16_s16_init(fm_demodulate_s16_s16_state_t* const state, const float sampling_rate, const float deviation_hz) {
	state->z1.i = state->z1.q = 0;
	state->k = sampling_rate / (2.0f * M_PI * deviation_hz);
}

void fm_demodulate_s16_s16(
	fm_demodulate_s16_s16_state_t* const state,
	const complex_s16_t* const src,
	int16_t* dst,
	int32_t n
) {
	complex_s16_t z1 = state->z1;
	// TODO: Gain compensation based on ratio of sampling rate and deviation?
	//const int32_t decimation_rate = 1;
	//const float k = state->k * 4096.0f / decimation_rate;
	const complex_s16_t* p = src;
	for(; n>0; n-=1) {
		const complex_s16_t s = *(p++);
		const complex_s32_t t = multiply_conjugate_s16_s32(s, z1);
		z1 = s;
		*(dst++) = fxpt_atan2(t.q >> 12, t.i >> 12) >> 1;
	}
	state->z1 = z1;
}
