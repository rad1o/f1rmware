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

#ifndef __ARM_INTRINSICS_H__
#define __ARM_INTRINSICS_H__

#include <stdint.h>

/* Definitions for select ARM Cortex-M4F instructions. inspired by
 * ARM's CMSIS library.
 */

__attribute__((always_inline)) static inline uint32_t __QADD16(uint32_t RN, uint32_t RM) {
	uint32_t RD;
	__asm volatile("qadd16 %0, %1, %2"
		: "=r"(RD)
		: "r"(RN),
		  "r"(RM)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __QSUB16(uint32_t RN, uint32_t RM) {
	uint32_t RD;
	__asm volatile("qsub16 %0, %1, %2"
		: "=r"(RD)
		: "r"(RN),
		  "r"(RM)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMLATB(uint32_t RM, uint32_t RS, uint32_t RN) {
	uint32_t RD;
	__asm volatile("smlatb %0, %1, %2, %3"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS),
		  "r"(RN)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMLABB(uint32_t RM, uint32_t RS, uint32_t RN) {
	uint32_t RD;
	__asm volatile("smlabb %0, %1, %2, %3"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS),
		  "r"(RN)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMUAD(uint32_t RM, uint32_t RS) {
	uint32_t RD;
	__asm volatile("smuad %0, %1, %2"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMUADX(uint32_t RM, uint32_t RS) {
	uint32_t RD;
	__asm volatile("smuadx %0, %1, %2"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMLAD(uint32_t RM, uint32_t RS, uint32_t RN) {
	uint32_t RD;
	__asm volatile("smlad %0, %1, %2, %3"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS),
		  "r"(RN)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMLADX(uint32_t RM, uint32_t RS, uint32_t RN) {
	uint32_t RD;
	__asm volatile("smladx %0, %1, %2, %3"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS),
		  "r"(RN)
	);
	return RD;
}

#define __SMLALD(RD,RM,RS) \
({ \
	uint32_t __RM = (RM), __RS = (RS), __RD_H = (uint32_t)((uint64_t)(RD) >> 32), __RD_L = (uint32_t)((uint64_t)(RD) & 0xFFFFFFFFUL); \
	__asm volatile ("smlald %0, %1, %2, %3" : "=r" (__RD_L), "=r" (__RD_H) : "r" (__RM), "r" (__RS), "0" (__RD_L), "1" (__RD_H) ); \
	(uint64_t)(((uint64_t)__RD_H << 32) | __RD_L); \
})

__attribute__((always_inline)) static inline uint32_t __SMUSD(uint32_t RM, uint32_t RS) {
	uint32_t RD;
	__asm volatile("smusd %0, %1, %2"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SMUSDX(uint32_t RM, uint32_t RS) {
	uint32_t RD;
	__asm volatile("smusdx %0, %1, %2"
		: "=r"(RD)
		: "r"(RM),
		  "r"(RS)
	);
	return RD;
}

/* NOTE: BFI is kinda weird because it modifies RD, copy __SMLALD style? */
__attribute__((always_inline)) static inline uint32_t __BFI(uint32_t RD, uint32_t RN, uint32_t LSB, uint32_t WIDTH) {
	__asm volatile("bfi %0, %1, %2, %3"
		: "+r"(RD)
		: "r"(RN),
		  "I"(LSB),
		  "I"(WIDTH)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __PKHBT(uint32_t RN, uint32_t RM, uint32_t LSL) {
	uint32_t RD;
	__asm volatile("pkhbt %0, %1, %2, lsl %3"
		: "=r"(RD)
		: "r"(RN),
		  "r"(RM),
		  "I"(LSL)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __PKHTB(uint32_t RN, uint32_t RM, uint32_t ASR) {
	uint32_t RD;
	__asm volatile("pkhtb %0, %1, %2, asr %3"
		: "=r"(RD)
		: "r"(RN),
		  "r"(RM),
		  "I"(ASR)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SXTH(uint32_t RM, uint32_t ROR) {
	uint32_t RD;
	__asm volatile("sxth %0, %1, ror %2"
		: "=r"(RD)
		: "r"(RM),
		  "I"(ROR)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SXTB16(uint32_t RM, uint32_t ROR) {
	uint32_t RD;
	__asm volatile("sxtb16 %0, %1, ror %2"
		: "=r"(RD)
		: "r"(RM),
		  "I"(ROR)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __SXTAH(uint32_t RN, uint32_t RM, uint32_t ROR) {
	uint32_t RD;
	__asm volatile("sxtah %0, %1, %2, ror %3"
		: "=r"(RD)
		: "r"(RN),
		  "r"(RM),
		  "I"(ROR)
	);
	return RD;
}

__attribute__((always_inline)) static inline uint32_t __RBIT(uint32_t RM) {
	uint32_t RD;
	__asm volatile("rbit %0, %1"
		: "=r"(RD)
		: "r"(RM)
	);
	return RD;
}

__attribute__((always_inline)) static inline void __set_MSP(uint32_t RN) {
	__asm volatile("msr msp, %0"
		: 
		: "r"(RN)
	);
}

__attribute__((always_inline)) static inline void __ISB() {
	__asm volatile("isb" : : : "memory");
}

__attribute__((always_inline)) static inline void __DSB() {
	__asm volatile("dsb" : : : "memory");
}

__attribute__((always_inline)) static inline void __DMB() {
	__asm volatile("dmb" : : : "memory");
}

__attribute__((always_inline)) static inline void __SEV() {
	__asm volatile("sev" : : : "memory");
}

__attribute__((always_inline)) static inline void __WFE() {
	__asm volatile("wfe" : : : "memory");
}

__attribute__((always_inline)) static inline void __WFI() {
	__asm volatile("wfi" : : : "memory");
}

#endif/*__ARM_INTRINSICS_H__*/
