#ifndef STM_FATFS_SD_H
#define STM_FATFS_SD_H
#include "stm_fatfs.h"
#include "stm_sd.h"
#ifdef __cplusplus
extern "C" {
#endif
fatfs_disk_config_t fatfs_disk_from_sd(sd_handle_t sd);
#ifdef __cplusplus
}
#endif
#endif
