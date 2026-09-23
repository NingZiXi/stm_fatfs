#include "stm_fatfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL:%d:%s\n",__LINE__,#x); return 1; } } while (0)
#define SECTORS 8192U
static unsigned char media[SECTORS * 512U];
static stm_err_t initialize(void *p) { (void)p; return STM_OK; }
static stm_err_t status(void *p) { (void)p; return STM_OK; }
static stm_err_t read_blocks(void *p,void *data,uint64_t sector,uint32_t count) { (void)p; if(sector+count>SECTORS)return STM_ERR_OUT_OF_RANGE; memcpy(data,media+sector*512U,(size_t)count*512U); return STM_OK; }
static stm_err_t write_blocks(void *p,const void *data,uint64_t sector,uint32_t count) { (void)p; if(sector+count>SECTORS)return STM_ERR_OUT_OF_RANGE; memcpy(media+sector*512U,data,(size_t)count*512U); return STM_OK; }
static stm_err_t sync(void *p) { (void)p; return STM_OK; }
static stm_err_t geometry(void *p,uint64_t *count,uint32_t *size,uint32_t *block,uint8_t *protect) { (void)p; *count=SECTORS;*size=512;*block=8;*protect=0;return STM_OK; }
static const fatfs_disk_ops_t ops={initialize,status,read_blocks,write_blocks,sync,geometry,NULL};
int main(void) {
    memset(media,0xFF,sizeof media);
    BYTE pdrv=0xFF; fatfs_disk_handle_t disk=NULL;
    fatfs_disk_config_t config={&ops,NULL};
    CHECK(fatfs_disk_register(&config,&pdrv,&disk)==STM_OK && pdrv==0);
    FATFS fs={0}; BYTE work[4096];
    CHECK(f_mount(&fs,"0:",1)==FR_NO_FILESYSTEM);
    MKFS_PARM parm={FM_FAT,0,0,0,0};
    CHECK(f_mkfs("0:",&parm,work,sizeof work)==FR_OK);
    CHECK(f_mount(&fs,"0:",1)==FR_OK);
    FIL file={0}; const char message[]="FatFs glue works\n"; char actual[sizeof message]={0}; UINT n=0;
    CHECK(f_open(&file,"0:/test.txt",FA_CREATE_ALWAYS|FA_WRITE|FA_READ)==FR_OK);
    CHECK(f_write(&file,message,sizeof message,&n)==FR_OK && n==sizeof message);
    CHECK(f_sync(&file)==FR_OK && f_lseek(&file,0)==FR_OK);
    CHECK(f_read(&file,actual,sizeof actual,&n)==FR_OK && n==sizeof actual && memcmp(actual,message,sizeof actual)==0);
    CHECK(f_close(&file)==FR_OK && f_mount(NULL,"0:",0)==FR_OK);
    CHECK(fatfs_disk_unregister(&disk)==STM_OK && !disk);
    return 0;
}
