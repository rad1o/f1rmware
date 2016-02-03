
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

#define DEFAULT_FREQ 433920000
static volatile int64_t freq = DEFAULT_FREQ;


#define TX_SAMPLES_LEN   1024


// Two samples per tx_buffer entry
#define TX_BUFFER_LEN   (TX_SAMPLES_LEN * 2)
int8_t tx_buffer[TX_BUFFER_LEN];
uint32_t tx_buffer_index = 0;
volatile uint32_t low_trigger = 0;
volatile uint32_t high_trigger = 0;


// Two bytes per sample
#define PRE_BUFFER_LEN   (TX_SAMPLES_LEN * 2)
int8_t pre_buffer[PRE_BUFFER_LEN];


static void sgpio_isr_tx() {
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

    if(tx_buffer_index == 0) {
        low_trigger = 1;
    }

    if(tx_buffer_index == TX_BUFFER_LEN / 2) {
        high_trigger = 1;
    }
}

static void rf_init() {
	cpu_clock_pll1_max_speed();
	
	//sgpio_set_slice_mode(false);

	ssp1_init();
	rf_path_init();
	rf_path_set_direction(RF_PATH_DIRECTION_TX);

	rf_path_set_lna(1);
	max2837_set_lna_gain(62);	/* 8dB increments */
	max2837_set_vga_gain(62);	/* 2dB increments, up to 62dB */
    max2837_set_txvga_gain(47);

	systick_set_reload(0xfffff);
	systick_set_clocksource(1);
	systick_counter_enable();

#if 0
    // RX DMA code, kept for future reference
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
}


static void pwrsckt_init()
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
	OFF(MIC_AMP_DIS); // Enable audio amp

	delayms(500); // doesn't work without
	cpu_clock_set(204); // WARP SPEED! :-)
	si5351_init();
	rf_init(); //portapack_init();
	sample_rate_set(8000000);
    baseband_streaming_enable();

	// defaults:

    freq = DEFAULT_FREQ;

}

static void pwrsckt_stop()
{
    //nvic_disable_irq(NVIC_DMA_IRQ);
	//sgpio_dma_stop();
	sgpio_cpld_stream_disable();
	OFF(EN_VDD);
	OFF(EN_1V8);
	ON(MIC_AMP_DIS);
	systick_set_clocksource(0);
	systick_set_reload(12e6/SYSTICKSPEED/1000);
}

int8_t *rf_getBuffer(uint32_t *len)
{
    int8_t *buf = NULL;

    if(low_trigger) {
        buf = &tx_buffer[TX_BUFFER_LEN / 2];
        low_trigger = 0;
    }

    if(high_trigger) {
        buf = &tx_buffer[0];
        high_trigger = 0;
    }

    if(len) {
        *len = TX_BUFFER_LEN / 4;
    }

    return buf;
}

void rf_send(int8_t *buffer, uint32_t len)
{
    while(low_trigger == 0 && high_trigger == 0) {
        __WFI();
    }
}

void off(int8_t *buf, uint32_t n)
{
    while(n--) {
        *buf++ = 0x00;
        *buf++ = 0x00;
    }
}

void on(int8_t *buf, uint32_t n)
{
    while(n--) {
        *buf++ = 0x7F;
        *buf++ = 0x00;
    }
}

void send_onSymbol(void)
{
    int8_t *buf;
    int i;
    for(i=0; i < 4; i++) {
        buf = rf_getBuffer(NULL);
        on(buf, TX_BUFFER_LEN / 2);
        rf_send(buf, TX_BUFFER_LEN / 2);
    }
}

void send_offSymbol(void)
{
    int8_t *buf;
    int i;
    for(i=0; i < 4; i++) {
        buf = rf_getBuffer(NULL);
        off(buf, TX_BUFFER_LEN / 2);
        rf_send(buf, TX_BUFFER_LEN / 2);
    }
}

void send_oneBit(void)
{
    send_onSymbol();
    send_offSymbol();
    send_offSymbol();
    send_offSymbol();
    send_onSymbol();
    send_offSymbol();
    send_offSymbol();
    send_offSymbol();
}

