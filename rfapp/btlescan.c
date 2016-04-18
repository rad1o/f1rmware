/* BT LE advertisement scanner
 *
 * Copyright (C) 2016 Hans-Werner Hilse <hwhilse@gmail.com>
 *
 * some parts (receive/filters) are
 *   Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
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

#include <rad1olib/setup.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <rad1olib/pins.h>

#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/tuning.h>
#include <common/max2837.h>
#include <common/si5351c.h>
#include <common/streaming.h>
#include <libopencm3/lpc43xx/i2c.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/dac.h>
#include <libopencm3/lpc43xx/adc.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/cm3/vector.h>

#include <portalib/arm_intrinsics.h>
#include <portalib/complex.h>
#include <portalib/fxpt_atan2.h>
#include <stddef.h>

#include "cossin1024.h"

/***********************************************************************/

/* atan2 CORDIC approximation
 *
 * special feature: no divison!
 * thus it is fit especially well for the M0 core
 *
 * (c) 2009, Jasper Vijn
 * http://www.coranac.com/documents/arctangent/
 *
 * adapted to generate 1 bit less precision in order to be able to
 * work at a sample rate of 1500000
 */

#define BRAD_PI_SHIFT 15
#define BRAD_PI (1<<BRAD_PI_SHIFT)

// Get the octant a coordinate pair is in.
#define OCTANTIFY(_x, _y, _o)   do {                            \
    int _t; _o= 0;                                              \
    if(_y<  0)  {            _x= -_x;   _y= -_y; _o += 4; }     \
    if(_x<= 0)  { _t= _x;    _x=  _y;   _y= -_t; _o += 2; }     \
    if(_x<=_y)  { _t= _y-_x; _x= _x+_y; _y=  _t; _o += 1; }     \
} while(0);
static const uint16_t atan2Cordic_list[] = { 
    0x4000, 0x25C8, 0x13F6, 0x0A22, 0x0516, 0x028C, 0x0146, 0x00A3, 
    0x0051, 0x0029, 0x0014, 0x000A, 0x0005, 0x0003, 0x0001, 0x0001
};
// atan via CORDIC (coordinate rotations).
// Returns [0,2pi), where pi ~ 0x8000.
static inline uint16_t atan2Cordic(int x, int y)
{
    if(y==0)    return (x>=0 ? 0 : BRAD_PI);

    int phi;

    OCTANTIFY(x, y, phi);
    phi *= BRAD_PI/4;

    // Scale up a bit for greater accuracy.
    if(1 /* x < 0x10000 */)
    { 
        x *= 0x1000;
        y *= 0x1000;
    }

    int i, tmp, dphi=0;
    for(i=1; i<7; i++)
    {
        if(y>=0)
        {
            tmp= x + (y>>i);
            y  = y - (x>>i);
            x  = tmp;
            dphi += atan2Cordic_list[i];
        }
        else
        {
            tmp= x - (y>>i);
            y  = y + (x>>i);
            x  = tmp;
            dphi -= atan2Cordic_list[i];
        }
    }
    return phi + (dphi>>1);
}
/***********************************************************************/

/* data whitening: we use a lookup table for
 * the whitening XOR values (byte-wise)
 */
#define WHITEN_TABLE_SIZE 0x3F+5
static uint8_t whiten_table[WHITEN_TABLE_SIZE];
static void whiten_prepare(int channel) {
    uint8_t lfsr = 0x40 | channel;
    uint8_t *b = whiten_table;
    for(int i=WHITEN_TABLE_SIZE; i; i--) {
        for(int j=8; j; j--) {
            *b >>= 1;
            if(lfsr & 1) {
                *b |= 0x80;
                lfsr ^= 0x44 << 1;
            }
            lfsr >>= 1;
        }
        b++;
    }
}

/* BTLE CRC24 (initialize this with *lfsr=0x555555) */
static inline void crc24(uint32_t *lfsr, uint8_t d) {
    for(int i=8; i; i--) {
        const uint32_t t = *lfsr >> 23;
        *lfsr = *lfsr << 1;
        if((t&1) != (d&1)) *lfsr ^= 0x00065B;
        d>>=1;
    }
}

