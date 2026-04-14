/***
	*************************************************************************************************
	*	@version V1.0
	*	@author  鹿小班科技	
	*	@brief   usart 接口相关函数
   *************************************************************************************************
   *  @description
	*
	*	实验平台：鹿小班STM32F407ZGT6核心板 （型号：LXB407ZG-P1）
	* 客服微信：19949278543
	*
>>>>> 文件说明：
	*
	*  1.初始化USART1的引脚 PA9/PA10，
	*  2.配置USART1工作在收发模式、数位8位、停止位1位、无校验、不使用硬件控制流控制，
	*    串口的波特率设置为115200，若需要更改波特率直接修改usart.h里的宏定义USART1_BaudRate。
	*  3.重定义fputc函数,用以支持使用printf函数打印数据
	*
	************************************************************************************************
***/


#include "usart.h"  
#include "misc.h"

#include <string.h>
#include <stdlib.h>


static volatile char s_esp32RxCurrentLine[ESP32_UART_LINE_MAX_LEN];
static volatile uint16_t s_esp32RxCurrentLen = 0U;
static volatile uint8_t s_esp32RxOverflow = 0U;

static volatile char s_esp32LineQueue[ESP32_UART_LINE_QUEUE_SIZE][ESP32_UART_LINE_MAX_LEN];
static volatile uint8_t s_esp32LineWriteIndex = 0U;
static volatile uint8_t s_esp32LineReadIndex = 0U;
static volatile uint8_t s_esp32LineCount = 0U;


static void Esp32Uart_SendChar(uint8_t ch)
{
	USART_SendData(USART2, ch);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
	{
	}
}


static uint8_t Esp32StrStartsWith(const char *str, const char *prefix)
{
	size_t prefixLen = 0U;

	if ((str == NULL) || (prefix == NULL))
	{
		return 0U;
	}

	prefixLen = strlen(prefix);
	if (strlen(str) < prefixLen)
	{
		return 0U;
	}

	return (strncmp(str, prefix, prefixLen) == 0) ? 1U : 0U;
}


static void Esp32SafeCopy(char *dst, uint16_t dstSize, const char *src)
{
	uint16_t copyLen = 0U;

	if ((dst == NULL) || (src == NULL) || (dstSize == 0U))
	{
		return;
	}

	copyLen = (uint16_t)strlen(src);
	if (copyLen >= dstSize)
	{
		copyLen = (uint16_t)(dstSize - 1U);
	}

	memcpy(dst, src, copyLen);
	dst[copyLen] = '\0';
}


static void Esp32WeatherClear(WeatherInfo *weather)
{
	if (weather == NULL)
	{
		return;
	}

	memset(weather, 0, sizeof(WeatherInfo));
}


