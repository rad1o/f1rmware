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

#ifndef __PORTAPACK_DRIVER_H__
#define __PORTAPACK_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

// #include <delay.h>

#include "portapack.h"

void portapack_lcd_reset(const bool active);
void portapack_lcd_backlight(const bool on);
void portapack_lcd_write_data_fast(const uint32_t value);
void portapack_lcd_write(const uint32_t rs, const uint32_t value);
void portapack_lcd_frame_sync();

void portapack_noise_test(const unsigned int n);

void portapack_lcd_touch_sense_hi_z();
void portapack_lcd_touch_sense_off();
void portapack_lcd_touch_sense_pressure();
void portapack_lcd_touch_sense_x();
void portapack_lcd_touch_sense_y();

#define SWITCH_INC		(1 << 5)
#define SWITCH_DEC		(1 << 6)

#define SWITCH_UP		(1 << 3)
#define SWITCH_DOWN		(1 << 2)
#define SWITCH_LEFT		(1 << 1)
#define SWITCH_RIGHT	(1 << 0)
#define SWITCH_SELECT	(1 << 4)

uint32_t portapack_read_switches();

void portapack_encoder_init();
int encoder_update();
int32_t portapack_encoder_delta();

void portapack_driver_init();

void portapack_audio_codec_write(const uint_fast8_t address, const uint_fast16_t data);

extern int8_t* const sample_buffer_0;
extern int8_t* const sample_buffer_1;
extern device_state_t* const device_state;
extern uint8_t* const ipc_m4_buffer;
extern uint8_t* const ipc_m0_buffer;
extern const size_t ipc_m4_buffer_size;
extern const size_t ipc_m0_buffer_size;

#endif/*__PORTAPACK_DRIVER_H__*/
