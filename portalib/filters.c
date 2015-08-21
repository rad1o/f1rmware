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

#include "filters.h"

/* Wideband audio filter */
/* 96kHz int16_t input
 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop
 * -> 48kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 */
const int16_t taps_64_lp_156_198[64] = {
    -27,    166,    104,    -36,   -174,   -129,    109,    287,
    148,   -232,   -430,   -130,    427,    597,     49,   -716,
   -778,    137,   1131,    957,   -493,  -1740,  -1121,   1167,
   2733,   1252,  -2633,  -4899,  -1336,   8210,  18660,  23254,
  18660,   8210,  -1336,  -4899,  -2633,   1252,   2733,   1167,
  -1121,  -1740,   -493,    957,   1131,    137,   -778,   -716,
     49,    597,    427,   -130,   -430,   -232,    148,    287,
    109,   -129,   -174,    -36,    104,    166,    -27,      0
};

/* Narrowband audio filter */
/* 96kHz int16_t input
 * -> FIR filter, <3kHz (0.031fs) pass, >6kHz (0.063fs) stop
 * -> 48kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 */
/* TODO: Review this filter, it's very quick and dirty. */
const int16_t taps_64_lp_031_063[64] = {
	  -254,    255,    244,    269,    302,    325,    325,    290,
	   215,     99,    -56,   -241,   -442,   -643,   -820,   -950,
	 -1009,   -974,   -828,   -558,   -160,    361,    992,   1707,
	  2477,   3264,   4027,   4723,   5312,   5761,   6042,   6203,
	  6042,   5761,   5312,   4723,   4027,   3264,   2477,   1707,
	   992,    361,   -160,   -558,   -828,   -974,  -1009,   -950,
	  -820,   -643,   -442,   -241,    -56,     99,    215,    290,
	   325,    325,    302,    269,    244,    255,   -254,      0,
};
