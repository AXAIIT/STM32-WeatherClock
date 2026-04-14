#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <zlib_turbo.h>
#include <time.h>

// ====================== WiFi / API 配置 ======================
const char* ssid     = "GY-00";
const char* password = "a123456789";

const char* apiHost  = "https://pr4wcuufht.re.qweatherapi.com";
const char* apiKey   = "25f37cf17a564e78bb98a8df6c8f252b";
const char* location = "113.36,23.12";   // 广州天河区

// ====================== UART1 配置 ======================
// ESP32 TX(GPIO17) -> STM32 RX
// ESP32 RX(GPIO18) <- STM32 TX
#define STM32_UART_TX_PIN 17
#define STM32_UART_RX_PIN 18
#define STM32_UART_BAUD   115200

// ====================== Gzip / JSON 缓冲区 ======================
#define MAX_COMPRESSED   4096
#define MAX_UNCOMPRESSED 12288

zlib_turbo zt;
uint8_t compressed[MAX_COMPRESSED];
uint8_t uncompressed[MAX_UNCOMPRESSED];

// ====================== 时间 / 调度 / 缓存 配置 ======================
bool todaySynced = false;

// 定时更新周期
const unsigned long REALTIME_INTERVAL_MS          = 600000UL;   // 实时天气：10 分钟
const unsigned long FORECAST_INTERVAL_MS          = 3600000UL;  // 3 天预报：1 小时
const unsigned long WIFI_RECONNECT_INTERVAL_MS    = 10000UL;    // WiFi 重连：10 秒

// 无有效缓存时的失败重试周期
const unsigned long REALTIME_RETRY_INTERVAL_MS    = 60000UL;    // 实时天气失败后：1 分钟重试
const unsigned long FORECAST_RETRY_INTERVAL_MS    = 300000UL;   // 预报失败后：5 分钟重试

// 手动刷新保护
const unsigned long MANUAL_FETCH_MIN_INTERVAL_MS  = 60000UL;    // 手动刷新最小间隔：1 分钟

// 缓存 TTL
const unsigned long REALTIME_CACHE_TTL_MS         = REALTIME_INTERVAL_MS;
const unsigned long FORECAST_CACHE_TTL_MS         = FORECAST_INTERVAL_MS;

// 调度时间戳
unsigned long lastRealtimeAttemptMillis     = 0;
unsigned long lastForecastAttemptMillis     = 0;
unsigned long lastWiFiReconnectMillis       = 0;
unsigned long lastManualFetchRequestMillis  = 0;

// 成功时间戳（用于缓存有效性判断）
unsigned long lastRealtimeSuccessMillis     = 0;
unsigned long lastForecastSuccessMillis     = 0;

// ====================== UART 接收稳健配置 ======================
#define UART_MAX_LINE_LEN          128
#define UART_RX_IDLE_TIMEOUT_MS    120
#define UART_LOG_INTERVAL_MS       3000
#define UART_TX_MIN_INTERVAL_MS    20

String uartRxLine = "";
bool uartLineOverflow = false;
unsigned long uartLastByteMillis = 0;
unsigned long uartLastLogMillis  = 0;
unsigned long uartLastTxMillis   = 0;

// ====================== 实时天气缓存 ======================
struct WeatherData {
  bool valid = false;

  String updateTime;
  String obsTime;
  String temp;
  String feelsLike;

  String icon;      // 原始图标代码（关键：发给 STM32 的 code）
  String rawText;   // 原始天气文本
  String text;      // 规范化后的最终显示文本（辅助）

  String humidity;
  String windDir;
  String wind360;
  String windScale;
  String windSpeed;
  String precip;
  String pressure;
  String vis;
  String cloud;
  String dew;
  String localTime;
};

WeatherData lastWeather;

// ====================== 3 天预报缓存（仅 ESP32 内部使用） ======================
struct DailyForecastItem {
  String fxDate;
  String tempMax;
  String tempMin;

  String iconDay;     // 关键：白天 code
  String iconNight;   // 关键：夜间 code

  String textDay;     // 按 icon 规范化后的白天文本
  String textNight;   // 按 icon 规范化后的夜间文本
};

