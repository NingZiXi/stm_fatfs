# stm_fatfs

`stm_fatfs` 是官方 FatFs 的磁盘粘合层。它负责固定盘号注册、`diskio` 路由、几何信息和错误转换；文件、目录、挂载和格式化仍由官方 `f_*` API 完成。核心不依赖 STM32 HAL、RTOS、RTT 或日志。

当前发布版本为 **v1.0.1**，默认使用 FatFs R0.14b。组件目录中的 `fatfs` 源码是可审计的固定副本，保留 ChaN 的授权头；原创粘合代码采用本目录的 MIT 许可。应用也可以通过 `STM_FATFS_SOURCE_DIR` 提供自己的 FatFs 源码或复用已经存在的 `fatfs` target。

## 接入

```cmake
add_subdirectory(stm_common)
add_subdirectory(stm_sd)       # 只有需要 SD 桥接时才需要
add_subdirectory(stm_fatfs)
target_link_libraries(app PRIVATE stm_fatfs)
```

`stm_fatfs` 核心只有 FatFs 和磁盘回调依赖。检测到同级 `stm_sd` 或已有 `stm_sd` target 时，默认额外生成可选的 `stm_fatfs_sd`；没有 SD 时可设置 `-DSTM_FATFS_WITH_SD=OFF` 或让组件跳过该桥接 target。

```cmake
set(STM_FATFS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fatfs")
set(STM_FATFS_WITH_SD ON)
add_subdirectory(stm_fatfs)
target_link_libraries(app PRIVATE stm_fatfs_sd)
```

注册一个 SD 设备后，盘号在注销前保持不变：

```c
#include "stm_fatfs_sd.h"
#include "ff.h"

BYTE pdrv = 0xFFU;
fatfs_disk_handle_t disk = NULL;
fatfs_disk_config_t config = fatfs_disk_from_sd(card);
stm_err_t error = fatfs_disk_register(&config, &pdrv, &disk);
if (error == STM_OK) {
    FATFS fs = {0};
    FRESULT result = f_mount(&fs, "0:", 1);
    /* 首次使用时由应用明确调用 f_mkfs；组件不会自动格式化。 */
}
```

`disk_initialize/status/read/write/ioctl` 是 FatFs 的全局入口，因此注册表也是进程级的；磁盘回调上下文仍由每个绑定保存。注销前必须关闭文件并 `f_mount(NULL, "0:", 0)`，应用负责停止并发访问。无 `trim` 回调时不伪造 `CTRL_TRIM`。首版按完整物理盘映射，不在桥接层叠加分区偏移。

项目拥有 `ffconf.h` 的最终选择。组件示例配置为 512 字节扇区、读写、长文件名和可选 exFAT；RTOS 重入、时间戳和掉电一致性需要应用按实际系统补齐。`ffsystem_portable.c` 仅提供 malloc/free 和固定时间戳弱替代，带 RTC 的工程应提供自己的 `get_fattime`。

## 验证

```sh
cmake -S tests -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

主机测试使用内存磁盘执行格式化、挂载、文件写入/同步/读取、卸载和注销；它不代表 SD 卡电气时序或掉电安全。`example/` 需要应用提供 FatFs 配置和板级初始化，不自动加入库目标。
