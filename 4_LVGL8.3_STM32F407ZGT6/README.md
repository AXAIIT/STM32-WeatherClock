# 阶段四：集成 LVGL v8.3 图形框架

> **天气时钟项目** | Stage 4 of 8

本阶段将开源图形库 **LVGL（Light and Versatile Graphics Library）v8.3.11** 移植到 STM32F407ZGT6 + MSP3525 彩屏平台。LVGL 提供现代化的 UI 控件体系（按钮、标签、图表、动画等）和渲染引擎，是后续天气时钟 UI 界面的核心驱动框架。

---

## 目录

- [本阶段目标](#本阶段目标)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [LVGL 简介](#lvgl-简介)
- [硬件清单](#硬件清单)
- [硬件连线](#硬件连线)
- [工程结构](#工程结构)
- [移植要点](#移植要点)
- [LVGL 配置说明](#lvgl-配置说明)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [常见问题](#常见问题)

---

## 本阶段目标

- [x] 将 LVGL v8.3.11 源码添加到 Keil 工程
- [x] 实现 **显示驱动移植**（`lv_port_disp`）——将 LVGL 帧缓冲刷到 SPI LCD
- [x] 实现 **输入设备移植**（`lv_port_indev`）——将 FT6336 触摸坐标接入 LVGL
- [x] 配置 TIM6 中断为 LVGL 提供 1ms 系统 Tick
- [x] 主循环以 5ms 周期调用 `lv_timer_handler()`
- [x] 创建第一个 LVGL UI：按钮 + "Hello world!" 文字标签，验证框架运行正常

---

## 相比上一阶段的变化

| 对比项 | Stage 3 | Stage 4（本阶段）|
|--------|---------|-----------------|
| 图形渲染 | 自定义底层图形库（直接写像素）| LVGL v8.3 高级 UI 框架 |
| UI 控件 | 无（纯底层绘图）| 按钮、标签、图像等丰富控件 |
| 触摸处理 | 直接调用 FT6336 API | 通过 LVGL 输入设备抽象层 |
| 刷新驱动 | 手动调用 LCD_Fill 等函数 | LVGL 自动调度渲染，脏区刷新 |
| 新增文件 | — | `Hardware/LVGL/`（整个 LVGL 源码 + 移植层）|

---

## LVGL 简介

[LVGL](https://lvgl.io/) 是一个开源嵌入式图形库，特点：
- 支持多种显示接口（SPI、并口、MIPI DSI 等）
- 内置丰富控件：按钮、滑块、图表、键盘、列表、弧形进度条等
- 支持动画、透明度、字体抗锯齿
- 内存可配置，最低 32KB RAM 可运行
- 活跃社区，MIT License 开源

本项目使用版本：**LVGL v8.3.11**

---

## 硬件清单

与 Stage 3 完全相同，无新增硬件。

| 元器件 | 型号 | 数量 |
|--------|------|------|
| 主控 | STM32F407ZGT6 | 1 |
| TFT 屏 | MSP3525 / MSP3526（320×480）| 1 |
| 触摸控制器 | FT6336（屏幕内置）| 1 |
| 调试器 | ST-Link V2 | 1 |

---

## 硬件连线

与 [Stage 3](../3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md) 完全相同，请参考 Stage 3 连线图。

| STM32 引脚 | 功能 | 连接到 |
|-----------|------|--------|
| PB3/PB4/PB5 | SPI1 SCK/MISO/MOSI | LCD SPI |
| PB12/PB13/PB14/PB15 | RST/LED/RS/CS | LCD 控制 |
| PB0/PF11 | 软件 I2C SCL/SDA | FT6336 触摸 |
| PB1/PC5 | INT/RST | FT6336 触摸 |

---

## 工程结构

```
4_LVGL8.3_STM32F407ZGT6/
├── Hardware/
│   ├── LCD/                   # LCD 底层驱动（继承自 Stage 3）
│   ├── LCD_SPI/               # SPI1 驱动（继承自 Stage 3）
│   ├── TOUCH/                 # FT6336 驱动（继承自 Stage 3）
│   ├── LED/                   # LED 驱动
│   ├── Usart/                 # USART1 驱动
│   └── LVGL/
│       ├── lv_conf.h          # ★ LVGL 全局配置文件
│       ├── lvgl/              # LVGL v8.3.11 源码（src/、demos/）
│       └── examples/
│           └── porting/
│               ├── lv_port_disp.c / .h    # ★ 显示驱动移植
│               └── lv_port_indev.c / .h   # ★ 输入设备移植
├── System/LCD_Timer/          # TIM6 定时器（为 LVGL 提供 Tick）
└── User/
    ├── main.c                 # 初始化序列 + LVGL_Test() 示例
    └── stm32f4xx_it.c         # TIM6 ISR → lv_tick_inc(1)
```

---

## 移植要点

### 1. 显示驱动移植（`lv_port_disp.c`）

LVGL 需要一个"刷新回调函数"，将内部帧缓冲区（draw buffer）的像素数据传输到实际屏幕：

```c
// LVGL 显示缓冲区（双缓冲，每个缓冲区 320×20 = 6400 像素 × 2 字节 = 12.5KB）
static lv_color_t buf1[320 * 20];
static lv_color_t buf2[320 * 20];

// 刷新回调：LVGL 渲染完成后调用，将数据写入 LCD
static void disp_flush(lv_disp_drv_t *disp_drv,
                       const lv_area_t *area,
                       lv_color_t *color_p)
{
    // 设置 LCD 写入区域
    LCD_SetWindows(area->x1, area->y1, area->x2, area->y2);

    // 批量写入像素（SPI DMA 或逐字节）
    uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    for (uint32_t i = 0; i < size; i++) {
        SPI_WriteByte(color_p->full >> 8);   // 高字节
        SPI_WriteByte(color_p->full & 0xFF); // 低字节
        color_p++;
    }

    lv_disp_flush_ready(disp_drv); // 通知 LVGL 刷新完成
}
```

### 2. 输入设备移植（`lv_port_indev.c`）

```c
// 触摸读取回调：LVGL 每次 indev 扫描时调用
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    if (FT6336_Scan(&x, &y) == 1) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

### 3. LVGL Tick 配置（`stm32f4xx_it.c`）

TIM6 每 1ms 触发一次中断，调用 `lv_tick_inc()` 为 LVGL 内部计时系统提供时基：

```c
void TIM6_DAC_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET) {
        lv_tick_inc(1);   // ← LVGL 时基，每 1ms 加 1
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    }
}
```

### 4. 主循环（`User/main.c`）

```c
int main(void)
{
    delay_init(168);       // SysTick 延时初始化
    LED_Init();
    Usart_Config();        // USART1 调试串口
    LCD_Init();            // LCD 硬件初始化
    Touch_Init();          // FT6336 触摸初始化
    LCD_direction(1);      // 竖屏模式（Portrait）

    lv_init();                  // LVGL 内核初始化
    lv_port_disp_init();        // 注册显示驱动
    lv_port_indev_init();       // 注册触摸输入驱动
    LCD_Timer_Init();           // 启动 TIM6（开始为 LVGL 提供 Tick）

    LVGL_Test();           // 创建测试 UI（按钮 + 文字标签）

    while(1) {
        lv_timer_handler();     // LVGL 任务调度（每次调用约 5ms）
        delay_ms(5);
    }
}
```

---

## LVGL 配置说明（`Hardware/LVGL/lv_conf.h`）

| 配置项 | 值 | 说明 |
|--------|------|------|
| `LV_COLOR_DEPTH` | 16 | 16位 RGB565，与屏幕一致 |
| `LV_MEM_SIZE` | 48 × 1024 | LVGL 内部堆内存 48KB |
| `LV_DISP_DEF_REFR_PERIOD` | 5 ms | 显示刷新周期 |
| `LV_INDEV_DEF_READ_PERIOD` | 5 ms | 输入设备扫描周期 |
| `LV_DPI_DEF` | 130 | 屏幕 DPI（影响字体大小比例）|
| `LV_FONT_MONTSERRAT_14` | 1 | 启用内置字体 |
| `LV_USE_PERF_MONITOR` | 0 | 性能监控（调试时可开启）|

> **内存说明：** STM32F407ZGT6 共 192KB SRAM，其中 LVGL 占用 48KB，显示双缓冲约 25KB，剩余用于栈和其他变量，需合理分配。

---

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM 5.x |
| LVGL | v8.3.11 |
| 固件库 | STM32F4xx SPL V1.8.0 |

---

## 如何编译与烧录

1. 打开 Keil 工程，注意 LVGL 源码文件数量较多（约 100+ 个 .c 文件），首次编译时间较长（约 2~5 分钟）
2. 确认 Keil 编译优化等级为 **-O1** 或以上（LVGL 需要足够优化才能流畅运行）
3. 连线与 Stage 3 相同
4. 烧录后观察屏幕

---

## 运行现象

屏幕显示一个 LVGL 原生 UI 界面：
- 屏幕中央一个 **灰色按钮**，标签文字为 "Button"
- 按钮下方一行 **"Hello world!"** 文字标签
- 触摸按钮后，按钮出现按下视觉反馈（颜色变化）

这说明 LVGL 显示驱动、触摸驱动、定时器 Tick 全部工作正常。

---

## 常见问题

**Q: 编译报 "Out of memory" 或 LVGL 断言失败？**
- 增大 `LV_MEM_SIZE`，但需确认 SRAM 总量够用
- 减小显示缓冲区大小（`320 * 10` 而非 `320 * 20`）

**Q: 屏幕刷新很慢或花屏？**
- 确认 `lv_disp_flush_ready()` 在每次 `disp_flush` 回调结束时被调用
- 检查 SPI 速度是否设置为高速（`SPI_BaudRatePrescaler_2`）

**Q: 触摸坐标偏移或方向错误？**
- 在 `lv_port_indev.c` 的读取回调中对坐标进行镜像或旋转校正
- 坐标变换需与 `LCD_direction()` 设置的屏幕方向一致

---

*上一阶段：[Stage 3 - SPI + TFT 彩色液晶屏驱动](../3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md)*  
*下一阶段：[Stage 5 - GUI Guider 可视化 UI 设计](../5_LVGL8.3_STM32F407ZGT6_Gui_Guider/README.md)*