struct DailyForecastData {
  bool valid = false;
  String updateTime;
  DailyForecastItem days[3];
};

DailyForecastData lastDailyForecast;

// ====================== 函数声明 ======================
void connectToWiFi();
void ensureWiFiConnected();

void initializeNTP();
void checkAndPerformDailyNTPSync();
String getLocalTimeString();
bool isLocalTimeValid();

void initSTM32Uart();
void handleSTM32Uart();
void processSTM32Command(String cmdLine);
void sendToSTM32(const String& msg, bool forceSend = false);
void logUartLimited(const String& msg);

void sendTimeSyncToSTM32(const char* prefix = "SYS");

bool fetchAndProcessWeather(bool pushToSTM32 = true, const char* prefix = "EVT");
bool fetchAndProcessDailyForecast();

void printWeatherInfo(const JsonDocument& doc, time_t now);
void cacheWeatherData(const JsonDocument& doc, time_t now);
void sendFullWeatherToSTM32(const char* prefix = "EVT");

void cacheDailyForecastData(const JsonDocument& doc);
void printDailyForecastInfo();

bool isRealtimeCacheFresh();
bool isForecastCacheFresh();

String normalizeWeatherText(const String& rawText, const String& icon);

// ====================== 工具函数：限速日志 ======================
void logUartLimited(const String& msg) {
  unsigned long now = millis();
  if (now - uartLastLogMillis >= UART_LOG_INTERVAL_MS) {
    uartLastLogMillis = now;
    Serial.println(msg);
  }
}

// ====================== 工具函数：本地时间有效性 ======================
bool isLocalTimeValid() {
  return time(nullptr) > 100000;
}

