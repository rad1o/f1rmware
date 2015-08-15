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

#include "portapack.h"

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/lpc43xx/m4/nvic.h>

#include <hackrf_core.h>
#include <rf_path.h>
#include <sgpio.h>
#include <streaming.h>
#include <max2837.h>
#include <gpdma.h>
#include <sgpio_dma.h>

#include "portapack_driver.h"
//#include "audio.h"
//#include "cpld.h"
//#include "i2s.h"
//#include "m0_startup.h"
//#include "rtc.h"

//#include "lcd.h"
#include "tuning.h"

#include "complex.h"
#include "decimate.h"
#include "demodulate.h"

#include "rx_fm_broadcast.h"
#include "rx_fm_narrowband.h"
//#include "rx_ais.h"
#include "rx_am.h"
//#include "rx_tpms_ask.h"
//#include "rx_tpms_fsk.h"
#include "specan.h"

// #include "ipc.h"
// #include "ipc_m4.h"
// #include "ipc_m0_client.h"

gpdma_lli_t lli_rx[2];

uint32_t baseband_timestamp() {
	return systick_get_value();
}

static uint32_t systick_difference(const uint32_t t1, const uint32_t t2) {
	return (t1 - t2) & 0xffffff;
}

uint32_t sctr=0;
#include <libopencm3/lpc43xx/dac.h>

uint16_t* sram = (uint16_t*)0x20000000;
uint16_t* send = (uint16_t*)0x20004000;
void copy_to_audio_output(const int16_t* const source, const size_t sample_count) {
    sctr+=sample_count;
    /*
    if(sram < send){
        for(size_t i=0; i<sample_count; i++) {
            sram[0]=source[i];
            sram++;
        };
    };
    */
    /* XXX: TODO
	if( sample_count != I2S_BUFFER_SAMPLE_COUNT ) {
		return;
	}

	int16_t* const audio_tx_buffer = portapack_i2s_tx_empty_buffer();
	for(size_t i=0, j=0; i<I2S_BUFFER_SAMPLE_COUNT; i++, j++) {
		audio_tx_buffer[i*2] = audio_tx_buffer[i*2+1] = source[j];
	}
    */
}

/*
static void rx_tpms_ask_packet_handler(const void* const payload, const size_t payload_length, void* const context) {
	(void)context;
	ipc_command_packet_data_received(&device_state->ipc_m0, (uint8_t*)payload, payload_length);
}

static void rx_tpms_ask_init_wrapper(void* const _state) {
	rx_tpms_ask_init(_state, rx_tpms_ask_packet_handler);
}

static void rx_tpms_fsk_packet_handler(const void* const payload, const size_t payload_length, void* const context) {
	(void)context;
	ipc_command_packet_data_received(&device_state->ipc_m0, (uint8_t*)payload, payload_length);
}

static void rx_tpms_fsk_init_wrapper(void* const _state) {
	rx_tpms_fsk_init(_state, rx_tpms_fsk_packet_handler);
}

static void rx_ais_packet_handler(const void* const payload, const size_t payload_length, void* const context) {
	(void)context;
	ipc_command_packet_data_received(&device_state->ipc_m0, (uint8_t*)payload, payload_length);
}

static void rx_ais_init_wrapper(void* const _state) {
	rx_ais_init(_state, rx_ais_packet_handler);
}
*/

static volatile receiver_baseband_handler_t receiver_baseband_handler = NULL;

typedef enum {
	RECEIVER_CONFIGURATION_SPEC = 0,
	RECEIVER_CONFIGURATION_NBAM = 1,
	RECEIVER_CONFIGURATION_NBFM = 2,
	RECEIVER_CONFIGURATION_WBFM = 3,
	RECEIVER_CONFIGURATION_TPMS = 4,
	RECEIVER_CONFIGURATION_TPMS_FSK = 5,
	RECEIVER_CONFIGURATION_AIS = 6,
} receiver_configuration_id_t;

typedef struct receiver_configuration_t {
	receiver_state_init_t init;
	receiver_baseband_handler_t baseband_handler;
	int64_t tuning_offset;
	uint32_t sample_rate;
	uint32_t baseband_bandwidth;
	uint32_t baseband_decimation;
	bool enable_audio;
} receiver_configuration_t;