static void Esp32ParseWeatherItem(WeatherInfo *weather, const char *key, const char *value)
{
	if ((weather == NULL) || (key == NULL) || (value == NULL))
	{
		return;
	}

	if (strcmp(key, "updateTime") == 0)
	{
		Esp32SafeCopy(weather->updateTime, (uint16_t)sizeof(weather->updateTime), value);
	}
	else if (strcmp(key, "obsTime") == 0)
	{
		Esp32SafeCopy(weather->obsTime, (uint16_t)sizeof(weather->obsTime), value);
	}
	else if (strcmp(key, "temp") == 0)
	{
		weather->temp = atoi(value);
	}
	else if (strcmp(key, "feelsLike") == 0)
	{
		weather->feelsLike = atoi(value);
	}
	else if (strcmp(key, "text") == 0)
	{
		Esp32SafeCopy(weather->text, (uint16_t)sizeof(weather->text), value);
	}
	else if (strcmp(key, "humidity") == 0)
	{
		weather->humidity = atoi(value);
	}
	else if (strcmp(key, "windDir") == 0)
	{
		Esp32SafeCopy(weather->windDir, (uint16_t)sizeof(weather->windDir), value);
	}
	else if (strcmp(key, "wind360") == 0)
	{
		weather->wind360 = atoi(value);
	}
	else if (strcmp(key, "windScale") == 0)
	{
		weather->windScale = atoi(value);
	}
	else if (strcmp(key, "windSpeed") == 0)
	{
		weather->windSpeed = atoi(value);
	}
	else if (strcmp(key, "precip") == 0)
	{
		weather->precip = (float)atof(value);
	}
	else if (strcmp(key, "pressure") == 0)
	{
		weather->pressure = atoi(value);
	}
	else if (strcmp(key, "vis") == 0)
	{
		weather->vis = atoi(value);
	}
	else if (strcmp(key, "cloud") == 0)
	{
		weather->cloud = atoi(value);
	}
	else if (strcmp(key, "dew") == 0)
	{
		weather->dew = atoi(value);
	}
	else if (strcmp(key, "localTime") == 0)
	{
		Esp32SafeCopy(weather->localTime, (uint16_t)sizeof(weather->localTime), value);
	}
	else if (strcmp(key, "todayFxDate") == 0)
	{
		Esp32SafeCopy(weather->todayFxDate, (uint16_t)sizeof(weather->todayFxDate), value);
	}
	else if (strcmp(key, "todayTempMax") == 0)
	{
		weather->todayTempMax = atoi(value);
	}
	else if (strcmp(key, "todayTempMin") == 0)
	{
		weather->todayTempMin = atoi(value);
	}
	else if (strcmp(key, "todayTextDay") == 0)
	{
		Esp32SafeCopy(weather->todayTextDay, (uint16_t)sizeof(weather->todayTextDay), value);
	}
	else if (strcmp(key, "todayTextNight") == 0)
	{
		Esp32SafeCopy(weather->todayTextNight, (uint16_t)sizeof(weather->todayTextNight), value);
	}
}


static uint8_t Esp32ParseTimeSyncLine(TimeSyncInfo *timeSync, const char *line)
{
	char lineCopy[ESP32_UART_LINE_MAX_LEN];
	char *payload = NULL;
	char *token = NULL;
	uint8_t hasYear = 0U;
	uint8_t hasMonth = 0U;
	uint8_t hasDay = 0U;
	uint8_t hasHour = 0U;
	uint8_t hasMin = 0U;
	uint8_t hasSec = 0U;

	if ((timeSync == NULL) || (line == NULL))
	{
		return 0U;
	}

	memset(timeSync, 0, sizeof(TimeSyncInfo));
	Esp32SafeCopy(lineCopy, (uint16_t)sizeof(lineCopy), line);

	payload = strchr(lineCopy, ',');
	if (payload == NULL)
	{
		return 0U;
	}
	payload++;

	token = strtok(payload, ",");
	while (token != NULL)
	{
		char *eq = strchr(token, '=');
		if (eq != NULL)
		{
			*eq = '\0';
			eq++;

			if (strcmp(token, "year") == 0)
			{
				timeSync->year = atoi(eq);
				hasYear = 1U;
			}
			else if (strcmp(token, "month") == 0)
			{
				timeSync->month = atoi(eq);
				hasMonth = 1U;
			}
			else if (strcmp(token, "day") == 0)
			{
				timeSync->day = atoi(eq);
				hasDay = 1U;
			}
			else if (strcmp(token, "hour") == 0)
			{
				timeSync->hour = atoi(eq);
				hasHour = 1U;
			}
			else if (strcmp(token, "min") == 0)
			{
				timeSync->min = atoi(eq);
				hasMin = 1U;
			}
			else if (strcmp(token, "sec") == 0)
			{
				timeSync->sec = atoi(eq);
				hasSec = 1U;
			}
			else if (strcmp(token, "wday") == 0)
			{
				timeSync->wday = atoi(eq);
			}
			else if (strcmp(token, "epoch") == 0)
			{
				timeSync->epoch = atol(eq);
			}
		}

		token = strtok(NULL, ",");
	}

	if (hasYear && hasMonth && hasDay && hasHour && hasMin && hasSec)
	{
		timeSync->valid = 1U;
		return 1U;
	}

	return 0U;
}