// ====================== 工具函数：获取本地时间字符串 ======================
String getLocalTimeString() {
  time_t now = time(nullptr);
  if (now < 100000) {
    return "TIME_INVALID";
  }

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

// ====================== 天气文本规范化 ======================
// 根据和风天气 icon 代码，将原始 text 规范化为最终显示文本
String normalizeWeatherText(const String& rawText, const String& icon) {
  String code = icon;
  code.trim();

  // 1xx：晴 / 多云
  if (code == "100") return "晴";
  if (code == "101") return "多云";
  if (code == "102") return "少云";
  if (code == "103") return "晴间多云";
  if (code == "104") return "阴";

  // 夜间晴 / 多云
  if (code == "150") return "晴";
  if (code == "151") return "多云";
  if (code == "152") return "少云";
  if (code == "153") return "晴间多云";

  // 3xx：雨
  if (code == "300") return "阵雨";
  if (code == "301") return "强阵雨";
  if (code == "302") return "雷阵雨";
  if (code == "303") return "强雷阵雨";
  if (code == "304") return "雷阵雨伴有冰雹";
  if (code == "305") return "小雨";
  if (code == "306") return "中雨";
  if (code == "307") return "大雨";
  if (code == "308") return "极端降雨";
  if (code == "309") return "毛毛雨";
  if (code == "310") return "暴雨";
  if (code == "311") return "大暴雨";
  if (code == "312") return "特大暴雨";
  if (code == "313") return "冻雨";
  if (code == "314") return "小到中雨";
  if (code == "315") return "中到大雨";
  if (code == "316") return "大到暴雨";
  if (code == "317") return "暴雨到大暴雨";
  if (code == "318") return "大暴雨到特大暴雨";
  if (code == "350") return "阵雨";
  if (code == "351") return "强阵雨";
  if (code == "399") return "雨";

  // 4xx：雪
  if (code == "400") return "小雪";
  if (code == "401") return "中雪";
  if (code == "402") return "大雪";
  if (code == "403") return "暴雪";
  if (code == "404") return "雨夹雪";
  if (code == "405") return "雨雪天气";
  if (code == "406") return "阵雨夹雪";
  if (code == "407") return "阵雪";
  if (code == "408") return "小到中雪";
  if (code == "409") return "中到大雪";
  if (code == "410") return "大到暴雪";
  if (code == "456") return "阵雨夹雪";
  if (code == "457") return "阵雪";
  if (code == "499") return "雪";

  // 5xx：雾 / 霾 / 沙尘
  if (code == "500") return "薄雾";
  if (code == "501") return "雾";
  if (code == "502") return "霾";
  if (code == "503") return "扬沙";
  if (code == "504") return "浮尘";
  if (code == "507") return "沙尘暴";
  if (code == "508") return "强沙尘暴";
  if (code == "509") return "浓雾";
  if (code == "510") return "强浓雾";
  if (code == "511") return "中度霾";
  if (code == "512") return "重度霾";
  if (code == "513") return "严重霾";
  if (code == "514") return "大雾";
  if (code == "515") return "特强浓雾";

  // 9xx：冷热 / 未知
  if (code == "900") return "热";
  if (code == "901") return "冷";
  if (code == "999") return "未知";

  // 兜底：优先保留原始 text
  if (rawText.length() > 0) {
    return rawText;
  }
  return "未知";
}

// ====================== 缓存有效性判断 ======================
bool isRealtimeCacheFresh() {
  return lastWeather.valid &&
         (millis() - lastRealtimeSuccessMillis <= REALTIME_CACHE_TTL_MS);
}

bool isForecastCacheFresh() {
  return lastDailyForecast.valid &&
         (millis() - lastForecastSuccessMillis <= FORECAST_CACHE_TTL_MS);
}

// ====================== UART 发送 ======================
void sendToSTM32(const String& msg, bool forceSend) {
  unsigned long now = millis();

  if (!forceSend) {
    if (now - uartLastTxMillis < UART_TX_MIN_INTERVAL_MS) {
      return;
    }
  } else {
    unsigned long delta = now - uartLastTxMillis;
    if (delta < UART_TX_MIN_INTERVAL_MS) {
      delay(UART_TX_MIN_INTERVAL_MS - delta);
    }
  }

  Serial1.println(msg);
  uartLastTxMillis = millis();

  Serial.print("[TX->STM32] ");
  Serial.println(msg);
}

// ====================== 时间同步包 ======================
void sendTimeSyncToSTM32(const char* prefix) {
  time_t now = time(nullptr);
  if (now < 100000) {
    return;
  }

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char msg[160];
  snprintf(
      msg, sizeof(msg),
      "%s:TIME_SYNC,year=%d,month=%d,day=%d,hour=%d,min=%02d,sec=%02d,wday=%d,epoch=%ld",
      prefix,
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec,
      timeinfo.tm_wday,
      (long)now
  );

  sendToSTM32(String(msg), true);
}

// ====================== WiFi 连接 ======================
void connectToWiFi() {
  Serial.println("\n\n=== 开始连接 WiFi ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("正在连接到 ");
  Serial.print(ssid);
  Serial.print(" ...");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 40) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi 已连接成功！");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());
    sendToSTM32("SYS:WIFI_OK," + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi 连接失败！");
    sendToSTM32("SYS:WIFI_FAIL");
  }
}

void ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastWiFiReconnectMillis < WIFI_RECONNECT_INTERVAL_MS) return;

  lastWiFiReconnectMillis = now;
  Serial.println("WiFi 已断开，尝试重连...");
  sendToSTM32("SYS:WIFI_RECONNECTING");

  WiFi.disconnect(true, true);
  delay(200);
  connectToWiFi();
}

// ====================== NTP 时间同步 ======================
void initializeNTP() {
  Serial.println("正在通过 NTP 校准时间（中国服务器）...");
  configTime(0, 0, "cn.pool.ntp.org", "ntp.aliyun.com");
  setenv("TZ", "CST-8", 1);
  tzset();

  int retry = 0;
  while (time(nullptr) < 8 * 3600 * 2 && retry < 30) {
    delay(500);
    retry++;
  }

  Serial.println("开机时间校准完成！");
  sendToSTM32("SYS:TIME_OK," + getLocalTimeString());
  sendTimeSyncToSTM32("SYS");
}

