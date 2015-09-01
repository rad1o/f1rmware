
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

#include <portalib/complex.h>

#include <math.h>

#define DEFAULT_FREQ 2531000000
#define DEFAULT_MODE MODE_SPECTRUM

static volatile int64_t freq = DEFAULT_FREQ;

#define MODE_SPECTRUM 10
#define MODE_WATERFALL 20

static volatile int displayMode = DEFAULT_MODE;

// How long to wait before starting fast scroll
#define FAST_CHANGE_DELAY 100

// How much to change per 10 ms when scrolling fast
#define FAST_CHANGE_CHANGE 2000000
#define TX_BUFFER_LEN   512

uint32_t tx_buffer[TX_BUFFER_LEN];
uint32_t tx_buffer_index = 0;

static void sgpio_isr_tx() {
    TOGGLE(LED2);
	SGPIO_CLR_STATUS_1 = (1 << SGPIO_SLICE_A);

	uint32_t* const p = (uint32_t*)&tx_buffer[tx_buffer_index];
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
	tx_buffer_index = (tx_buffer_index + 32) & (TX_BUFFER_LEN - 1);
}


void rf_init() {
	cpu_clock_pll1_max_speed();
	
	//sgpio_set_slice_mode(false);

	ssp1_init();
	rf_path_init();
	rf_path_set_direction(RF_PATH_DIRECTION_TX);

	rf_path_set_lna(0);
	max2837_set_lna_gain(16);	/* 8dB increments */
	max2837_set_vga_gain(20);	/* 2dB increments, up to 62dB */

	systick_set_reload(0xfffff); 
	systick_set_clocksource(1);
	systick_counter_enable();

#if 0
	sgpio_dma_init();

	sgpio_dma_configure_lli(&lli_rx[0], 1, false, sample_buffer_0, 4096);
	sgpio_dma_configure_lli(&lli_rx[1], 1, false, sample_buffer_1, 4096);

	gpdma_lli_create_loop(&lli_rx[0], 2);

	gpdma_lli_enable_interrupt(&lli_rx[0]);
	gpdma_lli_enable_interrupt(&lli_rx[1]);

	nvic_set_priority(NVIC_DMA_IRQ, 0);
	nvic_enable_irq(NVIC_DMA_IRQ);
#endif

	vector_table.irq[NVIC_SGPIO_IRQ] = sgpio_isr_tx;

	//set_rx_mode(RECEIVER_CONFIGURATION_SPEC);

	//set_frequency(device_state->tuned_hz);
}

static void txtest_init()
{
	// RF initialization from ppack.c:
	dac_init(false);
	cpu_clock_set(204); // WARP SPEED! :-)
	hackrf_clock_init();
	rf_path_pin_setup();
	/* Configure external clock in */
	//scu_pinmux(SCU_PINMUX_GP_CLKIN, SCU_CLK_IN | SCU_CONF_FUNCTION1);
	sgpio_configure_pin_functions();
	ON(EN_VDD);
	ON(EN_1V8);
	ON(MIC_AMP_DIS);
	delayms(500); // doesn't work without
	cpu_clock_set(204); // WARP SPEED! :-)
	si5351_init();
	rf_init(); //portapack_init();
	sample_rate_set(8000000);
    baseband_streaming_enable();

	//set_rx_mode(RECEIVER_CONFIGURATION_SPEC);
	//specan_register_callback(spectrum_callback);

	// defaults:

	// the frequency is either initialized to DEFAULT_FREQ or set by the frequency menu
	// resetting it here would break the menu.
	//freq = DEFAULT_FREQ;

	displayMode = DEFAULT_MODE;
}

static void txtest_stop()
{
//	nvic_disable_irq(NVIC_DMA_IRQ);
	//sgpio_dma_stop();
	sgpio_cpld_stream_disable();
	OFF(EN_VDD);
	OFF(EN_1V8);
	ON(MIC_AMP_DIS);
	systick_set_clocksource(0);
	systick_set_reload(12e6/SYSTICKSPEED/1000);

	//specan_register_callback(0);
}

void gnerate_signal(int div)
{

    int i;
    for(i=0; i<TX_BUFFER_LEN * 2; i+=2) {
        int8_t i1 = sin(((float)i)/((float)div)*M_PI) * 30.;
        int8_t i2 = sin(((float)i + 1)/((float)div)*M_PI) * 30.;

        int8_t q1 = i1;
        int8_t q2 = i2;
        tx_buffer[i/2] = (q2 << 24) | (i2<<16) | (q1<<8) | i1;
    }
}

//# MENU txtest
void txtest()
{
	int buttonPressTime;
	txtest_init();
	ssp1_set_mode_max2837();
	set_freq(freq);

    int div = 8;
    gnerate_signal(div);
	while(1)
	{
		//getInputWaitRepeat does not seem to work?
		switch(getInputRaw())
		{
			case BTN_UP:
                div--;
                gnerate_signal(div);
                while(getInputRaw()==BTN_UP)
                    ;
				break;
			case BTN_DOWN:
                div++;
                gnerate_signal(div);
                while(getInputRaw()==BTN_DOWN)
                    ;
				break;
			case BTN_LEFT:
				break;
			case BTN_RIGHT:
				break;
			case BTN_ENTER:
				txtest_stop();
				return;
		}
	}
}