// 函数：usart IO口初始化
//
void  USART_GPIO_Config	(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd ( USART1_TX_CLK|USART1_RX_CLK, ENABLE); 	//IO口时钟配置

	//IO配置
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;   	 //复用模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   //推挽
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;		 //上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //速度等级

	//初始化 TX	引脚
	GPIO_InitStructure.GPIO_Pin = USART1_TX_PIN;	 
	GPIO_Init(USART1_TX_PORT, &GPIO_InitStructure);	
	//初始化 RX 引脚													   
	GPIO_InitStructure.GPIO_Pin = USART1_RX_PIN;	
	GPIO_Init(USART1_RX_PORT, &GPIO_InitStructure);		
	
	//IO复用，复用到USART1
	GPIO_PinAFConfig(USART1_TX_PORT,USART1_TX_PinSource,GPIO_AF_USART1); 
	GPIO_PinAFConfig(USART1_RX_PORT,USART1_RX_PinSource,GPIO_AF_USART1);	
}

// 函数：USART 口初始化
//
void Usart_Config(void)
{		
	USART_InitTypeDef USART_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	// IO口初始化
	USART_GPIO_Config();
		 
	// 配置串口各项参数
	USART_InitStructure.USART_BaudRate 	 = USART1_BaudRate; //波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; //数据位8位
	USART_InitStructure.USART_StopBits   = USART_StopBits_1; //停止位1位
	USART_InitStructure.USART_Parity     = USART_Parity_No ; //无校验
	USART_InitStructure.USART_Mode 	    = USART_Mode_Rx | USART_Mode_Tx; //发送和接收模式
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 不使用硬件流控制
	
	USART_Init(USART1,&USART_InitStructure); //初始化串口1
	USART_Cmd(USART1,ENABLE);	//使能串口1
}

// 函数：重定义fputc函数
//
int fputc(int c, FILE *fp)
{

	USART_SendData( USART1,(u8)c );	// 发送单字节数据
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完毕 

	return (c); //返回字符
}


void Esp32Uart_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(ESP32_USART_TX_CLK | ESP32_USART_RX_CLK, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_InitStructure.GPIO_Pin = ESP32_USART_TX_PIN;
	GPIO_Init(ESP32_USART_TX_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = ESP32_USART_RX_PIN;
	GPIO_Init(ESP32_USART_RX_PORT, &GPIO_InitStructure);

	GPIO_PinAFConfig(ESP32_USART_TX_PORT, ESP32_USART_TX_PinSource, GPIO_AF_USART2);
	GPIO_PinAFConfig(ESP32_USART_RX_PORT, ESP32_USART_RX_PinSource, GPIO_AF_USART2);

	USART_InitStructure.USART_BaudRate = ESP32_USART_BaudRate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART2, &USART_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART2, ENABLE);

	s_esp32RxCurrentLen = 0U;
	s_esp32RxOverflow = 0U;
	s_esp32LineWriteIndex = 0U;
	s_esp32LineReadIndex = 0U;
	s_esp32LineCount = 0U;
}


