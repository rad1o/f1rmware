
#include <rad1olib/light_ws2812_cortex.h>
#include <rad1olib/setup.h>
#include <rad1olib/systick.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/cm3/systick.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
#include <r0ketlib/keyin.h>
#include <r0ketlib/intin.h>
#include <r0ketlib/menu.h>
#include <r0ketlib/select.h>
#include <r0ketlib/idle.h>
#include <fatfs/ff.h>
#include <r0ketlib/fs_util.h>

#include <r0ketlib/fs_util.h>
#include <rad1olib/pins.h>
#include <rad1olib/systick.h>

#include <portalib/portapack.h>
#include <portalib/specan.h>
#include <common/hackrf_core.h>
#include <common/rf_path.h>
#include <common/sgpio.h>
#include <common/sgpio_dma.h>
#include <common/tuning.h>
#include <common/max2837.h>
#include <common/streaming.h>
#include <libopencm3/lpc43xx/dac.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/lpc43xx/sgpio.h>
#include <libopencmsis/core_cm3.h>

#include <portalib/complex.h>

#include <math.h>
#include <string.h>

#define OOK_FREQ 433920000 // frequency we expect the signal to be at
#define SAMPLE_RATE_RAW 8000000 // this can only be one of a few presets, see sample_rate_set()
#define FILTER_BANDWIDTH 1750000 // this can only be one of a few presets, see max2837_set_lpf_bandwidth()

#define RX_DECIMATION 2 // [1; 8]
#define RX_SAMPLE_RATE ((double)SAMPLE_RATE_RAW / RX_DECIMATION)
#define RX_IQ_OFFSET (-RX_SAMPLE_RATE / 16.)
#define RX_IQ_CENTER (OOK_FREQ - RX_IQ_OFFSET)

#define TX_DECIMATION 1 // [1; 8]
#define TX_SAMPLE_RATE ((double)SAMPLE_RATE_RAW / TX_DECIMATION)
#define TX_IQ_OFFSET (TX_SAMPLE_RATE / 16.)
#define TX_IQ_CENTER (OOK_FREQ - TX_IQ_OFFSET)

#define TX_CHUNK_TIME (16. / TX_SAMPLE_RATE)
#define TX_S2CHUNK(s) ((uint32_t)((s) / TX_CHUNK_TIME + .5))
#define TX_CHUNKS_PULSE TX_S2CHUNK( 265e-6)
#define TX_CHUNKS_BIT   TX_S2CHUNK(2345e-6)

#define MESSAGE_BITS_PAYLOAD 18
#define MESSAGE_BITS_GAP      6

static int8_t tx_buffer[2][32] = {
	{
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
		 0x00,  0x00,
	}, {
		 0x7F,  0x00,
		 0x75,  0x30,
		 0x59,  0x59,
		 0x30,  0x75,
		 0x00,  0x7F,
		-0x30,  0x75,
		-0x59,  0x59,
		-0x75,  0x30,
		-0x7F,  0x00,
		-0x75, -0x30,
		-0x59, -0x59,
		-0x30, -0x75,
		 0x00, -0x7F,
		 0x30, -0x75,
		 0x59, -0x59,
		 0x75, -0x30,
	}
};

typedef struct {
	uint32_t count; // in units of 16 samples, stored value is one less than the actual count
	int8_t *buffer;
} ook_word_t;

static volatile ook_word_t ook_word_current = { 0, tx_buffer[0] };
static volatile ook_word_t ook_word_next    = { 0, tx_buffer[0] };

static volatile int blacnt = 0;