void checkAndPerformDailyNTPSync() {
  time_t now = time(nullptr);
  if (now <= 100000) return;

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_hour == 0 && timeinfo.tm_min <= 30 && !todaySynced) {
    Serial.println("\n=== 凌晨 0 点检测到，执行 NTP 时间校准 ===");
    configTime(0, 0, "cn.pool.ntp.org", "ntp.aliyun.com");
    setenv("TZ", "CST-8", 1);
    tzset();

    todaySynced = true;
    Serial.println("时间校准完成（已与真实世界对齐）");
    sendToSTM32("SYS:TIME_RESYNC_OK," + getLocalTimeString());
    sendTimeSyncToSTM32("SYS");
  }

  if (timeinfo.tm_hour >= 1) {
    todaySynced = false;
  }
}

// ====================== UART1 初始化 ======================
void initSTM32Uart() {
  pinMode(STM32_UART_RX_PIN, INPUT_PULLUP);
  Serial1.begin(STM32_UART_BAUD, SERIAL_8N1, STM32_UART_RX_PIN, STM32_UART_TX_PIN);
  delay(100);

  Serial.println("UART1 初始化完成");
  Serial.print("UART1 RX Pin = "); Serial.println(STM32_UART_RX_PIN);
  Serial.print("UART1 TX Pin = "); Serial.println(STM32_UART_TX_PIN);

  sendToSTM32("SYS:ESP32_READY");
}

// ====================== UART 接收处理 ======================
void handleSTM32Uart() {
  while (Serial1.available() > 0) {
    char c = (char)Serial1.read();
    uartLastByteMillis = millis();

    if (c == '\r') continue;

    if (c == '\n') {
      if (uartLineOverflow) {
        logUartLimited("[UART] 检测到超长数据行，已丢弃");
        uartLineOverflow = false;
        uartRxLine = "";
        continue;
      }

      uartRxLine.trim();
      if (uartRxLine.length() > 0) {
        Serial.print("[RX<-STM32] ");
        Serial.println(uartRxLine);
        processSTM32Command(uartRxLine);
      }

      uartRxLine = "";
      continue;
    }

    if ((unsigned char)c < 32 || (unsigned char)c > 126) continue;

    if (uartRxLine.length() >= UART_MAX_LINE_LEN) {
      uartLineOverflow = true;
      continue;
    }

    uartRxLine += c;
  }

  if (uartRxLine.length() > 0 && millis() - uartLastByteMillis > UART_RX_IDLE_TIMEOUT_MS) {
    uartRxLine = "";
    uartLineOverflow = false;
    logUartLimited("[UART] 半包接收超时，已丢弃");
  }
}

// ====================== STM32 命令解析 ======================
void processSTM32Command(String cmdLine) {
  cmdLine.trim();
  if (!cmdLine.startsWith("CMD:")) {
    logUartLimited("[UART] 非 CMD: 前缀数据，已忽略");
    return;
  }

  String cmd = cmdLine.substring(4);
  cmd.trim();

  if (cmd == "PING") {
    sendToSTM32("RSP:PONG");
  }
  else if (cmd == "GET_TIME") {
    sendToSTM32("RSP:TIME," + getLocalTimeString());
  }
  else if (cmd == "SYNC_TIME") {
    if (isLocalTimeValid()) sendTimeSyncToSTM32("RSP");
    else sendToSTM32("RSP:TIME_SYNC,INVALID", true);
  }
  else if (cmd == "GET_WEATHER") {
    sendFullWeatherToSTM32("RSP");
  }
  else if (cmd == "FETCH_WEATHER") {
    unsigned long now = millis();

    if (isRealtimeCacheFresh()) {
      sendToSTM32("RSP:FETCH_WEATHER,CACHE_HIT");
      sendFullWeatherToSTM32("RSP");
      return;
    }

    if (now - lastManualFetchRequestMillis < MANUAL_FETCH_MIN_INTERVAL_MS) {
      if (lastWeather.valid) {
        sendToSTM32("RSP:FETCH_WEATHER,STALE_CACHE");
        sendFullWeatherToSTM32("RSP");
      } else {
        sendToSTM32("RSP:FETCH_WEATHER,TOO_FAST");
      }
      return;
    }

    lastManualFetchRequestMillis = now;
    sendToSTM32("RSP:FETCH_WEATHER,REFRESHING");

    if (!isForecastCacheFresh()) {
      fetchAndProcessDailyForecast();
    }

    bool ok = fetchAndProcessWeather(false, "RSP");
    if (ok) {
      sendToSTM32("RSP:FETCH_WEATHER,UPDATED");
      sendFullWeatherToSTM32("RSP");
    } else {
      if (lastWeather.valid) {
        sendToSTM32("RSP:FETCH_WEATHER,STALE_CACHE");
        sendFullWeatherToSTM32("RSP");
      } else {
        sendToSTM32("RSP:FETCH_WEATHER,FAIL", true);
      }
    }
  }
  else if (cmd == "GET_WIFI") {
    if (WiFi.status() == WL_CONNECTED) {
      sendToSTM32("RSP:WIFI,CONNECTED," + WiFi.localIP().toString());
    } else {
      sendToSTM32("RSP:WIFI,DISCONNECTED");
    }
  }
  else if (cmd.startsWith("ECHO,")) {
    sendToSTM32("RSP:ECHO," + cmd.substring(5));
  }
  else {
    logUartLimited("[UART] 未知命令，已静默忽略: " + cmd);
  }
}

