#ifndef STM_FATFS_H
#define STM_FATFS_H
#include <stdint.h>
#include "stm_err.h"
#include "diskio.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STM_FATFS_VERSION "1.0.1"
#define STM_FATFS_MAX_DISKS 4U
#define FATFS_ERR_NO_MEDIA ((stm_err_t)0x5001)
#define FATFS_ERR_PROTECTED ((stm_err_t)0x5002)
typedef struct {
    stm_err_t (*initialize)(void *ctx);
    stm_err_t (*status)(void *ctx);
    stm_err_t (*read)(void *ctx, void *data, uint64_t sector, uint32_t count);
    stm_err_t (*write)(void *ctx, const void *data, uint64_t sector, uint32_t count);
    stm_err_t (*sync)(void *ctx);
    stm_err_t (*get_geometry)(void *ctx, uint64_t *sector_count, uint32_t *sector_size,
                              uint32_t *erase_block_sectors, uint8_t *write_protected);
    stm_err_t (*trim)(void *ctx, uint64_t first, uint64_t last);
} fatfs_disk_ops_t;
typedef struct {
    const fatfs_disk_ops_t *ops;
    void *ctx;
} fatfs_disk_config_t;
typedef struct fatfs_disk_context *fatfs_disk_handle_t;
/* Registration is process-global because FatFs diskio has no user context.
 * Registration does not initialize or format media. Delete only after f_mount(NULL).
 * The returned pdrv is stable until unregister and is the first argument in "0:". */
stm_err_t fatfs_disk_register(const fatfs_disk_config_t *config, BYTE *out_pdrv,
                              fatfs_disk_handle_t *out_handle);
stm_err_t fatfs_disk_unregister(fatfs_disk_handle_t *handle);
stm_err_t fatfs_disk_get_last_error(fatfs_disk_handle_t handle, stm_err_t *out_error);
stm_err_t fatfs_disk_get_status(fatfs_disk_handle_t handle, DSTATUS *out_status);
#ifdef __cplusplus
}
#endif
#endif
