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

#include "specan.h"

#include "arm_intrinsics.h"

#include "complex.h"
//#include "window.h"
#include "fft.h"

#include <math.h>

#include "portapack_driver.h"
//  #include "ipc_m0_client.h"

#define max(a,b) \
       ({ __typeof__ (a) _a = (a); \
               __typeof__ (b) _b = (b); \
             _a > _b ? _a : _b; })
#define min(a,b) \
       ({ __typeof__ (a) _a = (a); \
               __typeof__ (b) _b = (b); \
             _a < _b ? _a : _b; })

typedef struct specan_state_t {
	float avg[256];
	float peak[256];
	uint8_t avg_log[256];
	uint8_t peak_log[256];
	size_t sample_frames;
	size_t frame_count;
	float mag_2_scale;
	float spectrum_floor;
	float spectrum_gain;
} specan_state_t;

static const float log_k = 0.00000000001f; // to prevent log10f(0), which is bad...

static volatile specan_callback_t  specan_callback = 0;

void specan_register_callback(specan_callback_t callback)
{
	specan_callback = callback;
}


void specan_init(void* const _state) {
	specan_state_t* const state = (specan_state_t*)_state;

	for(size_t i=0; i<ARRAY_SIZE(state->avg); i++) {
		state->avg[i] = log_k;
		state->peak[i] = log_k;
		state->avg_log[i] = 0;
		state->peak_log[i] = 0;
	}
	state->sample_frames = 16;
	state->frame_count = 0;
	//const float mag_scale = 0.7071067811865476f / (256.0f * state->sample_frames);
	const float mag_scale = 1.4142f / (256.0f * state->sample_frames);
	state->mag_2_scale = (mag_scale * mag_scale); // * (1.0f/50.0f/0.001f);
	//state->spectrum_floor = -4.5f;
	state->spectrum_floor = -10.0f;
	state->spectrum_gain = 8.0f;
	
	specan_callback = 0; // reset callback
}

void specan_acknowledge_frame(void* const _state) {
	specan_state_t* const state = (specan_state_t*)_state;
	state->frame_count = 0;
}

static void specan_calculate_averages(specan_state_t* const state) {
	for(size_t i=0; i<256; i++) {
		const float avg = state->avg[i];
		state->avg[i] = log_k;
		const float avg_log = 10.0f * log10f(avg * state->mag_2_scale);
		const int avg_log_n = (int)roundf((avg_log - state->spectrum_floor) * state->spectrum_gain);
		const uint8_t avg_n_log_sat = max(min(avg_log_n, 255), 0);
		state->avg_log[i] = avg_n_log_sat;
	}
}

static void specan_calculate_peaks(specan_state_t* const state) {
	for(size_t i=0; i<256; i++) {
		const float peak = state->peak[i];
		state->peak[i] = log_k;
		const float peak_log = log10f(peak * state->mag_2_scale);
		const int peak_log_n = (int)roundf((peak_log - state->spectrum_floor) * state->spectrum_gain);
		const uint8_t peak_n_log_sat = max(min(peak_log_n, 255), 0);
		state->peak_log[i] = peak_n_log_sat;
	}
}

void specan_baseband_handler(void* const _state, complex_s8_t* const in, const size_t sample_count_in, baseband_timestamps_t* const timestamps) {
	specan_state_t* const state = (specan_state_t*)_state;
	(void)sample_count_in;
	(void)timestamps;
#if 1

	// avg_log should be:
	// 		-9 (bin_mag=0)
	//		-2.107210f (bin_mag=0.0078125, minimum non-zero signal)
	//		 0.150515f (bin_mag=1.414..., peak I, peak Q).

	if( state->frame_count == (state->sample_frames + 0) ) {
		specan_calculate_averages(state);
		state->frame_count += 1;
		return;
	}

	if( state->frame_count == (state->sample_frames + 1) ) {
		specan_calculate_peaks(state);
		state->frame_count += 1;
//		ipc_command_spectrum_data(&device_state->ipc_m0, state->avg_log, state->peak_log, 256);
		if(specan_callback)
			specan_callback(state->avg_log, 256);
		state->frame_count = 0; // hack; next if below is otherwise not reached due to missing UI thread
		return;
	}

	if( state->frame_count > (state->sample_frames + 1) ) {
		/* Waiting for UI thread to claim the data and reset frame_count */
		return;
	}

	complex_t spectrum[256];
	for(uint32_t i=0; i<256; i++) {
		const uint32_t i_rev = __RBIT(i) >> 24;

		const int32_t real = in[i].i;
		const float real_f = (float)real;
		//spectrum[i_rev].r = real_f * window[i];
		spectrum[i_rev].r = real_f; // "rectangular" window (no window ;-)
		
		const int32_t imag = in[i].q;
		const float imag_f = (float)imag;
		//spectrum[i_rev].i = imag_f * window[i];
		spectrum[i_rev].i = imag_f; // "rectangular" window (no window ;-)
	}
	
	fft_c_preswapped((float*)spectrum, 256);

	for(size_t i=0; i<256; i++) {
		const float real = spectrum[i].r;
		const float imag = spectrum[i].i;
		const float mag = real * real + imag * imag;
		state->avg[i] += mag;
		if( mag > state->peak[i] ) {
			state->peak[i] = mag;
		}
	}

	state->frame_count += 1;
#endif
}

