#include "stm32f4xx.h"                  // Device header
#include "MyI2C.h"
#include "LCD_delay.h"
#include "AHT10.h"
#include "FreeRTOS.h"
#include "task.h"

#define AHT10_ADDR_WRITE          0x70U
#define AHT10_ADDR_READ           0x71U
#define AHT10_CMD_SOFT_RESET      0xBAU
#define AHT10_CMD_INIT_AHT10      0xE1U
#define AHT10_CMD_INIT_AHT20      0xBEU
#define AHT10_CMD_TRIGGER_MEASURE 0xACU

#define AHT10_STATUS_BUSY_MASK    0x80U
#define AHT10_STATUS_CAL_MASK     0x08U

int retry = 0;

static uint8_t AHT10_WriteCommand3(uint8_t command, uint8_t data1, uint8_t data2)
{
    MyI2C_Start();
    MyI2C_SendByte(AHT10_ADDR_WRITE);
    if (MyI2C_ReceiveAck() != 0)
    {
        MyI2C_Stop();
        return 1;
    }

    MyI2C_SendByte(command);
    if (MyI2C_ReceiveAck() != 0)
    {
        MyI2C_Stop();
        return 2;
    }

    MyI2C_SendByte(data1);
    if (MyI2C_ReceiveAck() != 0)
    {
        MyI2C_Stop();
        return 3;
    }

    MyI2C_SendByte(data2);
    if (MyI2C_ReceiveAck() != 0)
    {
        MyI2C_Stop();
        return 4;
    }

    MyI2C_Stop();
    return 0;
}

static void AHT10_DelayMs(uint32_t delay_ms_value)
{
    if (delay_ms_value == 0U)
    {
        return;
    }

    if ((__get_IPSR() == 0U) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
    {
        TickType_t ticks = pdMS_TO_TICKS(delay_ms_value);
        if (ticks == 0U)
        {
            ticks = 1U;
        }
        vTaskDelay(ticks);
        return;
    }

    while (delay_ms_value > 0U)
    {
        u16 step = (delay_ms_value > 65535U) ? 65535U : (u16)delay_ms_value;
        delay_ms(step);
        delay_ms_value -= step;
    }
}

uint8_t AHT10_CheckStatus(void) {
    MyI2C_Start();
    MyI2C_SendByte(AHT10_ADDR_READ);  // AHT10 读地址 (0x38 << 1 | 1)
    if (MyI2C_ReceiveAck() != 0) {
        MyI2C_Stop();
        return 0xFF;  // 设备无响应
    }
    
    uint8_t status = MyI2C_ReceiveByte();
    MyI2C_SendAck(1);  // 发送 NACK
    MyI2C_Stop();

    return status;
}



/**
  * 函    数：MPU6050写寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：无
  */
uint8_t AHT10_Init(void) {  
    MyI2C_Init();  
    AHT10_DelayMs(40);  // 等待上电稳定

    MyI2C_Start();
    MyI2C_SendByte(AHT10_ADDR_WRITE);
    if (MyI2C_ReceiveAck() != 0)
    {
        MyI2C_Stop();
        return 1;  // 设备无应答
    }
    MyI2C_SendByte(AHT10_CMD_SOFT_RESET);
    if (MyI2C_ReceiveAck() != 0)
    {
        MyI2C_Stop();
        return 2;  // 复位命令发送失败
    }
    MyI2C_Stop();

    AHT10_DelayMs(30);

    uint8_t status = AHT10_CheckStatus();
    if (status == 0xFF)
    {
        return 3;  // 状态读取失败
    }
    if ((status & AHT10_STATUS_CAL_MASK) != 0U)
    {
        return 0;  // 已完成校准
    }

    if (AHT10_WriteCommand3(AHT10_CMD_INIT_AHT10, 0x08, 0x00) != 0)
    {
        if (AHT10_WriteCommand3(AHT10_CMD_INIT_AHT20, 0x08, 0x00) != 0)
        {
            return 4;  // 初始化命令发送失败
        }
    }

    AHT10_DelayMs(20);

    status = AHT10_CheckStatus();
    if (status == 0xFF)
    {
        return 5;  // 状态读取失败
    }
    if ((status & AHT10_STATUS_CAL_MASK) == 0U)
    {
        return 6;  // 校准未完成
    }

    return 0;  // 初始化成功
}

uint8_t AHT10_ReadData(float *humidity, float *temperature) { 
    if ((humidity == NULL) || (temperature == NULL))
    {
        return 9;
    }

    MyI2C_Start(); 
    MyI2C_SendByte(AHT10_ADDR_WRITE);  // 发送 AHT10 写地址 
    if (MyI2C_ReceiveAck() != 0) { 
        MyI2C_Stop(); 
        return 6;  // 设备无应答 
    } 
 
    MyI2C_SendByte(AHT10_CMD_TRIGGER_MEASURE);  // 触发测量命令 
    if (MyI2C_ReceiveAck() != 0) { 
        MyI2C_Stop(); 
        return 7;  // 命令发送失败 
    } 
 
    MyI2C_SendByte(0x33); 
    if (MyI2C_ReceiveAck() != 0) {
        MyI2C_Stop();
        return 10;
    }
    MyI2C_SendByte(0x00); 
    if (MyI2C_ReceiveAck() != 0) {
        MyI2C_Stop();
        return 11;
    }
    MyI2C_Stop(); 

    uint8_t status = 0;
    uint8_t wait_cnt = 0;
    for (wait_cnt = 0; wait_cnt < 20U; wait_cnt++)
    {
        AHT10_DelayMs(10);
        status = AHT10_CheckStatus();
        if (status == 0xFF)
        {
            return 12;
        }
        if ((status & AHT10_STATUS_BUSY_MASK) == 0U)
        {
            break;
        }
    }
    if (wait_cnt >= 20U)
    {
        return 13;
    }
 
    // 读取 6 字节数据 
    MyI2C_Start(); 
    MyI2C_SendByte(AHT10_ADDR_READ);  // 发送 AHT10 读地址 
    if (MyI2C_ReceiveAck() != 0) { 
        MyI2C_Stop(); 
        return 8;  // 设备无应答 
    } 
 
    uint8_t buffer[6]; 
    for (uint8_t i = 0; i < 6; i++) { 
        buffer[i] = MyI2C_ReceiveByte(); 
        if (i < 5) MyI2C_SendAck(0);  // 发送ACK 
        else MyI2C_SendAck(1);  // 最后一个字节发送NACK 
    } 
    MyI2C_Stop(); 
 
    // 计算湿度（20bit） 
    uint32_t raw_humidity = ((buffer[1] << 16) | (buffer[2] << 8) | buffer[3]) >> 4; 
    *humidity = (raw_humidity / 1048576.0f) * 100.0f; 
 
    // 计算温度（20bit） 
    uint32_t raw_temperature = ((buffer[3] & 0x0F) << 16) | (buffer[4] << 8) | buffer[5]; 
    *temperature = (raw_temperature / 1048576.0f) * 200.0f - 50.0f; 
 
    return 0;  // 读取成功 
}


void Display_Temp_Humidity(void) { 
    float humidity, temperature; 
    if (AHT10_ReadData(&humidity, &temperature) == 0) { 
        uint16_t temp_int = (uint16_t)temperature;   // 整数部分 
        uint16_t temp_dec = (uint16_t)((temperature - temp_int) * 100);  // 小数部分，保留两位 

        uint16_t humi_int = (uint16_t)humidity; 
        uint16_t humi_dec = (uint16_t)((humidity - humi_int) * 100);
    }
}