void send_zeroBit(void)
{
    send_onSymbol();
    send_offSymbol();
    send_offSymbol();
    send_offSymbol();
    send_onSymbol();
    send_onSymbol();
    send_onSymbol();
    send_offSymbol();
}

void switch_socket(uint32_t address, uint32_t socket, bool on)
{
    // 5 bit address: 01010
    for(int i = 0; i < 5; i++) {
        if(address & 0x10) {
            send_oneBit();
        } else {
            send_zeroBit();
        }
        address <<= 1;
    }

    // 5 bit which socket (A-E): 00010
    for(int i = 0; i < 5; i++) {
        if(socket == i) {
            send_oneBit();
        } else {
            send_zeroBit();
        }
    }

    // 2 bit on/off: 01 / 10
    if(on) {
        send_oneBit();
        send_zeroBit();
    } else {
        send_zeroBit();
        send_oneBit();
    }

    // trailer
    // on symbol
    // off symbol
    send_onSymbol();

    for(int i = 0; i < 14; i++) {
        send_offSymbol();
    }
}

#define M_ADDR 0
#define M_SOCKET 1
#define M_DWIM 2
#define M_EXIT 3
#define MENUITEMS 3
int mline=3;
int addr=0x1f;
int socket=3;

static void pwrsckt_status() {

    lcdClear();
    lcdPrintln("Power Socket");
    lcdPrintln("------------");

    lcdPrint("  Addr: ");
    int j=16;
    while(j>0){
        if(addr&j)
            lcdPrint("1");
        else
            lcdPrint("0");
        j/=2;
    };
    lcdPrint(" (");
    lcdPrint(IntToStr(addr,2,F_HEX|F_LONG|F_ZEROS));
    lcdPrint(")");
    lcdNl();

    lcdPrint("  Sock:   ");
    switch (socket){
        case 0:
            lcdPrint("A");
            break;
        case 1:
            lcdPrint("B");
            break;
        case 2:
            lcdPrint("C");
            break;
        case 3:
            lcdPrint("D");
            break;
        case 4:
            lcdPrint("E");
            break;
    };
    lcdPrint("    (");
    lcdPrint(IntToStr(socket,1,0));
    lcdPrint(")");
    lcdNl();

    lcdPrintln("");
    lcdPrintln("  ON/OFF");
    lcdPrintln("");
    lcdPrintln("  Exit");

    lcdSetCrsr(0,8*(mline+2+(mline>M_SOCKET)+(mline>M_DWIM)));
    lcdPrint(">");
    lcdDisplay();
};

//# MENU pwrsckt
void pwrsckt()
{
	int buttonPressTime;
	pwrsckt_init();
	ssp1_set_mode_max2837();
	set_freq(freq);

    off(tx_buffer, TX_BUFFER_LEN);
    pwrsckt_status();

	while(1)
	{
		//getInputWaitRepeat does not seem to work?
        pwrsckt_status();
		switch(getInputWaitRepeat())
		{
			case BTN_UP:
                mline--;
                if (mline<0)
                    mline=3;
				break;
			case BTN_DOWN:
                mline++;
                if (mline>MENUITEMS)
                    mline=0;
				break;
			case BTN_LEFT:
                    switch (mline){
                        case M_ADDR:
                            addr--;
                            if(addr<0)
                                addr=0x1f;
                            break;
                        case M_SOCKET:
                            socket--;
                            if(socket<0)
                                socket=4;
                            break;
                        case M_DWIM:
                            while(getInputRaw()==BTN_LEFT) {
                                switch_socket(addr, socket, 0);
                            };
                            break;
                        case M_EXIT:
                            break;
                    };
                    getInputWaitRelease();
				break;
			case BTN_RIGHT:
                    switch (mline){
                        case M_ADDR:
                            addr++;
                            if(addr>0x1f)
                                addr=0;
                            break;
                        case M_SOCKET:
                            socket++;
                            if(socket>4)
                                socket=0;
                            break;
                        case M_DWIM:
                            while(getInputRaw()==BTN_RIGHT) {
                                switch_socket(addr, socket, 1);
                            }
                            break;
                        case M_EXIT:
                            pwrsckt_stop();
                            return;
                    };
                    getInputWaitRelease();
				break;
			case BTN_ENTER:
				return;
		}
	}
}
