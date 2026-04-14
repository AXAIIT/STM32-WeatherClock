# 阶段三：硬件 SPI + 320×480 TFT 彩屏 + 电容触摸

> **天气时钟项目** | Stage 3 of 8

本阶段引入项目最核心的显示硬件：**MSP3525 / MSP3526 TFT 彩色液晶屏**（320×480，ILI9488 或兼容驱动 IC）及 **FT6336 电容触摸控制器**，通过硬件 SPI1 高速驱动屏幕，搭建完整的图形输出与触摸输入能力，为后续 LVGL UI 框架的接入打好硬件基础。

---

## 目录

- [本阶段目标](#本阶段目标)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [硬件清单](#硬件清单)
- [硬件连线（重要）](#硬件连线重要)
- [工程结构](#工程结构)
- [核心功能说明](#核心功能说明)
- [屏幕测试内容](#屏幕测试内容)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [常见问题](#常见问题)

---

## 本阶段目标

- [x] 配置 **SPI1 硬件接口**（PB3/PB4/PB5），主机模式，最高速度
- [x] 驱动 **320×480 TFT 彩屏**（16位 RGB565 色深）
- [x] 驱动 **FT6336 电容触摸控制器**（I2C 软件模拟，支持 2 点触控）
- [x] 实现完整的 LCD 基础图形库（填色、矩形、圆形、三角形、字符、图片）
- [x] 实现汉字显示与旋转功能
- [x] 测试触摸坐标读取

---

## 相比上一阶段的变化

| 对比项 | Stage 2 | Stage 3（本阶段）|
|--------|---------|-----------------|
| 显示输出 | 无（仅串口） | 320×480 TFT 彩屏全图形输出 |
| 用户交互 | 无 | 电容触摸屏（2 点触控）|
| 新增外设 | — | SPI1、FT6336 I2C、LCD 控制 GPIO |
| 新增文件 | — | `Hardware/LCD/`、`Hardware/LCD_SPI/`、`Hardware/TOUCH/` |
| 主程序 | 打印时钟信息 | 全屏图形测试套件 |

---

## 硬件清单

| 元器件 | 型号 / 规格 | 数量 | 说明 |
|--------|-------------|------|------|
| 主控开发板 | STM32F407ZGT6 | 1 | Cortex-M4，168MHz |
| TFT 彩色液晶屏 | MSP3525 或 MSP3526 | 1 | 3.5寸，320×480，SPI 接口，ILI9488 驱动 IC |
| 电容触摸控制器 | FT6336（屏幕内置）| 1 | I2C 接口，支持 2 点触控 |
| 调试器 | ST-Link V2 | 1 | 程序烧录 |
| 杜邦线 | 母对母 | 若干 | 连线 |

> **MSP3525 与 MSP3526 的区别：** 两款屏幕接口相同，引脚定义一致，驱动兼容，主要区别在面板厂商批次，使用同一套代码驱动。

---

## 硬件连线（重要）

### SPI 液晶屏连线

```
STM32F407ZGT6          MSP3525/MSP3526 TFT 屏
┌──────────────┐        ┌───────────────────────┐
│  PB3  (SCK) ─┼────────┼── SCK（时钟）          │
│  PB4  (MISO)─┼────────┼── MISO（主机输入）     │
│  PB5  (MOSI)─┼────────┼── MOSI（主机输出）     │
│  PB12 (RST) ─┼────────┼── RST（复位，低有效）  │
│  PB13 (LED) ─┼────────┼── LED（背光控制）      │
│  PB14 (RS)  ─┼────────┼── RS/DC（数据/命令选择）│
│  PB15 (CS)  ─┼────────┼── CS（片选，低有效）   │
│  3.3V       ─┼────────┼── VCC                  │
│  GND        ─┼────────┼── GND                  │
└──────────────┘        └───────────────────────┘
```

### 电容触摸连线（FT6336，I2C 软件模拟）

```
STM32F407ZGT6          FT6336（屏幕内置）
┌──────────────┐        ┌──────────────────┐
│  PB0  (SCL) ─┼────────┼── SCL（I2C 时钟）│
│  PF11 (SDA) ─┼────────┼── SDA（I2C 数据）│
│  PB1  (INT) ─┼────────┼── INT（触摸中断） │
│  PC5  (RST) ─┼────────┼── RST（触摸复位）│
└──────────────┘        └──────────────────┘
```

### 引脚汇总表

| STM32 引脚 | 功能 | 连接到 | 说明 |
|-----------|------|--------|------|
| PB3 | AF5（SPI1_SCK） | LCD SCK | SPI 时钟 |
| PB4 | AF5（SPI1_MISO）| LCD MISO | SPI 数据输入（屏幕→MCU）|
| PB5 | AF5（SPI1_MOSI）| LCD MOSI | SPI 数据输出（MCU→屏幕）|
| PB12 | GPIO_Output | LCD RST | 屏幕硬件复位 |
| PB13 | GPIO_Output | LCD LED | 背光开关（高电平点亮）|
| PB14 | GPIO_Output | LCD RS/DC | 高=数据，低=命令 |
| PB15 | GPIO_Output | LCD CS | 片选（低有效）|
| PB0 | GPIO_Output | FT6336 SCL | 触摸 I2C 时钟（软件模拟）|
| PF11 | GPIO_OD | FT6336 SDA | 触摸 I2C 数据（开漏，软件模拟）|
| PB1 | GPIO_Input | FT6336 INT | 触摸中断（下降沿触发）|
| PC5 | GPIO_Output | FT6336 RST | 触摸控制器复位 |

---

## 工程结构

```
3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/
├── App/
│   └── LVGL_myGui/            # GUI 资源目录（本阶段预留，下一阶段填充）
├── Hardware/
│   ├── LCD/
│   │   ├── lcd.c              # LCD 图形库（画点、线、面、字符、图片）
│   │   └── lcd.h              # LCD 宏定义（屏幕尺寸、颜色、引脚）
│   ├── LCD_SPI/
│   │   ├── LCD_SPI.c          # 硬件 SPI1 驱动（初始化、字节收发、速度设置）
│   │   └── LCD_SPI.h          # SPI 接口声明
│   └── TOUCH/
│       ├── touch.c / touch.h  # 触摸抽象层
│       ├── ft6336.c           # FT6336 驱动（I2C 读写、坐标解析）
│       └── ft6336.h           # FT6336 寄存器定义、I2C 地址
├── Library/                   # STM32F4xx SPL
├── System/LCD_Timer/          # TIM6（继承自 Stage 2）
├── App/delay、led、usart       # 继承自前序阶段
└── User/
    ├── main.c                 # 全屏图形测试套件
    └── stm32f4xx_it.c
```

---

## 核心功能说明

### 1. 硬件 SPI1 驱动（`Hardware/LCD_SPI/LCD_SPI.c`）

```c
// SPI1 初始化（主机模式，8位，CPOL高，CPHA 2边沿，MSB 优先）
void SPI1_Init(void);

// 发送/接收一个字节（全双工）
uint8_t SPI_WriteByte(uint8_t data);

// 动态调整 SPI 速度
// prescaler = SPI_BaudRatePrescaler_2  → 84MHz/2  = 42MHz（高速刷屏）
// prescaler = SPI_BaudRatePrescaler_16 → 84MHz/16 = 5.25MHz（低速初始化）
void SPI_SetSpeed(uint16_t prescaler);
```

> SPI1 挂载在 APB2 总线（84MHz），最高可达 42MHz 速率刷新屏幕，实测流畅无撕裂。

### 2. LCD 图形库（`Hardware/LCD/lcd.c`）

屏幕参数：
- 分辨率：320（宽）× 480（高）
- 色深：16bit RGB565
- 接口：SPI（单向写入为主）

主要 API：

```c
void LCD_Init(void);                               // 屏幕初始化（发送 ILI9488 命令序列）
void LCD_direction(uint8_t direction);             // 设置显示方向（0~3 四种旋转）
void LCD_Fill(uint16_t xsta, uint16_t ysta,
              uint16_t xend, uint16_t yend,
              uint16_t color);                     // 矩形区域填充颜色
void LCD_DrawPoint(uint16_t x, uint16_t y,
                   uint16_t color);                // 画点
void LCD_DrawLine(uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2,
                  uint16_t color);                 // 画线（Bresenham 算法）
void LCD_DrawRectangle(uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2,
                       uint16_t color);            // 画矩形框
void LCD_DrawCircle(uint16_t x0, uint16_t y0,
                    uint8_t r, uint16_t color);    // 画圆
void LCD_ShowChar(uint16_t x, uint16_t y,
                  uint8_t chr, uint8_t size,
                  uint8_t mode, uint16_t color);   // 显示 ASCII 字符
void LCD_ShowChinese(uint16_t x, uint16_t y,
                     uint8_t *str, uint8_t size,
                     uint16_t color);              // 显示汉字
void LCD_ShowPicture(uint16_t x, uint16_t y,
                     uint16_t length, uint16_t width,
                     const uint8_t pic[]);         // 显示图片（RGB565 数组）
```

### 3. FT6336 电容触摸驱动（`Hardware/TOUCH/ft6336.c`）

```c
// FT6336 I2C 地址
#define FT6336_ADDR_WRITE   0x70   // 写操作地址
#define FT6336_ADDR_READ    0x71   // 读操作地址

// 初始化触摸控制器（复位 → 配置 → 读取固件版本）
uint8_t FT6336_Init(void);

// 读取触摸点坐标（最多 2 点）
uint8_t FT6336_Scan(uint16_t *x, uint16_t *y);
```

触摸 I2C 采用软件模拟方式（bit-banging），避免占用硬件 I2C 资源，灵活性更高。

---

## 屏幕测试内容

`User/main.c` 中包含全面的 LCD 功能测试套件，上电后自动轮流演示：

1. **纯色填充测试** — 红、绿、蓝、白、黑全屏填充
2. **渐变矩形测试** — 绘制不同颜色的嵌套矩形
3. **圆形绘制测试** — 同心圆、填充圆
4. **三角形绘制测试**
5. **英文字符显示** — 多种字号的 ASCII 文字
6. **中文字符显示** — 汉字字模渲染
7. **图片显示测试** — RGB565 格式图片数组渲染
8. **屏幕旋转测试** — 0°/90°/180°/270° 四种方向
9. **动态数字测试** — 实时计数数字显示（验证刷新速度）
10. **触摸测试** — 触摸屏读取坐标，在对应位置画点

---

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM 5.x |
| 固件库 | STM32F4xx SPL V1.8.0 |
| 屏幕驱动 IC | ILI9488 或兼容 |
| 触摸 IC | FT6336 |

---

## 如何编译与烧录

1. 打开 Keil 工程（`.uvprojx`）
2. 按照上方**硬件连线**表格完成接线
3. 编译（F7），0 错误后烧录（F8）
4. 复位开发板，屏幕依次显示各项测试内容

---

## 运行现象

- 屏幕背光亮起
- 依次演示颜色填充、图形绘制、文字显示等测试项
- 触摸测试阶段：用手指触摸屏幕，触摸点位置出现小圆点

---

## 常见问题

**Q: 屏幕全白或全黑，无图像？**
- 检查 CS（PB15）是否拉低——SPI 通信时必须为低电平
- 检查 RST（PB12）是否正常复位（先拉低 10ms，再拉高）
- 用示波器或逻辑分析仪检查 SPI1 CLK（PB3）是否有波形
- 确认屏幕背光（PB13）已拉高

**Q: 屏幕显示颠倒或镜像？**
- 调用 `LCD_direction(direction)` 更改方向参数（0~3）

**Q: 触摸无响应？**
- 确认 SDA（PF11）引脚已配置为开漏输出（开漏模式是 I2C 标准要求）
- 检查 FT6336 复位引脚（PC5）是否正常完成复位时序
- 确认触摸 IC 的 I2C 地址为 0x38（7位地址）

---

*上一阶段：[Stage 2 - 定时器中断 + USART 串口调试](../2_stm32f407_Timer_Usart/README.md)*  
*下一阶段：[Stage 4 - LVGL 8.3 图形库集成](../4_LVGL8.3_STM32F407ZGT6/README.md)*