const receiver_configuration_t receiver_configurations[] = {
	[RECEIVER_CONFIGURATION_SPEC] = {
		.init = specan_init,
		.baseband_handler = specan_baseband_handler,
		.tuning_offset = 0,
		.sample_rate = 20000000,
		.baseband_bandwidth = 10000000,
		.baseband_decimation = 1,
		.enable_audio = false,
	},
	[RECEIVER_CONFIGURATION_NBAM] = {
		.init = rx_am_to_audio_init,
		.baseband_handler = rx_am_to_audio_baseband_handler,
		.tuning_offset = -768000,
		.sample_rate = 12288000,
		.baseband_bandwidth = 1750000,
		.baseband_decimation = 4,
		.enable_audio = true,
	},
	[RECEIVER_CONFIGURATION_NBFM] = {
		.init = rx_fm_narrowband_to_audio_init,
		.baseband_handler = rx_fm_narrowband_to_audio_baseband_handler,
		.tuning_offset = -768000,
		.sample_rate = 12288000,
		.baseband_bandwidth = 1750000,
		.baseband_decimation = 4,
		.enable_audio = true,
	},
	[RECEIVER_CONFIGURATION_WBFM] = {
		.init = rx_fm_broadcast_to_audio_init,
		.baseband_handler = rx_fm_broadcast_to_audio_baseband_handler,
		.tuning_offset = -768000,
		.sample_rate = 12288000,
		.baseband_bandwidth = 1750000,
		.baseband_decimation = 4,
		.enable_audio = true,
	},
/*	[RECEIVER_CONFIGURATION_TPMS] = {
		.init = rx_tpms_ask_init_wrapper,
		.baseband_handler = rx_tpms_ask_baseband_handler,
		.tuning_offset = -768000,
		.sample_rate = 12288000,
		.baseband_bandwidth = 1750000,
		.baseband_decimation = 4,
		.enable_audio = true,
	},
	[RECEIVER_CONFIGURATION_TPMS_FSK] = {
		.init = rx_tpms_fsk_init_wrapper,
		.baseband_handler = rx_tpms_fsk_baseband_handler,
		.tuning_offset = -614400,
		.sample_rate = 9830400,
		.baseband_bandwidth = 1750000,
		.baseband_decimation = 4,
		.enable_audio = true,
	},
	[RECEIVER_CONFIGURATION_AIS] = {
		.init = rx_ais_init_wrapper,
		.baseband_handler = rx_ais_baseband_handler,
		.tuning_offset = -614400,
		.sample_rate = 9830400,
		.baseband_bandwidth = 1750000,
		.baseband_decimation = 4,
		.enable_audio = false,
	}, */
};

const receiver_configuration_t* get_receiver_configuration() {
	return &receiver_configurations[device_state->receiver_configuration_index];
}

static complex_s8_t* get_completed_baseband_buffer() {
	const size_t current_lli_index = sgpio_dma_current_transfer_index(lli_rx, 2);
	const size_t finished_lli_index = 1 - current_lli_index;
	return (complex_s8_t*)lli_rx[finished_lli_index].cdestaddr;
}

complex_s8_t* wait_for_completed_baseband_buffer() {
	const size_t last_lli_index = sgpio_dma_current_transfer_index(lli_rx, 2);
	while( sgpio_dma_current_transfer_index(lli_rx, 2) == last_lli_index );
	return get_completed_baseband_buffer();
}

bool set_frequency(const int64_t new_frequency) {
	const receiver_configuration_t* const receiver_configuration = get_receiver_configuration();

	const int64_t tuned_frequency = new_frequency + receiver_configuration->tuning_offset;
	if( set_freq(tuned_frequency) ) {
		device_state->tuned_hz = new_frequency;
		return true;
	} else {
		return false;
	}
}

void increment_frequency(const int32_t increment) {
	const int64_t new_frequency = device_state->tuned_hz + increment;
	set_frequency(new_frequency);
}

static uint8_t receiver_state_buffer[4096];

void set_rx_mode(const size_t new_receiver_configuration_index) {
	if( new_receiver_configuration_index >= ARRAY_SIZE(receiver_configurations) ) {
		return;
	}
	
	// TODO: Mute audio, clear audio buffers?
//	i2s_mute();

	sgpio_dma_stop();
	sgpio_cpld_stream_disable();

	/* TODO: Ensure receiver_state_buffer is large enough for new mode, or start using
	 * heap to allocate necessary memory.
	 */
	const receiver_configuration_t* const old_receiver_configuration = get_receiver_configuration();
	device_state->receiver_configuration_index = new_receiver_configuration_index;
	const receiver_configuration_t* const receiver_configuration = get_receiver_configuration();

	if( old_receiver_configuration->tuning_offset != receiver_configuration->tuning_offset ) {
		set_frequency(device_state->tuned_hz);
	}

	sample_rate_set(receiver_configuration->sample_rate);
	baseband_filter_bandwidth_set(receiver_configuration->baseband_bandwidth);
	sgpio_cpld_stream_rx_set_decimation(receiver_configuration->baseband_decimation);

	receiver_configuration->init(receiver_state_buffer);
	receiver_baseband_handler = receiver_configuration->baseband_handler;

	sgpio_dma_rx_start(&lli_rx[0]);
	sgpio_cpld_stream_enable();

//	if( receiver_configuration->enable_audio ) {
//		i2s_unmute();
//	}
}

/*
void handle_command_spectrum_data_done(const void* const arg) {
	const ipc_command_spectrum_data_done_t* const command = (ipc_command_spectrum_data_done_t*)arg;
	(void)command;

	if( device_state->receiver_configuration_index == RECEIVER_CONFIGURATION_SPEC ) {
		specan_acknowledge_frame(&receiver_state_buffer);
	}
}
*/

/*
void rtc_isr() {
	rtc_counter_interrupt_clear();
//	ipc_command_rtc_second(&device_state->ipc_m0);
}
*/

#include "portapack_driver.h"

