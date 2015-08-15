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

#include "portapack_driver.h"

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/scu.h>

int8_t* const sample_buffer_0 = (int8_t*)0x20008000;
int8_t* const sample_buffer_1 = (int8_t*)0x2000c000;
device_state_t* const device_state = (device_state_t*)0x20007000;
uint8_t* const ipc_m4_buffer = (uint8_t*)0x20007c00;
uint8_t* const ipc_m0_buffer = (uint8_t*)0x20007800;
//const size_t ipc_m4_buffer_size = 1024;
//const size_t ipc_m0_buffer_size = 1024;