void Esp32Uart_RxIrqHandler(void)
{
	uint8_t rx = 0U;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) == RESET)
	{
		return;
	}

	rx = (uint8_t)(USART_ReceiveData(USART2) & 0xFFU);

	if (rx == '\r')
	{
		return;
	}

	if (rx == '\n')
	{
		if ((s_esp32RxCurrentLen > 0U) && (s_esp32RxOverflow == 0U) && (s_esp32LineCount < ESP32_UART_LINE_QUEUE_SIZE))
		{
			uint16_t i = 0U;
			for (i = 0U; i < s_esp32RxCurrentLen; i++)
			{
				s_esp32LineQueue[s_esp32LineWriteIndex][i] = s_esp32RxCurrentLine[i];
			}
			s_esp32LineQueue[s_esp32LineWriteIndex][s_esp32RxCurrentLen] = '\0';

			s_esp32LineWriteIndex++;
			if (s_esp32LineWriteIndex >= ESP32_UART_LINE_QUEUE_SIZE)
			{
				s_esp32LineWriteIndex = 0U;
			}
			s_esp32LineCount++;
		}

		s_esp32RxCurrentLen = 0U;
		s_esp32RxOverflow = 0U;
		return;
	}

	if (s_esp32RxOverflow != 0U)
	{
		return;
	}

	if (s_esp32RxCurrentLen < (ESP32_UART_LINE_MAX_LEN - 1U))
	{
		s_esp32RxCurrentLine[s_esp32RxCurrentLen++] = (char)rx;
	}
	else
	{
		s_esp32RxOverflow = 1U;
	}
}


uint8_t Esp32Uart_GetLine(char *outLine, uint16_t maxLen)
{
	uint8_t hasLine = 0U;

	if ((outLine == NULL) || (maxLen < 2U))
	{
		return 0U;
	}

	__disable_irq();
	if (s_esp32LineCount > 0U)
	{
		Esp32SafeCopy(outLine, maxLen, (const char *)s_esp32LineQueue[s_esp32LineReadIndex]);

		s_esp32LineReadIndex++;
		if (s_esp32LineReadIndex >= ESP32_UART_LINE_QUEUE_SIZE)
		{
			s_esp32LineReadIndex = 0U;
		}

		s_esp32LineCount--;
		hasLine = 1U;
	}
	__enable_irq();

	return hasLine;
}


void Esp32_SendLine(const char *line)
{
	size_t len = 0U;
	size_t i = 0U;

	if (line == NULL)
	{
		return;
	}

	len = strlen(line);
	for (i = 0U; i < len; i++)
	{
		Esp32Uart_SendChar((uint8_t)line[i]);
	}

	if ((len == 0U) || (line[len - 1U] != '\n'))
	{
		Esp32Uart_SendChar((uint8_t)'\n');
	}
}


void Esp32_SendCmd(const char *cmd)
{
	if (cmd == NULL)
	{
		return;
	}

	Esp32Uart_SendChar((uint8_t)'C');
	Esp32Uart_SendChar((uint8_t)'M');
	Esp32Uart_SendChar((uint8_t)'D');
	Esp32Uart_SendChar((uint8_t)':');
	while (*cmd != '\0')
	{
		Esp32Uart_SendChar((uint8_t)(*cmd));
		cmd++;
	}
	Esp32Uart_SendChar((uint8_t)'\n');
}


void Esp32Protocol_Init(Esp32ProtocolContext *ctx)
{
	if (ctx == NULL)
	{
		return;
	}

	memset(ctx, 0, sizeof(Esp32ProtocolContext));
	ctx->weatherState = WEATHER_IDLE;
}