static void sgpio_isr_tx(void) {
	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);
	++blacnt;

	uint32_t* const p = (uint32_t*)ook_word_current.buffer;
	__asm__(
		"ldr r0, [%[p], #0]\n\t"
		"str r0, [%[SGPIO_REG_SS], #44]\n\t"
		"ldr r0, [%[p], #4]\n\t"
		"str r0, [%[SGPIO_REG_SS], #20]\n\t"
		"ldr r0, [%[p], #8]\n\t"
		"str r0, [%[SGPIO_REG_SS], #40]\n\t"
		"ldr r0, [%[p], #12]\n\t"
		"str r0, [%[SGPIO_REG_SS], #8]\n\t"
		"ldr r0, [%[p], #16]\n\t"
		"str r0, [%[SGPIO_REG_SS], #36]\n\t"
		"ldr r0, [%[p], #20]\n\t"
		"str r0, [%[SGPIO_REG_SS], #16]\n\t"
		"ldr r0, [%[p], #24]\n\t"
		"str r0, [%[SGPIO_REG_SS], #32]\n\t"
		"ldr r0, [%[p], #28]\n\t"
		"str r0, [%[SGPIO_REG_SS], #0]\n\t"
		:
		: [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
		  [p] "l" (p)
		: "r0"
	);

	uint32_t count = ook_word_current.count;

	if(count == 0) {
		ook_word_current = ook_word_next;
		ook_word_next.count = 0;
	} else {
		ook_word_current.count = count - 1;
	}
}

#define RX_BUFLEN 128

static volatile int16_t rx_samples[RX_BUFLEN];
static volatile uint8_t rx_sample_index; // index of most recently written sample

static void sgpio_isr_rx() {
	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);
	--blacnt;

	static union {
		int8_t c[32];
		uint32_t i[8];
	} buffer;

	__asm__(
		"ldr r0, [%[SGPIO_REG_SS], #44]\n\t"
		"str r0, [%[p], #0]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #20]\n\t"
		"str r0, [%[p], #4]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #40]\n\t"
		"str r0, [%[p], #8]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #8]\n\t"
		"str r0, [%[p], #12]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #36]\n\t"
		"str r0, [%[p], #16]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #16]\n\t"
		"str r0, [%[p], #20]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #32]\n\t"
		"str r0, [%[p], #24]\n\t"
		"ldr r0, [%[SGPIO_REG_SS], #0]\n\t"
		"str r0, [%[p], #28]\n\t"
		:
		: [SGPIO_REG_SS] "l" (SGPIO_PORT_BASE + 0x100),
		  [p] "l" (buffer.i)
		: "r0"
	);

	static int16_t lp_r = 0;
	static int16_t lp_i = 0;

	for(int i = 0; i < 32; i += 2) {
		// multiply by complex sine wave to shift desired frequency to zero
		int32_t sr = buffer.c[i + 0] * (int32_t)tx_buffer[1][i + 1]
		           + buffer.c[i + 1] * (int32_t)tx_buffer[1][i + 0];
		int32_t si = buffer.c[i + 0] * (int32_t)tx_buffer[1][i + 0]
		           - buffer.c[i + 1] * (int32_t)tx_buffer[1][i + 1];

		// run through a TP1 filter with a mostly-guessed parameter
		const int32_t lp_t1 = .99 * .99 * (1 << 15); // 1.15
		const int32_t lp_t2 = (1 << 15) - lp_t1;     // 1.15

		lp_r = ((int32_t)lp_r * lp_t1 + (int32_t)sr * lp_t2) >> 15;
		lp_i = ((int32_t)lp_i * lp_t1 + (int32_t)si * lp_t2) >> 15;
	}

	// drop 127 out of 128 samples
	static int counter = 0;
	if(counter == 0) {
		counter = 7;
		int32_t abssq = ((int32_t)lp_r * lp_r + (int32_t)lp_i * lp_i) >> 15;

		uint8_t index = rx_sample_index;
		index = index == RX_BUFLEN - 1 ? 0 : index + 1;
		rx_samples[index] = abssq;
		rx_sample_index = index;
	} else {
		--counter;
	}
}

enum {
	GARAGE_MENU_MESSAGE,
	GARAGE_MENU_TX,
	GARAGE_MENU_RX,
	GARAGE_MENU_AMP,
	GARAGE_MENU_BBTXVGA,
	GARAGE_MENU_BBRXVGA,
	GARAGE_MENU_BBRXLNA,
	GARAGE_MENU_EXIT
};

static struct {
	uint32_t message;
	uint8_t current_item;
	uint8_t current_subitem;
	uint8_t gain;
	uint8_t rxvga;
	uint8_t rxlna;
	uint8_t tx;
	uint8_t rx;
} garage_state;