// ====================== 缓存实时天气数据 ======================
void cacheWeatherData(const JsonDocument& doc, time_t now) {
  JsonObjectConst nowObj = doc["now"].as<JsonObjectConst>();

  String rawIcon = String((const char*)nowObj["icon"]);
  String rawText = String((const char*)nowObj["text"]);

  lastWeather.valid      = true;
  lastWeather.updateTime = String((const char*)doc["updateTime"]);
  lastWeather.obsTime    = String((const char*)nowObj["obsTime"]);
  lastWeather.temp       = String((const char*)nowObj["temp"]);
  lastWeather.feelsLike  = String((const char*)nowObj["feelsLike"]);

  lastWeather.icon       = rawIcon;
  lastWeather.rawText    = rawText;
  lastWeather.text       = normalizeWeatherText(rawText, rawIcon);

  lastWeather.humidity   = String((const char*)nowObj["humidity"]);
  lastWeather.windDir    = String((const char*)nowObj["windDir"]);
  lastWeather.wind360    = String((const char*)nowObj["wind360"]);
  lastWeather.windScale  = String((const char*)nowObj["windScale"]);
  lastWeather.windSpeed  = String((const char*)nowObj["windSpeed"]);
  lastWeather.precip     = String((const char*)nowObj["precip"]);
  lastWeather.pressure   = String((const char*)nowObj["pressure"]);
  lastWeather.vis        = String((const char*)nowObj["vis"]);
  lastWeather.cloud      = String((const char*)nowObj["cloud"]);
  lastWeather.dew        = String((const char*)nowObj["dew"]);
  lastWeather.localTime  = getLocalTimeString();

  (void)now;
}

// ====================== 缓存 3 日预报数据 ======================
void cacheDailyForecastData(const JsonDocument& doc) {
  JsonArrayConst dailyArr = doc["daily"].as<JsonArrayConst>();
  if (dailyArr.isNull() || dailyArr.size() == 0) {
    lastDailyForecast.valid = false;
    return;
  }

  lastDailyForecast.valid = true;
  lastDailyForecast.updateTime = String((const char*)doc["updateTime"]);

  for (int i = 0; i < 3; ++i) {
    if (i < (int)dailyArr.size()) {
      JsonObjectConst dayObj = dailyArr[i].as<JsonObjectConst>();

      String iconDay   = String((const char*)dayObj["iconDay"]);
      String iconNight = String((const char*)dayObj["iconNight"]);
      String textDay   = String((const char*)dayObj["textDay"]);
      String textNight = String((const char*)dayObj["textNight"]);

      lastDailyForecast.days[i].fxDate    = String((const char*)dayObj["fxDate"]);
      lastDailyForecast.days[i].tempMax   = String((const char*)dayObj["tempMax"]);
      lastDailyForecast.days[i].tempMin   = String((const char*)dayObj["tempMin"]);
      lastDailyForecast.days[i].iconDay   = iconDay;
      lastDailyForecast.days[i].iconNight = iconNight;
      lastDailyForecast.days[i].textDay   = normalizeWeatherText(textDay, iconDay);
      lastDailyForecast.days[i].textNight = normalizeWeatherText(textNight, iconNight);
    } else {
      lastDailyForecast.days[i].fxDate    = "";
      lastDailyForecast.days[i].tempMax   = "";
      lastDailyForecast.days[i].tempMin   = "";
      lastDailyForecast.days[i].iconDay   = "";
      lastDailyForecast.days[i].iconNight = "";
      lastDailyForecast.days[i].textDay   = "";
      lastDailyForecast.days[i].textNight = "";
    }
  }
}

