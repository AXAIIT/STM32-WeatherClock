#include "stm32f4xx.h"
#include "led.h"   
#include "delay.h"  
#include "usart.h"
#include "LCD_Timer.h"


/**
  * @brief  获取并打印时钟频率信息
  * @param  无
  * @retval 无
  */
 void Print_ClockInfo(void)
 {
	 uint32_t sysclk_freq = 0;
	 uint32_t hclk_freq = 0;
	 uint32_t pclk1_freq = 0;
	 uint32_t pclk2_freq = 0;
	 uint32_t tim6_freq = 0;
	 uint32_t pll_m, pll_n, pll_p, pll_source;
	 
	 // 根据RCC->CFGR寄存器的SWS位判断当前系统时钟源
	 uint32_t clock_source = RCC->CFGR & RCC_CFGR_SWS;
	 
	 if(clock_source == RCC_CFGR_SWS_HSI)
	 {
		 // HSI作为系统时钟
		 sysclk_freq = HSI_VALUE;
		 printf("系统时钟源: HSI\r\n");
	 }
	 else if(clock_source == RCC_CFGR_SWS_HSE)
	 {
		 // HSE作为系统时钟
		 sysclk_freq = HSE_VALUE;
		 printf("系统时钟源: HSE\r\n");
	 }
	 else if(clock_source == RCC_CFGR_SWS_PLL)
	 {
		 // PLL作为系统时钟，需要计算PLL频率
		 pll_source = RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC;
		 pll_m = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
		 pll_n = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6;
		 pll_p = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1) * 2;
		 
		 if(pll_source == RCC_PLLCFGR_PLLSRC_HSE)
		 {
			 // PLL时钟源为HSE
			 sysclk_freq = (HSE_VALUE / pll_m) * pll_n / pll_p;
			 printf("系统时钟源: PLL(HSE)\r\n");
		 }
		 else
		 {
			 // PLL时钟源为HSI
			 sysclk_freq = (HSI_VALUE / pll_m) * pll_n / pll_p;
			 printf("系统时钟源: PLL(HSI)\r\n");
		 }
		 
		 // 打印PLL配置参数
		 printf("PLL配置: M=%d, N=%d, P=%d\r\n", pll_m, pll_n, pll_p);
	 }
	 
	 // 计算HCLK频率(AHB总线)
	 uint32_t hpre = (RCC->CFGR & RCC_CFGR_HPRE) >> 4;
	 uint32_t hpre_div_table[16] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 8, 16, 64, 128, 256, 512};
	 hclk_freq = sysclk_freq / hpre_div_table[hpre];
	 
	 // 计算PCLK1频率(APB1总线)
	 uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> 10;
	 uint32_t ppre1_div_table[8] = {1, 1, 1, 1, 2, 4, 8, 16};
	 pclk1_freq = hclk_freq / ppre1_div_table[ppre1 & 0x7];
	 
	 // 计算PCLK2频率(APB2总线)
	 uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> 13;
	 uint32_t ppre2_div_table[8] = {1, 1, 1, 1, 2, 4, 8, 16};
	 pclk2_freq = hclk_freq / ppre2_div_table[ppre2 & 0x7];
	 
	 // 计算TIM6频率
	 // 如果APB1预分频器>1，则TIM6时钟为APB1频率×2；否则等于APB1频率
	 if(ppre1_div_table[ppre1 & 0x7] > 1)
	 {
		 tim6_freq = pclk1_freq * 2;
	 }
	 else
	 {
		 tim6_freq = pclk1_freq;
	 }
	 
	 // 打印各时钟频率信息
	 printf("===== 时钟频率信息 =====\r\n");
	 printf("SYSCLK: %d Hz\r\n", sysclk_freq);
	 printf("HCLK  : %d Hz\r\n", hclk_freq);
	 printf("PCLK1 : %d Hz\r\n", pclk1_freq);
	 printf("PCLK2 : %d Hz\r\n", pclk2_freq);
	 printf("TIM6  : %d Hz\r\n", tim6_freq);
	 printf("========================\r\n\r\n");
 }



/***************************************************************************************************
*	函 数 名: main
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 运行主程序
*	说    明: 无
****************************************************************************************************/
int main(void)
{
	LCD_Timer_Init();
	uint16_t a = 128;   //测试变量
	float b = 9.123456; //测试变量	
			
	Delay_Init();		//延时函数初始化
	LED_Init();			//LED初始化
	Usart_Config ();	// USART初始化函数
	
	printf("STM32 串口实验 \r\n");
	printf("printf函数测试 \r\n");	

	Print_ClockInfo();
		
	while (1)
	{
		// LED1_Toggle;
		// Delay_ms(1000);
//		printf("十进制格式：  %d\r\n",a); 
//		printf("十六进制格式：%x\r\n",a);
//		printf("小数格式：    %f\r\n",b);	
	}
	
}


void TIM6_DAC_IRQHandler(void){
    if(TIM_GetITStatus(TIM6, TIM_IT_Update) == SET){
		static uint16_t timer_counter = 0;
		timer_counter++;
        LED1_Toggle;  // 直接翻转LED状态
		printf("%d ms\r\n", timer_counter);
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    }
}