static struct {
	uint8_t sample_index; // next index to be fetched
	int16_t threshold;
	int16_t max;
	int16_t max_counter;
	uint32_t last_edge_time;
	int32_t last_edge_state;
	uint8_t bit_count;
	uint32_t message;
	uint32_t last_message;
	uint32_t message_count;
} garage_rx_state;

// process all outstanding samples from rx queue
static void rx_process(void) {
	for(;;) {
		uint8_t index = garage_rx_state.sample_index;
		index = index == RX_BUFLEN - 1 ? 0 : index + 1;

		if(index == rx_sample_index) {
			return; // we're done for now
		}

		int16_t abssq = rx_samples[index];
		garage_rx_state.sample_index = index;

		if(abssq > garage_rx_state.max) {
			garage_rx_state.max = abssq;
		}

		if(abssq > garage_rx_state.threshold) {
			garage_rx_state.threshold = abssq;
		}

		if(garage_rx_state.max_counter == 0) {
			garage_rx_state.max_counter = 2048 - 1;
			garage_rx_state.threshold = garage_rx_state.max;
			garage_rx_state.max = 0;
		} else {
			--garage_rx_state.max_counter;
		}

		int32_t edge_state = abssq > garage_rx_state.threshold / 2;
		++garage_rx_state.last_edge_time;

		if(garage_rx_state.last_edge_state != edge_state) {
			if(garage_rx_state.last_edge_time < 2) {
				// presumably a glitch; ignore
				continue;
			} else if(garage_rx_state.last_edge_state && garage_rx_state.last_edge_time >= 5 && garage_rx_state.last_edge_time < 13) {
				// short pulse: 0
				garage_rx_state.message >>= 1;
				++garage_rx_state.bit_count;
			} else if(garage_rx_state.last_edge_state && garage_rx_state.last_edge_time >= 60 && garage_rx_state.last_edge_time < 68) {
				// long pulse: 1
				garage_rx_state.message >>= 1;
				garage_rx_state.message |= 1 << 17;
				++garage_rx_state.bit_count;
			} else if(!garage_rx_state.last_edge_state && garage_rx_state.last_edge_time >= 5 && garage_rx_state.last_edge_time < 13) {
			} else if(!garage_rx_state.last_edge_state && garage_rx_state.last_edge_time >= 60 && garage_rx_state.last_edge_time < 68) {
			} else if(!garage_rx_state.last_edge_state && garage_rx_state.last_edge_time >= 75 && garage_rx_state.last_edge_time < 750) {
				// relatively short gap: inter-word space
				garage_rx_state.bit_count = 0;
			} else if(!garage_rx_state.last_edge_state && garage_rx_state.last_edge_time >= 750) {
				// long gap: inter-transmission space
				garage_rx_state.bit_count = 0;
				garage_rx_state.last_message = 1 << 18; // impossible message
			} else {
				// everything else is unknown
				garage_rx_state.bit_count = 0;
				garage_rx_state.last_message = 1 << 18; // impossible message
			}

			if(garage_rx_state.bit_count == 18) {
				if(garage_rx_state.message == garage_rx_state.last_message) {
					garage_state.message = garage_rx_state.message;
					++garage_rx_state.message_count;
				}
				garage_rx_state.last_message = garage_rx_state.message;
				garage_rx_state.bit_count = 0;
			}

			garage_rx_state.last_edge_state = edge_state;
			garage_rx_state.last_edge_time = 0;
		}
	}
}

static void rf_apply_settings(void) {
	ssp1_set_mode_max2837();

	max2837_set_lpf_bandwidth(FILTER_BANDWIDTH);
	max2837_set_lna_gain(garage_state.rxlna); // 8dB increments, up to 40dB
	max2837_set_vga_gain(garage_state.rxvga); // 2dB increments, up to 62dB
	max2837_set_txvga_gain(garage_state.gain & ((1 << 6) - 1));

	rf_path_set_lna((garage_state.gain & 1 << 7) != 0 ? 1 : 0);
}

