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
#include <lpcapi/msc/msc_usbd_cfg.h>
#include <lpcapi/msc/msc_disk.h>

#include <rad1olib/sdmmc.h>
#include <lpcapi/sdif_18xx_43xx.h>
#include <lpcapi/sdmmc_18xx_43xx.h>
#include <lpcapi/chip_lpc43xx.h>

#include <rad1olib/pins.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include <r0ketlib/display.h>
#include <r0ketlib/print.h>
#include <r0ketlib/itoa.h>
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static const uint8_t g_InquiryStr[] = {'R', 'A', 'D', '1', 'O', ' ', ' ', ' ',	   \
									   'S', 'D', ' ', 'M', 'M', 'C', ' ', ' ',	   \
									   'D', 'i', 's', 'k', ' ', ' ', ' ', ' ',	   \
									   '1', '.', '0', ' ', };
/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
static uint8_t *disk_buffer = (uint8_t *) BUFFER_BASE;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* USB device mass storage class read callback routine */
static void sd_rd(uint32_t offset, uint8_t * *buff_adr, uint32_t length, uint32_t hi_offset)
{
    uint8_t buf[512];
    if(((offset & 0x1FF) == 0) && ((length & 0x1FF) == 0)) {
#ifdef I_FIXED_THE_BROKEN_64BIT_OFFSETS
        const int64_t o64 = (int64_t)offset | ((int64_t)hi_offset << 32);
#else
        const int64_t o64 = (int64_t)offset;
#endif
        const uint32_t block = o64>>9;
        TOGGLE(LED2);
        Chip_SDMMC_ReadBlocks(LPC_SDMMC, *buff_adr, block, length>>9);
        TOGGLE(LED2);
        //*buff_adr = disk_buffer;
    } else {
        TOGGLE(LED4);
    }
}

/* USB device mass storage class write callback routine */
static void sd_wr(uint32_t offset, uint8_t * *buff_adr, uint32_t length, uint32_t hi_offset)
{
    if(((offset & 0x1FF) == 0) && ((length & 0x1FF) == 0)) {
#ifdef I_FIXED_THE_BROKEN_64BIT_OFFSETS
        const int64_t o64 = (int64_t)offset | ((int64_t)hi_offset << 32);
#else
        const int64_t o64 = (int64_t)offset;
#endif
        const uint32_t block = o64>>9;
        TOGGLE(LED2);
        Chip_SDMMC_WriteBlocks(LPC_SDMMC, *buff_adr, block, length>>9);
        TOGGLE(LED2);
    } else {
        TOGGLE(LED4);
    }
}

/* USB device mass storage class verify callback routine */
static ErrorCode_t sd_verify(uint32_t offset, uint8_t *src, uint32_t length, uint32_t hi_offset)
{
    uint8_t buf[512];
    if(((offset & 0x1FF) == 0) && ((length & 0x1FF) == 0)) {
#ifdef I_FIXED_THE_BROKEN_64BIT_OFFSETS
        int64_t o64 = (int64_t)offset | ((int64_t)hi_offset << 32);
#else
        int64_t o64 = (int64_t)offset;
#endif
        while(length > 0) {
            const uint32_t block = o64>>9;
            TOGGLE(LED2);
            Chip_SDMMC_ReadBlocks(LPC_SDMMC, buf, o64>>9, 1);
            TOGGLE(LED2);
            if (memcmp((void *) buf, src, 512)) {
                return ERR_FAILED;
            }
            length -= 512;
            o64 += 512;
            src += 512;
        }
        return LPC_OK;
    } else {
        return ERR_FAILED;
    }
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Memory storage based MSC_Disk init routine */
ErrorCode_t sd_msc_init(USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_MSC_INIT_PARAM_T msc_param;
	ErrorCode_t ret = LPC_OK;

	memset((void *) &msc_param, 0, sizeof(USBD_MSC_INIT_PARAM_T));
	msc_param.mem_base = pUsbParam->mem_base;
	msc_param.mem_size = pUsbParam->mem_size;
	/* mass storage paramas */
	msc_param.InquiryStr = (uint8_t *) g_InquiryStr;
	msc_param.BlockSize = 512;
    uint32_t blocks = Chip_SDMMC_GetDeviceBlocks(LPC_SDMMC);
#ifdef I_FIXED_THE_BROKEN_64BIT_OFFSETS
	msc_param.MemorySize = (blocks < (1<<23)) ? blocks<<9 : 0xFFFFFFFF;
	msc_param.MemorySize64 = 512ULL * blocks;
	msc_param.BlockCount = blocks;
#else
	msc_param.MemorySize = (blocks < (1<<23)) ? blocks<<9 : 0xFFFFFE00;
	msc_param.BlockCount = (blocks < (1<<23)) ? blocks : ((1<<23)-1);
#endif
	/* Install memory storage callback routines */
	msc_param.MSC_Write = sd_wr;
	msc_param.MSC_Read = sd_rd;
	msc_param.MSC_Verify = sd_verify;
	msc_param.MSC_GetWriteBuf = NULL;
	msc_param.intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, USB_DEVICE_CLASS_STORAGE);

	ret = USBD_API->msc->init(hUsb, &msc_param);
	/* update memory variables */
	pUsbParam->mem_base = msc_param.mem_base;
	pUsbParam->mem_size = msc_param.mem_size;

	return ret;
}
