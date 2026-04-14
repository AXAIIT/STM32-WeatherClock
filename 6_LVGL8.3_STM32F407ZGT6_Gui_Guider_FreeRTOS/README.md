# 阶段六：引入 FreeRTOS 实时操作系统

> **天气时钟项目** | Stage 6 of 8

本阶段在完整天气时钟 UI 的基础上，引入 **FreeRTOS** 实时操作系统，将原本在 `while(1)` 中串行执行的各项任务拆分为独立的 RTOS 任务，为后续接入传感器、网络通信等耗时操作提供多任务并发能力。

---

## 目录

- [本阶段目标](#本阶段目标)
- [为什么需要 FreeRTOS](#为什么需要-freertos)
- [相比上一阶段的变化](#相比上一阶段的变化)
- [硬件清单与连线](#硬件清单与连线)
- [工程结构](#工程结构)
- [FreeRTOS 配置说明](#freertos-配置说明)
- [任务设计](#任务设计)
- [核心代码说明](#核心代码说明)
- [开发环境](#开发环境)
- [如何编译与烧录](#如何编译与烧录)
- [运行现象](#运行现象)
- [常见问题](#常见问题)

---

## 本阶段目标

- [x] 将 **FreeRTOS** 内核源码集成到 Keil 工程
- [x] 配置 `FreeRTOSConfig.h`（任务优先级、堆大小、Tick 频率等）
- [x] 将 LVGL UI 刷新移入独立 **LVGL 任务**
- [x] 创建独立 **LED 任务**（与 UI 任务解耦）
- [x] 使用 `vApplicationTickHook` 为 LVGL 提供 Tick（替代 TIM6 中断方式）
- [x] 验证 FreeRTOS 多任务调度与 LVGL 协同工作正常

---

## 为什么需要 FreeRTOS

在没有 RTOS 的情况下（Stage 5），主循环是这样工作的：

```c
while(1) {
    lv_timer_handler();   // 处理 UI 刷新（可能耗时 5~20ms）
    delay_ms(5);          // 固定等待（浪费 CPU）
    // 无法在此期间处理传感器读取、网络通信等操作
}
```

**问题：**
- LVGL 刷新是阻塞操作，期间无法响应传感器中断
- 网络通信（ESP32 AT 指令）需要等待响应，阻塞期间 UI 会卡顿
- 随着功能增加，`while(1)` 会越来越臃肿，时序难以保证

**引入 FreeRTOS 后：**
- 每个功能模块运行在独立任务中，互不阻塞
- 高优先级任务（LVGL 刷新）保证 UI 流畅度
- 低优先级任务（传感器读取、网络通信）在 UI 空闲时运行
- `vTaskDelay` 让任务"睡眠"时 CPU 可以切换到其他任务，不浪费算力

---

## 相比上一阶段的变化

| 对比项 | Stage 5 | Stage 6（本阶段）|
|--------|---------|-----------------|
| 运行模式 | 裸机 `while(1)` 轮询 | FreeRTOS 多任务调度 |
| LVGL Tick | TIM6 中断回调 `lv_tick_inc(1)` | FreeRTOS Tick Hook `vApplicationTickHook` |
| LVGL 刷新 | 主循环 `lv_timer_handler()` + `delay_ms(5)` | 独立 LVGL 任务，动态延时（1~20ms）|
| LED 控制 | 主循环内 | 独立 LED 任务（`vTaskDelay` 非阻塞）|
| 新增文件 | — | `Hardware/FreeRTOS/`（FreeRTOS 内核 + 配置）|

---

## 硬件清单与连线

与 Stage 3/4/5 完全相同，无新增硬件。详见 [Stage 3 连线图](../3_MSP3525_MSP3526_STM32F407ZGT6_Hardware_SPI_Displace_Screen/README.md#硬件连线重要)。

---

## 工程结构

```
6_LVGL8.3_STM32F407ZGT6_Gui_Guider_FreeRTOS/
├── Hardware/
│   ├── FreeRTOS/                          # ★ 新增
│   │   ├── FreeRTOSConfig.h               # FreeRTOS 全局配置
│   │   ├── include/                       # FreeRTOS 头文件
│   │   ├── portable/
│   │   │   ├── GCC/ARM_CM4F/             # Cortex-M4F 移植层
│   │   │   └── MemMang/heap_4.c          # 内存管理（heap_4 方案）
│   │   ├── tasks.c                        # 任务调度核心
│   │   ├── queue.c                        # 队列
│   │   ├── timers.c                       # 软件定时器
│   │   ├── semphr.h                       # 信号量
│   │   └── ...                            # 其他内核文件
│   ├── LCD/ LCD_SPI/ TOUCH/ LED/ Usart/   # 继承自前序阶段
│   └── LVGL/                              # LVGL v8.3.11 + 移植层
├── App/LVGL_myGui/                        # GUI Guider 生成代码（同 Stage 5）
└── User/
    ├── main.c                             # ★ FreeRTOS 任务创建 + 调度启动
    └── stm32f4xx_it.c                     # 中断（FreeRTOS SysTick 接管）
```

---

## FreeRTOS 配置说明（`FreeRTOSConfig.h`）

| 配置项 | 值 | 说明 |
|--------|------|------|
| `configUSE_PREEMPTION` | 1 | 使用抢占式调度器 |
| `configCPU_CLOCK_HZ` | `SystemCoreClock`（168MHz）| CPU 主频 |
| `configTICK_RATE_HZ` | 1000 | Tick 频率 1kHz（1ms 精度）|
| `configMAX_PRIORITIES` | 10 | 最大任务优先级数（0=最低，9=最高）|
| `configMINIMAL_STACK_SIZE` | 128 | 最小任务栈（128 × 4 = 512 字节）|
| `configTOTAL_HEAP_SIZE` | 20 × 1024 | FreeRTOS 堆 20KB |
| `configUSE_TIMERS` | 1 | 启用软件定时器 |
| `configUSE_MUTEXES` | 1 | 启用互斥锁 |
| `configUSE_COUNTING_SEMAPHORES` | 1 | 启用计数信号量 |
| `configUSE_TICK_HOOK` | 1 | 启用 Tick Hook（用于 LVGL tick）|
| `configUSE_STATS_FORMATTING_FUNCTIONS` | 1 | 启用任务状态统计 |

### 内存分配说明

```
STM32F407ZGT6 SRAM 分配（192KB）：
┌─────────────────────────────────────┐
│  FreeRTOS Heap（heap_4.c）  20 KB   │  任务栈、队列、信号量等动态分配
│  LVGL 内存池                48 KB   │  lv_conf.h: LV_MEM_SIZE
│  LVGL 显示双缓冲           ~25 KB   │  320×20×2 × 2 个缓冲区
│  全局变量 + 栈              ~20 KB  │  编译器静态分配
│  剩余可用                  ~79 KB   │
└─────────────────────────────────────┘
合计：~192 KB
```

---

## 任务设计

本阶段共创建 **2 个用户任务**：

### LVGL 任务（`LVGL_Task`）

```c
static void LVGL_Task(void *pvParameters)
{
    while(1)
    {
        uint32_t wait_ms = lv_timer_handler(); // 执行 LVGL 渲染和事件处理

        // 动态调整任务等待时间（让 CPU 有机会处理其他任务）
        if(wait_ms < 1)       wait_ms = 1;
        else if(wait_ms > 20) wait_ms = 20;

        vTaskDelay(pdMS_TO_TICKS(wait_ms)); // 非阻塞等待
    }
}
// 栈大小：1024 words（4KB）
// 优先级：configMAX_PRIORITIES - 2（= 8，高优先级保证 UI 流畅）
```

### LED 任务（`LED_Task`）

```c
static void LED_Task(void *pvParameters)
{
    while(1)
    {
        LED1_Toggle;
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms 非阻塞等待
    }
}
// 栈大小：128 words（512 字节）
// 优先级：1（最低用户优先级）
```

### FreeRTOS Tick Hook（替代 TIM6 中断）

```c
// 此函数在每次 FreeRTOS Tick 中断时自动调用（1kHz = 1ms）
void vApplicationTickHook(void)
{
    lv_tick_inc(1); // 为 LVGL 提供 1ms 时基
}
```

> Stage 5 中 TIM6 中断负责调用 `lv_tick_inc(1)`，引入 FreeRTOS 后，FreeRTOS 本身的 SysTick 中断（1kHz）接管了这一职责，TIM6 可以释放给其他用途。

### 主函数

```c
int main(void)
{
    // 硬件初始化
    delay_init(168);
    LED_Init();
    Usart_Config();
    LCD_Init();
    Touch_Init();
    LCD_direction(1);

    // LVGL 初始化
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    // 创建 UI（同 Stage 5）
    setup_ui(&guider_ui);
    events_init(&guider_ui);

    // 创建 FreeRTOS 任务
    xTaskCreate(LVGL_Task, "LVGL", 1024, NULL, configMAX_PRIORITIES-2, NULL);
    xTaskCreate(LED_Task,  "LED",  128,  NULL, 1,                       NULL);

    // 启动调度器（此后 main 不再返回）
    vTaskStartScheduler();

    while(1); // 不会执行到这里
}
```

---

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM 5.x |
| FreeRTOS | V10.4.x |
| LVGL | v8.3.11 |
| 固件库 | STM32F4xx SPL V1.8.0 |

---

## 如何编译与烧录

1. 打开 Keil 工程
2. 确认 `FreeRTOS/portable/GCC/ARM_CM4F/` 中的移植文件已添加到工程
3. 确认 `FreeRTOS/portable/MemMang/heap_4.c` 已添加（内存管理实现）
4. Keil 工程选项 → Target → 勾选 **Use MicroLIB**（避免标准库与 FreeRTOS 冲突）
5. 编译并烧录

---

## 运行现象

- 屏幕显示与 Stage 5 相同的天气时钟 UI
- LED 以 1Hz 频率闪烁（由独立任务控制）
- UI 刷新与 LED 闪烁完全独立，互不干扰
- 串口可以用于输出 FreeRTOS 任务运行状态（若启用 `vTaskList()`）

---

## 常见问题

**Q: 系统启动后立即 HardFault？**
- 检查 `FreeRTOSConfig.h` 中 `configTOTAL_HEAP_SIZE` 是否超过实际 SRAM
- 确认 Keil 栈（Stack）和堆（Heap）设置不与 FreeRTOS 堆冲突
- STM32F407 需要使用 `ARM_CM4F` 移植层（带 FPU），不能使用 `ARM_CM3`

**Q: LVGL UI 卡顿？**
- 提高 LVGL 任务优先级
- 减小显示缓冲区脏区（减少每次刷新的像素量）
- 确认 `vApplicationTickHook` 中 `lv_tick_inc(1)` 已正确调用

**Q: `vTaskStartScheduler()` 调用后没有任务运行？**
- 检查 FreeRTOS 堆是否被耗尽（`configTOTAL_HEAP_SIZE` 过小）
- 检查 `xTaskCreate` 返回值是否为 `pdPASS`

---

*上一阶段：[Stage 5 - GUI Guider 可视化 UI 设计](../5_LVGL8.3_STM32F407ZGT6_Gui_Guider/README.md)*  
*下一阶段：[Stage 7 - AHT10 温湿度传感器](../7_AHT10/README.md)*