static void rf_set_direction(int tx) {
	baseband_streaming_disable();

	rf_apply_settings();

	if(tx) {
		rf_path_set_direction(RF_PATH_DIRECTION_TX);

		set_freq(TX_IQ_CENTER);

		vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_tx;
	} else {
		rf_path_set_direction(RF_PATH_DIRECTION_RX);

		set_freq(RX_IQ_CENTER);
		sgpio_cpld_stream_rx_set_decimation(RX_DECIMATION);

		vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_rx;
	}

	baseband_streaming_enable();
}

static void rf_init() {
	dac_init(false);
	cpu_clock_set(204); // WARP SPEED! :-)

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

	delayms(500); // doesn't work without
	cpu_clock_set(204); // WARP SPEED! :-)
	si5351_init();

	cpu_clock_pll1_max_speed();
	ssp1_init();
	rf_path_init();

	sample_rate_set(SAMPLE_RATE_RAW);

	rf_set_direction(0);

	baseband_streaming_enable();
}

static void garage_init(void) {
	garage_state.message = 0x14CCC;
	garage_state.current_item = GARAGE_MENU_MESSAGE;
	garage_state.current_subitem = 0;
	garage_state.gain = 0x90;
	garage_state.rxlna = 16;
	garage_state.rxvga = 20;
	garage_state.tx = 0;
	garage_state.rx = 0;

	// Turn off the RGB LEDs.  Note that ws2812_sendarray() changes the core
	// clock frequency, so this should better not be called if interrupts are
	// enabled. tx_buffer[0] is re-used because it only consists of zeros.
	ws2812_sendarray((uint8_t *)tx_buffer[0], 24);

	rf_init();
}

static void garage_stop(void) {
	baseband_streaming_disable();

	OFF(EN_VDD);
	OFF(EN_1V8);
}

// transmit a pulse of length on, followed by a gap of length off
// both lengths are in muliples of TX_CHUNK_TIME.
static void tx_duration(uint32_t on, uint32_t off) {
	while(ook_word_next.count != 0) ;
	ook_word_next.buffer = tx_buffer[1];
	ook_word_next.count = on;

	while(ook_word_next.count != 0) ;
	ook_word_next.buffer = tx_buffer[0];
	ook_word_next.count = off;
}

static void garage_tx(uint32_t message) {
	for(int i = 0; i < MESSAGE_BITS_PAYLOAD; ++i) {
		uint32_t on, off;

		if((message & 1 << i) != 0) {
			off = TX_CHUNKS_PULSE;
			on = TX_CHUNKS_BIT - off;
		} else {
			on = TX_CHUNKS_PULSE;
			off = TX_CHUNKS_BIT - on;
		}

		if(i == MESSAGE_BITS_PAYLOAD - 1) {
			off += MESSAGE_BITS_GAP * TX_CHUNKS_BIT;
		}

		tx_duration(on, off);
	}
}

// 19x16
//               111111111
//     0123456789012345678
//    +-------------------+
//  0 |Garage Door Opener |
//  1 |~~~~~~~~~~~~~~~~~~~|
//  2 |                   |
//  3 |   #   #   #       |
//  4 |  -|- - - - - -#-# |
//  5 | # | #   #   #     |
//  6 |                   |
//  7 |  Transmit!        |
//  8 |  Reveive!         |
//  9 |                   |
// 10 |  AMP: on (+14 dB) |
// 11 |  BBTXVGA: +20 dB  |
// 12 |  BBRXVGA: +20 dB  |
// 13 |  BBRXLNA: +20 dB  |
// 14 |                   |
// 15 |  Exit             |
//    +-------------------+

