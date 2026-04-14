# Weather Clock 字库制作（18px/24px）

本目录用于生成项目动态中文显示所需字库：

- `FONT18.BIN`（`todayTextDay` 等 18px 文本）
- `FONT24.BIN`（`weather_main` 等 24px 文本）

字库字符来源采用单一词库文件：

- `weather_phrases.txt`（天气词唯一维护入口）

构建脚本会在每次生成前自动重建：

- `chars_17.txt`
- `chars_18.txt`
- `chars_24.txt`

请不要手工长期维护 `chars_18.txt` / `chars_24.txt`，以免再次出现漏字。

## 前置条件

- 已安装 Node.js（`node -v` 可用）
- 已安装字库工具：

```powershell
npm.cmd install -g lv_font_conv
```

> 重要：本工程使用 `LVGL 8.3.x`，生成字体时必须使用 `--lv-version 8`（脚本已内置）。

> 若你看到脚本输出 `lv-version: N/A`，说明 `lv_font_conv` 版本不兼容。该情况下即使 `FONTxx.BIN` 能加载，也可能出现“有字形覆盖日志但屏幕仍乱码”的现象。

可先检查：

```powershell
lv_font_conv.cmd -h
```

输出中必须包含 `--lv-version` 选项。

## 生成命令

在本目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1
```

显式指定 LVGL 版本（推荐）：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1 -LvVersion 8
```

指定字体文件（推荐微软雅黑）：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1 -FontPath "C:\Windows\Fonts\simhei.ttf"
```

指定输出目录：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1 -OutputDir ".\out"
```

低内存设备推荐使用默认 `Bpp=2`（脚本默认），可显著降低运行时字库内存占用。

如需更高质量可显式改回 4：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1 -Bpp 4
```

如需新增天气词（例如“转雾”），只需修改 `weather_phrases.txt` 后重新执行脚本。

## 输出与部署

默认输出到 `tools/font/out/`：

- `FONT18.BIN`
- `FONT24.BIN`

脚本会输出字体文件大小；若体积异常小，会直接报错中止。

将两文件拷贝到 SD 卡，例如：

- `S:/font/FONT18.BIN`
- `S:/font/FONT24.BIN`

然后在固件里分别加载并绑定到对应字号控件。

## STM32 工程最短接入步骤

1. 在 Keil 将 `App/LVGL_myGui/src/custom/sd_font_storage_fatfs.c` 加入编译组。
2. 在 Keil 将 `App/LVGL_myGui/src/custom/fatfs_diskio_bridge.c` 与 `App/LVGL_myGui/src/custom/sd_fatfs_port.c` 加入编译组。
3. 导入 FatFs 核心文件并加入编译：`ff.c`、`ff.h`、`ffconf.h`、`diskio.h`。
4. 在 Keil 将 `User/sd_port_user_template.c` 加入编译组，并在 Target C/C++ 宏里映射你的底层函数。
5. 确认 `lv_conf.h` 中已开启 `LV_USE_FS_FATFS 1`，且盘符为 `S`。
6. SD 卡放置：`S:/font/FONT18.BIN`、`S:/font/FONT24.BIN`。

启动日志期望：不再出现 `FatFs missing`，并打印 `SD fonts loaded`。

## 宏映射示例

在 Keil `Options for Target -> C/C++ -> Define` 添加（按你的函数名替换）：

`WEATHERCLOCK_SD_PORT_INIT=MySdInit,WEATHERCLOCK_SD_PORT_READ=MySdReadBlocks,WEATHERCLOCK_SD_PORT_WRITE=MySdWriteBlocks,WEATHERCLOCK_SD_PORT_SECTOR_COUNT=MySdGetSectorCount,WEATHERCLOCK_SD_PORT_BLOCK_SIZE=MySdGetEraseBlockSize`
