# ESP32 WiFi 固件

> **天气时钟项目** — ESP32 端固件（`esp32-firmware` 分支）

本固件运行在 **ESP32-S3** 上，负责连接 WiFi、通过[和风天气 API](https://dev.qweather.com/) 获取实时天气与 3 天预报、通过 NTP 同步网络时间，并将所有数据通过 **UART** 发送给 STM32F407 主控。

---

## 功能概览

| 功能 | 说明 |
|------|------|
| WiFi 连接 | 自动连接，断线后每 10 秒重连 |
| NTP 时间同步 | 上电同步一次，每天定时再同步 |
| 实时天气 | 每 10 分钟向和风天气 API 请求一次 |
| 3 天天气预报 | 每 1 小时请求一次 |
| UART 通信 | 接收 STM32 指令，主动推送天气/时间数据 |
| 天气文本规范化 | 将 API 返回的 icon 代码转换为中文天气描述 |
| Gzip 解压 | API 响应为 Gzip 压缩，使用 zlib_turbo 解压 |

---

## 硬件连线

```
ESP32-S3               STM32F407ZGT6
┌──────────────┐        ┌──────────────┐
│  GPIO17 (TX)─┼────────┼── PC7 (RX6)  │  USART6
│  GPIO18 (RX)─┼────────┼── PC6 (TX6)  │  115200 bps
│  GND        ─┼────────┼── GND        │
└──────────────┘        └──────────────┘
```

> **注意：** TX 接 RX，RX 接 TX（交叉连接）。ESP32 需要独立 3.3V 供电，峰值电流约 500mA，建议不要从 STM32 调试接口取电。

---

## 依赖库

在 Arduino IDE 的库管理器中安装以下库：

| 库名 | 说明 |
|------|------|
| `ArduinoJson` | JSON 解析（推荐 v7.x）|
| `zlib_turbo` | Gzip 解压缩 |

ESP32 核心库（`WiFi.h`、`HTTPClient.h`、`WiFiClientSecure.h`、`time.h`）由 ESP32 Arduino 开发包自带，无需单独安装。

---

## 开发环境

| 工具 | 版本 |
|------|------|
| Arduino IDE | 2.x |
| ESP32 Arduino 开发包 | 3.x（Espressif 官方）|
| 目标芯片 | ESP32-S3 |

**安装 ESP32 开发包：**
Arduino IDE → 首选项 → 附加开发板管理器 URL 填入：
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
然后在开发板管理器中搜索 `esp32` 安装。

---

## 配置说明

烧录前修改 `WiFi.ino` 顶部的配置区：

```cpp
// ======= 必须修改 =======
const char* ssid     = "你的WiFi名称";
const char* password = "你的WiFi密码";

// ======= 和风天气 API =======
// 前往 https://dev.qweather.com/ 免费注册并创建应用
const char* apiHost  = "https://你的专属API域名.qweatherapi.com";
const char* apiKey   = "你的API Key";
const char* location = "经度,纬度";   // 例如广州天河：113.36,23.12
```

> **如何获取和风天气 API：**
> 1. 注册 [和风天气开发者账号](https://dev.qweather.com/)
> 2. 创建应用，选择**免费订阅**
> 3. 在控制台获取 `API Host`（专属域名）和 `API Key`

---

## 更新频率配置

```cpp
const unsigned long REALTIME_INTERVAL_MS  = 600000UL;  // 实时天气：10 分钟
const unsigned long FORECAST_INTERVAL_MS  = 3600000UL; // 3天预报：1 小时
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000UL; // 断线重连：10 秒
```

> 免费版和风天气 API 每天有请求次数限制（约 1000 次/天），默认 10 分钟一次实时天气约消耗 144 次/天，在免费额度内。

---

## UART 通信协议

ESP32 与 STM32 之间通过串口传输结构化文本数据。

### STM32 → ESP32（指令）

| 指令 | 说明 |
|------|------|
| `GET_WEATHER` | 立即推送一次天气数据（有缓存时直接发缓存）|
| `GET_TIME` | 立即推送一次当前时间 |

### ESP32 → STM32（数据，主动推送或响应指令）

所有消息格式为 `前缀:字段1,字段2,...\n`

**时间同步：**
```
SYS:TIME,2025,9,20,14,35,22,6
         年   月 日 时 分 秒 星期(1=周一,7=周日)
```

**实时天气：**
```
EVT:WEATHER,26,72,100,晴,3,SE,15
            温度 湿度 图标代码 天气描述 风级 风向 风速(km/h)
```

**3 天预报（今日 + 明日 + 后日）：**
```
EVT:FORECAST,18,28,100,101,20,30,101,104,22,32,102,305
             今日min max 白天icon 夜间icon | 明日... | 后日...
```

**WiFi 状态：**
```
EVT:WIFI,CONNECTED
EVT:WIFI,DISCONNECTED
```

---

## 烧录步骤

1. 用 USB 线连接 ESP32-S3 开发板
2. Arduino IDE 选择正确的开发板型号（`ESP32S3 Dev Module`）和串口
3. 修改顶部配置区（WiFi 账号、API Key、地区坐标）
4. 点击 **上传**（→）
5. 打开串口监视器（115200 bps），观察启动日志确认连接成功

---

## 运行日志示例

```
[WiFi] 正在连接 GY-00...
[WiFi] 已连接，IP: 192.168.1.105
[NTP]  时间同步完成: 2025-09-20 14:35:22
[WTHR] 请求实时天气...
[WTHR] 温度: 26°C  湿度: 72%  天气: 晴(100)
[UART] → EVT:TIME,2025,9,20,14,35,22,6
[UART] → EVT:WEATHER,26,72,100,晴,3,SE,15
[UART] → EVT:FORECAST,18,28,100,101,20,30,101,104
```

---

## 支持的天气类型

固件内置和风天气全部 icon 代码到中文的映射（`normalizeWeatherText` 函数），覆盖：

- 晴 / 多云 / 阴（100~104，150~153）
- 雨类（300~399）：阵雨、小雨、中雨、大雨、暴雨、雷阵雨等
- 雪类（400~499）：小雪、中雪、大雪、暴雪、雨夹雪等
- 雾霾沙尘（500~515）：薄雾、雾、霾、沙尘暴等
- 极端天气（900~901）：热、冷

---

*主工程（STM32 端）：[main 分支](../../tree/main)*
