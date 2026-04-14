# Weather Clock 字库制作（17px/30px）

本目录用于生成项目动态中文显示所需字库：

- `font_cn_17.bin`（`todayTextDay` 等 17px 文本）
- `font_cn_30.bin`（`weather_main`、`星期X` 等 30px 文本）

## 前置条件

- 已安装 Node.js（`node -v` 可用）
- 已安装字库工具：

```powershell
npm.cmd install -g lv_font_conv
```

## 生成命令

在本目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1
```

指定字体文件（推荐微软雅黑）：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1 -FontPath "C:\Windows\Fonts\simhei.ttf"
```

指定输出目录：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_lvgl_weather_fonts.ps1 -OutputDir ".\out"
```

## 输出与部署

默认输出到 `tools/font/out/`：

- `font_cn_17.bin`
- `font_cn_30.bin`

将两文件拷贝到 SD 卡，例如：

- `S:/font/font_cn_17.bin`
- `S:/font/font_cn_30.bin`

然后在固件里分别加载并绑定到对应字号控件。
