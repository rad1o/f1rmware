/*
 * light weight WS2812 lib - ARM Cortex M0/M0+ version
 *
 * Created: 07.07.2013
 *  Author: Tim (cpldcpu@gmail.com)
 */

#include "light_ws2812_cortex.h"
/*
 * Based on:
 * https://www.adafruit.com/datasheets/WS2812B.pdf
 */

#define ws2812_DEL1 "	nop		\n\t"
#define ws2812_DEL2 "   nop \n\t nop \n\t"
#define ws2812_DEL4 ws2812_DEL2 ws2812_DEL2
#define ws2812_DEL8 ws2812_DEL4 ws2812_DEL4
#define ws2812_DEL16 ws2812_DEL8 ws2812_DEL8
#define ws2812_DEL32 ws2812_DEL16 ws2812_DEL16

#define us400	(((ws2812_cpuclk/1000)*400)/1000000)-1
#define us850	(((ws2812_cpuclk/1000)*850)/1000000)-7

#define us800	(((ws2812_cpuclk/1000)*800)/1000000)-2
#define us450	(((ws2812_cpuclk/1000)*450)/1000000)-4

void ws2812_sendarray(uint8_t *data,int datlen)
{
	uint32_t maskhi = ws2812_mask_set;
	uint32_t masklo = ws2812_mask_clr;
	volatile uint32_t *set = ws2812_port_set;
	volatile uint32_t *clr = ws2812_port_clr;
	uint32_t i = 0;
	uint32_t curbyte;
	
/* Workaround to match CPU speed to the ws2812 "baudrate" */	
	uint32_t old_cpu_speed = _cpu_speed;
	cpu_clock_set(51);

    __asm__ volatile(
        "CPSID I \n\t"
		ws2812_DEL32
    );

	while (datlen--) {
		curbyte=*data++/1;

	__asm__ volatile(
			"		lsl %[dat],#24				\n\t"
			"		movs %[ctr],#8				\n\t"
			"ilop%=:							\n\t"
			"		movs   %[dat], %[dat], lsl #1 \n\t"
			"		str %[maskhi], [%[set]]		\n\t"
			"		bcs one%=					\n\t"
#if (us400&1)
			ws2812_DEL1
#endif
#if (us400&2)
			ws2812_DEL2
#endif
#if (us400&4)
			ws2812_DEL4
#endif
#if (us400&8)
			ws2812_DEL8
#endif
#if (us400&16)
			ws2812_DEL16
#endif
#if (us400&32)
			ws2812_DEL32
#endif
			"		str %[masklo], [%[clr]]		\n\t"
#if (us850&1)
			ws2812_DEL1
#endif
#if (us850&2)
			ws2812_DEL2
#endif
#if (us850&4)
			ws2812_DEL4
#endif
#if (us850&8)
			ws2812_DEL8
#endif
#if (us850&16)
			ws2812_DEL16
#endif
#if (us850&32)
			ws2812_DEL32
#endif
			"		b done%=					\n\t"
			"one%=:								\n\t"
#if (us800&1)
			ws2812_DEL1
#endif
#if (us800&2)
			ws2812_DEL2
#endif
#if (us800&4)
			ws2812_DEL4
#endif
#if (us800&8)
			ws2812_DEL8
#endif
#if (us800&16)
			ws2812_DEL16
#endif
#if (us800&32)
			ws2812_DEL32
#endif
			"		str %[masklo], [%[clr]]		\n\t"
#if (us450&1)
			ws2812_DEL1
#endif
#if (us450&2)
			ws2812_DEL2
#endif
#if (us450&4)
			ws2812_DEL4
#endif
#if (us450&8)
			ws2812_DEL8
#endif
#if (us450&16)
			ws2812_DEL16
#endif
#if (us450&32)
			ws2812_DEL32
#endif
            " done%=:"
			"		subs %[ctr], #1				\n\t"
			"		bne 	ilop%=					\n\t"
			"end%=:								\n\t"
			:	[ctr] "+r" (i)
			:	[dat] "r" (curbyte), [set] "r" (set), [clr] "r" (clr), [masklo] "r" (masklo), [maskhi] "r" (maskhi)
			);
	}

    __asm__ volatile(
	    ws2812_DEL32
        "CPSIE I \n\t"
    );

	/* Reset CPU speed to previous */
	cpu_clock_set(old_cpu_speed);
}

