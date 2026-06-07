#include "led.h"
#include "delay.h"

//初始化PA15 PA1   
//LED IO初始化
void LED_Init(void)
{
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
	//PB3 PB4 需要设置复用
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);	//打开GPIO使能时钟，先开复用功能
 GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//映射PB3 PB4 口到GPIO功能 
	

		//LED1-->PB5 LED2-->PB4 LED3-->PB3
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_4|GPIO_Pin_3;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //输出口
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //频率
 GPIO_Init(GPIOB, &GPIO_InitStructure);	  				 //初始化PB口
	//PB5 PB4 PB3
 GPIO_SetBits(GPIOB,GPIO_Pin_5|GPIO_Pin_4|GPIO_Pin_3); 	//关闭灯 			

}
 

/*****************************************************
跑马灯
*****************************************************/
void LED_Run(void)
{
	
	GPIO_ResetBits(GPIOB,GPIO_Pin_5); 	
	delay_ms(1000);
	GPIO_ResetBits(GPIOB,GPIO_Pin_4); 	
	delay_ms(1000);
	GPIO_ResetBits(GPIOB,GPIO_Pin_3); 	
	delay_ms(1000);
	GPIO_SetBits(GPIOB,GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5); 	
}
 

/*****************************************************
开启模块 低电平对模块进行供电 高电平不对模块进行供电
*****************************************************/
void CSTX_4GCTR_Init(void)
{  
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //使能PC端口时钟
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;				 		//PA12直接控制模块进行供电端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);		    //PC3 输出高
	
		GPIO_SetBits(GPIOA,GPIO_Pin_12); 	
		delay_ms(1000);
		GPIO_ResetBits(GPIOA,GPIO_Pin_12); 	 
}









