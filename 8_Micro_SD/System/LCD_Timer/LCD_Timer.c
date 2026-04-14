#include "LCD_Timer.h"

/**
 *
 时间(秒) = ((TIM_Period + 1) × (TIM_Prescaler + 1)) ÷ 定时器时钟频率
 **/
void LCD_Timer_Init(void){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	TIM_InternalClockConfig(TIM6);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 84 - 1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseInitStructure);
	 
	TIM_ClearFlag(TIM6, TIM_FLAG_Update);
	 
	//TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM6, TIM_IT_Update, DISABLE);
	 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	 
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_SetCounter(TIM6, 0); 
	TIM_Cmd(TIM6, ENABLE);	 
}

/*
 void TIM6_DAC_IRQHandler(void){
  	if(TIM_GetITStatus(TIM6, TIM_IT_Update) == SET){
		LED1_Toggle;
  		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
  	}
  } 
*/
