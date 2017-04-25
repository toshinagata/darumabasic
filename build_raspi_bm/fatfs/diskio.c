/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include <stdlib.h>

#include "emmc.h"

/* Definitions of physical drive number for each drive */
#define DEV_MMC     0
#define DEV_USB     1

static struct emmc_block_dev *emmc_dev;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
	case DEV_MMC :
		if (emmc_dev == NULL)
		{
			return STA_NOINIT;
		}
		return 0;

	case DEV_USB :
	//	result = USB_disk_status();
		// translate the reslut code here
		return stat;
	}
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

	switch (pdrv) {
	case DEV_MMC :
		if (emmc_dev == NULL)
		{
		//	log_printf("%s:%d disk_initialize\n", __FILE__, __LINE__);
			if (sd_card_init((struct block_device **)&emmc_dev) != 0)
			{
				return STA_NOINIT;
			}
		}
		return 0;

	case DEV_USB :
	//	result = USB_disk_initialize();
		// translate the reslut code here
		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_MMC :
        {
            size_t buf_size = count * emmc_dev->bd.block_size;
            if (sd_read(buff, buf_size, sector) < buf_size)
            {
                return RES_ERROR;
            }
			
            return RES_OK;
        }

	case DEV_USB :
		// translate the arguments here

	//	result = USB_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_MMC :
        {
            size_t buf_size = count * emmc_dev->bd.block_size;
            if (sd_write((uint8_t *)buff, buf_size, sector) < buf_size)
            {
                return RES_ERROR;
            }
			
            return RES_OK;
        }

	case DEV_USB :
		// translate the arguments here

	//	result = USB_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_MMC :
		if (cmd == CTRL_SYNC)
		{
			return RES_OK;
		}
		if (cmd == GET_SECTOR_COUNT)
		{
			*(DWORD *)buff = emmc_dev->bd.num_blocks;
			return RES_OK;
		}
		if (cmd == GET_SECTOR_SIZE)
		{
			*(DWORD *)buff = emmc_dev->bd.block_size;
			return RES_OK;
		}
		if (cmd == GET_BLOCK_SIZE)
		{
			*(DWORD *)buff = emmc_dev->bd.block_size;
			return RES_OK;
		}
		return RES_PARERR;

	case DEV_USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}

extern void *bs_malloc(size_t size);
extern void bs_free(void *ptr);

/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/
/* If a NULL is returned, the file function fails with FR_NOT_ENOUGH_CORE.
 */

void* ff_memalloc (	/* Returns pointer to the allocated memory block */
				   UINT msize		/* Number of bytes to allocate */
				   )
{
	return bs_malloc(msize);	/* Allocate a new memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
				 void* mblock	/* Pointer to the memory block to free */
				 )
{
	bs_free(mblock);	/* Discard the memory block with POSIX API */
}