// ====================== 发送完整实时天气包 ======================
void sendFullWeatherToSTM32(const char* prefix) {
  if (!lastWeather.valid) {
    sendToSTM32(String(prefix) + ":WEATHER,NODATA", true);
    return;
  }

  sendToSTM32(String(prefix) + ":WEATHER_BEGIN", true);

  sendToSTM32("ITEM:updateTime=" + lastWeather.updateTime, true);
  sendToSTM32("ITEM:obsTime=" + lastWeather.obsTime, true);
  sendToSTM32("ITEM:temp=" + lastWeather.temp, true);
  sendToSTM32("ITEM:feelsLike=" + lastWeather.feelsLike, true);

  // 关键字段：code/icon 主链路
  sendToSTM32("ITEM:icon=" + lastWeather.icon, true);
  sendToSTM32("ITEM:text=" + lastWeather.text, true);

  sendToSTM32("ITEM:humidity=" + lastWeather.humidity, true);
  sendToSTM32("ITEM:windDir=" + lastWeather.windDir, true);
  sendToSTM32("ITEM:wind360=" + lastWeather.wind360, true);
  sendToSTM32("ITEM:windScale=" + lastWeather.windScale, true);
  sendToSTM32("ITEM:windSpeed=" + lastWeather.windSpeed, true);
  sendToSTM32("ITEM:precip=" + lastWeather.precip, true);
  sendToSTM32("ITEM:pressure=" + lastWeather.pressure, true);
  sendToSTM32("ITEM:vis=" + lastWeather.vis, true);
  sendToSTM32("ITEM:cloud=" + lastWeather.cloud, true);
  sendToSTM32("ITEM:dew=" + lastWeather.dew, true);
  sendToSTM32("ITEM:localTime=" + lastWeather.localTime, true);

  if (lastDailyForecast.valid && lastDailyForecast.days[0].fxDate.length() > 0) {
    sendToSTM32("ITEM:todayFxDate=" + lastDailyForecast.days[0].fxDate, true);
    sendToSTM32("ITEM:todayTempMax=" + lastDailyForecast.days[0].tempMax, true);
    sendToSTM32("ITEM:todayTempMin=" + lastDailyForecast.days[0].tempMin, true);

    // 关键字段：day/night code 主链路
    sendToSTM32("ITEM:todayCodeDay=" + lastDailyForecast.days[0].iconDay, true);
    sendToSTM32("ITEM:todayCodeNight=" + lastDailyForecast.days[0].iconNight, true);

    // 文本作为辅助（已按 icon 规范化）
    sendToSTM32("ITEM:todayTextDay=" + lastDailyForecast.days[0].textDay, true);
    sendToSTM32("ITEM:todayTextNight=" + lastDailyForecast.days[0].textNight, true);
  }

  sendToSTM32(String(prefix) + ":WEATHER_END", true);
}