/* packet CRC check & parsing */
static void parse(uint8_t *data, int pdu_len) {
    /* calculate CRC over received data */
    uint32_t crc = 0x555555;
    for(int i = 0; i < pdu_len+2; i++)
        crc24(&crc, data[i]);

    /* compare with CRC from packet data
     *
     * contrary to the rest of the packet's data, the CRC is stored
     * MSB first. We reverse it first.
     */
    uint32_t crc_shift = data[2+pdu_len] | data[2+1+pdu_len] << 8 | data[2+2+pdu_len] << 16;
    uint32_t crc_pkg = 0;
    for(int i=24; i; i--, crc_shift>>=1) crc_pkg = (crc_pkg << 1) | (crc_shift & 1);

    if((crc & 0xFFFFFF) != crc_pkg) return; /* invalid CRC */

    const int pdu_type = data[0] & 0xF;
    const int pdu_txadd = (data[0] >> 6) & 1;
    const int pdu_rxadd = data[0] >> 7;

    /* for advertisements, output sender's MAC address
     * and - if present - the name
     */
    if((pdu_type == 0 || pdu_type == 2) && pdu_len > 6) {
        lcdPrint(IntToStr(data[7],2,F_HEX|F_LONG|F_ZEROS));
        lcdPrint(IntToStr(data[6],2,F_HEX|F_LONG|F_ZEROS));
        lcdPrint(IntToStr(data[5],2,F_HEX|F_LONG|F_ZEROS));
        lcdPrint(IntToStr(data[4],2,F_HEX|F_LONG|F_ZEROS));
        lcdPrint(IntToStr(data[3],2,F_HEX|F_LONG|F_ZEROS));
        lcdPrint(IntToStr(data[2],2,F_HEX|F_LONG|F_ZEROS));
        lcdPrint(" ");
        pdu_len -= 6;
        uint8_t *p = data+8;
        while(pdu_len > 0) {
            pdu_len -= p[0] + 1;
            if(p[0] > 0 && p[1] == 9) {
                p[p[0]+1] = 0;
                lcdPrint((const char*)(p+2));
                break;
            }
            p += p[0] + 1;
        }
        lcdNl();
        lcdDisplay();
    }
}

static enum {
    STATE_WAIT_ACCESS_ADDRESS,
    STATE_READ_PKG,
    STATE_CRC
} state = STATE_WAIT_ACCESS_ADDRESS;
static void decoder(int b) {
    /* for now, we only wait for packets marked with the BTLE
     * advertise address
     */
    const uint32_t btle_access_address = 0x8E89BED6;
    /* max data length is actually lower by specification, but we
     * prepare for the worst case - after all, the packet data might
     * be corrupted. This is the largest length that can occur under
     * every circumstances
     */
    static uint8_t data[0x3F+2+3];
    /* we shift all data into this buffer, large enough to hold
     * a full access address (32 bit)
     */
    static uint32_t shift;
    /* bit counter, only used when we detected the address we were
     * waiting for
     */
    static int bit;
    /* same is true for the byte counter */
    static int byte = 0;
    /* we read the pdu_len right after receiving two bytes of packet data */
    static int pdu_len;
    /* a pointer into the whiten LUT, incremented for each byte of packet
     * data
     */
    static uint8_t *whiten = whiten_table;
    /* order on wire is LSB first */
    shift = (shift >> 1) | (b << 31);
    bit++;
    /* we either wait for the access address or read packet data */
    if(state == STATE_WAIT_ACCESS_ADDRESS) {
        if(shift == btle_access_address) {
            /* start of packet detected */
            state = STATE_READ_PKG;
            bit = 0;
            whiten = whiten_table;
        }
    } else if(bit == 8) {
        bit = 0;
        /* read byte into data buffer, de-whiten it */
        data[byte++] = *(whiten++) ^ (shift >> 24);
        if(byte == 2) {
            /* PDU Header, only read length here */
            pdu_len = data[1] & 0x3F;
        } else if(byte == pdu_len+2+3) {
            /* rest of packet complete, analyze: */
            parse(data, pdu_len);
            /* reset state */
            shift = 0;
            state = STATE_WAIT_ACCESS_ADDRESS;
            byte = 0;
            pdu_len = 0;
        }
    }
}

/* demodulation of GFSK using the most simple approach available:
 * we just calculate the frequency and decide on +/-.
 */
static void gfsk_demodulate(int q, int i) {
    static uint16_t old_w = 0;
    const uint16_t w = atan2Cordic(i, q);
    const int16_t f = (int16_t)(w - old_w) /* - offset */;
    old_w = w;
    decoder((f > 0) ? 1 : 0);
}

/* for now, we only handle channel 39 */
static int64_t frequency = 2480000000;

static int16_t rxlna_enable = 1;
static int16_t lna_gain_db = 8;
static int16_t vga_gain_db = 20;
static uint32_t rxsamplerate = 1000000;
static uint16_t rxdecimation = 1;
static uint32_t rxbandwidth = 1750000;

static void rflib_set_frequency(const int64_t new_frequency, const int32_t offset) {
    const int64_t tuned_frequency = new_frequency + offset;
    ssp1_set_mode_max2837();
    if(set_freq(tuned_frequency)) {
        frequency = new_frequency;
    }
}

