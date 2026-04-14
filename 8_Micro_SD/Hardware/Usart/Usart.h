#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "stm32f4xx.h"
#include <stdint.h>

/*----------------------USART配置宏 ------------------------*/

#define  USART1_BaudRate  115200

#define  USART1_TX_PIN				GPIO_Pin_9					// TX 引脚
#define	USART1_TX_PORT				GPIOA							// TX 引脚端口
#define	USART1_TX_CLK				RCC_AHB1Periph_GPIOA		// TX 引脚时钟
#define  USART1_TX_PinSource     GPIO_PinSource9			// 引脚源

#define  USART1_RX_PIN				GPIO_Pin_10             // RX 引脚
#define	USART1_RX_PORT				GPIOA                   // RX 引脚端口
#define	USART1_RX_CLK				RCC_AHB1Periph_GPIOA    // RX 引脚时钟
#define  USART1_RX_PinSource     GPIO_PinSource10        // 引脚源


#define ESP32_USART_BaudRate      115200

#define ESP32_USART_TX_PIN         GPIO_Pin_2
#define ESP32_USART_TX_PORT        GPIOA
#define ESP32_USART_TX_CLK         RCC_AHB1Periph_GPIOA
#define ESP32_USART_TX_PinSource   GPIO_PinSource2

#define ESP32_USART_RX_PIN         GPIO_Pin_3
#define ESP32_USART_RX_PORT        GPIOA
#define ESP32_USART_RX_CLK         RCC_AHB1Periph_GPIOA
#define ESP32_USART_RX_PinSource   GPIO_PinSource3

#define ESP32_UART_LINE_MAX_LEN    192U
#define ESP32_UART_LINE_QUEUE_SIZE 8U


typedef enum
{
	WEATHER_IDLE = 0,
	WEATHER_RECEIVING
} WeatherParseState;


typedef struct {
	char updateTime[40];
	char obsTime[40];
	int temp;
	int feelsLike;
	char text[32];
	uint8_t textUtf8Valid;
	int weatherCode;
	uint8_t hasWeatherCode;
	int humidity;
	char windDir[32];
	int wind360;
	int windScale;
	int windSpeed;
	float precip;
	int pressure;
	int vis;
	int cloud;
	int dew;

	char localTime[32];

	char todayFxDate[16];
	int todayTempMax;
	int todayTempMin;
	char todayTextDay[32];
	uint8_t todayTextDayUtf8Valid;
	int todayCodeDay;
	uint8_t hasTodayCodeDay;
	char todayTextNight[32];
	uint8_t todayTextNightUtf8Valid;
	int todayCodeNight;
	uint8_t hasTodayCodeNight;

	uint8_t valid;
} WeatherInfo;


typedef struct {
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
	int wday;
	long epoch;
	uint8_t valid;
} TimeSyncInfo;


typedef enum
{
	ESP32_EVT_NONE = 0,
	ESP32_EVT_PONG,
	ESP32_EVT_TIME_STRING,
	ESP32_EVT_TIME_SYNC,
	ESP32_EVT_WEATHER_BEGIN,
	ESP32_EVT_WEATHER_END,
	ESP32_EVT_WEATHER_READY,
	ESP32_EVT_FETCH_STATUS,
	ESP32_EVT_WIFI_STATUS,
	ESP32_EVT_SYS_STATUS,
	ESP32_EVT_OTHER
} Esp32ParseEvent;


typedef struct
{
	WeatherParseState weatherState;
	WeatherInfo weather;
	TimeSyncInfo timeSync;

	char timeText[32];
	char fetchWeatherStatus[24];
	char wifiStatus[16];
	char wifiIp[32];
	char lastSysMsg[64];

	uint8_t weatherReady;
	uint8_t timeSyncReady;
} Esp32ProtocolContext;


/*---------------------- 函数声明 ----------------------------*/

void  Usart_Config (void);	// USART初始化函数

void  Esp32Uart_Config(void);
void  Esp32Uart_RxIrqHandler(void);
uint8_t Esp32Uart_GetLine(char *outLine, uint16_t maxLen);

void  Esp32_SendLine(const char *line);
void  Esp32_SendCmd(const char *cmd);

void  Esp32Protocol_Init(Esp32ProtocolContext *ctx);
Esp32ParseEvent Esp32Protocol_ParseLine(Esp32ProtocolContext *ctx, const char *line);

#endif //__USART_H

