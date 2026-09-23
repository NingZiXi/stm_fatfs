#ifndef STM_FATFS_DISKIO_H
#define STM_FATFS_DISKIO_H
#include "ff.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef BYTE DSTATUS;
typedef enum { RES_OK = 0, RES_ERROR, RES_WRPRT, RES_NOTRDY, RES_PARERR } DRESULT;
#define STA_NOINIT 0x01U
#define STA_NODISK 0x02U
#define STA_PROTECT 0x04U
#define CTRL_SYNC 0U
#define GET_SECTOR_COUNT 1U
#define GET_SECTOR_SIZE 2U
#define GET_BLOCK_SIZE 3U
#define CTRL_TRIM 4U
DSTATUS disk_initialize(BYTE pdrv);
DSTATUS disk_status(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);
#ifdef __cplusplus
}
#endif
#endif
