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
	int result;

#ifdef CFG_HAVE_SD
	switch (pdrv) {
	case FLASH :
#endif
		// ?
		return RES_OK;
#ifdef CFG_HAVE_SD
	case SD :
		result = MMC_disk_status();
		// translate the reslut code here
		return stat;
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
		result = MMC_disk_initialize();

		// translate the reslut code here

		return stat;
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
		// translate the arguments here

		result = MMC_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
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
		// translate the arguments here

		result = MMC_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
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
