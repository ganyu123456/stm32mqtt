#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"	 
#include "math.h"			
#include "stdio.h"
#include "stm32f10x_flash.h"
#include "stdlib.h"
#include "string.h"
#include "wdg.h"
#include "timer.h"
#include "stm32f10x_tim.h"
#include "4G.h"
#include "adc.h"
#include "spi.h"
#include "flash.h"
#include "stm32f10x_adc.h"
#include "dht11.h"

///////////////下面是液晶屏头文件/////////////////////
#include "Lcd_Driver.h"
#include "GUI.h"
#include "delay.h"
#include "Picture.h"
#include "QDTFT_demo.h"
/////////////////////////////////////////////////////


void RCC1_Configuration(void)
{
  /* 使能APB2时钟 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA  | RCC_APB2Periph_GPIOB  | 
                         RCC_APB2Periph_GPIOC  | RCC_APB2Periph_GPIOD  | 
                         RCC_APB2Periph_GPIOE  | RCC_APB2Periph_GPIOF  | 
                         RCC_APB2Periph_AFIO   | RCC_APB2Periph_USART1 , ENABLE);

  /* 使能APB1时钟 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 | RCC_APB1Periph_SPI2, ENABLE);  

  /* 使能APB时钟 */
}

///***使用于4G版本**************/
 int main(void)
 {
	 
	 	RCC1_Configuration();	 
    delay_init();	    	 													//延时函数初始化	  
    NVIC_Configuration(); 	 											//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
		CSTX_4GCTR_Init();       										  //初始化CSTX_4G的供电引脚 对模块进行供电
		uart_init(115200);														//串口1初始化，可连接PC进行打印模块返回数据
	  Uart1_SendStr("UART1 Init Successful\r\n");
    LED_Init();		  															//初始化与LED连接的硬件接口
		LED_Run();																		//初始化跑马灯
    uart2_init(115200);														//初始化和EC200连接串口	
		Uart2_SendStr("UART2 Init Successful\r\n");
		uart3_init(115200);
		Uart3_SendStr("UART3 Init Successful\r\n");
		printf("\r\n ############ http://www.csgsm.com/ ############\r\n ############("__DATE__ " - " __TIME__ ")############\r\n"); 
	 		//////////下面是液晶屏显示代码///////////////////////////
		Lcd_Init();
		LCD_LED_SET;//通过IO控制背光亮				
		//Redraw_Mainmenu();//绘制主菜单(部分内容由于分辨率超出物理值可能无法显示)
		//Color_Test();//简单纯色填充测试
		Num_Test();//数码管字体测试
			
		showimage(gImage_qq);//图片显示示例
		Font_Test();//中英文显示测试	
		delay_ms(1200);
		//LCD_LED_CLR;//IO控制背光灭	
	/////////////////////////////////////////////////////////////
	
		Init_TIM2();
		clean_time2_flags();
		Adc_Init();	  																//ADC初始化
    CSTX_4G_Init();																//对设备初始化
		Start_GPS();																  //打开GPS，并等待GPS定位成功

    CSTX_4G_RegALIYUNIOT();												//直接注册到EMQX
		Gui_DrawFont_GBK16(0,70,RED,WHITE,"connect ok");
		Clear_Buffer_UART1();													//清空串口1的数据
		DHT11_Init();																	//初始化温湿度 用PA11 
		while(1)
    { 
				CSTX_4G_ALYIOTADC(); //上传ADC值
				delay_ms(1000);
				CSTX_4G_ALYIOTSenddata();	//上传温湿度值			
				delay_ms(1000);
				Get_GPS_RMC();//上传GPS数值，已纠偏
				delay_ms(1000);
				LED1=!LED1; //系统运行指示灯
				showimage(gImage_qq);
			  Gui_DrawFont_GBK16(0,50,RED,WHITE,"Data ok");
			  Gui_DrawFont_GBK16(0,70,RED,WHITE,"connect ok");
				CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
				IWDG_Feed();//喂狗 
    }	 
 }






