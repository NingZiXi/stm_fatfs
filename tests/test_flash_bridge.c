#include "stm_fatfs_flash.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL:%d:%s\n",__LINE__,#x); return 1; } } while (0)
#define ERASE_BYTES 4096U
#define MEDIA_BYTES (ERASE_BYTES * 2U)
static unsigned char media[MEDIA_BYTES];
static unsigned erase_count, write_count, fail_erase;
static unsigned protected;
static int marker;

stm_err_t flash_get_info(flash_handle_t handle, flash_info_t *info)
{
    if (handle != (flash_handle_t)&marker || !info) return STM_ERR_INVALID_ARG;
    *info = (flash_info_t){.size_bytes = MEDIA_BYTES, .erase_size = ERASE_BYTES, .ready = 1U};
    return STM_OK;
}
stm_err_t flash_get_status(flash_handle_t handle, flash_status_t *state)
{
    if (handle != (flash_handle_t)&marker || !state) return STM_ERR_INVALID_ARG;
    *state = (flash_status_t){.valid_mask = FLASH_STATUS_PROTECTED,
        .flags = protected ? FLASH_STATUS_PROTECTED : 0U};
    return STM_OK;
}
stm_err_t flash_read(flash_handle_t handle, uint32_t offset, void *data, size_t size)
{
    if (handle != (flash_handle_t)&marker || !data || offset > MEDIA_BYTES || size > MEDIA_BYTES-offset)
        return STM_ERR_OUT_OF_RANGE;
    memcpy(data, media+offset, size); return STM_OK;
}
stm_err_t flash_erase(flash_handle_t handle, uint32_t offset, size_t size)
{
    if (handle != (flash_handle_t)&marker || offset%ERASE_BYTES || size != ERASE_BYTES)
        return STM_ERR_INVALID_ARG;
    if (fail_erase) return STM_ERR_IO;
    memset(media+offset, 0xFF, size); erase_count++; return STM_OK;
}
stm_err_t flash_write(flash_handle_t handle, uint32_t offset, const void *data, size_t size)
{
    if (handle != (flash_handle_t)&marker || offset > MEDIA_BYTES || size > MEDIA_BYTES-offset)
        return STM_ERR_OUT_OF_RANGE;
    memcpy(media+offset, data, size); write_count++; return STM_OK;
}

int main(void)
{
    memset(media, 0xA5, sizeof media);
    fatfs_disk_config_t config = fatfs_disk_from_flash((flash_handle_t)&marker);
    BYTE drive = 0xFF;
    fatfs_disk_handle_t disk = NULL;
    CHECK(fatfs_disk_register(&config, &drive, &disk) == STM_OK && drive == 0U);
    CHECK(disk_initialize(drive) == 0U);
    LBA_t sectors = 0;
    DWORD block = 0;
    CHECK(disk_ioctl(drive, GET_SECTOR_COUNT, &sectors) == RES_OK && sectors == 16U);
    CHECK(disk_ioctl(drive, GET_BLOCK_SIZE, &block) == RES_OK && block == 8U);
    unsigned char input[1024]; memset(input, 0x33, sizeof input);
    CHECK(disk_write(drive, input, 7U, 2U) == RES_OK);
    CHECK(erase_count == 2U && write_count == 2U);
    for (size_t i = 0; i < sizeof media; ++i)
        CHECK(media[i] == ((i >= 7U*512U && i < 9U*512U) ? 0x33 : 0xA5));
    unsigned char output[1024] = {0};
    CHECK(disk_read(drive, output, 7U, 2U) == RES_OK && !memcmp(input, output, sizeof input));
    CHECK(disk_write(drive, input, 16U, 1U) == RES_PARERR);
    protected = 1U;
    CHECK(disk_initialize(drive) == STA_PROTECT);
    CHECK(disk_status(drive) == STA_PROTECT);
    CHECK(disk_write(drive, input, 0U, 1U) == RES_WRPRT);
    protected = 0U;
    CHECK(disk_initialize(drive) == 0U);
    fail_erase = 1U;
    CHECK(disk_write(drive, input, 0U, 1U) == RES_ERROR);
    stm_err_t error = STM_OK;
    CHECK(fatfs_disk_get_last_error(disk, &error) == STM_OK && error == STM_ERR_IO);
    CHECK(fatfs_disk_unregister(&disk) == STM_OK && !disk);
    return 0;
}
