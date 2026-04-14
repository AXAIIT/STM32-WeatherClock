# 阶段七：AHT10 温湿度传感器

> **天气时钟项目** | Stage 7 of 8

本阶段为天气时钟接入**第一个真实数据源**：通过软件 I2C 驱动 **AHT10 温湿度传感器**，采集室内实时温度与相对湿度，并将数据动态显示在 LVGL UI 界面上。同时本阶段也加入了 ESP32 WiFi 模块的通信框架，为最终接入网络天气数据铺路。

---

## 目录

- [本阶段目标](#本阶段目标)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [硬件清单](#硬件清单)
- [硬件连线（重要）](#硬件连线重要)
- [工程结构](#工程结构)
- [AHT10 传感器说明](#aht10-传感器说明)
- [核心代码说明](#核心代码说明)
- [FreeRTOS 任务设计](#freertos-任务设计)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [常见问题](#常见问题)

---

## 本阶段目标

- [x] 编写 **AHT10 软件 I2C 驱动**（PB10/PB11）
- [x] 实现温湿度数据读取与解析
- [x] 将传感器数据接入 LVGL UI 的室内温湿度显示区域
- [x] 在 FreeRTOS 任务中实现周期性传感器采集
- [x] 初步实现日期计算函数（星期计算）
- [x] 搭建 ESP32 通信协议框架（为 Stage 8 的网络天气做准备）

---

## 相比上一阶段的变化

| 对比项 | Stage 6 | Stage 7（本阶段）|
|--------|---------|-----------------|
| 温湿度数据 | UI 显示占位符 "--" | AHT10 采集真实室内温湿度 |
| 传感器驱动 | 无 | AHT10 I2C 驱动（软件模拟）|
| 星期计算 | 无 | `CalcWeekdayMonday0()` 函数 |
| ESP32 通信 | 无 | 通信框架初步搭建（`g_esp32Ctx`）|
| 新增硬件 | 无 | AHT10 传感器模块 |
| 新增文件 | — | `Hardware/AHT10/`、`Hardware/I2C/` |

---

## 硬件清单

| 元器件 | 型号 / 规格 | 数量 | 说明 |
|--------|-------------|------|------|
| 主控开发板 | STM32F407ZGT6 | 1 | Cortex-M4，168MHz |
| TFT 彩色液晶屏 | MSP3525 / MSP3526 | 1 | 继承自前序阶段 |
| 温湿度传感器 | AHT10 | 1 | I2C 接口，精度 ±0.3°C / ±2% RH |
| 调试器 | ST-Link V2 | 1 | |
| 杜邦线 | 母对母 | 若干 | |

### AHT10 传感器参数

| 参数 | 值 |
|------|---|
| 供电电压 | 1.8V ~ 3.6V（推荐 3.3V）|
| 通信接口 | I2C（标准模式 100kHz / 快速模式 400kHz）|
| I2C 地址 | 0x38（7位地址）|
| 温度量程 | -40°C ~ +85°C |
| 温度精度 | ±0.3°C |
| 湿度量程 | 0% ~ 100% RH |
| 湿度精度 | ±2% RH |
| 测量时间 | 75ms（触发测量后等待时间）|
| 封装 | LCC（3×3mm 贴片）/ 模块板 |

---

## 硬件连线（重要）

### AHT10 与 STM32 连线

```
STM32F407ZGT6          AHT10 传感器模块
┌──────────────┐        ┌─────────────────┐
│  PB10 (SCL) ─┼────────┼── SCL（I2C 时钟）│
│  PB11 (SDA) ─┼────────┼── SDA（I2C 数据）│
│  3.3V       ─┼────────┼── VCC（供电）    │
│  GND        ─┼────────┼── GND（共地）    │
└──────────────┘        └─────────────────┘
```

> **注意：** I2C 总线需要上拉电阻。AHT10 模块板通常已集成 4.7kΩ 上拉电阻。若使用裸芯片，需在 SCL 和 SDA 线上各加一个 4.7kΩ 电阻到 3.3V。

### 完整引脚汇总（本阶段新增）

| STM32 引脚 | 功能 | 连接到 | 说明 |
|-----------|------|--------|------|
| PB10 | GPIO_Output（软件 I2C SCL）| AHT10 SCL | 传感器 I2C 时钟 |
| PB11 | GPIO_OD（软件 I2C SDA）| AHT10 SDA | 传感器 I2C 数据（开漏）|

其余引脚与 Stage 3 相同（LCD SPI、触摸 I2C）。

---

## 工程结构

```
7_AHT10/
├── Hardware/
│   ├── AHT10/
│   │   ├── AHT10.c              # ★ AHT10 传感器驱动
│   │   └── AHT10.h              # 引脚定义、函数声明
│   ├── I2C/
│   │   ├── myI2C.c              # 软件 I2C 底层实现（bit-banging）
│   │   └── myI2C.h              # I2C 时序函数声明
│   ├── FreeRTOS/                # FreeRTOS 内核（同 Stage 6）
│   ├── LCD/ LCD_SPI/ TOUCH/     # 继承自前序阶段
│   ├── LED/ Usart/              # 继承自前序阶段
│   └── LVGL/                    # LVGL v8.3.11（同 Stage 6）
├── App/LVGL_myGui/              # GUI Guider 生成 UI（同 Stage 5）
└── User/
    ├── main.c                   # 任务创建 + AHT10 初始化 + ESP32 框架
    └── stm32f4xx_it.c
```

---

## AHT10 传感器说明

### 通信协议

AHT10 使用 I2C 协议，操作流程如下：

```
上电 → 等待 40ms → 发送初始化命令 → 等待 → 触发测量 → 等待 75ms → 读取 6 字节数据 → 解析
```

**命令字节：**

| 命令 | 字节序列 | 说明 |
|------|---------|------|
| 初始化 | `0xE1, 0x08, 0x00` | 校准使能 |
| 触发测量 | `0xAC, 0x33, 0x00` | 开始测量 |
| 软复位 | `0xBA` | 重置传感器 |

**读取 6 字节数据格式：**

```
字节0：状态字节（Bit7=忙标志，Bit3=校准标志）
字节1：湿度数据 [19:12]
字节2：湿度数据 [11:4]
字节3：湿度数据 [3:0] | 温度数据 [19:16]
字节4：温度数据 [15:8]
字节5：温度数据 [7:0]
```

**数据解析公式：**

```c
// 湿度（原始 20bit 值 → 百分比）
uint32_t raw_humi = (byte1 << 12) | (byte2 << 4) | (byte3 >> 4);
float humidity = (float)raw_humi / 1048576.0f * 100.0f;

// 温度（原始 20bit 值 → 摄氏度）
uint32_t raw_temp = ((byte3 & 0x0F) << 16) | (byte4 << 8) | byte5;
float temperature = (float)raw_temp / 1048576.0f * 200.0f - 50.0f;
```

### 驱动 API

```c
// 头文件：Hardware/AHT10/AHT10.h

// 引脚定义
#define AHT10_SCL_SDA_GPIO_CLK    RCC_AHB1Periph_GPIOB
#define AHT10_SCL_SDA_GPIO_PORT   GPIOB
#define AHT10_SCL_GPIO_PIN        GPIO_Pin_10   // PB10 → SCL
#define AHT10_SDA_GPIO_PIN        GPIO_Pin_11   // PB11 → SDA

// 函数接口
uint8_t AHT10_Init(void);
// 返回值：0=成功，非0=失败（传感器无响应或校准失败）

uint8_t AHT10_ReadData(float *humidity, float *temperature);
// 参数：humidity（湿度，%RH），temperature（温度，°C）
// 返回值：0=读取成功，非0=读取失败

void Display_Temp_Humidity(void);
// 读取传感器数据并刷新 LVGL UI 中的室内温湿度显示
```

---

## 核心代码说明

### 软件 I2C 驱动（`Hardware/I2C/myI2C.c`）

采用 GPIO bit-banging 方式模拟 I2C 时序，适合引脚灵活分配：

```c
void I2C_Start(void);          // 起始信号
void I2C_Stop(void);           // 停止信号
void I2C_SendByte(uint8_t byte); // 发送 1 字节
uint8_t I2C_ReadByte(uint8_t ack); // 接收 1 字节（ack=1发ACK，ack=0发NACK）
uint8_t I2C_WaitAck(void);     // 等待应答
```

> AHT10 的触摸 I2C（PB0/PF11）和传感器 I2C（PB10/PB11）使用**两套独立的软件 I2C**，互不干扰。

### AHT10 初始化流程（`Hardware/AHT10/AHT10.c`）

```c
uint8_t AHT10_Init(void)
{
    // 1. 初始化 I2C GPIO（PB10 推挽输出，PB11 开漏输出）
    AHT10_GPIO_Init();

    // 2. 上电延时 40ms（传感器稳定时间）
    AHT10_DelayMs(40);

    // 3. 发送初始化命令（0xE1 0x08 0x00）
    AHT10_WriteCommand3(0xE1, 0x08, 0x00);
    AHT10_DelayMs(10);

    // 4. 检查状态字节：Bit3（校准位）应为 1
    uint8_t status = AHT10_CheckStatus();
    if ((status & 0x08) == 0) return 1; // 校准失败

    return 0; // 初始化成功
}
```

### LVGL UI 更新

```c
// 更新室内温湿度显示（在 FreeRTOS 任务中调用）
static void UpdateIndoorTempHumi(float temp, float humidity)
{
    // 更新温度标签（如 "23.5"）
    lv_label_set_text_fmt(guider_ui.screen_1_indoor_temp, "%.1f", temp);

    // 更新湿度标签（如 "65"）
    lv_label_set_text_fmt(guider_ui.screen_1_indoor_humi, "%d", (int)humidity);
}

// 更新室外天气（来自 ESP32 网络数据，本阶段为占位实现）
static void UpdateOutdoorTempHumi(int temp, int humidity,
                                   int todayMin, int todayMax)
{
    lv_label_set_text_fmt(guider_ui.screen_1_outdoor_temp, "%d", temp);
    lv_label_set_text_fmt(guider_ui.screen_1_outdoor_humi, "%d%%", humidity);
    lv_label_set_text_fmt(guider_ui.screen_1_temp_low,  "%d°", todayMin);
    lv_label_set_text_fmt(guider_ui.screen_1_temp_high, "%d°", todayMax);
}
```

### 星期计算函数

```c
// 给定年月日，计算星期几（0=周一，6=周日，Zeller 公式）
int CalcWeekdayMonday0(int year, int month, int day)
{
    if (month < 3) { month += 12; year--; }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13*(month+1))/5 + k + k/4 + j/4 - 2*j) % 7;
    // h: 1=周日, 2=周一, ..., 7=周六
    int weekday = ((h + 5) % 7); // 转换为 0=周一, 6=周日
    return weekday;
}
```

---

## FreeRTOS 任务设计

本阶段 LVGL 任务扩展为处理传感器数据更新：

```c
static void LVGL_Task(void *pvParameters)
{
    uint32_t sensor_tick = 0;

    AHT10_Init(); // 传感器初始化

    while(1)
    {
        uint32_t wait_ms = lv_timer_handler();

        // 每 2000ms 采集一次温湿度
        sensor_tick += (wait_ms < 1 ? 1 : wait_ms > 20 ? 20 : wait_ms);
        if (sensor_tick >= 2000) {
            sensor_tick = 0;
            float temp, humi;
            if (AHT10_ReadData(&humi, &temp) == 0) {
                UpdateIndoorTempHumi(temp, humi);
            }
        }

        if(wait_ms < 1)       wait_ms = 1;
        else if(wait_ms > 20) wait_ms = 20;
        vTaskDelay(pdMS_TO_TICKS(wait_ms));
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

---

## 如何编译与烧录

1. 按照连线表将 AHT10 传感器接到 PB10（SCL）和 PB11（SDA）
2. 确认 AHT10 模块 VCC 接 3.3V，GND 共地
3. 打开 Keil 工程，编译并烧录

---

## 运行现象

- 屏幕显示天气时钟 UI
- **室内温湿度区域每 2 秒刷新一次**，显示 AHT10 测量的真实温度（°C）和相对湿度（%RH）
- 星期显示区域根据系统时间计算（本阶段时间来源为初始值，下一阶段 ESP32 同步网络时间）
- 室外天气区域仍为占位符（Stage 8 接入 ESP32 后填充）

---

## 常见问题

**Q: AHT10 初始化失败（返回非 0）？**
- 检查 PB10/PB11 是否正确连接
- 用示波器或逻辑分析仪检查 I2C 波形
- AHT10 模块 VCC 必须为 1.8V~3.6V，**不可接 5V**
- 检查是否有上拉电阻（3.3kΩ 或 4.7kΩ 到 VCC）

**Q: 温度读数异常（如 -50°C 或超过 100°C）？**
- 确认数据解析中的位移操作是否正确
- 读取后检查状态字节 Bit7（忙标志）是否为 0（测量完成）

**Q: 触摸 I2C（PB0/PF11）与 AHT10 I2C（PB10/PB11）冲突？**
- 两套 I2C 使用完全独立的 GPIO 和软件实现，不会冲突
- 确认两套 I2C 的初始化函数分别调用

---

*上一阶段：[Stage 6 - FreeRTOS 实时操作系统](../6_LVGL8.3_STM32F407ZGT6_Gui_Guider_FreeRTOS/README.md)*  
*下一阶段：[Stage 8 - MicroSD 卡 + FatFS 文件系统](../8_Micro_SD/README.md)*
