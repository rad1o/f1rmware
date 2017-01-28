/*
 * @brief This file contains a USB MSC RAM example using USB ROM Drivers.
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "app_usbd_cfg.h"
#include "msc_disk.h"
#include <lpcapi/romapi_18xx_43xx.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <string.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static USBD_HANDLE_T g_hUsb;

/* Endpoint 0 patch that prevents nested NAK event processing */
static uint32_t g_ep0RxBusy = 0;/* flag indicating whether EP0 OUT/RX buffer is busy. */
static USB_EP_HANDLER_T g_Ep0BaseHdlr;	/* variable to store the pointer to base EP0 handler */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
const USBD_API_T *g_pUsbApi;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* EP0_patch part of WORKAROUND for artf45032. */
ErrorCode_t EP0_patch(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	switch (event) {
	case USB_EVT_OUT_NAK:
		if (g_ep0RxBusy) {
			/* we already queued the buffer so ignore this NAK event. */
			return LPC_OK;
		}
		else {
			/* Mark EP0_RX buffer as busy and allow base handler to queue the buffer. */
			g_ep0RxBusy = 1;
		}
		break;

	case USB_EVT_SETUP:	/* reset the flag when new setup sequence starts */
	case USB_EVT_OUT:
		/* we received the packet so clear the flag. */
		g_ep0RxBusy = 0;
		break;
	}
	return g_Ep0BaseHdlr(hUsb, data, event);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from USB0
 * @return	Nothing
 */
#include <rad1olib/pins.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>
void USB0_IRQHandler(void)
{
	TOGGLE(RAD1O_LED1);
	USBD_API->hw->ISR(g_hUsb);
}

/**
 * @brief	Find the address of interface descriptor for given class type.
 * @return	If found returns the address of requested interface else returns NULL.
 */
USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass)
{
	USB_COMMON_DESCRIPTOR *pD;
	USB_INTERFACE_DESCRIPTOR *pIntfDesc = 0;
	uint32_t next_desc_adr;

	pD = (USB_COMMON_DESCRIPTOR *) pDesc;
	next_desc_adr = (uint32_t) pDesc;

	while (pD->bLength) {
		/* is it interface descriptor */
		if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {

			pIntfDesc = (USB_INTERFACE_DESCRIPTOR *) pD;
			/* did we find the right interface descriptor */
			if (pIntfDesc->bInterfaceClass == intfClass) {
				break;
			}
		}
		pIntfDesc = 0;
		next_desc_adr = (uint32_t) pD + pD->bLength;
		pD = (USB_COMMON_DESCRIPTOR *) next_desc_adr;
	}

	return pIntfDesc;
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
#include <rad1olib/setup.h>
#include <common/usb.h>
#include <r0ketlib/display.h>
#include <r0ketlib/print.h>

#include <rad1olib/spi-flash.h>

int dwim(void)
{
	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;
	USB_CORE_CTRL_T *pCtrl;

	flashInit();

	/* Initialize board and chip */
//	SystemCoreClockUpdate();
//	Board_Init();
	lcdPrint("enter");lcdNl();lcdDisplay();

	usb_clock_init();

	/* enable clocks and pinmux */
//	USB_init_pin_clk();  // is actually Chip_USB0_Init()

	/* Setup default USB PLL state for a 480MHz output and attach */
//	Chip_Clock_SetupPLL(CLKIN_CRYSTAL, CGU_USB_PLL, &usbPLLSetup);

	/* enable USB PLL */
//	Chip_Clock_EnablePLL(CGU_USB_PLL);

	/* Wait for PLL lock */
//	while (!(Chip_Clock_GetPLLStatus(CGU_USB_PLL) & CGU_PLL_LOCKED)) {}


	/* Setup USB0 base clock as clock out from USB PLL */
//	Chip_Clock_SetBaseClock( CLK_BASE_USB0, CLKIN_USBPLL, true, true);

	/* enable USB main clock */
//	Chip_Clock_EnableBaseClock(CLK_BASE_USB0);
//	Chip_Clock_EnableOpts(CLK_MX_USB0, true, true, 1);
	/* enable USB0 phy */
//	Chip_CREG_EnableUSB0Phy();
// is actually        LPC_CREG->CREG0 &= ~(1 << 5);
	//usb_phy_enable();


	/* Init USB API structure */
	g_pUsbApi = (const USBD_API_T *) LPC_ROM_API->usbdApiBase;

	/* initialize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB_BASE;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;
	usb_param.max_num_ep = 2;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) USB_DeviceDescriptor;
	desc.string_desc = (uint8_t *) USB_StringDescriptor;

#define USE_USB0
#ifdef USE_USB0
	desc.high_speed_desc = USB_HsConfigDescriptor;
	desc.full_speed_desc = USB_FsConfigDescriptor;
	desc.device_qualifier = (uint8_t *) USB_DeviceQualifier;
#else
	/* Note, to pass USBCV test full-speed only devices should have both
	   descriptor arrays point to same location and device_qualifier set to 0.
	 */
	desc.high_speed_desc = USB_FsConfigDescriptor;
	desc.full_speed_desc = USB_FsConfigDescriptor;
	desc.device_qualifier = 0;
#endif

	/* USB Initialization */
	ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {
		lcdPrint("hw_init=ok");lcdNl();lcdDisplay();

		/*	WORKAROUND for artf45032 ROM driver BUG:
		    Due to a race condition there is the chance that a second NAK event will
		    occur before the default endpoint0 handler has completed its preparation
		    of the DMA engine for the first NAK event. This can cause certain fields
		    in the DMA descriptors to be in an invalid state when the USB controller
		    reads them, thereby causing a hang.
		 */
		pCtrl = (USB_CORE_CTRL_T *) g_hUsb;	/* convert the handle to control structure */
		g_Ep0BaseHdlr = pCtrl->ep_event_hdlr[0];/* retrieve the default EP0_OUT handler */
		pCtrl->ep_event_hdlr[0] = EP0_patch;/* set our patch routine as EP0_OUT handler */

		ret = mscDisk_init(g_hUsb, &desc, &usb_param);
		if (ret == LPC_OK) {
			lcdPrint("mscDisk=ok");lcdNl();lcdDisplay();
			/*  enable USB interrrupts */
			nvic_enable_irq(NVIC_USB0_IRQ);
//			NVIC_EnableIRQ(LPC_USB_IRQ);
			/* now connect */
			USBD_API->hw->Connect(g_hUsb, 1);
		}
	}

	lcdPrint("done");lcdNl();lcdDisplay();
	while (1) {
		/* Sleep until next IRQ happens */
		__WFI();
	}
}
