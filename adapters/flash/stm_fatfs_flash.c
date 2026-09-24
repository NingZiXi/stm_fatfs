#include "stm_fatfs_flash.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_DISK_SECTOR_SIZE 512U

static stm_err_t map_flash_error(stm_err_t error)
{
    if (error == FLASH_ERR_PROTECTED) {
        return FATFS_ERR_PROTECTED;
    }
    return error;
}

static stm_err_t flash_geometry(flash_handle_t flash, flash_info_t *info)
{
    if (!flash || !info) {
        return STM_ERR_INVALID_ARG;
    }
    stm_err_t error = flash_get_info(flash, info);
    if (error != STM_OK) {
        return error;
    }
    if (!info->ready || info->size_bytes == 0U || info->erase_size == 0U ||
        info->erase_size % FLASH_DISK_SECTOR_SIZE != 0U ||
        info->size_bytes % FLASH_DISK_SECTOR_SIZE != 0U) {
        return STM_ERR_INVALID_CONFIG;
    }
    return STM_OK;
}

static stm_err_t initialize(void *ctx)
{
    flash_info_t info;
    return map_flash_error(flash_geometry((flash_handle_t)ctx, &info));
}

static stm_err_t status(void *ctx)
{
    flash_info_t info;
    stm_err_t error = flash_geometry((flash_handle_t)ctx, &info);
    if (error != STM_OK) {
        return map_flash_error(error);
    }
    flash_status_t state;
    error = flash_get_status((flash_handle_t)ctx, &state);
    if (error != STM_OK) {
        return map_flash_error(error);
    }
    if ((state.valid_mask & FLASH_STATUS_PROTECTED) &&
        (state.flags & FLASH_STATUS_PROTECTED)) {
        return FATFS_ERR_PROTECTED;
    }
    return STM_OK;
}

static stm_err_t read_blocks(void *ctx, void *data, uint64_t sector, uint32_t count)
{
    if (!ctx || (count != 0U && !data) || sector > UINT32_MAX / FLASH_DISK_SECTOR_SIZE ||
        count > (UINT32_MAX / FLASH_DISK_SECTOR_SIZE)) {
        return STM_ERR_INVALID_ARG;
    }
    flash_info_t info;
    stm_err_t error = flash_geometry((flash_handle_t)ctx, &info);
    if (error != STM_OK) {
        return map_flash_error(error);
    }
    if (sector > info.size_bytes / FLASH_DISK_SECTOR_SIZE ||
        count > info.size_bytes / FLASH_DISK_SECTOR_SIZE - sector) {
        return STM_ERR_OUT_OF_RANGE;
    }
    if (count == 0U) {
        return STM_OK;
    }
    return map_flash_error(flash_read((flash_handle_t)ctx,
                                      (uint32_t)(sector * FLASH_DISK_SECTOR_SIZE), data,
                                      (size_t)count * FLASH_DISK_SECTOR_SIZE));
}

static stm_err_t write_blocks(void *ctx, const void *data, uint64_t sector, uint32_t count)
{
    if (!ctx || (count != 0U && !data) || sector > UINT32_MAX / FLASH_DISK_SECTOR_SIZE) {
        return STM_ERR_INVALID_ARG;
    }
    flash_handle_t flash = (flash_handle_t)ctx;
    flash_info_t info;
    stm_err_t error = flash_geometry(flash, &info);
    if (error != STM_OK) {
        return map_flash_error(error);
    }
    const uint64_t sector_count = info.size_bytes / FLASH_DISK_SECTOR_SIZE;
    if (sector > sector_count || count > sector_count - sector) {
        return STM_ERR_OUT_OF_RANGE;
    }
    if (count == 0U) {
        return STM_OK;
    }

    uint8_t *scratch = (uint8_t *)malloc(info.erase_size);
    if (!scratch) {
        return STM_ERR_NO_MEM;
    }
    const uint64_t first_byte = sector * FLASH_DISK_SECTOR_SIZE;
    const uint64_t last_byte = first_byte + (uint64_t)count * FLASH_DISK_SECTOR_SIZE;
    const uint64_t erase_size = info.erase_size;
    uint64_t erase_offset = (first_byte / erase_size) * erase_size;
    const uint8_t *source = (const uint8_t *)data;

    while (erase_offset < last_byte) {
        error = flash_read(flash, (uint32_t)erase_offset, scratch, info.erase_size);
        if (error != STM_OK) {
            break;
        }
        const uint64_t copy_begin = first_byte > erase_offset ? first_byte : erase_offset;
        const uint64_t erase_end = erase_offset + erase_size;
        const uint64_t copy_end = last_byte < erase_end ? last_byte : erase_end;
        const size_t copy_offset = (size_t)(copy_begin - erase_offset);
        const size_t copy_size = (size_t)(copy_end - copy_begin);
        const size_t source_offset = (size_t)(copy_begin - first_byte);
        memcpy(scratch + copy_offset, source + source_offset, copy_size);

        error = flash_erase(flash, (uint32_t)erase_offset, info.erase_size);
        if (error == STM_OK) {
            error = flash_write(flash, (uint32_t)erase_offset, scratch, info.erase_size);
        }
        if (error != STM_OK) {
            break;
        }
        erase_offset = erase_end;
    }
    free(scratch);
    return map_flash_error(error);
}

static stm_err_t sync(void *ctx)
{
    if (!ctx) {
        return STM_ERR_INVALID_ARG;
    }
    return STM_OK;
}

static stm_err_t geometry(void *ctx, uint64_t *count, uint32_t *size,
                          uint32_t *erase_block, uint8_t *write_protected)
{
    if (!count || !size || !erase_block || !write_protected) {
        return STM_ERR_INVALID_ARG;
    }
    flash_info_t info;
    stm_err_t error = flash_geometry((flash_handle_t)ctx, &info);
    if (error != STM_OK) {
        return map_flash_error(error);
    }
    flash_status_t state;
    error = flash_get_status((flash_handle_t)ctx, &state);
    if (error != STM_OK) {
        return map_flash_error(error);
    }
    *count = info.size_bytes / FLASH_DISK_SECTOR_SIZE;
    *size = FLASH_DISK_SECTOR_SIZE;
    *erase_block = info.erase_size / FLASH_DISK_SECTOR_SIZE;
    *write_protected = (uint8_t)((state.valid_mask & FLASH_STATUS_PROTECTED) &&
                                 (state.flags & FLASH_STATUS_PROTECTED));
    return STM_OK;
}

static const fatfs_disk_ops_t ops = {
    initialize, status, read_blocks, write_blocks, sync, geometry, NULL
};

fatfs_disk_config_t fatfs_disk_from_flash(flash_handle_t flash)
{
    return (fatfs_disk_config_t){&ops, flash};
}
