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

#ifndef __DEMODULATE_H__
#define __DEMODULATE_H__

#include <stdint.h>
#include <stddef.h>

#include "complex.h"

void am_demodulate_s16_s16(
	complex_s16_t* src,
	uint16_t* dst,
	int32_t n
);

void am_demodulate_s16_f32(
	complex_s16_t* src,
	float* dst,
	int32_t n
);

typedef struct fm_demodulate_s32_s32_state_t {
	complex_s32_t z1;
	float k;
} fm_demodulate_s32_s32_state_t;

void fm_demodulate_s32_s32_init(fm_demodulate_s32_s32_state_t* const state, const float sampling_rate, const float deviation_hz);
void fm_demodulate_s32_s32(
	fm_demodulate_s32_s32_state_t* const state,
	const complex_s32_t* const src,
	int32_t* dst,
	int32_t n
);

typedef struct fm_demodulate_s16_s16_state_t {
	complex_s16_t z1;
	float k;
} fm_demodulate_s16_s16_state_t;

void fm_demodulate_s16_s16_init(fm_demodulate_s16_s16_state_t* const state, const float sampling_rate, const float deviation_hz);
void fm_demodulate_s16_s16(
	fm_demodulate_s16_s16_state_t* const state,
	const complex_s16_t* const src,
	int16_t* dst,
	int32_t n
);

#endif/*__DEMODULATE_H__*/
