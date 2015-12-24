/*
 * @brief Programming API used with MSC disk
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
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

#ifndef __MSC_DISK_H_
#define __MSC_DISK_H_

#include <lpcapi/usbd_rom_api.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @ingroup EXAMPLES_USBDROM_18XX43XX_MSC_RAM
 * @{
 */

/* MSC Disk Image Definitions */
/* Mass Storage Memory Layout */
#define BUFFER_BASE               0x2000C000
#define MSC_MEM_DISK_SIZE               ((uint32_t) FLASHFS_LENGTH)
#define MSC_MEM_DISK_BLOCK_SIZE         512
#define MSC_MEM_DISK_BLOCK_COUNT        (MSC_MEM_DISK_SIZE / MSC_MEM_DISK_BLOCK_SIZE)
//#define MSC_USB_DISK_BLOCK_SIZE         512

/**
 * @brief	MSC disk init routine
 * @param	hUsb		: Handle to USBD stack instance
 * @param	pDesc		: Pointer to configuration descriptor
 * @param	pUsbParam	: Pointer USB param structure returned by previous init call
 * @return	Always returns LPC_OK.
 */
ErrorCode_t mscDisk_init (USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam);

uint32_t mscDisk_minAddressWR(void);
uint32_t mscDisk_maxAddressWR(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __MSC_DISK_H_ */
