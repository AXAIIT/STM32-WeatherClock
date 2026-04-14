#include "LCD_delay.h"
#include "sys.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "LCD_test.h"

int main(void)
{	
	delay_init(168);     //初始化延时函数
	LCD_Init();	   //液晶屏初始化
  //循环测试
	while(1)
	{
		main_test(); 		//测试主界面
		Test_Read();        //读ID和GRAM测试
		Test_Color();  		//简单刷屏填充测试
		Test_FillRec();		//GUI矩形绘图测试
		Test_Circle(); 		//GUI画圆测试
		Test_Triangle();    //GUI三角形绘图测试
		English_Font_test();//英文字体示例测试
		Chinese_Font_test();//中文字体示例测试
		Pic_test();			//图片显示示例测试
		Test_Dynamic_Num(); //动态数字显示
		Rotate_Test();   //旋转显示测试
		//如果不带触摸，或者不需要触摸功能，请注释掉下面触摸屏测试项
		Touch_Test();		//触摸屏手写测试  
	}
}