Esp32ParseEvent Esp32Protocol_ParseLine(Esp32ProtocolContext *ctx, const char *line)
{
	if ((ctx == NULL) || (line == NULL) || (line[0] == '\0'))
	{
		return ESP32_EVT_NONE;
	}

	if (Esp32StrStartsWith(line, "SYS:TIME_SYNC,") || Esp32StrStartsWith(line, "RSP:TIME_SYNC,"))
	{
		if (Esp32ParseTimeSyncLine(&(ctx->timeSync), line) != 0U)
		{
			ctx->timeSyncReady = 1U;
			return ESP32_EVT_TIME_SYNC;
		}
		return ESP32_EVT_OTHER;
	}

	if (Esp32StrStartsWith(line, "EVT:WEATHER_BEGIN") || Esp32StrStartsWith(line, "RSP:WEATHER_BEGIN"))
	{
		Esp32WeatherClear(&(ctx->weather));
		ctx->weatherState = WEATHER_RECEIVING;
		return ESP32_EVT_WEATHER_BEGIN;
	}

	if (Esp32StrStartsWith(line, "EVT:WEATHER_END") || Esp32StrStartsWith(line, "RSP:WEATHER_END"))
	{
		ctx->weatherState = WEATHER_IDLE;
		ctx->weather.valid = 1U;
		ctx->weatherReady = 1U;
		return ESP32_EVT_WEATHER_READY;
	}

	if (Esp32StrStartsWith(line, "ITEM:") && (ctx->weatherState == WEATHER_RECEIVING))
	{
		const char *kv = line + 5;
		const char *eq = strchr(kv, '=');
		if (eq != NULL)
		{
			char key[40];
			char value[96];
			uint16_t keyLen = (uint16_t)(eq - kv);

			if (keyLen >= sizeof(key))
			{
				keyLen = (uint16_t)(sizeof(key) - 1U);
			}

			memcpy(key, kv, keyLen);
			key[keyLen] = '\0';
			Esp32SafeCopy(value, (uint16_t)sizeof(value), eq + 1);
			Esp32ParseWeatherItem(&(ctx->weather), key, value);
		}
		return ESP32_EVT_OTHER;
	}

	if (strcmp(line, "RSP:PONG") == 0)
	{
		return ESP32_EVT_PONG;
	}

	if (Esp32StrStartsWith(line, "RSP:TIME,"))
	{
		Esp32SafeCopy(ctx->timeText, (uint16_t)sizeof(ctx->timeText), line + 9);
		return ESP32_EVT_TIME_STRING;
	}

	if (Esp32StrStartsWith(line, "RSP:FETCH_WEATHER,"))
	{
		Esp32SafeCopy(ctx->fetchWeatherStatus, (uint16_t)sizeof(ctx->fetchWeatherStatus), line + 18);
		return ESP32_EVT_FETCH_STATUS;
	}

	if (Esp32StrStartsWith(line, "SYS:WIFI_OK,"))
	{
		Esp32SafeCopy(ctx->wifiStatus, (uint16_t)sizeof(ctx->wifiStatus), "CONNECTED");
		Esp32SafeCopy(ctx->wifiIp, (uint16_t)sizeof(ctx->wifiIp), line + 12);
		return ESP32_EVT_WIFI_STATUS;
	}

	if (strcmp(line, "SYS:WIFI_FAIL") == 0)
	{
		Esp32SafeCopy(ctx->wifiStatus, (uint16_t)sizeof(ctx->wifiStatus), "DISCONNECTED");
		ctx->wifiIp[0] = '\0';
		return ESP32_EVT_WIFI_STATUS;
	}

	if (strcmp(line, "SYS:WIFI_RECONNECTING") == 0)
	{
		Esp32SafeCopy(ctx->wifiStatus, (uint16_t)sizeof(ctx->wifiStatus), "RECONNECTING");
		return ESP32_EVT_WIFI_STATUS;
	}

	if (Esp32StrStartsWith(line, "RSP:WIFI,CONNECTED,"))
	{
		Esp32SafeCopy(ctx->wifiStatus, (uint16_t)sizeof(ctx->wifiStatus), "CONNECTED");
		Esp32SafeCopy(ctx->wifiIp, (uint16_t)sizeof(ctx->wifiIp), line + 19);
		return ESP32_EVT_WIFI_STATUS;
	}

	if (strcmp(line, "RSP:WIFI,DISCONNECTED") == 0)
	{
		Esp32SafeCopy(ctx->wifiStatus, (uint16_t)sizeof(ctx->wifiStatus), "DISCONNECTED");
		ctx->wifiIp[0] = '\0';
		return ESP32_EVT_WIFI_STATUS;
	}

	if (Esp32StrStartsWith(line, "SYS:"))
	{
		Esp32SafeCopy(ctx->lastSysMsg, (uint16_t)sizeof(ctx->lastSysMsg), line + 4);
		return ESP32_EVT_SYS_STATUS;
	}

	return ESP32_EVT_OTHER;
}


