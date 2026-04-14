#include "stm32f4xx.h"
#include "led.h"   
#include "delay.h"


int main(void)
{
	LED_Init();			//LED≥ı ºªØ

	while (1)
	{		
		LED1_Toggle;
		Delay_ms(1000);	
	}	
}





