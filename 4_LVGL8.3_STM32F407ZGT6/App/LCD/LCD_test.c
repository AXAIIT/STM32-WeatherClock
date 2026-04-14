#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "LCD_delay.h"
#include "gui.h"
#include "LCD_test.h"
#include "touch.h"
#include "pic.h"
#include "ft6336.h"

//========================variable==========================//
u16 ColorTab[5]={RED,GREEN,BLUE,YELLOW,BRED};//定义颜色数组
u16 ColornTab[8]={RED,MAGENTA,GREEN,DARKBLUE,BLUE,BLACK,LIGHTGREEN};
const u16 POINT_COLOR_TBL = RED;
//=====================end of variable======================//

/*****************************************************************************
 * @name       :void DrawTestPage(u8 *str)
 * @date       :2018-08-09 
 * @function   :Drawing test interface
 * @parameters :str:the start address of the Chinese and English strings
 * @retvalue   :None
******************************************************************************/ 
void DrawTestPage(u8 *str)
{
//绘制固定栏up
LCD_Clear(WHITE);
LCD_Fill(0,0,lcddev.width,20,BLUE);
//绘制固定栏down
LCD_Fill(0,lcddev.height-20,lcddev.width,lcddev.height,BLUE);
POINT_COLOR=WHITE;
Gui_StrCenter(0,2,WHITE,BLUE,str,16,1);//居中显示
Gui_StrCenter(0,lcddev.height-18,WHITE,BLUE,"http://www.lcdwiki.com",16,1);//居中显示
//绘制测试区域
//LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE);
}

