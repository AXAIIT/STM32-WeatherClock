# 阶段二：定时器中断 + USART 串口调试

> **天气时钟项目** | Stage 2 of 8

在 Stage 1 工程模板的基础上，本阶段新增两项关键外设驱动：**TIM6 基本定时器**（1ms 周期中断）和 **USART1 串口**（115200bps 调试输出），为后续所有阶段的时序控制和运行状态监控奠定基础。

---

## 目录

- [本阶段目标](#本阶段目标)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [硬件清单](#硬件清单)
- [硬件连线](#硬件连线)
- [工程结构](#工程结构)
- [核心功能说明](#核心功能说明)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [常见问题](#常见问题)

---

## 本阶段目标

- [x] 配置 **TIM6 基本定时器**，产生精确 1ms 周期中断
- [x] 配置 **USART1** 串口（PA9/PA10，115200bps，8N1）
- [x] 重定向 `printf` 到串口，方便调试输出
- [x] 上电后通过串口打印系统时钟信息（PLL、HCLK、PCLK1、PCLK2、TIM6 时钟频率）
- [x] 用定时器中断驱动 LED 精确闪烁（取代简单的 delay_ms 阻塞方案）

---

## 相比上一阶段的变化

| 对比项 | Stage 1 | Stage 2（本阶段）|
|--------|---------|-----------------|
| LED 控制方式 | `delay_ms` 阻塞等待 | TIM6 中断计数，非阻塞翻转 |
| 调试手段 | 无（仅观察 LED）| USART1 串口 + printf |
| 定时精度 | SysTick 阻塞延时 | TIM6 硬件中断，1ms 精度 |
| 新增文件 | — | `Hardware/usart/Usart.c/h`、`System/LCD_Timer/LCD_Timer.c/h` |

---

## 硬件清单

| 元器件 | 型号 / 规格 | 数量 | 说明 |
|--------|-------------|------|------|
| 主控开发板 | STM32F407ZGT6 | 1 | Cortex-M4，168MHz |
| USB-TTL 串口模块 | CH340 / CP2102 / FT232 | 1 | 用于接收串口调试信息 |
| LED | 板载 PC13 | 1 | 状态指示 |
| 调试器 | ST-Link V2 | 1 | 程序烧录 |
| 杜邦线 | 母对母 | 若干 | 串口连线 |

---

## 硬件连线

### USART1 串口（连接 USB-TTL 模块）

```
STM32F407ZGT6          USB-TTL 模块
┌──────────────┐        ┌─────────────┐
│  PA9  (TX) ──┼────────┼── RX        │
│  PA10 (RX) ──┼────────┼── TX        │
│  GND       ──┼────────┼── GND       │
└──────────────┘        └─────────────┘
```

> **注意：** TX 接 RX，RX 接 TX（交叉连接）。USB-TTL 模块的 VCC 无需连接（开发板已有独立供电）。

### 引脚复用功能

| STM32 引脚 | 复用功能 | 说明 |
|-----------|---------|------|
| PA9 | AF7（USART1_TX）| 串口发送 |
| PA10 | AF7（USART1_RX）| 串口接收 |
| PC13 | GPIO_Output | 板载 LED |

---

## 工程结构

```
2_stm32f407_Timer_Usart/
├── App/
│   ├── delay.c / delay.h      # SysTick 延时（继承自 Stage 1）
│   └── led.c / led.h          # LED 驱动（继承自 Stage 1）
├── Hardware/
│   └── usart/
│       ├── Usart.c            # USART1 初始化 + fputc 重定向
│       └── Usart.h            # 接口声明
├── System/
│   └── LCD_Timer/
│       ├── LCD_Timer.c        # TIM6 定时器初始化（1ms 中断）
│       └── LCD_Timer.h        # 接口声明
├── Library/                   # STM32F4xx 标准外设库
├── Start/                     # 启动文件
└── User/
    ├── main.c                 # 主程序（打印时钟信息，定时翻转 LED）
    ├── stm32f4xx_it.c         # 中断服务函数（TIM6 ISR）
    └── system_stm32f4xx.c    # 系统时钟 168MHz
```

---

## 核心功能说明

### 1. USART1 串口驱动（`Hardware/usart/Usart.c`）

**初始化参数：**

| 参数 | 值 |
|------|---|
| 波特率 | 115200 bps |
| 数据位 | 8 位 |
| 停止位 | 1 位 |
| 校验位 | 无 |
| 流控制 | 无 |
| GPIO 模式 | 复用推挽（AF7） |

**printf 重定向：**

```c
// 将 C 标准库的 fputc 重定向到 USART1
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}
```

> 重定向后可直接使用 `printf()`，输出内容从 PA9 发出，通过 USB-TTL 模块在 PC 串口助手中显示。

### 2. TIM6 定时器（`System/LCD_Timer/LCD_Timer.c`）

TIM6 是 STM32F407 上的基本定时器（无 PWM/捕获功能），仅用于产生精确的定时中断。

**定时器配置：**

```c
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

// APB1 总线时钟 = 42MHz，定时器时钟 = 42×2 = 84MHz
TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;    // 84MHz / 84 = 1MHz（1μs/计数）
TIM_TimeBaseStructure.TIM_Period    = 1000 - 1;  // 1MHz / 1000 = 1kHz（1ms 溢出）
TIM_TimeBaseStructure.TIM_ClockDivision = 0;
TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
```

**中断优先级：**
- 抢占优先级（Preemption Priority）：0
- 子优先级（Sub Priority）：0

**中断服务函数（`User/stm32f4xx_it.c`）：**

```c
void TIM6_DAC_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
    {
        // 在此处可进行 1ms 周期任务（计时、LED 翻转等）
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    }
}
```

### 3. 主程序（`User/main.c`）

上电后通过串口打印系统时钟诊断信息，然后进入 LED 定时翻转循环：

```
系统时钟信息（115200 波特率，串口助手可查看）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PLL  Clock: 168000000 Hz   (168 MHz)
HCLK Clock: 168000000 Hz   (168 MHz)
PCLK1 Clock: 42000000 Hz   (42 MHz)
PCLK2 Clock: 84000000 Hz   (84 MHz)
TIM6 Clock:  84000000 Hz   (84 MHz)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM 5.x |
| 固件库 | STM32F4xx SPL V1.8.0 |
| 串口工具 | SSCOM、PuTTY、串口调试助手等 |
| 波特率 | **115200** bps |

---

## 如何编译与烧录

1. 打开 `2_stm32f407_Timer_Usart/` 下的 Keil 工程（`.uvprojx`）
2. 编译（F7），确认 0 错误
3. 连接 ST-Link，烧录（F8）
4. 将 PA9 → USB-TTL 的 RX，PA10 → USB-TTL 的 TX，GND 共地
5. 打开串口助手，选择正确 COM 口，波特率 **115200**，按复位键
6. 观察串口输出的时钟信息

---

## 运行现象

1. **串口输出**：上电后约 100ms 内，串口助手收到系统时钟频率信息
2. **LED 闪烁**：PC13 LED 以定时器中断驱动，精确 1Hz 闪烁（1ms 中断计满 1000 次翻转）

---

## 常见问题

**Q: 串口无输出？**
- 检查 TX/RX 是否交叉连接（PA9 → RX，PA10 → TX）
- 确认波特率为 115200
- 检查 USB-TTL 模块驱动是否安装（CH340/CP2102 需安装驱动）
- Keil 工程是否勾选了 `Use MicroLIB`（microlib 支持 printf 重定向）

**Q: TIM6 中断不触发？**
- 检查 NVIC 是否开启了 TIM6 全局中断
- 确认 `TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE)` 已调用
- 确认 `TIM_Cmd(TIM6, ENABLE)` 已启动定时器

---

*上一阶段：[Stage 1 - STM32F407 工程模板](../1_stm32f407Template/README.md)*  
*下一阶段：[Stage 3 - SPI + TFT 彩色液晶屏驱动](../3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md)*
