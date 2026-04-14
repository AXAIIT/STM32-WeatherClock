#ifndef __AHT10_H
#define __AHT10_H

#define  AHT10_SCL_SDA_GPIO_CLK        RCC_AHB1Periph_GPIOB
#define  AHT10_SCL_SDA_GPIO_PORT       GPIOB
#define  AHT10_SCL_GPIO_PIN       	   GPIO_Pin_10
#define  AHT10_SDA_GPIO_PIN       	   GPIO_Pin_11

uint8_t AHT10_Init(void);
uint8_t AHT10_ReadData(float *humidity, float *temperature);
void Display_Temp_Humidity(void);

#endif