// ====================== 获取并处理实时天气 ======================
bool fetchAndProcessWeather(bool pushToSTM32, const char* prefix) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi 未连接，无法请求实时天气");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(apiHost) + "/v7/weather/now?location=" + location + "&key=" + apiKey;
  http.begin(client, url);
  http.addHeader("Accept-Encoding", "gzip");

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("\n=== 开始接收并解压实时天气 Gzip 数据 ===");

    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = 0;
    memset(compressed, 0, sizeof(compressed));

    unsigned long startMs = millis();
    while (http.connected() && bytesRead < MAX_COMPRESSED) {
      if (stream->available()) {
        int chunk = stream->readBytes(compressed + bytesRead, MAX_COMPRESSED - bytesRead);
        if (chunk > 0) bytesRead += chunk;
      } else {
        if (millis() - startMs > 3000) break;
        delay(1);
      }
    }

    int uncompSize = zt.gzip_info(compressed, bytesRead);
    if (uncompSize > 0 && uncompSize < MAX_UNCOMPRESSED) {
      memset(uncompressed, 0, sizeof(uncompressed));

      if (zt.gunzip(compressed, bytesRead, uncompressed) == ZT_SUCCESS) {
        uncompressed[uncompSize] = 0;

        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, uncompressed);

        if (!error && doc["code"] == "200") {
          time_t now = time(nullptr);

          printWeatherInfo(doc, now);
          cacheWeatherData(doc, now);
          lastRealtimeSuccessMillis = millis();

          if (pushToSTM32) {
            sendFullWeatherToSTM32(prefix);
          }

          http.end();
          return true;
        } else {
          Serial.println("实时天气 JSON 解析失败或接口返回非 200");
        }
      } else {
        Serial.println("实时天气 Gzip 解压失败");
      }
    } else {
      Serial.println("实时天气无效的解压长度");
    }
  } else {
    Serial.print("实时天气 HTTP 请求失败，状态码: ");
    Serial.println(httpCode);
  }

  http.end();
  return false;
}

// ====================== 获取并处理近 3 天预报 ======================
bool fetchAndProcessDailyForecast() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi 未连接，无法请求每日天气预报");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(apiHost) + "/v7/weather/3d?location=" + location + "&key=" + apiKey;
  http.begin(client, url);
  http.addHeader("Accept-Encoding", "gzip");

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("\n=== 开始接收并解压 3 天天气预报 Gzip 数据 ===");

    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = 0;
    memset(compressed, 0, sizeof(compressed));

    unsigned long startMs = millis();
    while (http.connected() && bytesRead < MAX_COMPRESSED) {
      if (stream->available()) {
        int chunk = stream->readBytes(compressed + bytesRead, MAX_COMPRESSED - bytesRead);
        if (chunk > 0) bytesRead += chunk;
      } else {
        if (millis() - startMs > 3000) break;
        delay(1);
      }
    }

    int uncompSize = zt.gzip_info(compressed, bytesRead);
    if (uncompSize > 0 && uncompSize < MAX_UNCOMPRESSED) {
      memset(uncompressed, 0, sizeof(uncompressed));

      if (zt.gunzip(compressed, bytesRead, uncompressed) == ZT_SUCCESS) {
        uncompressed[uncompSize] = 0;

        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, uncompressed);

        if (!error && doc["code"] == "200") {
          cacheDailyForecastData(doc);
          printDailyForecastInfo();
          lastForecastSuccessMillis = millis();

          http.end();
          return true;
        } else {
          Serial.println("每日预报 JSON 解析失败或接口返回非 200");
        }
      } else {
        Serial.println("每日预报 Gzip 解压失败");
      }
    } else {
      Serial.println("每日预报无效的解压长度");
    }
  } else {
    Serial.print("每日预报 HTTP 请求失败，状态码: ");
    Serial.println(httpCode);
  }

  http.end();
  return false;
}