static void garage_status(void) {
	lcdClear();
	lcdPrintln("Garage Door Opener ");
	lcdPrintln("~~~~~~~~~~~~~~~~~~~");

	lcdNl();

	for(int i = 3; i >= 0; --i) {
		if(i == 2) {
			// state 2 is not possible to select with three-way switches
			continue;
		}

		char str[19];
		str[0] = ' ';
		str[18] = '\0';

		for(int j = 0; j < 9; ++j) {
			if(((garage_state.message >> (j * 2)) & 3) == i) {
				str[1 + 2 * j] = '#';
			} else if(garage_state.current_item == GARAGE_MENU_MESSAGE && garage_state.current_subitem == 9 - j) {
				str[1 + 2 * j] = '|';
			} else {
				str[1 + 2 * j] = ' ';
			}

			if(i == 1 && j != 0) {
				str[0 + 2 * j] = '-';
			} else {
				str[0 + 2 * j] = ' ';
			}
		}

		if(i == 1 && garage_state.current_item == GARAGE_MENU_MESSAGE) {
			str[0] = '>';
		} else {
			str[0] = ' ';
		}

		lcdPrintln(str);
	}

	lcdNl();

	lcdPrint(garage_state.current_item == GARAGE_MENU_TX ? "> " : "  ");
	lcdPrint("Transmit!");
	lcdPrintln(garage_state.tx ? " ) ) )" : "");

	lcdPrint(garage_state.current_item == GARAGE_MENU_RX ? "> " : "  ");
	lcdPrint("Receive! ");
	if(garage_state.rx) {
		lcdPrintInt(garage_rx_state.message_count);
		lcdPrintln(" ( (");
	} else {
		lcdNl();
	}

	lcdNl();

	lcdPrint(garage_state.current_item == GARAGE_MENU_AMP ? "> " : "  ");
	lcdPrint("AMP: ");
	lcdPrintln((garage_state.gain & (1 << 7)) != 0 ? "on (+14 dB)" : "off");
	lcdPrint(garage_state.current_item == GARAGE_MENU_BBTXVGA ? "> " : "  ");
	lcdPrint("BBTXVGA: +");
	lcdPrint((garage_state.gain & ((1 << 6) - 1)) <= 9 ? " " : "");
	lcdPrintInt(garage_state.gain & ((1 << 6) - 1));
	lcdPrintln(" dB");
	lcdPrint(garage_state.current_item == GARAGE_MENU_BBRXVGA ? "> " : "  ");
	lcdPrint("BBRXVGA: +");
	lcdPrint(garage_state.rxvga <= 9 ? " " : "");
	lcdPrintInt(garage_state.rxvga);
	lcdPrintln(" dB");
	lcdPrint(garage_state.current_item == GARAGE_MENU_BBRXLNA ? "> " : "  ");
	lcdPrint("BBRXLNA: +");
	lcdPrint(garage_state.rxlna <= 9 ? " " : "");
	lcdPrintInt(garage_state.rxlna);
	lcdPrintln(" dB");

	lcdNl();

	lcdPrint(garage_state.current_item == GARAGE_MENU_EXIT ? "> " : "  ");
	lcdPrintln("Exit");

	lcdDisplay();
};

static void garage_menu_tx(void) {
	garage_state.tx = 1;
	garage_status();
	rf_set_direction(1);

	while(getInputRaw() != BTN_NONE) {
		garage_tx(garage_state.message);
	}

	rf_set_direction(0);
	garage_state.tx = 0;
}

static void garage_menu_rx(int left) {
	memset(&garage_rx_state, 0x00, sizeof(garage_rx_state));
	garage_state.rx = 1;

	garage_status();

	rf_apply_settings();

	uint32_t count = garage_rx_state.message_count;
	for(;;) {
		int btn = getInputRaw();
		if(left) {
			if(btn != BTN_NONE && btn != BTN_LEFT) {
				break;
			}
		} else {
			if(btn == BTN_NONE) {
				break;
			}
		}

		rx_process();

		if(count != garage_rx_state.message_count) {
			count = garage_rx_state.message_count;
			garage_status();
		}
	}

	garage_state.rx = 0;
}

