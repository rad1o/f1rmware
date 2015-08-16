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

#ifndef __DECIMATE_H__
#define __DECIMATE_H__

#include <stdint.h>
#include <stddef.h>

#include "complex.h"

typedef struct translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t {
	uint32_t q1_i0;
	uint32_t q0_i1;
} translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t;

void translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t* const state);
size_t translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t* const state,
	complex_s8_t* const src_and_dst,
	const size_t sample_count
);

typedef struct fir_cic3_decim_2_s8_s16_state_t {
	uint32_t i;
	uint32_t q;
} fir_cic3_decim_2_s8_s16_state_t;

void fir_cic3_decim_2_s8_s16_init(fir_cic3_decim_2_s8_s16_state_t* const state);
size_t fir_cic3_decim_2_s8_s16(
	fir_cic3_decim_2_s8_s16_state_t* const state,
	complex_s8_t* const src_and_dst,
	const size_t sample_count
);

typedef struct fir_cic3_decim_2_s16_s32_state_t {
	uint32_t iq0;
	uint32_t iq1;
} fir_cic3_decim_2_s16_s32_state_t;

void fir_cic3_decim_2_s16_s32_init(fir_cic3_decim_2_s16_s32_state_t* const state);
size_t fir_cic3_decim_2_s16_s32(
	fir_cic3_decim_2_s16_s32_state_t* const state,
	complex_s16_t* const src_and_dst,
	const size_t sample_count
);

typedef struct fir_cic3_decim_2_s16_s16_state_t {
	uint32_t iq0;
	uint32_t iq1;
} fir_cic3_decim_2_s16_s16_state_t;

void fir_cic3_decim_2_s16_s16_init(fir_cic3_decim_2_s16_s16_state_t* const state);
size_t fir_cic3_decim_2_s16_s16(
	fir_cic3_decim_2_s16_s16_state_t* const state,
	complex_s16_t* const src,
	complex_s16_t* const dst,
	const size_t sample_count
);

typedef struct fir_cic4_decim_2_real_s16_s16_state_t {
	int16_t z[7];
} fir_cic4_decim_2_real_s16_s16_state_t;

void fir_cic4_decim_2_real_s16_s16_init(fir_cic4_decim_2_real_s16_s16_state_t* const state);
size_t fir_cic4_decim_2_real_s16_s16(
	fir_cic4_decim_2_real_s16_s16_state_t* const state,
	int16_t* src,
	int16_t* dst,
	const size_t sample_count
);

typedef struct fir_64_decim_2_real_s16_s16_state_t {
	const int16_t* taps;
	size_t taps_count;
	int16_t z[66];
} fir_64_decim_2_real_s16_s16_state_t;

void fir_64_decim_2_real_s16_s16_init(
	fir_64_decim_2_real_s16_s16_state_t* const state,
	const int16_t* const taps,
	const size_t taps_count
);

size_t fir_64_decim_2_real_s16_s16(
	fir_64_decim_2_real_s16_s16_state_t* const state,
	int16_t* src,
	int16_t* dst,
	const size_t sample_count
);

typedef struct fir_64_decim_8_cplx_s16_s16_state_t {
	const int16_t* taps;
	size_t taps_count;
	complex_s16_t z[72];
} fir_64_decim_8_cplx_s16_s16_state_t;

void fir_64_decim_8_cplx_s16_s16_init(
	fir_64_decim_8_cplx_s16_s16_state_t* const state,
	const int16_t* const taps,
	const size_t taps_count
);

size_t fir_64_decim_8_cplx_s16_s16(
	fir_64_decim_8_cplx_s16_s16_state_t* const state,
	complex_s16_t* src,
	complex_s16_t* dst,
	const size_t sample_count
);

#endif/*__DECIMATE_H__*/