// ====================== 串口监视器输出实时天气 ======================
void printWeatherInfo(const JsonDocument& doc, time_t now) {
  JsonObjectConst nowObj = doc["now"].as<JsonObjectConst>();

  String displayText = normalizeWeatherText(
      String((const char*)nowObj["text"]),
      String((const char*)nowObj["icon"])
  );

  Serial.println("========== 实时天气（广州天河区） ==========");
  Serial.print("更新时间: "); Serial.println((const char*)doc["updateTime"]);
  Serial.print("观测时间: "); Serial.println((const char*)nowObj["obsTime"]);
  Serial.print("温度: ");     Serial.print((const char*)nowObj["temp"]);       Serial.println(" ℃");
  Serial.print("体感温度: "); Serial.print((const char*)nowObj["feelsLike"]);  Serial.println(" ℃");
  Serial.print("天气状况: "); Serial.println(displayText);
  Serial.print("湿度: ");     Serial.print((const char*)nowObj["humidity"]);   Serial.println(" %");
  Serial.print("风向: ");     Serial.print((const char*)nowObj["windDir"]);
  Serial.print("（"); Serial.print((const char*)nowObj["wind360"]); Serial.print("°） 风力: ");
  Serial.print((const char*)nowObj["windScale"]); Serial.print(" 级  风速: ");
  Serial.print((const char*)nowObj["windSpeed"]); Serial.println(" km/h");
  Serial.print("降水量: ");   Serial.print((const char*)nowObj["precip"]);     Serial.println(" mm");
  Serial.print("气压: ");     Serial.print((const char*)nowObj["pressure"]);   Serial.println(" hPa");
  Serial.print("能见度: ");   Serial.print((const char*)nowObj["vis"]);        Serial.println(" km");
  Serial.print("云量: ");     Serial.print((const char*)nowObj["cloud"]);      Serial.println(" %");
  Serial.print("露点温度: "); Serial.print((const char*)nowObj["dew"]);        Serial.println(" ℃");

  char timeStr[64];
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S（本地时间）", &timeinfo);

  Serial.print("当前设备本地时间: ");
  Serial.println(timeStr);
  Serial.println("=========================================");
}

// ====================== 串口监视器输出 3 天预报 ======================
void printDailyForecastInfo() {
  if (!lastDailyForecast.valid) {
    Serial.println("暂无 3 天天气预报缓存");
    return;
  }

  Serial.println("========== 近 3 天天气预报（仅 ESP32 本地） ==========");
  Serial.print("更新时间: ");
  Serial.println(lastDailyForecast.updateTime);

  for (int i = 0; i < 3; ++i) {
    if (lastDailyForecast.days[i].fxDate.length() == 0) continue;

    Serial.print("日期: ");
    Serial.println(lastDailyForecast.days[i].fxDate);
    Serial.print("白天天气: ");
    Serial.println(lastDailyForecast.days[i].textDay);
    Serial.print("夜间天气: ");
    Serial.println(lastDailyForecast.days[i].textNight);
    Serial.print("白天代码: ");
    Serial.println(lastDailyForecast.days[i].iconDay);
    Serial.print("夜间代码: ");
    Serial.println(lastDailyForecast.days[i].iconNight);
    Serial.print("最高气温: ");
    Serial.print(lastDailyForecast.days[i].tempMax);
    Serial.println(" ℃");
    Serial.print("最低气温: ");
    Serial.print(lastDailyForecast.days[i].tempMin);
    Serial.println(" ℃");
    Serial.println("-----------------------------------");
  }

  Serial.println("==================================================");
}

// ====================== Arduino 主函数 ======================
void setup() {
  Serial.begin(115200);
  delay(100);

  initSTM32Uart();
  connectToWiFi();
  initializeNTP();

  // 上电先抓预报，再抓实时，首次包即可带 today 信息
  lastForecastAttemptMillis = millis();
  fetchAndProcessDailyForecast();

  lastRealtimeAttemptMillis = millis();
  fetchAndProcessWeather(true, "EVT");

  todaySynced = false;
  Serial.println("=============================");
}

void loop() {
  handleSTM32Uart();
  ensureWiFiConnected();
  checkAndPerformDailyNTPSync();

  unsigned long nowMs = millis();

  unsigned long forecastDue = lastDailyForecast.valid ? FORECAST_INTERVAL_MS : FORECAST_RETRY_INTERVAL_MS;
  if (nowMs - lastForecastAttemptMillis >= forecastDue) {
    lastForecastAttemptMillis = nowMs;
    fetchAndProcessDailyForecast();
  }

  unsigned long realtimeDue = lastWeather.valid ? REALTIME_INTERVAL_MS : REALTIME_RETRY_INTERVAL_MS;
  if (nowMs - lastRealtimeAttemptMillis >= realtimeDue) {
    lastRealtimeAttemptMillis = nowMs;
    fetchAndProcessWeather(true, "EVT");
  }

  delay(2);
}