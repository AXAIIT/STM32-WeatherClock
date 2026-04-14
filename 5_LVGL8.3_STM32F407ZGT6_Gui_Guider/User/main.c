#include "LCD_delay.h"
#include "sys.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "LCD_test.h"
#include "usart.h"
#include "LCD_Timer.h"
#include "led.h"

#include "gui_guider.h"
#include "events_init.h"

#include "lvgl.h"                // 它为整个LVGL提供了更完整的头文件引用
#include "lv_port_disp.h"        // LVGL的显示支持
#include "lv_port_indev.h"       // LVGL的触屏支持

lv_ui guider_ui;



void LVGL_Test(void) {
    // 按钮
    lv_obj_t *myBtn = lv_btn_create(lv_scr_act());                               // 创建按钮; 父对象：当前活动屏幕
    lv_obj_set_pos(myBtn, 10, 10);                                               // 设置坐标
    lv_obj_set_size(myBtn, 120, 50);                                             // 设置大小
   
    // 按钮上的文本
    lv_obj_t *label_btn = lv_label_create(myBtn);                                // 创建文本标签，父对象：上面的btn按钮
    lv_obj_align(label_btn, LV_ALIGN_CENTER, 0, 0);                              // 对齐于：父对象
    lv_label_set_text(label_btn, "Test");                                        // 设置标签的文本
 
    // 独立的标签
    lv_obj_t *myLabel = lv_label_create(lv_scr_act());                           // 创建文本标签; 父对象：当前活动屏幕
    lv_label_set_text(myLabel, "Hello world!");                                  // 设置标签的文本
    lv_obj_align(myLabel, LV_ALIGN_CENTER, 0, 0);                                // 对齐于：父对象
    lv_obj_align_to(myBtn, myLabel, LV_ALIGN_OUT_TOP_MID, 0, -20);               // 对齐于：某对象
}


void TIM6_DAC_IRQHandler(void){
    if(TIM_GetITStatus(TIM6, TIM_IT_Update) == SET){
		lv_tick_inc(1);

		static uint16_t timer_counter = 0;
		timer_counter++;
        
        // 每500ms切换一次LED状态（500次1ms中断）
        if(timer_counter >= 500) {
            timer_counter = 0;
            LED1_Toggle;
        }
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    }
}


int main(void)
{
	//要严格的初始化顺序，
	//不同的初始化顺序可能会导致无法完成初始化，启动系统
    delay_init(168);     // 初始化延时函数
	LED_Init();         // LED初始化
    Usart_Config();     // USART初始化函数
	printf("系统时钟: %d MHz\r\n", SystemCoreClock / 1000000);
    printf("开始初始化...\r\n");
    
    // 1. 屏幕初始化
    LCD_Init();
    printf("屏幕初始化完成\r\n");
    
    // 2. 触屏初始化
    printf("开始触屏初始化...\r\n");
    if(tp_dev.init())
    {
        printf("触屏初始化失败!\r\n");
    }
    else
    {
        printf("触屏初始化成功!\r\n");
    }
    
    // 3. 设置为横屏模式
    LCD_direction(1);
    printf("设置横屏模式完成\r\n");
    
    // 4. 清屏并设置背景
    LCD_Clear(WHITE);
	printf("清屏完成, 颜色: %d\r\n", WHITE);  // 检查 WHITE 值是否为预期 (通常 0xFFFF 为白)
    printf("清屏完成\r\n");
    
    printf("LVGL初始化开始\r\n");
    lv_init();                             // LVGL 初始化
    lv_port_disp_init();                   // 注册LVGL的显示任务
    lv_port_indev_init();                  // 注册LVGL的触屏检测任务
    printf("LVGL初始化完成\r\n");
    
    LCD_Timer_Init();
    
    printf("全部初始化完成…………\r\n");
	
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);  // 所有初始化完成后启用中断
    
    setup_ui(&guider_ui);
    events_init(&guider_ui);
	
	
    while(1)
    {
        // delay_ms(5);
        // lv_timer_handler();
        static uint32_t last_time = 0;
		uint32_t now = lv_tick_get();
		if (now - last_time >= 5) {  // 5ms 间隔，约 200 Hz
			lv_timer_handler();
			last_time = now;
		}
    }
}

