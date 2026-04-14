# 天气图标 BIN 构建（SD-only）

本目录用于批量生成天气图标 `.bin` 文件，供固件从 SD 卡读取。

- 运行时读取路径：`S:/icon/weather/<天气码>.bin`
- 不使用回退逻辑：缺失即隐藏图标并打日志

## 文件说明

- `build_weather_icons.ps1`：构建脚本
- `build_weather_icons_by_group.ps1`：按天气类型分组批量构建（推荐，少量图标即可）
- `weather_codes.txt`：需要生成的天气码列表
- `weather_code_groups.csv`：天气码到分组映射表

## 推荐：分组图标方案（你只需准备少量图）

在 `group_icons` 目录放入下列文件（文件名必须一致，优先使用 `.png`）：

- `sunny.png`
- `cloudy.png`
- `overcast.png`
- `rain.png`
- `thunder.png`
- `sleet_hail.png`
- `snow.png`
- `fog_haze.png`
- `dust.png`
- `hot_cold.png`

若你已经有对应 `.bin`，也可放同名 `.bin`（例如 `sunny.bin`），脚本会自动兼容。

然后执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_weather_icons_by_group.ps1 -GroupIconDir .\group_icons -OutputDir .\out -Force
```

脚本会先把分组 PNG 转成 BIN（自动缩放到 `88x67`，或你指定的 `Width/Height`），再按 `weather_code_groups.csv` 复制为各天气码对应文件（例如 `104.bin`、`151.bin` 等）。

可选参数：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_weather_icons_by_group.ps1 -GroupIconDir .\group_icons -OutputDir .\out -Width 88 -Height 67 -ColorFormat RGB565A8 -Force
```

## 构建命令

```powershell
powershell -ExecutionPolicy Bypass -File .\build_weather_icons.ps1
```

可选参数：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_weather_icons.ps1 -SourceImage ".\sample.png" -Width 88 -Height 67 -ColorFormat RGB565A8 -Force
```

默认命令会自动执行：

- 从工程内 `App/LVGL_myGui/src/generated/images/_Sunny_Cloudy_alpha_88x67.c` 提取像素数据
- 生成 `template_100.bin`（LVGL v8 可读格式）
- 按 `weather_codes.txt` 批量复制为 `<天气码>.bin`

## 依赖

`build_weather_icons_by_group.ps1` 支持两种 PNG 转换方式：

- 优先使用 `lv_img_conv`
- 若环境中 `lv_img_conv` 不可用，会自动回退到脚本内置的 Windows 原生转换（`RGB565A8`）

`build_weather_icons.ps1` 依赖 `lv_img_conv` 将图片转换为 LVGL `.bin`。

```powershell
npm.cmd install -g lv_img_conv
```

如果你已有一个可用的 `.bin` 模板图标，也可直接复制批量生成：

```powershell
powershell -ExecutionPolicy Bypass -File .\build_weather_icons.ps1 -SourceImage ".\100.bin"
```

## 部署

将 `out` 目录下生成的所有 `.bin` 拷贝到：

- `S:/icon/weather/`

例如：

- `S:/icon/weather/100.bin`
- `S:/icon/weather/101.bin`
- `S:/icon/weather/104.bin`

上板日志可查看：`[UI][ICON] main ...`、`[UI][ICON] day ...`
