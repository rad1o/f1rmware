/*
 * @brief Configuration file needed for USB ROM stack based applications.
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
#include <libopencm3/lpc43xx/memorymap.h>
#include <lpcapi/api_error.h>


#ifndef __APP_USB_CFG_H_
#define __APP_USB_CFG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** @ingroup EXAMPLES_USBDROM_18XX43XX_CDC
 * @{
 */

/* Comment below and uncomment USE_USB1 to enable USB-1 */
#define USE_USB0
/* #define USE_USB1 */

/* Manifest constants to select appropriate USB instance */
#ifdef USE_USB0
#define LPC_USB_BASE            USB0_BASE
#define LPC_USB                 LPC_USB0
#define LPC_USB_IRQ             USB0_IRQn
#define USB_IRQHandler          USB0_IRQHandler
#define USB_init_pin_clk        Chip_USB0_Init
#else
#undef USB_IRQHandler
#define LPC_USB_BASE            LPC_USB1_BASE
#define LPC_USB                 LPC_USB1
#define LPC_USB_IRQ             USB1_IRQn
#define USB_IRQHandler          USB1_IRQHandler
#define USB_init_pin_clk        Chip_USB1_Init
#endif

/* Manifest constants defining interface numbers and endpoints used by a
   particular interface in this application.
 */
#define USB_CDC_CIF_NUM         0
#define USB_CDC_DIF_NUM         1
#define USB_CDC_IN_EP           0x81
#define USB_CDC_OUT_EP          0x01
#define USB_CDC_INT_EP          0x83
#include <lpcapi/usbd_rom_api.h>

/* On LPC18xx/43xx the USB controller requires endpoint queue heads to start on
   a 4KB aligned memory. Hence the mem_base value passed to USB stack init should
   be 4KB aligned. The following manifest constants are used to define this memory.
 */
#define USB_STACK_MEM_BASE      0x20008000
#define USB_STACK_MEM_SIZE      0x00002000

/* USB descriptor arrays defined *_desc.c file */
extern const uint8_t USB_cdc_DeviceDescriptor[];
extern uint8_t USB_cdc_HsConfigDescriptor[];
extern uint8_t USB_cdc_FsConfigDescriptor[];
extern const uint8_t USB_cdc_StringDescriptor[];
extern const uint8_t USB_cdc_DeviceQualifier[];

/**
 * @brief	Find the address of interface descriptor for given class type.
 * @param	pDesc		: Pointer to configuration descriptor in which the desired class
 *			interface descriptor to be found.
 * @param	intfClass	: Interface class type to be searched.
 * @return	If found returns the address of requested interface else returns NULL.
 */
extern USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __APP_USB_CFG_H_ */