//# MENU garage_door
void garage(void) {
	int buttonPressTime;
	garage_init();
	ssp1_set_mode_max2837();

	while(1)
	{
		garage_status();

		switch(getInputWaitRepeat())
		{
			case BTN_UP:
				if(garage_state.current_item == GARAGE_MENU_MESSAGE && garage_state.current_subitem != 0) {
					uint8_t shift = (9 - garage_state.current_subitem) * 2;
					uint32_t val = (garage_state.message >> shift) & 3;
					if(val == 0) {
						garage_state.message += 1 << shift;
					} else if(val == 1) {
						garage_state.message += 2 << shift;
					}
				} else if(garage_state.current_item != GARAGE_MENU_MESSAGE) {
					--garage_state.current_item;
				}
				break;
			case BTN_DOWN:
				if(garage_state.current_item == GARAGE_MENU_MESSAGE && garage_state.current_subitem != 0) {
					uint8_t shift = (9 - garage_state.current_subitem) * 2;
					uint32_t val = (garage_state.message >> shift) & 3;
					if(val == 3) {
						garage_state.message -= 2 << shift;
					} else if(val == 1) {
						garage_state.message -= 1 << shift;
					}
				} else if(garage_state.current_item != GARAGE_MENU_EXIT) {
					++garage_state.current_item;
				}
				break;
			case BTN_LEFT:
				if(garage_state.current_item == GARAGE_MENU_MESSAGE) {
					if(garage_state.current_subitem != 9) {
						++garage_state.current_subitem;
					} else {
						garage_state.current_subitem = 0;
					}
				} else if(garage_state.current_item == GARAGE_MENU_TX) {
					garage_menu_tx();
				} else if(garage_state.current_item == GARAGE_MENU_RX) {
					garage_menu_rx(1);
				} else if(garage_state.current_item == GARAGE_MENU_AMP && (garage_state.gain & ( 1 << 7)) != 0) {
					garage_state.gain &= ~(1 << 7);
				} else if(garage_state.current_item == GARAGE_MENU_BBTXVGA && (garage_state.gain & ((1 << 6) - 1)) != 0) {
					--garage_state.gain;
				} else if(garage_state.current_item == GARAGE_MENU_BBRXVGA && garage_state.rxvga > 0) {
					garage_state.rxvga -= 2;
				} else if(garage_state.current_item == GARAGE_MENU_BBRXLNA && garage_state.rxlna > 0) {
					garage_state.rxlna -= 8;
				} else if(garage_state.current_item == GARAGE_MENU_EXIT) {
					garage_stop();
					return;
				}
				break;
			case BTN_RIGHT:
				if(garage_state.current_item == GARAGE_MENU_MESSAGE) {
					if(garage_state.current_subitem != 0) {
						--garage_state.current_subitem;
					} else {
						garage_state.current_subitem = 9;
					}
				} else if(garage_state.current_item == GARAGE_MENU_TX) {
					garage_menu_tx();
				} else if(garage_state.current_item == GARAGE_MENU_RX) {
					garage_menu_rx(0);
				} else if(garage_state.current_item == GARAGE_MENU_AMP && (garage_state.gain & ( 1 << 7)) == 0) {
					garage_state.gain |= 1 << 7;
				} else if(garage_state.current_item == GARAGE_MENU_BBTXVGA && (garage_state.gain & ((1 << 6) - 1)) != 47) {
					++garage_state.gain;
				} else if(garage_state.current_item == GARAGE_MENU_BBRXVGA && garage_state.rxvga < 62) {
					garage_state.rxvga += 2;
				} else if(garage_state.current_item == GARAGE_MENU_BBRXLNA && garage_state.rxlna < 40) {
					garage_state.rxlna += 8;
				} else if(garage_state.current_item == GARAGE_MENU_EXIT) {
					garage_stop();
					return;
				}
				break;
			case BTN_ENTER:
				if(garage_state.current_item == GARAGE_MENU_MESSAGE) {
					if(garage_state.current_subitem != 0) {
						garage_state.current_subitem = 0;
					} else {
						garage_state.current_subitem = 9;
					}
				} else if(garage_state.current_item == GARAGE_MENU_TX) {
					garage_menu_tx();
				} else if(garage_state.current_item == GARAGE_MENU_RX) {
					garage_menu_rx(0);
				} else if(garage_state.current_item == GARAGE_MENU_AMP) {
					garage_state.gain ^= 1 << 7;
				} else if(garage_state.current_item == GARAGE_MENU_EXIT) {
					garage_stop();
					return;
				}
				break;
		}
	}
}