/* set amps */
static void set_rf_params() {
    ssp1_set_mode_max2837(); // need to reset this since display driver will hassle with SSP1
    max2837_set_lna_gain(lna_gain_db);     /* 8dB increments */
    max2837_set_vga_gain(vga_gain_db);     /* 2dB increments, up to 62dB */
}

/* portapack_init plus a bunch of stuff from here and there, cleaned up */
static void rf_init() {
    /* Release CPLD JTAG pins */
    scu_pinmux(SCU_PINMUX_CPLD_TDO, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4);
    scu_pinmux(SCU_PINMUX_CPLD_TCK, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    scu_pinmux(SCU_PINMUX_CPLD_TMS, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    scu_pinmux(SCU_PINMUX_CPLD_TDI, SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0);
    GPIO_DIR(PORT_CPLD_TDO) &= ~PIN_CPLD_TDO;
    GPIO_DIR(PORT_CPLD_TCK) &= ~PIN_CPLD_TCK;
    GPIO_DIR(PORT_CPLD_TMS) &= ~PIN_CPLD_TMS;
    GPIO_DIR(PORT_CPLD_TDI) &= ~PIN_CPLD_TDI;

    hackrf_clock_init();
    rf_path_pin_setup();

    /* Configure external clock in */
    scu_pinmux(SCU_PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);

    /* Disable unused clock outputs. They generate noise. */
    scu_pinmux(CLK0, SCU_CLK_IN | SCU_CONF_FUNCTION7);
    scu_pinmux(CLK2, SCU_CLK_IN | SCU_CONF_FUNCTION7);

    sgpio_configure_pin_functions();

    ON(EN_VDD);
    ON(EN_1V8);
    // doesn't work without:
    for(int i=250000; i>0; i--) {
        __asm__ volatile ("nop");
    }

    si5351_init();

    cpu_clock_pll1_max_speed();

    ssp1_init();

    rf_path_init();
}

static void rf_off() {
    OFF(EN_VDD);
    OFF(EN_1V8);
}

typedef union {
    uint32_t v32;
    struct {
        int8_t i0;
        int8_t q0;
        int8_t i1;
        int8_t q1;
    } __attribute__((packed));
} sgpio_val_t;

//# MENU BTLE_scan
void btlescan_menu() {
    int c=0;

    lcdClear();
    lcdDisplay();

    cpu_clock_set(204);

    rf_init();

    rf_path_set_direction(RF_PATH_DIRECTION_RX);
    rf_path_set_lna(rxlna_enable);

    rflib_set_frequency(frequency, -(rxsamplerate>>2));
    sample_rate_frac_set(rxsamplerate * rxdecimation * 2, 1);
    baseband_filter_bandwidth_set(rxbandwidth);
    sgpio_cpld_stream_rx_set_decimation(rxdecimation);
    set_rf_params();

    /* prepare whiten LUT for channel 39 */
    whiten_prepare(39);

    sgpio_cpld_stream_enable();
    while(1) {
	/* toggle LED when we lost data */
        //if(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A)) TOGGLE(LED4);
	/* wait for next bunch of sample data */
        while(!(SGPIO_STATUS_1 & (1 << SGPIO_SLICE_A))) {
            __asm__ volatile ("nop");
        }

        SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

        static uint32_t buffer[8];
        __asm__(
            "ldr r0, [%[SGPIO_REG_SS], #44]\n\t"
            "str r0, [%[buffer], #0]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #20]\n\t"
            "str r0, [%[buffer], #4]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #40]\n\t"
            "str r0, [%[buffer], #8]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #8]\n\t"
            "str r0, [%[buffer], #12]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #36]\n\t"
            "str r0, [%[buffer], #16]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #16]\n\t"
            "str r0, [%[buffer], #20]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #32]\n\t"
            "str r0, [%[buffer], #24]\n\t"
            "ldr r0, [%[SGPIO_REG_SS], #0]\n\t"
            "str r0, [%[buffer], #28]\n\t"
            :
            : [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
              [buffer] "l" (buffer)
            : "r0"
        );
        const sgpio_val_t *b = (sgpio_val_t*) buffer;
	/* process 4x4 samples (consisting each of 2xint8_t, I&Q) */
        for(int i=4; i; i--) {
	    /* shift by -fs/4, by multiplication of 1, i, -1, -i */
            gfsk_demodulate(b->q0, b->i0);
            gfsk_demodulate(-b->i1, b->q1);
            b++;
            gfsk_demodulate(-b->q0, -b->i0);
            gfsk_demodulate(b->i1, -b->q1);
            b++;
        }
	if(getInputRaw() == BTN_ENTER) break;
    }

    sgpio_cpld_stream_disable();
    rf_off();
    getInputWaitRelease();
    return;
}
