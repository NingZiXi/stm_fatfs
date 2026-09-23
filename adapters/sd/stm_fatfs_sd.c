#include "stm_fatfs_sd.h"
static stm_err_t init(void *p) { sd_handle_t d=p; stm_err_t e=sd_status(d); return e == SD_ERR_NO_MEDIA ? FATFS_ERR_NO_MEDIA : e; }
static stm_err_t status(void *p) { stm_err_t e=sd_status((sd_handle_t)p); return e == SD_ERR_NO_MEDIA ? FATFS_ERR_NO_MEDIA : e; }
static stm_err_t read_blocks(void *p,void *data,uint64_t s,uint32_t n) { stm_err_t e=sd_read_blocks((sd_handle_t)p,data,s,n); return e == SD_ERR_NO_MEDIA ? FATFS_ERR_NO_MEDIA : e; }
static stm_err_t write_blocks(void *p,const void *data,uint64_t s,uint32_t n) { stm_err_t e=sd_write_blocks((sd_handle_t)p,data,s,n); if(e == SD_ERR_NO_MEDIA) return FATFS_ERR_NO_MEDIA; if(e == SD_ERR_PROTECTED) return FATFS_ERR_PROTECTED; return e; }
static stm_err_t sync(void *p) { return sd_sync((sd_handle_t)p); }
static stm_err_t geometry(void *p,uint64_t *count,uint32_t *size,uint32_t *block,uint8_t *protect)
{
    sd_info_t i; stm_err_t e=sd_get_info((sd_handle_t)p,&i); if(e) return e;
    if(!count || !size || !block || !protect) return STM_ERR_INVALID_ARG;
    *count=i.geometry.sector_count; *size=i.geometry.sector_size; *block=i.geometry.erase_sectors ? i.geometry.erase_sectors : 1U; *protect=i.geometry.write_protected; return STM_OK;
}
static const fatfs_disk_ops_t ops={init,status,read_blocks,write_blocks,sync,geometry,NULL};
fatfs_disk_config_t fatfs_disk_from_sd(sd_handle_t sd) { return (fatfs_disk_config_t){&ops,sd}; }
