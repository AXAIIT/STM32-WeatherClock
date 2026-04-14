# 阶段八：MicroSD 卡 + FatFS 文件系统

> **天气时钟项目** | Stage 8 of 8 | **最终完整版本**

本阶段是天气时钟项目的最终阶段，在前七个阶段的基础上新增 **MicroSD 卡**支持（通过 FatFS 文件系统），同时完成 **ESP32 WiFi 通信协议**的全面集成，实现网络实时天气数据的获取与显示。至此，天气时钟项目所有功能模块全部到位。

---

## 目录

- [项目最终功能概览](#项目最终功能概览)
- [本阶段目标](#本阶段目标)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [完整硬件清单](#完整硬件清单)
- [完整硬件连线](#完整硬件连线)
- [工程结构](#工程结构)
- [MicroSD 与 FatFS 说明](#microsd-与-fatfs-说明)
- [ESP32 通信协议说明](#esp32-通信协议说明)
- [UI 模块化设计](#ui-模块化设计)
- [FreeRTOS 任务设计（最终版）](#freertos-任务设计最终版)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [整体项目回顾](#整体项目回顾)
- [常见问题](#常见问题)

---

## 项目最终功能概览

```
天气时钟 最终版功能
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📺 显示：3.5寸 320×480 TFT 彩色触摸屏（MSP3525/MSP3526）
🎨 UI框架：LVGL v8.3.11（GUI Guider 设计）
🕐 时间：实时时钟（由 ESP32 网络时间同步）
🌡️ 室内：AHT10 传感器（温度 ±0.3°C，湿度 ±2%RH）
🌤️ 室外：ESP32 获取网络天气数据（温度、湿度、天气状况）
📅 日期：年月日 + 星期（中文显示）
📶 WiFi：连接状态实时显示（图标动效）
💾 存储：MicroSD 卡（FatFS，字体/图片资源存储）
⚡ 系统：FreeRTOS 多任务（LVGL 任务 + 通信任务）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 本阶段目标

- [x] 集成 **MicroSD 卡驱动**（SPI 接口）
- [x] 集成 **FatFS** 文件系统（块级 I/O 到文件 I/O 的抽象层）
- [x] 实现 SD 卡字体文件读取（从 SD 卡加载 LVGL 字体，减少 Flash 占用）
- [x] 完成 **ESP32 通信协议**完整实现（WiFi 状态、网络时间、天气数据）
- [x] 实现 UI 模块化设计（`weather_display`、`ui_env_display`、`ui_time_display`）
- [x] 30 秒定时更新网络天气数据
- [x] WiFi 断线重连机制与 UI 状态更新

---

## 相比上一阶段的变化

| 对比项 | Stage 7 | Stage 8（本阶段）|
|--------|---------|-----------------|
| 存储 | 无（所有资源在 Flash 中）| MicroSD 卡 + FatFS |
| 网络天气 | 占位符，框架初步 | ESP32 完整通信，真实天气数据 |
| 时间同步 | 无（静态初始值）| ESP32 获取 NTP 网络时间 |
| UI 组织 | 所有更新在 LVGL 任务中 | 独立 UI 模块（weather/time/env）|
| 新增硬件 | — | MicroSD 卡槽模块 |
| 新增文件 | — | `Hardware/Micro_SD/`、`Hardware/FATFS/` |

---

## 完整硬件清单

| 元器件 | 型号 / 规格 | 数量 | 说明 |
|--------|-------------|------|------|
| 主控开发板 | STM32F407ZGT6 | 1 | Cortex-M4，168MHz，1MB Flash，192KB SRAM |
| TFT 彩色液晶屏 | MSP3525 / MSP3526 | 1 | 3.5寸，320×480，SPI 接口 |
| 温湿度传感器 | AHT10 | 1 | I2C 接口，室内环境监测 |
| WiFi 模块 | ESP32（或 ESP8266）| 1 | UART 接口，运行 AT 固件，获取天气/时间 |
| MicroSD 卡槽模块 | 通用 SPI MicroSD 模块 | 1 | FAT32 格式，存储字体/图片 |
| MicroSD 卡 | 任意品牌，≥1GB | 1 | 格式化为 FAT32 |
| 调试器 | ST-Link V2 | 1 | 程序烧录 |
| 杜邦线 | 母对母 | 若干 | |

---

## 完整硬件连线

### MicroSD 卡槽模块（SPI 接口）

```
STM32F407ZGT6          MicroSD 卡槽模块
┌──────────────┐        ┌──────────────────┐
│  PA5  (SCK) ─┼────────┼── SCK（SPI时钟）  │  SPI2 或 SPI3（与 LCD 不同总线）
│  PA6  (MISO)─┼────────┼── MISO（数据输出）│
│  PA7  (MOSI)─┼────────┼── MOSI（数据输入）│
│  PA4  (CS)  ─┼────────┼── CS（片选）      │
│  3.3V       ─┼────────┼── VCC（3.3V供电） │  注意：SD 卡不可接 5V
│  GND        ─┼────────┼── GND             │
└──────────────┘        └──────────────────┘
```

> **重要：** MicroSD 卡工作电压为 3.3V，绝对不可接 5V。若 SD 卡模块有 5V→3.3V 电平转换，可接 5V 供电，但信号线仍需为 3.3V 电平。

### ESP32 WiFi 模块（UART 接口）

```
STM32F407ZGT6          ESP32 模块
┌──────────────┐        ┌──────────────────┐
│  PC6  (TX2) ─┼────────┼── RX（接收）      │  USART6（或 USART2）
│  PC7  (RX2) ─┼────────┼── TX（发送）      │
│  GND        ─┼────────┼── GND             │
└──────────────┘        └──────────────────┘
```

> ESP32 需要单独供电（3.3V，峰值电流约 500mA），建议使用开发板上的 3.3V LDO 或独立供电，不要从 STM32 调试接口供电。

### 完整引脚汇总表

| STM32 引脚 | 功能 | 连接到 | 说明 |
|-----------|------|--------|------|
| PB3 | SPI1_SCK | LCD SCK | 液晶屏 SPI 时钟 |
| PB4 | SPI1_MISO | LCD MISO | 液晶屏 SPI 输入 |
| PB5 | SPI1_MOSI | LCD MOSI | 液晶屏 SPI 输出 |
| PB12/13/14/15 | GPIO | LCD RST/LED/RS/CS | 液晶屏控制 |
| PB0/PF11 | 软件 I2C | FT6336 SCL/SDA | 触摸屏 I2C |
| PB1/PC5 | GPIO | FT6336 INT/RST | 触摸中断/复位 |
| PB10/PB11 | 软件 I2C | AHT10 SCL/SDA | 温湿度传感器 I2C |
| PA5/PA6/PA7 | SPI SCK/MISO/MOSI | SD 卡 SPI | SD 卡 SPI 接口 |
| PA4 | GPIO_Output | SD 卡 CS | SD 卡片选 |
| PC6 | USART6_TX | ESP32 RX | WiFi 模块串口发送 |
| PC7 | USART6_RX | ESP32 TX | WiFi 模块串口接收 |
| PA9/PA10 | USART1_TX/RX | USB-TTL | 调试串口（可选）|
| PC13 | GPIO_Output | 板载 LED | 系统状态指示 |

---

## 工程结构

```
8_Micro_SD/
├── Hardware/
│   ├── Micro_SD/                          # ★ 新增：SD 卡驱动层
│   │   ├── fatfs_diskio_bridge.c / .h     # FatFS DiskIO 接口桥接
│   │   ├── sd_fatfs_port.c / .h           # SD 底层操作（块读写）
│   │   ├── sd_font_storage_fatfs.c / .h   # 从 SD 卡读取 LVGL 字体文件
│   │   └── sd_port_user_template.c / .h   # 用户 SD 配置模板
│   ├── FATFS/                             # ★ 新增：FatFS 文件系统
│   │   ├── ff.c / ff.h                    # FatFS 核心实现（Chan's FatFS）
│   │   ├── ffconf.h                       # FatFS 配置（扇区大小、卷数等）
│   │   ├── diskio.c / diskio.h            # 磁盘 I/O 接口定义
│   │   └── ffsystem.c                     # OS 依赖（FreeRTOS 互斥锁）
│   ├── AHT10/ I2C/                        # 温湿度传感器（同 Stage 7）
│   ├── FreeRTOS/                          # FreeRTOS 内核（同 Stage 6）
│   ├── LCD/ LCD_SPI/ TOUCH/ LED/ Usart/   # 继承自前序阶段
│   └── LVGL/                              # LVGL v8.3.11
├── App/
│   └── LVGL_myGui/
│       └── src/
│           ├── custom/
│           │   ├── custom.c / .h                    # ★ 自定义事件逻辑
│           │   ├── weather_display.c / .h           # ★ 天气数据显示模块
│           │   ├── ui_env_display.c / .h            # ★ 环境（温湿度）显示模块
│           │   ├── ui_time_display.c / .h           # ★ 时间日期显示模块
│           │   └── esp32_ui_controller.c / .h       # ★ ESP32 通信 + UI 联动
│           └── generated/                           # GUI Guider 生成代码（同 Stage 5）
└── User/
    ├── main.c                             # 最终主程序
    └── stm32f4xx_it.c
```

---

## MicroSD 与 FatFS 说明

### FatFS 配置（`Hardware/FATFS/ffconf.h`）

| 配置项 | 值 | 说明 |
|--------|------|------|
| `FF_FS_READONLY` | 0 | 允许读写 |
| `FF_VOLUMES` | 1 | 逻辑驱动器数量（1个 SD 卡）|
| `FF_STR_VOLUME_ID` | 0 | 不使用字符串卷 ID |
| `FF_MAX_SS` | 512 | 最大扇区大小（字节）|
| `FF_USE_MKFS` | 1 | 允许格式化 |
| `FF_USE_LFN` | 1 | 支持长文件名（需额外内存）|
| `FF_CODE_PAGE` | 936 | 简体中文代码页（GB2312）|
| `FF_FS_REENTRANT` | 1 | 多任务安全（使用 FreeRTOS 互斥锁）|

### SD 卡接口 API（`Hardware/Micro_SD/sd_fatfs_port.h`）

```c
// SD 卡底层初始化
uint8_t WeatherClock_SD_LowLevelInit(void);

// 块读取（sector 为扇区号，count 为扇区数）
uint8_t WeatherClock_SD_BlockRead(uint8_t *buffer,
                                   uint32_t sector,
                                   uint32_t count);

// 块写入
uint8_t WeatherClock_SD_BlockWrite(const uint8_t *buffer,
                                    uint32_t sector,
                                    uint32_t count);

// 获取扇区总数（用于计算 SD 卡容量）
uint8_t WeatherClock_SD_GetSectorCount(uint32_t *sector_count);

// 获取擦除块大小
uint8_t WeatherClock_SD_GetBlockSize(uint32_t *block_size);
```

### SD 卡字体文件存储（`sd_font_storage_fatfs.c`）

将 LVGL 自定义字体文件（.bin 格式）存储在 SD 卡上，运行时动态读取，大幅减少 Flash 占用：

```
SD 卡目录结构：
/fonts/
    Alatsi_75.bin          # 时间大字体
    Alatsi_50.bin          # 温湿度数字字体
    SourceHanSerifSC_30.bin # 中文字体
    WeekdayCN_30.bin       # 星期字体
/config/
    wifi.cfg               # WiFi SSID 和密码（明文，注意安全）
    weather.cfg            # 天气 API 配置
```

---

## ESP32 通信协议说明

### 通信方式

STM32 与 ESP32 通过 **USART6**（PC6/PC7）以 115200bps 进行串口通信，ESP32 运行自定义 AT 固件或 MicroPython/Arduino 程序，提供 WiFi 连接、网络时间、天气数据查询等功能。

### 自定义 AT 命令协议

```
STM32 发送命令 → ESP32 处理 → ESP32 回复数据

命令格式：AT+<CMD>[=<PARAM>]\r\n
响应格式：+<CMD>:<DATA>\r\nOK\r\n  或  ERROR\r\n
```

| 命令 | 说明 | 示例响应 |
|------|------|---------|
| `AT+GET_WIFI` | 查询 WiFi 连接状态 | `+WIFI:CONNECTED,MySSID` |
| `AT+GET_TIME` | 获取网络时间（NTP）| `+TIME:2025,9,20,14,35,22,6` |
| `AT+GET_WEATHER` | 获取当前天气 | `+WEATHER:26,72,SUNNY` |
| `AT+GET_FORECAST` | 获取天气预报 | `+FORECAST:18,28,22,30` |

### ESP32 UI 控制器（`esp32_ui_controller.c`）

```c
// 初始化 ESP32 通信上下文
void Esp32UiController_Init(Esp32Ctx_t *ctx);

// 处理来自 ESP32 的串口数据（在 LVGL 任务中周期调用）
void Esp32UiController_ProcessEsp32Protocol(void);

// 更新 WiFi 状态图标（连接/断开动效）
void Esp32UiController_UpdateWifiIndicatorUi(void);

// 发送 AT 命令到 ESP32
void Esp32_SendCmd(const char *cmd);

// 设置室外天气不可用（WiFi 断线时调用）
void UiEnvDisplay_SetOutdoorUnavailable(void);
```

### WiFi 状态管理

```c
typedef struct {
    bool     wifi_connected;        // WiFi 是否已连接
    bool     wifi_status_known;     // 是否已收到 WiFi 状态回复
    bool     wifi_icon_visible;     // WiFi 图标当前是否可见（用于闪烁动效）
    uint32_t wifi_blink_timer;      // 图标闪烁计时器（连接中时闪烁）
    uint32_t wifi_disconnect_timer; // 断线重试计时器
    uint32_t weather_update_timer;  // 天气更新计时器（30秒一次）

    // 当前天气数据
    int      outdoor_temp;
    int      outdoor_humi;
    int      today_temp_min;
    int      today_temp_max;
    char     weather_desc[16];      // 天气描述（如 "晴", "多云"）
} Esp32Ctx_t;
```

---

## UI 模块化设计

本阶段将 UI 更新逻辑拆分为独立模块，职责清晰：

| 模块 | 文件 | 职责 |
|------|------|------|
| 时间显示 | `ui_time_display.c/h` | 时、分、秒、日期、星期更新 |
| 环境显示 | `ui_env_display.c/h` | 室内温湿度（AHT10）更新，室外数据更新 |
| 天气显示 | `weather_display.c/h` | 天气图标、天气描述文字更新 |
| ESP32 控制 | `esp32_ui_controller.c/h` | 串口通信、WiFi 状态管理、数据解析 |
| 自定义事件 | `custom.c/h` | LVGL 事件回调（触摸交互等）|

---

## FreeRTOS 任务设计（最终版）

```c
// ── LVGL 任务（优先级 8/10）──────────────────────────────
static void LVGL_Task(void *pvParameters)
{
    uint32_t weather_timer = 0;
    uint32_t sensor_timer  = 0;
    uint32_t wifi_cmd_sent = 0;

    AHT10_Init();
    Esp32UiController_Init(&g_esp32Ctx);

    // 上电后请求 WiFi 状态
    Esp32_SendCmd("AT+GET_WIFI");

    while(1)
    {
        uint32_t wait_ms = lv_timer_handler();
        wait_ms = (wait_ms < 1) ? 1 : (wait_ms > 20 ? 20 : wait_ms);

        // 处理 ESP32 返回数据（解析 AT 响应，更新 UI）
        Esp32UiController_ProcessEsp32Protocol();

        // 更新 WiFi 图标状态（连接中闪烁，已连接常亮，断开隐藏）
        Esp32UiController_UpdateWifiIndicatorUi();

        // 每 2s 采集 AHT10
        sensor_timer += wait_ms;
        if (sensor_timer >= 2000) {
            sensor_timer = 0;
            float temp, humi;
            if (AHT10_ReadData(&humi, &temp) == 0)
                UiEnvDisplay_UpdateIndoor(temp, humi);
        }

        // 每 30s 更新天气
        weather_timer += wait_ms;
        if (weather_timer >= 30000) {
            weather_timer = 0;
            if (g_esp32Ctx.wifi_connected) {
                Esp32_SendCmd("AT+GET_WEATHER");
                Esp32_SendCmd("AT+GET_TIME");
            } else {
                UiEnvDisplay_SetOutdoorUnavailable();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

// ── LED 任务（优先级 1/10）────────────────────────────────
static void LED_Task(void *pvParameters)
{
    while(1) {
        LED1_Toggle;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

---

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM 5.x |
| FreeRTOS | V10.4.x |
| LVGL | v8.3.11 |
| FatFS | R0.15 （Chan's FatFS）|
| ESP32 固件 | 自定义 AT 固件 / Arduino / MicroPython |

---

## 如何编译与烧录

### STM32 端

1. 按照完整连线表完成所有硬件接线
2. 准备 MicroSD 卡（FAT32 格式，建议 ≤32GB），将字体文件放入 `/fonts/` 目录
3. 打开 Keil 工程，编译（首次编译较慢，约 3~5 分钟）
4. 烧录到 STM32F407ZGT6

### ESP32 端

1. 为 ESP32 烧录自定义 AT 固件（或 Arduino/MicroPython 程序）
2. 程序需实现：WiFi 连接、NTP 时间同步、调用天气 API（如 和风天气、心知天气等）
3. 通过 UART 接收 STM32 发来的 AT 命令，按协议格式回复数据

### 首次运行配置

如使用 SD 卡存储 WiFi 配置：
1. 在 SD 卡 `/config/wifi.cfg` 中写入 WiFi 信息（格式：`SSID=xxx\nPASS=xxx`）
2. ESP32 上电时自动读取并连接

---

## 运行现象

上电后屏幕显示天气时钟完整界面：

1. **启动阶段（0~5s）：**
   - WiFi 图标闪烁（正在连接中）
   - 时间显示 "--:--"，温湿度显示 "--"

2. **WiFi 连接成功后：**
   - WiFi 图标常亮
   - 时间从 ESP32 同步 NTP 时间，开始正常走时
   - 星期根据日期自动计算并显示（如 "星期六"）

3. **天气数据到达后：**
   - 室外温度、湿度、今日最高/最低温更新
   - 天气图标切换为对应天气（晴、多云、雨等）
   - 每 30 秒自动刷新一次

4. **AHT10 传感器（持续）：**
   - 室内温湿度每 2 秒更新一次，实时反映环境变化

5. **WiFi 断线时：**
   - WiFi 图标消失
   - 室外天气显示 "--"
   - 自动重连（间隔约 30 秒）

---

## 整体项目回顾

经过 8 个阶段的迭代开发，天气时钟项目从最基础的 LED 闪烁模板成长为一个完整的嵌入式 IoT 产品：

| 阶段 | 新增技术点 | 里程碑 |
|------|-----------|-------|
| **Stage 1** | STM32F407 工程模板、SysTick 延时、GPIO LED | 开发环境验证 |
| **Stage 2** | TIM6 定时器中断（1ms）、USART1 串口、printf | 基础外设驱动 |
| **Stage 3** | 硬件 SPI1、320×480 TFT 彩屏、FT6336 电容触摸 | 图形输出能力 |
| **Stage 4** | LVGL v8.3.11 移植（显示 + 触摸输入）| 现代 UI 框架 |
| **Stage 5** | GUI Guider 可视化设计、自定义字体、图片资源 | 完整 UI 外观 |
| **Stage 6** | FreeRTOS 多任务调度、Tick Hook | 实时操作系统 |
| **Stage 7** | AHT10 I2C 温湿度传感器、软件 I2C | 真实数据采集 |
| **Stage 8** | MicroSD + FatFS、ESP32 WiFi 通信 | 完整 IoT 系统 |

---

## 常见问题

**Q: SD 卡无法挂载（`f_mount` 返回错误）？**
- 确认 SD 卡已格式化为 FAT32（exFAT 不被标准 FatFS 支持）
- 检查 SPI CS 引脚是否正确拉低
- SD 卡初始化阶段 SPI 速度需较低（≤400kHz），初始化完成后才能提速
- 用示波器检查 SPI 波形，确认 SCK/MOSI/MISO 信号正常

**Q: ESP32 通信无响应？**
- 确认 USART6 TX/RX 交叉连接（STM32 TX → ESP32 RX，STM32 RX → ESP32 TX）
- 确认 ESP32 已正常启动（通电后等待 2~3 秒再发命令）
- 检查波特率是否一致（双方均为 115200）

**Q: 天气 API 无数据？**
- 确认 ESP32 已成功连接 WiFi（查看 `AT+GET_WIFI` 响应）
- 天气 API 通常需要注册账号获取 Key，检查 Key 是否已正确配置
- 检查 ESP32 是否能正常访问互联网（可用手机热点测试）

**Q: 内存不足（HardFault 或 LVGL 断言失败）？**
- 检查 FreeRTOS 堆（20KB）+ LVGL 堆（48KB）+ 显示缓冲（~25KB）总和是否超过 192KB SRAM
- 可适当减小 LVGL 堆（`LV_MEM_SIZE`）或显示缓冲行数

---

*上一阶段：[Stage 7 - AHT10 温湿度传感器](../7_AHT10/README.md)*  
*项目首页：[天气时钟 Weather Clock](../README.md)*
