#include "stm_fatfs_sd.h"
#include "stm_sd.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

volatile stm_err_t example_result = STM_OK;

/* card must be created by the board layer before this function is called. */
stm_err_t example_mount_and_rw(sd_handle_t card)
{
    BYTE pdrv = 0xFFU;
    fatfs_disk_handle_t disk = NULL;
    fatfs_disk_config_t disk_config = fatfs_disk_from_sd(card);
    example_result = fatfs_disk_register(&disk_config, &pdrv, &disk);
    if (example_result != STM_OK) { return example_result; }

    char path[4] = {(char)('0' + pdrv), ':', '/', '\0'};
    char file_path[32] = {0};
    (void)snprintf(file_path, sizeof(file_path), "%u:/storage-test.txt", (unsigned)pdrv);
    FATFS fs = {0};
    FIL file = {0};
    const char message[] = "stm_fatfs\n";
    char readback[sizeof(message)] = {0};
    UINT count = 0U;
    FRESULT result = f_mount(&fs, path, 1U);
    if (result == FR_NO_FILESYSTEM) {
        /* Do not silently format a user card. Let the application decide. */
        example_result = STM_ERR_INVALID_STATE;
    } else if (result == FR_OK &&
               f_open(&file, file_path, FA_CREATE_ALWAYS | FA_WRITE | FA_READ) == FR_OK &&
               f_write(&file, message, sizeof(message), &count) == FR_OK &&
               count == sizeof(message) && f_sync(&file) == FR_OK &&
               f_lseek(&file, 0U) == FR_OK &&
               f_read(&file, readback, sizeof(readback), &count) == FR_OK &&
               count == sizeof(readback) && memcmp(message, readback, sizeof(message)) == 0 &&
               f_close(&file) == FR_OK) {
        example_result = STM_OK;
    } else {
        example_result = STM_ERR_IO;
        (void)f_close(&file);
    }
    (void)f_mount(NULL, path, 0U);
    (void)fatfs_disk_unregister(&disk);
    return example_result;
}
