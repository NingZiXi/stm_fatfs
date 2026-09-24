# stm_fatfs 板级示例

示例入口展示把已经创建的 `sd_handle_t` 或 `flash_handle_t` 注册为 FatFs 物理盘、挂载并读写一个文件。`example_mount_and_rw()` 用于 SD 卡，`example_mount_and_rw_flash()` 用于 NOR Flash。它需要宿主工程提供 `ff.h`、`main.h`、FatFs 配置、SD 设备初始化和 HAL tick；不会自动格式化介质，也不会自动加入 `stm_fatfs` 库目标。NOR Flash 桥接按 512 字节逻辑扇区执行读改擦写，应用仍需自行确认分区和文件系统布局。

首次使用空白介质时，由应用在确认测试区域和备份策略后显式调用 `f_mkfs`，然后再挂载。卸载前必须关闭全部文件，注销盘号前必须先卸载。
