# 天气时钟 Weather Clock

基于 **STM32F407ZGT6** 的嵌入式天气时钟项目，采用逐阶段递进的方式，从零搭建一个具备触摸屏 UI、本地温湿度采集、网络实时天气获取、MicroSD 卡存储的完整 IoT 时钟设备。

项目共分 **8 个阶段**，每个阶段有独立的可运行工程和详细文档，适合嵌入式初学者跟随学习，也适合有经验的开发者作为参考模板。

---

## 最终效果

```
┌────────────────────────────┐
│  [WiFi]                    │
│                            │
│         14:35              │
│       2025/09/20           │
│          星期六             │
│                            │
│  室内  23.5°C    65% RH    │
│                            │
│  [晴天图标]  晴转多云        │
│  今日: 18°C ～ 28°C        │
│                            │
│  室外  26°C      72% RH    │
└────────────────────────────┘
     MSP3525  320×480 TFT
```

---

## 硬件平台

| 组件 | 型号 |
|------|------|
| 主控 MCU | STM32F407ZGT6（Cortex-M4 @ 168MHz，1MB Flash，192KB SRAM）|
| 显示屏 | MSP3525 / MSP3526（3.5寸，320×480，SPI 接口，ILI9488）|
| 触摸屏 | FT6336（电容式，I2C，2 点触控，内置于屏幕）|
| 温湿度传感器 | AHT10（I2C，±0.3°C / ±2%RH）|
| WiFi 模块 | ESP32-S3（UART，获取 NTP 时间和网络天气）|
| 存储 | MicroSD 卡（FAT32，存储字体 / 配置文件）|

---

## 软件技术栈

| 技术 | 说明 |
|------|------|
| **LVGL v8.3.11** | 嵌入式图形 UI 框架 |
| **GUI Guider** | NXP LVGL 可视化设计工具 |
| **FreeRTOS V10.4** | 实时操作系统，多任务调度 |
| **FatFS R0.15** | 嵌入式 FAT 文件系统 |
| STM32F4xx SPL | STM32 标准外设库 |
| 软件 I2C | GPIO bit-banging 模拟 I2C |
| 硬件 SPI1 | 最高 42MHz，驱动 TFT 屏幕 |

---

## 项目结构

```
Weather_Clock/
├── 1_stm32f407Template/            # Stage 1：工程模板（LED + 延时）
├── 2_stm32f407_Timer_Usart/        # Stage 2：定时器 + 串口调试
├── 3_MSP3525_..._Hardware_SPI_.../ # Stage 3：硬件 SPI + TFT 彩屏 + 触摸
├── 4_LVGL8.3_STM32F407ZGT6/        # Stage 4：LVGL 图形框架移植
├── 5_LVGL8.3_..._Gui_Guider/       # Stage 5：GUI Guider 设计天气时钟 UI
├── 6_LVGL8.3_..._Gui_Guider_FreeRTOS/ # Stage 6：引入 FreeRTOS
├── 7_AHT10/                        # Stage 7：AHT10 温湿度传感器
├── 8_Micro_SD/                     # Stage 8：MicroSD + FatFS（完整版）
├── imgs/                           # 项目截图 / 实物图
└── 界面设计/                        # GUI Guider 设计源文件
```

> ESP32 端固件代码在 [`esp32-firmware`](../../tree/esp32-firmware) 分支的 `ESP32_Firmware/` 目录下。

---

## 各阶段概览

| 阶段 | 工程名 | 新增技术点 | 里程碑 |
|------|--------|-----------|-------|
| [Stage 1](1_stm32f407Template/README.md) | STM32F407 工程模板 | GPIO LED、SysTick 延时 | 开发环境验证 ✓ |
| [Stage 2](2_stm32f407_Timer_Usart/README.md) | 定时器 + 串口 | TIM6 中断（1ms）、USART1 printf | 基础外设驱动 ✓ |
| [Stage 3](3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md) | SPI 彩屏 + 触摸 | 硬件 SPI1、320×480 TFT、FT6336 电容触摸 | 图形输出 ✓ |
| [Stage 4](4_LVGL8.3_STM32F407ZGT6/README.md) | LVGL 移植 | LVGL v8.3.11、显示/触摸驱动移植 | 现代 UI 框架 ✓ |
| [Stage 5](5_LVGL8.3_STM32F407ZGT6_Gui_Guider/README.md) | GUI Guider UI | 可视化设计、自定义字体、图片资源 | 完整界面 ✓ |
| [Stage 6](6_LVGL8.3_STM32F407ZGT6_Gui_Guider_FreeRTOS/README.md) | FreeRTOS | 多任务调度、Tick Hook | 实时系统 ✓ |
| [Stage 7](7_AHT10/README.md) | AHT10 传感器 | 软件 I2C、温湿度采集 | 真实数据 ✓ |
| [Stage 8](8_Micro_SD/README.md) | MicroSD + ESP32 | FatFS 文件系统、WiFi 天气通信 | 完整 IoT ✓ |

---

## 快速开始

### 环境要求

- **Keil MDK-ARM 5.x**（需安装 STM32F4 芯片包）
- **ST-Link V2** 调试器
- 按各阶段 README 的硬件清单准备对应器件

### 选择起点

- **嵌入式新手**：从 [Stage 1](1_stm32f407Template/README.md) 开始，逐阶段学习
- **已熟悉 STM32 基础**：从 [Stage 3](3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md) 开始学习 SPI 屏幕驱动
- **学习 LVGL**：直接看 [Stage 4](4_LVGL8.3_STM32F407ZGT6/README.md) 的移植方法
- **想要完整项目**：直接使用 [Stage 8](8_Micro_SD/README.md) 的最终版本

---

## 硬件连线速查（最终版 Stage 8）

| STM32 引脚 | 连接到 | 功能 |
|-----------|--------|------|
| PB3/PB4/PB5 | LCD SCK/MISO/MOSI | SPI1 液晶屏 |
| PB12/PB13/PB14/PB15 | LCD RST/LED/RS/CS | LCD 控制信号 |
| PB0/PF11 | FT6336 SCL/SDA | 触摸屏软件 I2C |
| PB1/PC5 | FT6336 INT/RST | 触摸中断/复位 |
| PB10/PB11 | AHT10 SCL/SDA | 温湿度传感器软件 I2C |
| PA5/PA6/PA7/PA4 | SD SCK/MISO/MOSI/CS | MicroSD 卡 SPI |
| PC6/PC7 | ESP32 RX/TX | WiFi 模块 USART6 |
| PA9/PA10 | USB-TTL RX/TX | 调试串口 USART1 |
| PC13 | 板载 LED | 系统状态指示 |

---

## License

MIT License — 自由使用、修改和分发，保留原作者署名即可。

欢迎 ⭐ Star、🍴 Fork 和提 💬 Issue！
