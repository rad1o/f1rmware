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

#ifndef __COMPLEX_H__
#define __COMPLEX_H__

#include <stdint.h>
 
typedef struct {
	float r;
	float i;
} complex_t;

typedef struct complex_s32_t {
	int32_t i;
	int32_t q;
} complex_s32_t;

typedef struct complex_s16_t {
	int16_t i;
	int16_t q;
} complex_s16_t;

typedef struct complex_s8_t {
	int8_t i;
	int8_t q;
} complex_s8_t;

static inline complex_s32_t multiply_conjugate_s32_s32(const complex_s32_t a, const complex_s32_t b) {
	/* (a + bj) * (c + dj) = (ac - bd) + (bc + ad)j */
	/* a = i, b = q
	 * c = iz1, d = qz1
	 */
	const complex_s32_t result = { a.i * b.i + a.q * b.q, a.q * b.i - a.i * b.q };
	return result;
}

static inline complex_s32_t multiply_conjugate_s16_s32(const complex_s16_t a, const complex_s16_t b) {
	/* (a + bj) * (c + dj) = (ac - bd) + (bc + ad)j */
	/* a = i, b = q
	 * c = iz1, d = qz1
	 */
	const complex_s32_t result = { a.i * b.i + a.q * b.q, a.q * b.i - a.i * b.q };
	return result;
}

#endif/*__COMPLEX_H__*/