/*****************************************************************************
 * @name       :void main_test(void)
 * @date       :2018-08-09 
 * @function   :Drawing the main Interface of the Comprehensive Test Program
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void main_test(void)
{
	DrawTestPage("全动电子综合测试程序");	
	Gui_StrCenter(0,30,RED,BLUE,"全动电子",16,1);//居中显示
	Gui_StrCenter(0,60,RED,BLUE,"综合测试程序",16,1);//居中显示	
	Gui_StrCenter(0,90,BRED,BLUE,"3.5\" IPS ST7796 320X480",16,1);//居中显示
	Gui_StrCenter(0,120,BLUE,BLUE,"xiaoFeng@QDtech 2022-12-20",16,1);//居中显示
	delay_ms(1500);		
	delay_ms(1500);
}

/*****************************************************************************
 * @name       :void Test_Color(void)
 * @date       :2018-08-09 
 * @function   :Color fill test(white,black,red,green,blue)
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Test_Color(void)
{
	//DrawTestPage("测试1:纯色填充测试");
	LCD_Fill(0,0,lcddev.width,lcddev.height,WHITE);
	Show_Str(20,30,BLACK,YELLOW,"WHITE",16,1);delay_ms(800);
	LCD_Fill(0,0,lcddev.width,lcddev.height,BLACK);
	Show_Str(20,30,WHITE,YELLOW,"BLACK",16,1);delay_ms(800);
	LCD_Fill(0,0,lcddev.width,lcddev.height,RED);
	Show_Str(20,30,BLUE,YELLOW,"RED ",16,1);delay_ms(800);
	LCD_Fill(0,0,lcddev.width,lcddev.height,GREEN);
	Show_Str(20,30,BLUE,YELLOW,"GREEN ",16,1);delay_ms(800);
	LCD_Fill(0,0,lcddev.width,lcddev.height,BLUE);
	Show_Str(20,30,RED,YELLOW,"BLUE ",16,1);delay_ms(800);
	LCD_Fill(0,0,lcddev.width,lcddev.height,GRAY);
	Show_Str(20,30,MAGENTA,YELLOW,"GRAY",16,1);delay_ms(800);
}

/*****************************************************************************
 * @name       :void Test_FillRec(void)
 * @date       :2018-08-09 
 * @function   :Rectangular display and fill test
								Display red,green,blue,yellow,pink rectangular boxes in turn,
								1500 milliseconds later,
								Fill the rectangle in red,green,blue,yellow and pink in turn
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Test_FillRec(void)
{
	u8 i=0;
	DrawTestPage("测试3:GUI矩形填充测试");
	LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE);
	for (i=0; i<5; i++) 
	{
		POINT_COLOR=ColorTab[i];
		LCD_DrawRectangle(lcddev.width/2-80+(i*15),lcddev.height/2-80+(i*15),lcddev.width/2-80+(i*15)+60,lcddev.height/2-80+(i*15)+60); 
	}
	delay_ms(1500);	
	LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE); 
	for (i=0; i<5; i++) 
	{
		POINT_COLOR=ColorTab[i];
		LCD_DrawFillRectangle(lcddev.width/2-80+(i*15),lcddev.height/2-80+(i*15),lcddev.width/2-80+(i*15)+60,lcddev.height/2-80+(i*15)+60); 
	}
	delay_ms(1500);
}

/*****************************************************************************
 * @name       :void Test_Circle(void)
 * @date       :2018-08-09 
 * @function   :circular display and fill test
								Display red,green,blue,yellow,pink circular boxes in turn,
								1500 milliseconds later,
								Fill the circular in red,green,blue,yellow and pink in turn
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Test_Circle(void)
{
	u8 i=0;
	DrawTestPage("测试4:GUI画圆填充测试");
	LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE);
	for (i=0; i<5; i++)  
		gui_circle(lcddev.width/2-80+(i*25),lcddev.height/2-50+(i*25),ColorTab[i],30,0);
	delay_ms(1500);	
	LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE); 
	for (i=0; i<5; i++) 
	  	gui_circle(lcddev.width/2-80+(i*25),lcddev.height/2-50+(i*25),ColorTab[i],30,1);
	delay_ms(1500);
}

/*****************************************************************************
 * @name       :void English_Font_test(void)
 * @date       :2018-08-09 
 * @function   :English display test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void English_Font_test(void)
{
	DrawTestPage("测试6:英文显示测试");
	Show_Str(10,30,BLUE,YELLOW,"6X12:abcdefghijklmnopqrstuvwxyz0123456789",12,0);
	Show_Str(10,45,BLUE,YELLOW,"6X12:ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",12,1);
	Show_Str(10,60,BLUE,YELLOW,"6X12:~!@#$%^&*()_+{}:<>?/|-+.",12,0);
	Show_Str(10,80,BLUE,YELLOW,"8X16:abcdefghijklmnopqrstuvwxyz0123456789",16,0);
	Show_Str(10,100,BLUE,YELLOW,"8X16:ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",16,1);
	Show_Str(10,120,BLUE,YELLOW,"8X16:~!@#$%^&*()_+{}:<>?/|-+.",16,0); 
	delay_ms(1200);
}

/*****************************************************************************
 * @name       :void Test_Triangle(void)
 * @date       :2018-08-09 
 * @function   :triangle display and fill test
								Display red,green,blue,yellow,pink triangle boxes in turn,
								1500 milliseconds later,
								Fill the triangle in red,green,blue,yellow and pink in turn
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Test_Triangle(void)
{
	u8 i=0;
	DrawTestPage("测试5:GUI Triangle填充测试");
	LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE);
	for(i=0;i<5;i++)
	{
		POINT_COLOR=ColorTab[i];
		Draw_Triangel(lcddev.width/2-80+(i*20),lcddev.height/2-20+(i*15),lcddev.width/2-50-1+(i*20),lcddev.height/2-20-52-1+(i*15),lcddev.width/2-20-1+(i*20),lcddev.height/2-20+(i*15));
	}
	delay_ms(1500);	
	LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE); 
	for(i=0;i<5;i++)
	{
		POINT_COLOR=ColorTab[i];
		Fill_Triangel(lcddev.width/2-80+(i*20),lcddev.height/2-20+(i*15),lcddev.width/2-50-1+(i*20),lcddev.height/2-20-52-1+(i*15),lcddev.width/2-20-1+(i*20),lcddev.height/2-20+(i*15));
	}
	delay_ms(1500);
}

/*****************************************************************************
 * @name       :void Chinese_Font_test(void)
 * @date       :2018-08-09 
 * @function   :chinese display test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Chinese_Font_test(void)
{	
	DrawTestPage("测试7:中文显示测试");
	Show_Str(10,30,BLUE,YELLOW,"16X16:全动电子技术有限公司欢迎您",16,0);
	Show_Str(10,50,BLUE,YELLOW,"16X16:Welcome全动电子",16,0);
	Show_Str(10,70,BLUE,YELLOW,"24X24:深圳市中文测试",24,1);
	Show_Str(10,100,BLUE,YELLOW,"32X32:字体测试",32,1);
	delay_ms(1200);
}

/*****************************************************************************
 * @name       :void Pic_test(void)
 * @date       :2018-08-09 
 * @function   :picture display test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Pic_test(void)
{
	DrawTestPage("测试8:图片显示测试");
	//LCD_Fill(0,20,lcddev.width,lcddev.height-20,WHITE);
	Gui_Drawbmp16(30,30,gImage_qq);
	Show_Str(30+12,75,BLUE,YELLOW,"QQ",16,1);
	Gui_Drawbmp16(90,30,gImage_qq);
	Show_Str(90+12,75,BLUE,YELLOW,"QQ",16,1);
	Gui_Drawbmp16(150,30,gImage_qq);
	Show_Str(150+12,75,BLUE,YELLOW,"QQ",16,1);
	delay_ms(1200);
}

/*****************************************************************************
 * @name       :void Rotate_Test(void)
 * @date       :2018-08-09 
 * @function   :rotate test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Rotate_Test(void)
{
	u8 i=0;
	u8 *Direction[4]={"Rotation:0","Rotation:90","Rotation:180","Rotation:270"};
	
	for(i=0;i<4;i++)
	{
	LCD_direction(i);
	DrawTestPage("测试10:屏幕旋转测试");
	Show_Str(20,30,BLUE,YELLOW,Direction[i],16,1);
	Gui_Drawbmp16(30,50,gImage_qq);
	delay_ms(1000);delay_ms(1000);
	}
	LCD_direction(USE_HORIZONTAL);
}


void Touch_Pen_Test(void)
{
//	u8 i=0,j=0;	 
 	u16 lastpos[2];		//最后一次的数据 
	DrawTestPage("测试12:触摸PEN测试");
	LCD_ShowString(lcddev.width-32,2,16,"RST",1);//显示清屏区域
	POINT_COLOR=RED;//设置画笔蓝色 //清除
	while(1)
	{
		//j++;
		tp_dev.scan();
		//for(t=0;t<CTP_MAX_TOUCH;t++)//最多5点触摸
		//{
			if((tp_dev.sta)&(1<<0))//判断是否有点触摸？
			{
				if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)//在LCD范围内
				{
					if(lastpos[0]==0XFFFF)
					{
						lastpos[0] = tp_dev.x[0];
						lastpos[1] = tp_dev.y[0];
					}
					if(tp_dev.x[0]>(lcddev.width-32)&&tp_dev.y[0]<18)
					{
							//if(j>1) //防止点击一次，多次清屏
							//{
							//	continue;
							//}
							tp_dev.x[0]=0xFFFF;
							tp_dev.x[0]=0xFFFF;
							DrawTestPage("测试12:触摸PEN测试");
							LCD_ShowString(lcddev.width-32,2,16,"RST",1);//显示清屏区域
							POINT_COLOR=RED;//设置画笔蓝色 //清除
					}
					else
					{
							LCD_DrawLine2(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],2,POINT_COLOR_TBL);//画线
					}
					lastpos[0]=tp_dev.x[0];
					lastpos[1]=tp_dev.y[0];
				}
			}else lastpos[0]=0XFFFF;
		//}
		
		//delay_ms(1);
		//i++;
		//if(i%30==0)LED0=!LED0;
		//if(j>4)
		//{
		//	j=0;
		//}
	} 
}

void Touch_Button_Test(void)
{  
	DrawTestPage("测试11:触摸按键测试");
	POINT_COLOR = BLACK;
	LCD_DrawRectangle(lcddev.width/2-50, 40, lcddev.width/2+50, 90);
	POINT_COLOR = RED;
	LCD_DrawFillRectangle(lcddev.width/2-49, 41, lcddev.width/2-1, 89);
	POINT_COLOR = BLUE;
	LCD_DrawFillRectangle(lcddev.width/2, 41, lcddev.width/2+49, 89);
	POINT_COLOR = WHITE;
	LCD_ShowString(lcddev.width/2-32,57,16,"ON",1);	
	POINT_COLOR = GRAY;
	LCD_FillRoundRectangle(lcddev.width/2-110, 150, lcddev.width/2+110, 169,10);
	POINT_COLOR = BRED;
	LCD_FillRoundRectangle(lcddev.width/2-110, 150, lcddev.width/2+10, 169,10);
	gui_circle(lcddev.width/2, 160,DARKBLUE,10, 1);
	POINT_COLOR = BLACK;
	LCD_ShowString(lcddev.width/2-125,152,16,"0",1);
	LCD_ShowString(lcddev.width/2+115,152,16,"100",1);
	POINT_COLOR = RED;
	LCD_ShowNum(lcddev.width/2-12,125,50,3,16);
	POINT_COLOR = BLACK;
	LCD_DrawRoundRectangle(lcddev.width/2-30, 220, lcddev.width/2+30, 250,8); 
	POINT_COLOR = LGRAYBLUE;
	LCD_FillRoundRectangle(lcddev.width/2-29, 221, lcddev.width/2+29, 248,7);
	POINT_COLOR = MAGENTA;
	LCD_ShowString(lcddev.width/2-15,227,16,"EXIT",1);
	Show_Str(lcddev.width/2-47,195,RED,WHITE,"按EXIT键退出",16,1);
	while(1)
	{
		tp_dev.scan();
		if((tp_dev.sta)&(1<<0))//判断是否有点触摸？
		{
			if((tp_dev.y[0]>40)&&(tp_dev.y[0]<90))
			{
				if((tp_dev.x[0]>(lcddev.width/2-50))&&(tp_dev.x[0]<(lcddev.width/2-1)))
				{
						POINT_COLOR = RED;
						LCD_DrawFillRectangle(lcddev.width/2-49, 41, lcddev.width/2-1, 89);
						POINT_COLOR = BLUE;
						LCD_DrawFillRectangle(lcddev.width/2, 41, lcddev.width/2+49, 89);
						POINT_COLOR = WHITE;
						LCD_ShowString(lcddev.width/2-32,57,16,"ON",1);	
				}
				if((tp_dev.x[0]>(lcddev.width/2))&&(tp_dev.x[0]<(lcddev.width/2+50)))
				{
						POINT_COLOR = BLUE;
						LCD_DrawFillRectangle(lcddev.width/2-49, 41, lcddev.width/2-1, 89);
						POINT_COLOR = RED;
						LCD_DrawFillRectangle(lcddev.width/2, 41, lcddev.width/2+49, 89);
						POINT_COLOR = WHITE;
						LCD_ShowString(lcddev.width/2+13,57,16,"OFF",1);	
				}
			}	
			if((tp_dev.x[0]>=(lcddev.width/2-100))&&(tp_dev.x[0]<=(lcddev.width/2+100))&&(tp_dev.y[0]>150)&&(tp_dev.y[0]<169))
			{
					POINT_COLOR = GRAY;
					LCD_FillRoundRectangle(tp_dev.x[0]-10, 150, lcddev.width/2+110, 169,10);
					POINT_COLOR = BRED;
					LCD_FillRoundRectangle(lcddev.width/2-110, 150, tp_dev.x[0]+10, 169,10);
					gui_circle(tp_dev.x[0], 160,DARKBLUE,10, 1);
					POINT_COLOR = RED;
					LCD_ShowNum(lcddev.width/2-12,125,(tp_dev.x[0]-(lcddev.width/2-100))/2,3,16);
			}
			if((tp_dev.x[0]>=(lcddev.width/2-30))&&(tp_dev.x[0]<=(lcddev.width/2+30))&&(tp_dev.y[0]>220)&&(tp_dev.y[0]<250))
			{
					POINT_COLOR = WHITE;
					LCD_DrawRoundRectangle(lcddev.width/2-30, 220, lcddev.width/2+30, 250,8); 
					POINT_COLOR = LBBLUE;
					LCD_FillRoundRectangle(lcddev.width/2-29, 221, lcddev.width/2+29, 248,7);
					POINT_COLOR = LIGHTGREEN;
					LCD_ShowString(lcddev.width/2-15,227,16,"EXIT",1);
					tp_dev.x[0]=0xFFFF;
					break;
			}
		}
	} 
}

/*****************************************************************************
 * @name       :void Touch_Test(void)
 * @date       :2018-08-09 
 * @function   :touch test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Touch_Test(void)
{
	if(tp_dev.init())
	{
		return;
	}
//	LED_Init();
	Touch_Button_Test();
	Touch_Pen_Test();
}

/*****************************************************************************
 * @name       :void Test_Read(void)
 * @date       :2018-11-13 
 * @function   :read test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Test_Read(void)
{
	u16 lcd_id,color;
	u8 buf[10] = {0},i;
	u8 cbuf[35] = {0};
	DrawTestPage("测试2:读ID和颜色值测试");
	lcd_id = LCD_Read_ID();
	sprintf((char *)buf,"ID:0x%x",lcd_id);
	Show_Str(50,25,BLUE,YELLOW,buf,16,1);
	for (i=0; i<7; i++) 
	{
		POINT_COLOR=ColornTab[i];
		LCD_DrawFillRectangle(20-10,55+i*25-10,20+10,55+i*25+10);
		color = LCD_ReadPoint(20,55+i*25);
		if(POINT_COLOR==color)
		{
			strcpy((char*)buf, "OK");
		}
		else
		{
			strcpy((char*)buf, "ERROR");
		}
		sprintf((char *)cbuf,"read color:0x%04X  %s",color, buf);
		Show_Str(20+20,55+i*25-8,POINT_COLOR,YELLOW,cbuf,16,1);
	}
	delay_ms(1500);
}

/*****************************************************************************
 * @name       :void Test_Dynamic_Num(void)
 * @date       :2018-11-13 
 * @function   :Dynamic number test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Test_Dynamic_Num(void)
{
		u8 i;
	DrawTestPage("测试9:动态数字显示");
	POINT_COLOR=BLUE;
	srand(123456);
	LCD_ShowString(15,50,16, " HCHO:           ug/m3",1);
	LCD_ShowString(15,70,16, "  CO2:           ppm",1);
	LCD_ShowString(15,90,16, " TVOC:           ug/m3",1);
	LCD_ShowString(15,110,16,"PM2.5:           ug/m3",1);
	LCD_ShowString(15,130,16," PM10:           ug/m3",1);
	LCD_ShowString(15,150,16,"  TEP:           C",1);
	LCD_ShowString(15,170,16,"  HUM:           %",1);
	POINT_COLOR=RED;
	for(i=0;i<15;i++)
	{
		LCD_ShowNum(100,50,rand()%10000,5,16);
		LCD_ShowNum(100,70,rand()%10000,5,16);
		LCD_ShowNum(100,90,rand()%10000,5,16);
		LCD_ShowNum(100,110,rand()%10000,5,16);
		LCD_ShowNum(100,130,rand()%10000,5,16);
		LCD_ShowNum(100,150,rand()%50,5,16);
		LCD_ShowNum(100,170,rand()%100,5,16);
		delay_ms(500);
	}
}
