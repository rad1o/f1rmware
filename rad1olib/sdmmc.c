#include <string.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/cgu.h>
#include <rad1olib/pins.h>
#include <r0ketlib/idle.h>
#include <libopencmsis/core_cm3.h>
#include <lpcapi/chip.h>

static volatile int32_t sdio_wait_exit = 0;

/**
 * @brief	SDIO controller interrupt handler
 * @return	Nothing
 */
void sdio_isr(void)
{
	/* All SD based register handling is done in the callback
	   function. The SDIO interrupt is not enabled as part of this
	   driver and needs to be enabled/disabled in the callbacks or
	   application as needed. This is to allow flexibility with IRQ
	   handling for applicaitons and RTOSes. */
	/* Set wait exit flag to tell wait function we are ready. In an RTOS,
	   this would trigger wakeup of a thread waiting for the IRQ. */
	NVIC_DisableIRQ(SDIO_IRQn);
	sdio_wait_exit = 1;
}

/* Delay callback for timed SDIF/SDMMC functions */
static void sdmmc_waitms(uint32_t time)
{
    delayms(time);
	return;
}

/**
 * @brief	Sets up the SD event driven wakeup
 * @param	bits : Status bits to poll for command completion
 * @return	Nothing
 */
static void sdmmc_setup_wakeup(void *bits)
{
	uint32_t bit_mask = *((uint32_t *)bits);
	/* Wait for IRQ - for an RTOS, you would pend on an event here with a IRQ based wakeup. */
	NVIC_ClearPendingIRQ(SDIO_IRQn);
	sdio_wait_exit = 0;
	Chip_SDIF_SetIntMask(LPC_SDMMC, bit_mask);
	NVIC_EnableIRQ(SDIO_IRQn);
}

/**
 * @brief	A better wait callback for SDMMC driven by the IRQ flag
 * @return	0 on success, or failure condition (-1)
 */
static uint32_t sdmmc_irq_driven_wait(void)
{
	uint32_t status;

	/* Wait for event, would be nice to have a timeout, but keep it  simple */
	while (sdio_wait_exit == 0) {}

	/* Get status and clear interrupts */
	status = Chip_SDIF_GetIntStatus(LPC_SDMMC);
	Chip_SDIF_ClrIntStatus(LPC_SDMMC, status);
	Chip_SDIF_SetIntMask(LPC_SDMMC, 0);

	return status;
}

void sdmmc_setup(void) {
    SETUPpin(SD_CD);
    SETUPpin(SD_CLK);
    SETUPpin(SD_CMD);
    SETUPpin(SD_DAT0);
    SETUPpin(SD_DAT1);
    SETUPpin(SD_DAT2);
    SETUPpin(SD_DAT3);

    // enable clock line for SD/MMC block
    CGU_BASE_SDIO_CLK = CGU_BASE_SDIO_CLK_AUTOBLOCK(1) |
        CGU_BASE_SDIO_CLK_CLK_SEL(CGU_SRC_PLL1);

	NVIC_DisableIRQ(SDIO_IRQn);
	/* Enable SD/MMC Interrupt */
	NVIC_EnableIRQ(SDIO_IRQn);

    Chip_SDIF_Init(LPC_SDMMC);
}

static mci_card_struct sdcardinfo;

uint32_t sdmmc_acquire(void) {
	memset(&sdcardinfo, 0, sizeof(sdcardinfo));
	sdcardinfo.card_info.evsetup_cb = sdmmc_setup_wakeup;
	sdcardinfo.card_info.waitfunc_cb = sdmmc_irq_driven_wait;
	sdcardinfo.card_info.msdelay_func = sdmmc_waitms;
    return Chip_SDMMC_Acquire(LPC_SDMMC, &sdcardinfo);
}
