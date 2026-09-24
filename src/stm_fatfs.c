#include "stm_fatfs.h"
#include <stdlib.h>
struct fatfs_disk_context {
    fatfs_disk_config_t cfg;
    BYTE pdrv;
    stm_err_t last_error;
    DSTATUS status;
};
static fatfs_disk_handle_t g_disks[STM_FATFS_MAX_DISKS];
static DRESULT map_result(fatfs_disk_handle_t d, stm_err_t e)
{
    d->last_error = e;
    if (e == STM_OK) return RES_OK;
    if (e == FATFS_ERR_PROTECTED) return RES_WRPRT;
    if (e == STM_ERR_INVALID_ARG || e == STM_ERR_OUT_OF_RANGE) return RES_PARERR;
    if (e == STM_ERR_INVALID_STATE || e == FATFS_ERR_NO_MEDIA) return RES_NOTRDY;
    return RES_ERROR;
}
static fatfs_disk_handle_t disk(BYTE pdrv) { return pdrv < STM_FATFS_MAX_DISKS ? g_disks[pdrv] : NULL; }
stm_err_t fatfs_disk_register(const fatfs_disk_config_t *cfg, BYTE *out_pdrv, fatfs_disk_handle_t *out)
{
    if (!cfg || !cfg->ops || !out_pdrv || !out || *out) return STM_ERR_INVALID_ARG;
    const fatfs_disk_ops_t *o = cfg->ops;
    if (!o->initialize || !o->status || !o->read || !o->write || !o->sync || !o->get_geometry) return STM_ERR_INVALID_CONFIG;
    for (BYTE i=0; i<STM_FATFS_MAX_DISKS; ++i) if (!g_disks[i]) {
        fatfs_disk_handle_t d = calloc(1, sizeof(*d));
        if (!d) return STM_ERR_NO_MEM;
        d->cfg = *cfg; d->pdrv = i; d->status = STA_NOINIT; d->last_error = STM_OK;
        g_disks[i] = d; *out_pdrv = i; *out = d; return STM_OK;
    }
    return STM_ERR_NO_MEM;
}
stm_err_t fatfs_disk_unregister(fatfs_disk_handle_t *p)
{
    if (!p) return STM_ERR_INVALID_ARG;
    if (!*p) return STM_OK;
    fatfs_disk_handle_t d=*p;
    if (g_disks[d->pdrv] != d) return STM_ERR_INVALID_CONTEXT;
    g_disks[d->pdrv]=NULL; free(d); *p=NULL; return STM_OK;
}
stm_err_t fatfs_disk_get_last_error(fatfs_disk_handle_t d, stm_err_t *e)
{ if (!d || !e) return STM_ERR_INVALID_ARG; *e=d->last_error; return STM_OK; }
stm_err_t fatfs_disk_get_status(fatfs_disk_handle_t d, DSTATUS *s)
{ if (!d || !s) return STM_ERR_INVALID_ARG; *s=d->status; return STM_OK; }
DSTATUS disk_initialize(BYTE pdrv)
{
    fatfs_disk_handle_t d=disk(pdrv); if (!d) return STA_NOINIT;
    stm_err_t e=d->cfg.ops->initialize(d->cfg.ctx);
    if (!e) {
        uint64_t count;
        uint32_t size;
        uint32_t block;
        uint8_t protect;
        e=d->cfg.ops->get_geometry(d->cfg.ctx,&count,&size,&block,&protect);
        if (!e && (size != 512U || !count || !block)) {
            e=STM_ERR_INVALID_CONFIG;
        }
        d->status=e ? STA_NOINIT : (protect ? STA_PROTECT : 0U);
    }
    d->last_error=e; return d->status;
}
DSTATUS disk_status(BYTE pdrv)
{
    fatfs_disk_handle_t d=disk(pdrv); if (!d) return STA_NOINIT;
    stm_err_t e=d->cfg.ops->status(d->cfg.ctx);
    d->last_error=e;
    if (e == STM_OK) {
        uint64_t count;
        uint32_t size;
        uint32_t block;
        uint8_t protect;
        stm_err_t geometry_error = d->cfg.ops->get_geometry(
            d->cfg.ctx, &count, &size, &block, &protect);
        if (geometry_error != STM_OK) {
            d->last_error=geometry_error;
            d->status=STA_NOINIT;
        } else {
            d->status=(DSTATUS)((d->status & STA_NOINIT) |
                                (protect ? STA_PROTECT : 0U));
        }
    } else if (e == FATFS_ERR_PROTECTED) {
        d->status=(DSTATUS)(d->status | STA_PROTECT);
    } else {
        d->status=STA_NOINIT;
    }
    return d->status;
}
DRESULT disk_read(BYTE pdrv,BYTE *buff,LBA_t sector,UINT count)
{
    fatfs_disk_handle_t d=disk(pdrv); if (!d || !buff || !count) return RES_PARERR;
    if (d->status & STA_NOINIT) return RES_NOTRDY;
    return map_result(d,d->cfg.ops->read(d->cfg.ctx,buff,(uint64_t)sector,count));
}
DRESULT disk_write(BYTE pdrv,const BYTE *buff,LBA_t sector,UINT count)
{
    fatfs_disk_handle_t d=disk(pdrv); if (!d || !buff || !count) return RES_PARERR;
    if (d->status & STA_NOINIT) return RES_NOTRDY;
    if (d->status & STA_PROTECT) return RES_WRPRT;
    return map_result(d,d->cfg.ops->write(d->cfg.ctx,buff,(uint64_t)sector,count));
}
DRESULT disk_ioctl(BYTE pdrv,BYTE cmd,void *buff)
{
    fatfs_disk_handle_t d=disk(pdrv); if (!d || (d->status & STA_NOINIT)) return RES_NOTRDY;
    uint64_t count; uint32_t size,block; uint8_t protect;
    stm_err_t e;
    switch(cmd) {
    case CTRL_SYNC: return map_result(d,d->cfg.ops->sync(d->cfg.ctx));
    case GET_SECTOR_COUNT: if(!buff) return RES_PARERR; e=d->cfg.ops->get_geometry(d->cfg.ctx,&count,&size,&block,&protect); if(e) return map_result(d,e); if(count>UINT32_MAX && sizeof(LBA_t)<8) return RES_PARERR; *(LBA_t *)buff=(LBA_t)count; return RES_OK;
    case GET_SECTOR_SIZE: if(!buff) return RES_PARERR; e=d->cfg.ops->get_geometry(d->cfg.ctx,&count,&size,&block,&protect); if(e) return map_result(d,e); *(WORD *)buff=(WORD)size; return RES_OK;
    case GET_BLOCK_SIZE: if(!buff) return RES_PARERR; e=d->cfg.ops->get_geometry(d->cfg.ctx,&count,&size,&block,&protect); if(e) return map_result(d,e); *(DWORD *)buff=(DWORD)block; return RES_OK;
    case CTRL_TRIM: if(!d->cfg.ops->trim || !buff) return RES_PARERR; { LBA_t *range=buff; e=d->cfg.ops->trim(d->cfg.ctx,(uint64_t)range[0],(uint64_t)range[1]); return map_result(d,e); }
    default: return RES_PARERR;
    }
}
