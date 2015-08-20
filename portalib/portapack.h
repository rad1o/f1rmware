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

#ifndef __PORTAPACK_H__
#define __PORTAPACK_H__

#include <stdint.h>
#include <stdbool.h>

#include <gpdma.h>

#include "complex.h"

//#define CPLD_PROGRAM 1
//#define LCD_BACKLIGHT_TEST

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define M_PI (3.14159265358979323846264338327950f)

typedef struct baseband_timestamps_t {
	uint32_t start;
	uint32_t decimate_end;
	uint32_t channel_filter_end;
	uint32_t demodulate_end;
	uint32_t audio_end;
} baseband_timestamps_t;

typedef void (*receiver_state_init_t)(void* const state);
typedef void (*receiver_baseband_handler_t)(void* const state, complex_s8_t* const data, const size_t sample_count, baseband_timestamps_t* const timestamps);

typedef struct dsp_metrics_t {
	uint32_t duration_decimate;
	uint32_t duration_channel_filter;
	uint32_t duration_demodulate;
	uint32_t duration_audio;
	uint32_t duration_all;
	uint32_t duration_all_millipercent;
} dsp_metrics_t;

typedef struct device_state_t {
	int64_t tuned_hz;
	int32_t lna_gain_db;
	int32_t if_gain_db;
	int32_t bb_gain_db;
	int32_t audio_out_gain_db;
	size_t receiver_configuration_index;
	
	int32_t encoder_position;

//	ipc_channel_t ipc_m4;
//	ipc_channel_t ipc_m0;

	dsp_metrics_t dsp_metrics;
} device_state_t;

typedef enum {
	RECEIVER_CONFIGURATION_SPEC = 0,
	RECEIVER_CONFIGURATION_NBAM = 1,
	RECEIVER_CONFIGURATION_NBFM = 2,
	RECEIVER_CONFIGURATION_WBFM = 3,
	RECEIVER_CONFIGURATION_TPMS = 4,
	RECEIVER_CONFIGURATION_TPMS_FSK = 5,
	RECEIVER_CONFIGURATION_AIS = 6,
} receiver_configuration_id_t;


void portapack_init();
void portapack_run();

bool set_frequency(const int64_t new_frequency);
void set_rx_mode(const size_t new_receiver_configuration_index);

void copy_to_audio_output(const int16_t* const source, const size_t sample_count);

complex_s8_t* wait_for_completed_baseband_buffer();

uint32_t baseband_timestamp();

#endif/*__PORTAPACK_H__*/
