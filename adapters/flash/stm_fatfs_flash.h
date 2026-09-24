#ifndef STM_FATFS_FLASH_H
#define STM_FATFS_FLASH_H

#include "stm_fatfs.h"
#include "stm_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build a FatFs disk binding for a sector-addressed NOR flash instance.
 *
 * The binding exposes 512-byte logical sectors. Writes use a read/modify/
 * erase/write cycle for each physical erase sector, so data outside the
 * requested logical sectors is preserved. The caller must serialize access
 * to the flash handle for the lifetime of the returned configuration.
 */
fatfs_disk_config_t fatfs_disk_from_flash(flash_handle_t flash);

#ifdef __cplusplus
}
#endif

#endif
