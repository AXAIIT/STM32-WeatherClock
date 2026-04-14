# AHT10 移植排障日志（STM32F1 裸机 → STM32F4 + FreeRTOS + LVGL）

> 项目：`Weather_Clock/7_AHT10`  
> 时间：2026-03-29  
> 目标：将旧版软件 I2C + AHT10 驱动从 **F1 无系统** 迁移到 **F4 + RTOS + UI**，并在“室内温湿度”区域每 10 秒刷新。  
> 结论：✅ 已完成，运行稳定，串口连续输出 `Init ret=0` 与 `Read OK`。

---

## 1. 背景与目标

### 1.1 原始状态

- `MyI2C` 为旧 F1 工程移植代码。
- `AHT10` 驱动按裸机思路编写，时序与错误处理较弱。
- 新工程是 STM32F4 + FreeRTOS + LVGL（UI 任务+触摸任务+系统 Tick）。

### 1.2 期望结果

- AHT10 在 F4 下稳定初始化和读取。
- 页面“室内温湿度”显示真实数据（整数）。
- 每 10 秒自动更新，不影响 UI 流畅性与任务调度。

---

## 2. 故障现象演进

1. **初期现象**：白屏，LED 闪几下后停止。  
2. **中期现象**：页面能刷出，但很快卡死，触摸无响应。  
3. **后期现象**：页面和 LED 正常，但温湿度不更新。  
4. **最终现象**：串口显示初始化/读取成功，页面同步更新。

---

## 3. 根因定位（按优先级）

### 3.1 平台兼容性问题（F1 → F4）

- `GPIO_Mode_Out_OD` 在当前 F4 SPL 中不可用（编译报错）。
- GPIO 时钟总线宏不匹配（应使用 AHB1）。

**影响**：编译失败或 GPIO 初始化异常。

### 3.2 RTOS 时基被破坏（核心问题）

- 软件 I2C 位时序使用 `delay_us()`。
- 该 `delay_us()`（当前配置 `SYSTEM_SUPPORT_OS=0`）会直接操作 `SysTick->LOAD/VAL/CTRL`。
- FreeRTOS 同时依赖 SysTick 进行任务调度。

**影响**：任务 Tick 异常，表现为 LED 停闪、界面卡死。

### 3.3 AHT10 初始化协议问题

- 初始化流程曾存在错误顺序（写事务中出现不合理命令/地址字节组合），导致 NACK。
- 串口关键证据：`[AHT10] Init ret=2`（地址可达但命令字节失败）。

**影响**：初始化失败，无法进入稳定读取。

### 3.4 可观测性不足

- 缺少阶段性日志时，难区分是 UI 失败、采集失败还是调度失败。

---

## 4. 修复过程（时间线）

### 4.1 修复 F4 GPIO 兼容

文件：`Hardware/I2C/MyI2C.c`、`Hardware/AHT10/AHT10.h`

- GPIO 配置改为 F4 字段：
  - `GPIO_Mode_OUT`
  - `GPIO_OType_OD`
  - `GPIO_PuPd_UP`
- 时钟改为：
  - `RCC_AHB1Periph_GPIOB`
  - `RCC_AHB1PeriphClockCmd(...)`

### 4.2 修复延时接口名不一致

文件：`Hardware/AHT10/AHT10.c`

- `Delay.h` → `LCD_delay.h`
- `Delay_ms(...)` → `delay_ms(...)`

### 4.3 引入 FreeRTOS 友好毫秒延时

文件：`Hardware/AHT10/AHT10.c`

- 增加 `AHT10_DelayMs()`：
  - 调度器运行且非中断：`vTaskDelay(pdMS_TO_TICKS(...))`
  - 否则回退 `delay_ms(...)`

### 4.4 消除 I2C 对 SysTick 干扰（关键）

文件：`Hardware/I2C/MyI2C.c`

- 微秒延时改为 DWT：
  - `MyI2C_DWT_Init()`
  - `MyI2C_DelayUs(...)`
- 所有位时序延时统一替换为 `MyI2C_DelayUs(10)`。
- 修复遗漏：`MyI2C_R_SDA()` 残留 `delay_us(10)`（该遗漏会继续破坏调度）。

### 4.5 增强 AHT10 初始化健壮性

文件：`Hardware/AHT10/AHT10.c`

- 初始化流程升级：
  1. 软复位 `0xBA`
  2. 状态位检查
  3. 初始化命令容错：先 `0xE1`，失败再尝试 `0xBE`
  4. 校准位确认
- ACK 检查完整化，读取流程加入 busy 位轮询。

### 4.6 接入 UI + 10s 刷新

文件：`User/main.c`

- 在 `LVGL_Task` 中：
  - 启动时执行 `AHT10_Init()`
  - 每 `10s` 调用 `AHT10_ReadData()`
  - 写入室内温湿度对象：
    - 温度：`guider_ui.screen_1_spangroup_7_span`
    - 湿度：`guider_ui.screen_1_spangroup_8_span`
- 显示按需求保留整数部分。

### 4.7 最小验证法

文件：`User/main.c`

- 启动后强制显示 `11/22`，验证 UI 通道。
- 串口打印关键状态：
  - `Init ret=...`
  - `Read OK/Read Fail ret=...`

---

## 5. 关键证据链

### 5.1 中间失败证据

- `[AHT10] Init ret=2`  
=> 证明 UI 链路正常，失败点在初始化命令阶段。

### 5.2 最终成功证据

- `[AHT10] Init ret=0`
- `[AHT10] Read OK: T=25, H=62`（连续输出）
- 页面、触摸、LED 同时正常。

---

## 6. 最终结果

- ✅ AHT10 已稳定运行于 STM32F4 + FreeRTOS + LVGL。
- ✅ 室内温湿度可正常显示并每 10 秒更新。
- ✅ 系统无再现“卡死/白屏/任务停滞”问题。

---

## 7. 可复用 Checklist（后续接入新传感器）

### 7.1 平台与库兼容

- [ ] 确认 GPIO 模式字段是否与目标芯片库匹配（F1/F4 差异）。
- [ ] 确认外设时钟总线宏（APB/AHB）正确。
- [ ] 确认头文件路径和函数名在新工程中一致。

### 7.2 RTOS 安全

- [ ] 禁止在任务中使用会改写 SysTick 的延时函数。
- [ ] 微秒级延时使用 DWT/TIM；毫秒级优先 `vTaskDelay`。
- [ ] UI API 只在 LVGL 所在线程调用。

### 7.3 协议健壮性

- [ ] 每个关键命令字节都检查 ACK。
- [ ] 初始化后检查状态位（校准/忙位）。
- [ ] 读取前轮询 busy 位，避免读到未完成数据。
- [ ] 错误码清晰可追踪。

### 7.4 可观测性

- [ ] 提供最小验证路径（先固定值验证 UI，再接真实数据）。
- [ ] 输出关键日志（Init ret、Read ret、关键值）。
- [ ] 出现卡死时保留 HardFault/栈溢出钩子日志。

---

## 8. 建议的收尾动作

- [ ] 去除最小验证固定值 `11/22`。
- [ ] 将读成功日志降为低频或可配置开关。
- [ ] 保留失败日志（Init/Read Fail）用于现场诊断。
- [ ] 将本日志与后续变更记录持续追加到 `docs/`。
