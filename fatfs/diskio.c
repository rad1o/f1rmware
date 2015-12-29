/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include <rad1olib/spi-flash.h>

#ifdef CFG_HAVE_SD
#ifndef CORE_M4
#define CORE_M4
#endif
#include <rad1olib/sdmmc.h>
#include <lpcapi/sdif_18xx_43xx.h>
#include <lpcapi/sdmmc_18xx_43xx.h>
#include <lpcapi/chip_lpc43xx.h>
#endif // CFG_HAVE_SD

/* Definitions of physical drive number for each drive */
#define FLASH	0	/* Example: Map ATA harddisk to physical drive 0 */
#define SD		1	/* Example: Map MMC/SD card to physical drive 1 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int __attribute__ ((unused)) result;

#ifdef CFG_HAVE_SD
	switch (pdrv) {
	case FLASH :
#endif
		// ?
		return RES_OK;
#ifdef CFG_HAVE_SD
	case SD :
		result = Chip_SDMMC_GetState(LPC_SDMMC);
		//TODO: translate the reslut code here
		return RES_OK;
	}
#endif
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

#ifdef CFG_HAVE_SD
	switch (pdrv) {
	case FLASH :
#endif
		flashInit();
		return RES_OK;

#ifdef CFG_HAVE_SD
	case SD :
        sdmmc_setup();
        uint32_t res = sdmmc_acquire();

		// translate the reslut code here

		return (res == 1) ? RES_OK : RES_ERROR;
	}
#endif
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;

#ifdef CFG_HAVE_SD
	switch (pdrv) {
	case FLASH :
#endif
		flash_read(FLASHFS_OFFSET+sector*512,count*512,buff);
		return RES_OK;

#ifdef CFG_HAVE_SD
	case SD :
		result = Chip_SDMMC_ReadBlocks(LPC_SDMMC, buff, sector, count);

		return (result == count*512) ? RES_OK : RES_ERROR;
    }
#endif

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

#ifdef CFG_HAVE_SD
	switch (pdrv) {
	case FLASH :
#endif
		flash_random_write(FLASHFS_OFFSET+sector*512,count*512,buff);

		return RES_OK;

#ifdef CFG_HAVE_SD
	case SD :
		result = Chip_SDMMC_WriteBlocks(LPC_SDMMC, (uint8_t*) buff, sector, count);

		return (result == count*512) ? RES_OK : RES_ERROR;
	}
#endif

	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

#ifdef CFG_HAVE_SD
	switch (pdrv) {
	case FLASH :
#endif

		// Process of the command for the ATA drive
		if(cmd==CTRL_SYNC)
			res=RES_OK;
		return res;

#ifdef CFG_HAVE_SD
	case SD :
		// Process of the command for the MMC/SD card
		return res;
	}
#endif

	return RES_PARERR;
}
#endif
