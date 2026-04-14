# 阶段一：STM32F407 工程模板

> **天气时钟项目** | Stage 1 of 8

本阶段是整个天气时钟项目的起点，搭建一个干净、可复用的 STM32F407ZGT6 裸机工程模板，验证开发环境和最基础的硬件驱动（LED + 延时）。

---

## 目录

- [项目背景](#项目背景)
- [本阶段目标](#本阶段目标)
- [硬件清单](#硬件清单)
- [硬件连线](#硬件连线)
- [工程结构](#工程结构)
- [核心功能说明](#核心功能说明)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [后续阶段预览](#后续阶段预览)

---

## 项目背景

**天气时钟**是一个基于 STM32F407ZGT6 的嵌入式综合项目，最终实现：
- 实时时钟与日期显示（星期、农历等）
- 本地温湿度采集（AHT10 传感器）
- 通过 ESP32 获取网络天气数据
- 480×320 TFT 彩色触摸屏 UI（LVGL 图形库）
- FreeRTOS 多任务管理
- MicroSD 卡存储字体与配置

本项目共分 **8 个阶段**逐步递进，本阶段（Stage 1）是最基础的一步：搭建模板工程。

---

## 本阶段目标

- [x] 搭建基于 STM32F4xx 标准外设库（SPL）的 Keil 工程模板
- [x] 实现 SysTick 精确延时（微秒 / 毫秒 / 秒级）
- [x] 实现板载 LED 的 GPIO 控制
- [x] 验证编译、烧录、运行全流程

---

## 硬件清单

| 元器件 | 型号 / 规格 | 数量 | 说明 |
|--------|-------------|------|------|
| 主控开发板 | STM32F407ZGT6 | 1 | Cortex-M4，168MHz，1MB Flash，192KB SRAM |
| LED | 板载 LED | 1 | 连接至 PC13，低电平点亮 |
| 调试器 | ST-Link V2 / DAP-Link | 1 | 用于程序下载与调试 |
| USB 线 | Micro-USB | 1 | 供电 |

---

## 硬件连线

本阶段硬件极为简单，仅使用板载 LED，无需外部接线。

```
STM32F407ZGT6 开发板
┌─────────────────────┐
│  PC13 ──── LED1(板载)│  低电平点亮
│  3.3V ──── VCC      │
│  GND  ──── GND      │
└─────────────────────┘
```

**调试器连线（ST-Link）：**

| ST-Link 引脚 | STM32 引脚 | 说明 |
|-------------|-----------|------|
| SWDIO | PA13 | 数据线 |
| SWCLK | PA14 | 时钟线 |
| GND | GND | 共地 |
| 3.3V | 3.3V | 供电（可选） |

---

## 工程结构

```
1_stm32f407Template/
├── App/
│   ├── delay.c / delay.h      # SysTick 延时驱动
│   └── led.c / led.h          # LED GPIO 驱动
├── Hardware/                  # 本阶段为空，后续阶段填充
├── Library/                   # STM32F4xx 标准外设库（SPL）
├── Start/                     # 启动文件（startup_stm32f40xx.s）& ARM CMSIS 头文件
├── System/                    # 系统时钟初始化
├── RTE/                       # Keil 运行时环境配置
└── User/
    ├── main.c                 # 主程序入口
    ├── stm32f4xx_conf.h       # SPL 外设使能配置
    ├── stm32f4xx_it.c         # 中断服务函数
    └── system_stm32f4xx.c    # 系统时钟（PLL 配置为 168MHz）
```

---

## 核心功能说明

### 1. 延时驱动（`App/delay.c`）

利用 ARM Cortex-M4 的 SysTick 定时器实现精确延时，无需占用任何片上定时器资源。

```c
void delay_us(uint32_t nus);   // 微秒延时（最大 798915μs @ 168MHz）
void delay_ms(uint16_t nms);   // 毫秒延时
void delay_s(uint16_t ns);     // 秒级延时
```

> SysTick 时钟 = HCLK / 8 = 168MHz / 8 = 21MHz，计数分辨率约 47.6ns。

### 2. LED 驱动（`App/led.c`）

```c
// 引脚定义
#define LED1_GPIO_PORT   GPIOC
#define LED1_GPIO_PIN    GPIO_Pin_13
#define LED1_GPIO_CLK    RCC_AHB1Periph_GPIOC

// 配置：推挽输出，上拉，速度 2MHz
void LED_Init(void);

// 宏操作
#define LED1_ON    GPIO_ResetBits(GPIOC, GPIO_Pin_13)   // 低电平点亮
#define LED1_OFF   GPIO_SetBits(GPIOC, GPIO_Pin_13)
#define LED1_Toggle  (GPIOC->ODR ^= GPIO_Pin_13)
```

### 3. 主程序（`User/main.c`）

```c
int main(void)
{
    LED_Init();          // 初始化 LED GPIO
    while(1)
    {
        LED1_Toggle;     // 翻转 LED
        delay_ms(1000);  // 延时 1 秒
    }
}
```

### 4. 系统时钟

通过 `system_stm32f4xx.c` 将系统时钟配置为 **168MHz**：
- 外部晶振（HSE）：8MHz
- PLL 配置：PLLM=8, PLLN=336, PLLP=2
- AHB 总线（HCLK）：168MHz
- APB1 总线（PCLK1）：42MHz（挂载 TIM2~7、USART2/3、I2C 等）
- APB2 总线（PCLK2）：84MHz（挂载 TIM1、SPI1、USART1 等）

---

## 开发环境

| 工具 | 版本 / 说明 |
|------|------------|
| IDE | Keil MDK-ARM 5.x |
| 编译器 | ARM Compiler 5（armcc）或 ARM Compiler 6（armclang）|
| 固件库 | STM32F4xx 标准外设库（SPL）V1.8.0 |
| 调试接口 | SWD（Serial Wire Debug）|
| 烧录工具 | Keil 内置 Flash 下载 / STM32CubeProgrammer |

---

## 如何编译与烧录

1. 使用 Keil MDK 打开 `1_stm32f407Template/` 目录下的 `.uvprojx` 工程文件
2. 确认目标器件为 `STM32F407ZGTx`
3. 点击 **Build**（F7）编译工程，确保 0 错误 0 警告
4. 连接 ST-Link 调试器到开发板
5. 点击 **Download**（F8）烧录程序
6. 按下复位键，观察 LED 闪烁

---

## 运行现象

- 板载 LED（PC13）以 **1Hz** 频率稳定闪烁（亮 1 秒，灭 1 秒）
- 说明工程模板、系统时钟、GPIO 配置、延时驱动均正常工作

---

## 后续阶段预览

| 阶段 | 新增内容 |
|------|---------|
| **Stage 2** | TIM6 定时器中断（1ms）+ USART1 串口调试输出 |
| **Stage 3** | 硬件 SPI1 + 320×480 TFT 彩屏驱动 + 电容触摸 FT6336 |
| **Stage 4** | 集成 LVGL v8.3 图形库 |
| **Stage 5** | 使用 GUI Guider 工具设计天气时钟 UI |
| **Stage 6** | 引入 FreeRTOS 实时操作系统 |
| **Stage 7** | AHT10 温湿度传感器驱动 |
| **Stage 8** | MicroSD 卡 + FatFS 文件系统 |

---

## License

本项目以学习交流为目的开源，欢迎 Star、Fork 与提 Issue。