void portapack_init() {
	cpu_clock_pll1_max_speed();
	
// 	portapack_cpld_jtag_io_init();

	device_state->tuned_hz = 2450000000;
	device_state->lna_gain_db = 0;
	device_state->if_gain_db = 32;
	device_state->bb_gain_db = 32;
	device_state->audio_out_gain_db = 0;
	device_state->receiver_configuration_index = RECEIVER_CONFIGURATION_SPEC;

//	ipc_channel_init(&device_state->ipc_m4, ipc_m4_buffer);
//	ipc_channel_init(&device_state->ipc_m0, ipc_m0_buffer);

//	portapack_i2s_init();

	sgpio_set_slice_mode(false);

	ssp1_init();
	rf_path_init();
	rf_path_set_direction(RF_PATH_DIRECTION_RX);

	rf_path_set_lna((device_state->lna_gain_db >= 14) ? 1 : 0);
	max2837_set_lna_gain(device_state->if_gain_db);	/* 8dB increments */
	max2837_set_vga_gain(device_state->bb_gain_db);	/* 2dB increments, up to 62dB */

//	m0_configure_for_spifi();
//	m0_run();

	systick_set_reload(0xfffff); 
	systick_set_clocksource(1);
	systick_counter_enable();

	sgpio_dma_init();

	sgpio_dma_configure_lli(&lli_rx[0], 1, false, sample_buffer_0, 4096);
	sgpio_dma_configure_lli(&lli_rx[1], 1, false, sample_buffer_1, 4096);

	gpdma_lli_create_loop(&lli_rx[0], 2);

	gpdma_lli_enable_interrupt(&lli_rx[0]);
	gpdma_lli_enable_interrupt(&lli_rx[1]);

	nvic_set_priority(NVIC_DMA_IRQ, 0);
	nvic_enable_irq(NVIC_DMA_IRQ);

//	nvic_set_priority(NVIC_M0CORE_IRQ, 255);
//	nvic_enable_irq(NVIC_M0CORE_IRQ);

	set_rx_mode(RECEIVER_CONFIGURATION_SPEC);
//	set_rx_mode(RECEIVER_CONFIGURATION_WBFM);

	set_frequency(device_state->tuned_hz);

/*	rtc_init();
	rtc_counter_interrupt_second_enable();
	nvic_set_priority(NVIC_RTC_IRQ, 255);
	nvic_enable_irq(NVIC_RTC_IRQ);
    */
}

#include <libopencm3/include/libopencm3/lpc43xx/gpio.h>
complex_s8_t * s8ram = (complex_s8_t *)0x20000000;
complex_s8_t * s8end = (complex_s8_t *)0x20004000;
void dma_isr() {
	complex_s8_t* const completed_buffer = get_completed_baseband_buffer();

    if(s8ram < s8end){
        for(size_t i=0; i<4096; i++) {
            s8ram[0]=completed_buffer[i];
            s8ram++;
        };
    };

	/* 12.288MHz
	 * -> CPLD decimation by 4
	 * -> 3.072MHz complex<int8>[2048] == 666.667 usec/block == 136000 cycles/sec
	 */
    gpio_set(GPIO2,GPIOPIN8);
	if( receiver_baseband_handler ) {
		baseband_timestamps_t timestamps;
		timestamps.start = timestamps.decimate_end = timestamps.channel_filter_end = timestamps.demodulate_end = baseband_timestamp();
		receiver_baseband_handler(receiver_state_buffer, completed_buffer, 2048, &timestamps);
		timestamps.audio_end = baseband_timestamp();

		device_state->dsp_metrics.duration_decimate = systick_difference(timestamps.start, timestamps.decimate_end);
		device_state->dsp_metrics.duration_channel_filter = systick_difference(timestamps.decimate_end, timestamps.channel_filter_end);
		device_state->dsp_metrics.duration_demodulate = systick_difference(timestamps.channel_filter_end, timestamps.demodulate_end);
		device_state->dsp_metrics.duration_audio = systick_difference(timestamps.demodulate_end, timestamps.audio_end);
		device_state->dsp_metrics.duration_all = systick_difference(timestamps.start, timestamps.audio_end);

		const receiver_configuration_t* const receiver_configuration = get_receiver_configuration();
		const float decimated_sampling_rate = (float)receiver_configuration->sample_rate / receiver_configuration->baseband_decimation;
		const float cycles_per_baseband_block = (2048.0f / decimated_sampling_rate) * 200000000.0f;
		device_state->dsp_metrics.duration_all_millipercent = (float)device_state->dsp_metrics.duration_all / cycles_per_baseband_block * 100000.0f;
	}
    gpio_clear(GPIO2,GPIOPIN8);

	/* Acknowledge interrupt at end. If acknowledged at the beginning of the
	 * ISR, a long-running baseband handler could make the baseband processing
	 * unresponsive, which could also lock up the UI. Instead, acknowledging
	 * at the end allows us to drop baseband frames if necessary.
	 */
	sgpio_dma_irq_tc_acknowledge();
}

#include "arm_intrinsics.h"

void portapack_run() {
	__WFE();
}
