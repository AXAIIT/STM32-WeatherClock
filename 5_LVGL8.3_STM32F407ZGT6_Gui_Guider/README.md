# 阶段五：使用 GUI Guider 设计天气时钟 UI

> **天气时钟项目** | Stage 5 of 8

本阶段使用 NXP 官方工具 **LVGL GUI Guider** 对天气时钟的完整界面进行可视化设计，将自动生成的 UI 代码集成到工程中，实现天气时钟的最终视觉效果（时间、日期、星期、温湿度、天气图标、WiFi 状态等 68+ 个 UI 元素）。

---

## 目录

- [本阶段目标](#本阶段目标)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [GUI Guider 简介](#gui-guider-简介)
- [UI 界面设计说明](#ui-界面设计说明)
- [硬件清单与连线](#硬件清单与连线)
- [工程结构](#工程结构)
- [代码结构说明](#代码结构说明)
- [自定义字体说明](#自定义字体说明)
- [图片资源说明](#图片资源说明)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)

---

## 本阶段目标

- [x] 使用 **LVGL GUI Guider** 工具完成天气时钟 UI 的可视化布局设计
- [x] 将 GUI Guider 自动生成的代码集成到 Keil 工程
- [x] 嵌入自定义字体（英文数字字体 + 中文字体）
- [x] 嵌入天气图标等图片资源（RGB565 数组格式）
- [x] 实现完整天气时钟界面的静态 UI 展示（数据占位，下一阶段接入真实数据）

---

## 相比上一阶段的变化

| 对比项 | Stage 4 | Stage 5（本阶段）|
|--------|---------|-----------------|
| UI 设计方式 | 手写 LVGL 代码（Hello World 示例）| GUI Guider 可视化设计 → 自动生成代码 |
| UI 元素数量 | 2 个（按钮 + 标签）| 68+ 个（完整天气时钟界面）|
| 字体 | LVGL 内置 Montserrat | 4 套自定义字体（英文 + 中文）|
| 图片资源 | 无 | 天气图标、背景图、UI 装饰图 |
| 代码组织 | main.c 手写 | 自动生成 gui_guider.c + events_init.c |

---

## GUI Guider 简介

[LVGL GUI Guider](https://www.nxp.com/design/software/development-software/gui-guider:GUI-GUIDER) 是 NXP 推出的免费 LVGL UI 设计工具：
- **拖拽式**设计控件布局，所见即所得
- 支持 LVGL v7 / v8 / v9
- 一键导出 C 代码（`gui_guider.c`、`events_init.c`）
- 集成字体转换工具（将 TTF 字体转换为 LVGL C 数组）
- 集成图片转换工具（将 PNG/JPG 转换为 RGB565 C 数组）

本项目 GUI Guider 设计文件位于根目录 `界面设计/` 文件夹（可用 GUI Guider 软件打开编辑）。

---

## UI 界面设计说明

天气时钟界面在 320×480 竖屏上的布局（单屏 `screen_1`）：

```
┌────────────────────────────┐
│  [WiFi 图标]               │  ← 右上角 WiFi 连接状态
│                            │
│        14:35               │  ← 大字体数字时间（Alatsi 75px）
│      2025/09/20            │  ← 日期（SourceHanSerifSC 17px）
│         星期六              │  ← 星期（WeekdayCN 30px）
│                            │
│  ┌──────────────────────┐  │
│  │  室内  23.5°C  65%RH │  │  ← AHT10 本地温湿度
│  └──────────────────────┘  │
│                            │
│  [天气图标]  晴转多云       │  ← 网络天气状况
│  今日: 18°C ~ 28°C         │  ← 今日最高/最低温
│                            │
│  [温度计图标] [湿度图标]    │  ← 室外气象数据
│     26°C           72%     │
└────────────────────────────┘
```

**主要 UI 元素列表：**

| 元素类型 | 说明 | 数量 |
|---------|------|------|
| `lv_img` | 背景图、天气图标、温湿度图标 | 15+ |
| `lv_label` | 时间、日期、星期、温度、湿度文字 | 20+ |
| `lv_spangroup` | 混合字体文字组（温度单位等）| 10+ |
| `lv_obj` | 容器、分割线 | 10+ |
| `lv_imgbtn` | 图片按钮 | 若干 |

---

## 硬件清单与连线

与 Stage 3/4 完全相同，无新增硬件。详见 [Stage 3 连线图](../3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md#硬件连线重要)。

---

## 工程结构

```
5_LVGL8.3_STM32F407ZGT6_Gui_Guider/
├── App/
│   └── LVGL_myGui/
│       └── src/
│           ├── custom/
│           │   ├── custom.c / custom.h       # 用户自定义逻辑（事件回调扩展）
│           │   └── lv_conf_ext.h             # LVGL 配置扩展
│           └── generated/                   # ★ GUI Guider 自动生成，勿手动修改
│               ├── gui_guider.c / .h         # UI 布局定义（所有控件创建代码）
│               ├── events_init.c / .h        # 事件绑定（控件与回调函数关联）
│               ├── guider_fonts/             # 自定义字体 C 文件
│               │   ├── font_Alatsi_Regular_25.c
│               │   ├── font_Alatsi_Regular_30.c
│               │   ├── font_Alatsi_Regular_50.c
│               │   ├── font_Alatsi_Regular_75.c
│               │   ├── font_Antonio_Regular_25.c
│               │   ├── font_SourceHanSerifSC_12.c
│               │   ├── font_SourceHanSerifSC_17.c
│               │   ├── font_SourceHanSerifSC_30.c
│               │   └── font_WeekdayCN_30.c
│               ├── guider_customer_fonts/    # 字体声明头文件
│               └── images/                  # 图片资源 C 文件（RGB565 数组）
├── Hardware/
│   ├── LCD/ LCD_SPI/ TOUCH/ LED/ Usart/     # 继承自前序阶段
│   └── LVGL/                                # LVGL v8.3.11 + 移植层
├── System/LCD_Timer/
└── User/
    ├── main.c                               # 调用 setup_ui() + events_init()
    └── stm32f4xx_it.c
```

---

## 代码结构说明

### GUI Guider 生成代码的使用方式

**`gui_guider.h`** 中定义了全局 UI 结构体，包含所有控件的句柄：

```c
typedef struct {
    lv_obj_t *screen_1;              // 主屏幕对象
    bool screen_1_del;

    // 时间显示
    lv_obj_t *screen_1_time_label;   // 大字体时间
    lv_obj_t *screen_1_date_label;   // 日期
    lv_obj_t *screen_1_week_label;   // 星期

    // 室内环境
    lv_obj_t *screen_1_indoor_temp;  // 室内温度
    lv_obj_t *screen_1_indoor_humi;  // 室内湿度

    // 天气信息
    lv_obj_t *screen_1_weather_icon; // 天气图标
    lv_obj_t *screen_1_weather_desc; // 天气描述
    lv_obj_t *screen_1_temp_high;    // 今日最高温
    lv_obj_t *screen_1_temp_low;     // 今日最低温

    // WiFi 状态
    lv_obj_t *screen_1_wifi_icon;    // WiFi 图标

    // ... 共 68+ 个控件句柄
} lv_ui;

extern lv_ui guider_ui;  // 全局 UI 实例
```

**主程序调用流程：**

```c
// main.c
lv_ui guider_ui;   // 声明全局 UI 实例

int main(void)
{
    // 硬件初始化（同 Stage 4）...

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    LCD_Timer_Init();

    setup_ui(&guider_ui);      // ← 创建所有控件、设置样式和布局
    events_init(&guider_ui);   // ← 绑定事件回调

    while(1) {
        lv_timer_handler();
        delay_ms(5);
    }
}
```

**更新 UI 数据（在 `custom.c` 中实现）：**

```c
// 更新时间显示
lv_label_set_text_fmt(guider_ui.screen_1_time_label, "%02d:%02d", hour, minute);

// 更新室内温度
lv_label_set_text_fmt(guider_ui.screen_1_indoor_temp, "%.1f°C", temperature);

// 更新 WiFi 图标可见性
lv_obj_set_style_img_opa(guider_ui.screen_1_wifi_icon,
                          wifi_connected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
```

---

## 自定义字体说明

| 字体名 | 字号 | 用途 | 文件 |
|--------|------|------|------|
| Alatsi Regular | 75px | 主时间数字（14:35）| `font_Alatsi_Regular_75.c` |
| Alatsi Regular | 50px | 室内温湿度大数字 | `font_Alatsi_Regular_50.c` |
| Alatsi Regular | 30px | 室外温度数字 | `font_Alatsi_Regular_30.c` |
| Alatsi Regular | 25px | 小数字、单位 | `font_Alatsi_Regular_25.c` |
| Antonio Regular | 25px | 温度单位符号 | `font_Antonio_Regular_25.c` |
| SourceHanSerifSC | 30px | 中文天气描述 | `font_SourceHanSerifSC_30.c` |
| SourceHanSerifSC | 17px | 日期、地区名 | `font_SourceHanSerifSC_17.c` |
| SourceHanSerifSC | 12px | 小号中文注释 | `font_SourceHanSerifSC_12.c` |
| WeekdayCN | 30px | 星期文字（周一~周日）| `font_WeekdayCN_30.c` |

> 字体 C 文件由 GUI Guider 内置的字体转换工具从 TTF 字体文件生成，仅包含用到的字符（Unicode 子集），大幅减少 Flash 占用。

---

## 图片资源说明

图片以 **RGB565 格式**存储为 C 数组，编译进 Flash：

| 图片 | 尺寸 | 说明 |
|------|------|------|
| `img_bg` | 320×480 | 主背景图 |
| `img_sunny_cloudy` | 64×64 | 晴转多云天气图标 |
| `img_celsius` | 24×24 | 摄氏度符号 |
| `img_humidity` | 24×24 | 湿度图标 |
| `img_temperature` | 24×24 | 温度计图标 |
| `img_wifi` | 24×24 | WiFi 信号图标 |
| `img_high` | 16×16 | 最高温箭头 |
| `img_low` | 16×16 | 最低温箭头 |
| `img_percentage` | 16×16 | 百分号符号图标 |

---

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM 5.x |
| LVGL | v8.3.11 |
| GUI Guider | v1.7.x（NXP 官网下载）|

---

## 如何编译与烧录

1. （可选）安装 GUI Guider，打开 `界面设计/` 文件夹中的设计文件查看或修改 UI
2. 将 GUI Guider 导出的代码替换 `App/LVGL_myGui/src/generated/` 中的文件（若有修改）
3. 打开 Keil 工程，确认所有字体 .c 文件已添加到工程
4. 编译（字体和图片数据量较大，Flash 占用约 400KB~600KB，注意 Keil 目标 Flash 设置）
5. 烧录，复位后屏幕显示完整天气时钟 UI

---

## 运行现象

屏幕显示完整的天气时钟静态界面：
- 时间区域显示占位符数字（如 "00:00"）
- 日期、星期区域显示占位文字
- 天气图标区域显示默认图标
- 温湿度区域显示 "--" 占位符

> 本阶段为纯 UI 展示，数据均为静态占位。Stage 7 接入 AHT10 传感器后将显示真实温湿度，Stage 8 接入 ESP32 网络模块后显示实时天气。

---

*上一阶段：[Stage 4 - LVGL v8.3 图形框架集成](../4_LVGL8.3_STM32F407ZGT6/README.md)*  
*下一阶段：[Stage 6 - FreeRTOS 实时操作系统](../6_LVGL8.3_STM32F407ZGT6_Gui_Guider_FreeRTOS/README.md)*
