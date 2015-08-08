/* vim: set ts=4 sw=4 expandtab: */
/*
 * @brief File contains callback to MSC driver backed by a memory disk.
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

#include <string.h>
#include "msc_usbd_cfg.h"
#include "msc_disk.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static const uint8_t g_InquiryStr[] = {'N', 'X', 'P', ' ', ' ', ' ', ' ', ' ',	   \
                                       'R', ' ', 'F', 'l', 'a', 's', 'h', ' ',     \
                                       'D', 'i', 's', 'k', ' ', ' ', ' ', ' ',     \
                                       '1', '.', '0', ' ', };
/*****************************************************************************
 * Private functions
 ****************************************************************************/

#include <rad1olib/spi-flash.h>
//#include <rad1olib/assert.h>
#define ASSERT(x) 
/* USB device mass storage class read callback routine */
volatile uint32_t min_address_wr = 0xffffffff;
volatile uint32_t max_address_wr = 0;

static void translate_rd(uint32_t offset, uint8_t * *buff_adr, uint32_t length, uint32_t hi_offset)
{
	ASSERT(hi_offset==0);
//	flash_read(offset,length,disk_buffer); *buff_adr = disk_buffer;
	flash_read(offset,length,*buff_adr);
}

/* USB device mass storage class write callback routine */
static void translate_wr(uint32_t offset, uint8_t * *buff_adr, uint32_t length, uint32_t hi_offset)
{
	ASSERT(hi_offset==0);

    if(min_address_wr > offset) {
        min_address_wr = offset;
    }

    if(max_address_wr < offset + length -1) {
        max_address_wr = offset + length - 1;
    }

	flash_random_write(offset, length, *buff_adr);
}

/* USB device mass storage class get write buffer callback routine */
static void translate_GetWrBuf(uint32_t offset, uint8_t * *buff_adr, uint32_t length, uint32_t hi_offset)
{
	ASSERT(hi_offset==0);
	; // NOP
}

/* USB device mass storage class verify callback routine */
static ErrorCode_t translate_verify(uint32_t offset, uint8_t *src, uint32_t length, uint32_t hi_offset)
{
	ASSERT(hi_offset==0);

	/* XXX 
	flash_read(offset,length,disk_buffer);
	if (memcmp((void *) disk_buffer, src, length)) {
		return ERR_FAILED;
	}
	*/

	return LPC_OK;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Memory storage based MSC_Disk init routine */
ErrorCode_t mscDisk_init(USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_MSC_INIT_PARAM_T msc_param;
	ErrorCode_t ret = LPC_OK;

	memset((void *) &msc_param, 0, sizeof(USBD_MSC_INIT_PARAM_T));
	msc_param.mem_base = pUsbParam->mem_base;
	msc_param.mem_size = pUsbParam->mem_size;
	/* mass storage paramas */
	msc_param.InquiryStr = (uint8_t *) g_InquiryStr;
	msc_param.BlockCount = MSC_MEM_DISK_BLOCK_COUNT;
	msc_param.BlockSize = MSC_MEM_DISK_BLOCK_SIZE;
	msc_param.MemorySize = MSC_MEM_DISK_SIZE;
	/* Install memory storage callback routines */
	msc_param.MSC_Write = translate_wr;
	msc_param.MSC_Read = translate_rd;
	msc_param.MSC_Verify = translate_verify;
	msc_param.MSC_GetWriteBuf = translate_GetWrBuf;
	msc_param.intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, USB_DEVICE_CLASS_STORAGE);

	ret = USBD_API->msc->init(hUsb, &msc_param);
	/* update memory variables */
	pUsbParam->mem_base = msc_param.mem_base;
	pUsbParam->mem_size = msc_param.mem_size;

	return ret;
}

uint32_t mscDisk_minAddressWR(void)
{
    return min_address_wr;
}

uint32_t mscDisk_maxAddressWR(void)
{
    return max_address_wr;
}
