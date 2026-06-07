#include "4G.h"
#include "string.h"
#include "usart.h"
#include "wdg.h"
#include "led.h"
#include "dht11.h"
#include "adc.h"
#include "stm32f10x_adc.h"
#include "flash.h"
#include "delay.h"

/////////////////////////////////////////////////////
u8 res=1;
char GPRMCSTR[128]; //转载GPS信息 GPRMC 经纬度存储的字符串
float tempdata[2];
struct 
{		//经纬度反了
    char Latitude[10];//经度原数据
    char longitude[9];//经度源数据
    char Latitudess[3];//整数部分
    char longitudess[2];
    char Latitudedd[7];//小数点部分
    char longitudedd[7];
    char TrueLatitude[10];//转换过数据
    char Truelongitude[9];//转换过数据
}LongLatidata;

char LongitudeStr[20];        //字符串格式温度
char LatitudeStr[20];        //字符串格式湿度
/////////////////////////////////////////////////////

char *strx,*extstrx;
u16 adcx1,adcx2; //ADC1和ADC2数值
int MQTTVAL=0;
CSTX_4G CSTX_4G_Status;	//模块的状态信息
int  errcount=0;	//发送命令失败次数 防止死循环
int  errCountData=0;
u8 EC20_CIMI[BUFLEN];
u8 EC20_CGSN[BUFLEN];
char ATSTR[BUFLEN];	//组建AT命令的函数
char IMEINUMBER[BUFLEN];//+CGSN: "869523052178994"


/*****************************************************
清空模块反馈的信息
*****************************************************/
void Clear_Buffer(void)//清空缓存
{
		printf(buf_uart2.buf);
	
		strx=strstr((const char*)buf_uart2.buf,(const char*)"+QIURC");//返回+QIURC:，表明接收到TCP服务器发回的数据
    if(strx)
    {
//				Gui_DrawFont_GBK16(16,10,RED,WHITE, "RECEIVE DATA");      
    }
		
    delay_ms(300);
    buf_uart2.index=0;
    memset(buf_uart2.buf,0,BUFLEN);
		IWDG_Feed();//喂狗
	
}

void Clear_Buffer1(void)//清空缓存，但不打印数据
{
//		printf(buf_uart2.buf);
	
		strx=strstr((const char*)buf_uart2.buf,(const char*)"+QIURC");//返回+QIURC:，表明接收到TCP服务器发回的数据
    if(strx)
    {
//				Gui_DrawFont_GBK16(16,10,RED,WHITE, "RECEIVE DATA");      
    }
		
    delay_ms(300);
    buf_uart2.index=0;
    memset(buf_uart2.buf,0,BUFLEN);
		IWDG_Feed();//喂狗
	
}


/*****************************************************
初始化模块 和单片机连接，获取卡号和信号质量
*****************************************************/
void CSTX_4G_Init(void)
{
		//打印初始化信息
		printf("start init EC800M\r\n");
		//发第一个命令ATE1
    Uart2_SendStr("ATE1\r\n"); 
    delay_ms(300);
		printf(buf_uart2.buf);      //打印串口收到的信息
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
    Clear_Buffer();	
    while(strx==NULL)
    {
				printf("单片机正在连接模块......\r\n");
        Clear_Buffer();	
        Uart2_SendStr("ATE1\r\n"); 
        delay_ms(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
    }
		printf("****单片机和模块连接成功*****\r\n");
		
		//获取模块的版本
		Uart2_SendStr("ATI\r\n");
		delay_ms(300);
		Clear_Buffer();	
		
		//获取卡号，类似是否存在卡的意思，比较重要。
    Uart2_SendStr("AT+CIMI\r\n");
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"460");//返460，表明识别到卡了
    while(strx==NULL)
    {
        Clear_Buffer();	
        Uart2_SendStr("AT+CIMI\r\n");//获取卡号，类似是否存在卡的意思，比较重要。
        delay_ms(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"460");//返回OK,说明卡是存在的
    }
		delay_ms(2000);
		memcpy(EC20_CIMI,buf_uart2.buf+10,15);
		Clear_Buffer();
		
		//获得GSM模块的IMEI序列号
		Uart2_SendStr("AT+CGSN\r\n");
		delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");
    while(strx==NULL)
    {
        Clear_Buffer();	
        Uart2_SendStr("AT+CGSN\r\n");
        delay_ms(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");
    }
		memcpy(EC20_CGSN,buf_uart2.buf+10,15);
		Clear_Buffer();	
		
		//打印IMEI和CIMI号
		printf("CIMI:%s，CGSN:%s\r\n",EC20_CIMI,EC20_CGSN);
		Clear_Buffer();	
		
		//查询激活状态
		Uart2_SendStr("AT+CGATT?\r\n");
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//返1
		Clear_Buffer();	
		while(strx==NULL)
		{
				Clear_Buffer();	
				Uart2_SendStr("AT+CGATT?\r\n");//获取激活状态
				delay_ms(300);
				strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//返回1,表明注网成功
		}
			
		//查看获取CSQ值
		Clear_Buffer();	
		Uart2_SendStr("AT+CSQ\r\n");
		delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CSQ:");//返回CSQ
		if(strx)
		{
				
				printf("信号质量是:%s 注意：信号最大值是31 \r\n",buf_uart2.buf+14);      
		}
		IWDG_Feed();//喂狗
}



/*****************************************************
关闭之前存在的和服务器的链接 可能反馈失败 
*****************************************************/
void CSTX_4G_ConTCP(void)
{		
		//关闭之前建立的链接
		Uart2_SendStr("AT+QICLOSE=0\r\n");//关闭socekt连接
		delay_ms(100);
		Uart2_SendStr("AT+QICLOSE=1\r\n");//关闭socekt连接
		delay_ms(100);
		Uart2_SendStr("AT+QICLOSE=2\r\n");//关闭socekt连接
		delay_ms(100);
	
    Clear_Buffer();
    IWDG_Feed();//喂狗
}

/*****************************************************
下面就是需要修改的地方，修改服务器的IP地址和端口号
*****************************************************/
#define SERVERIP "a1mM8WG8LPc.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define SERVERPORT "1883"
/*****************************************************
建立TCP链接 
*****************************************************/
void CSTX_4G_CreateTCPSokcet(void)//创建sokcet
{
		memset(ATSTR,0,BUFLEN);
		sprintf(ATSTR,"AT+QIOPEN=1,0,\"TCP\",\"%s\",%s,0,1\r\n",SERVERIP,SERVERPORT);
    Uart2_SendStr(ATSTR);//创建连接TCP,输入IP以及服务器端口号码 
    delay_ms(300);
	
		strx=strstr((const char*)buf_uart2.buf,(const char*)"+QIOPEN: 0,566");//检查是否登陆成功
		if(strx)
		{
			 return ;	//如果连接服务器失败就反馈 后面不需要判断是否成功了
		}
	
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QIOPEN: 0,0");//检查是否登陆成功
	  errcount=0;
		while(strx==NULL)
		{
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)"+QIOPEN: 0,0");//检查是否登陆成功
				delay_ms(100);
				if(errcount>100)     //超时退出死循环 表示服务器连接失败
        {
            errcount = 0;
            break;
        }
		}  
     Clear_Buffer();	
    
}

/*****************************************************
发送数据函数
*****************************************************/
void CSTX_4G_Senddata(int len,uint8_t *data)//发送字符串数据
{
//	  Gui_DrawFont_GBK16(16,10,RED,WHITE, "SEND DATA...");
		memset(ATSTR,0,BUFLEN);
		sprintf(ATSTR,"AT+QISEND=0,%d\r\n",len);
    Uart2_SendStr(ATSTR);
    delay_ms(300);
		//等待模块反馈 >
		strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
    while(strx==NULL)
    {
        errcount++;
        strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
        if(errcount>100)     //防止死循环跳出
        {
            errcount = 0;
            break;
        }
    }
		
		Uart2_SendStr((char *)data);//发送真正的数据
    delay_ms(300);
		
		strx=strstr((const char*)buf_uart2.buf,(const char*)"ERROR");//如果发送失败
		if(strx)
		{
				errCountData++;
//				Gui_DrawFont_GBK16(16,10,RED,WHITE, "SEND DATA NO");
				if(errCountData>3)     //超时退出死循环 表示服务器连接失败
        {
							__set_FAULTMASK(1);//关闭总中断
							NVIC_SystemReset();//请求单片机重启
        }
				return ;	//发送数据失败了就不要去下面判断是否成功了
		}
		
    strx=strstr((const char*)buf_uart2.buf,(const char*)"SEND OK");//检查是否发送成功
		errcount=0;
		while(strx==NULL)	//如果没有收到SEND OK就循环查询 
		{
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)"SEND OK");//检查是否发送成功
				delay_ms(100);
				if(errcount>100)     //超时退出死循环 表示服务器连接失败
        {
            errcount = 0;
            break;
        }
		}  
    Clear_Buffer();	//发送完毕清空
//	  Gui_DrawFont_GBK16(16,10,RED,WHITE, "SEND DATA OK");
}


/*****************************************************
收到服务器下发的数据就直接打印
*****************************************************/
void CSTX_4G_RECTCPData(void)
{
    if(strstr((const char*)buf_uart2.buf,(const char*)"LEDK"))//返回+QIURC:，表明接收到TCP服务器发回的数据
    {
			printf("收到服务器下发数据:%s",buf_uart2.buf); 
			GPIO_ResetBits(GPIOB,GPIO_Pin_3); 			
    }else if(strstr((const char*)buf_uart2.buf,(const char*)"LEDG"))
		{
			printf("收到服务器下发数据:%s",buf_uart2.buf);
			GPIO_SetBits(GPIOB,GPIO_Pin_3); 			
		}
		delay_ms(300);
		buf_uart2.index=0;
		memset(buf_uart2.buf,0,BUFLEN);
		IWDG_Feed();//喂狗   
}

/*****************************************************
注册到EMQX平台
*****************************************************/
void CSTX_4G_RegALIYUNIOT(void)//平台注册
{
		int errcount = 0;

	  memset(ATSTR,0,BUFLEN);
    sprintf(ATSTR,"AT+QMTOPEN=0,\"106.15.62.60\",1883\r\n");
    printf("ATSTR = %s \r\n",ATSTR);
    Uart2_SendStr(ATSTR);//登录EMQX平台
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTOPEN: 0,0");//返+QMTOPEN: 0,0
    while(strx==NULL)
    {
				errcount++;
				delay_ms(30);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTOPEN: 0,0");//返回OK
        if(errcount>10000)     //防止死循环
        {
            GPIO_SetBits(GPIOA,GPIO_Pin_12); 	
						delay_ms(1000);
						GPIO_ResetBits(GPIOA,GPIO_Pin_12); 	 
            delay_ms(300);
            NVIC_SystemReset();	//没有创建TCP SOCKET就重启系统等到服务器就绪
        }
    }
    Clear_Buffer();

    memset(ATSTR,0,BUFLEN);
    sprintf(ATSTR,"AT+QMTCONN=0,\"cstx123456\",\"admin\",\"public\"\r\n");
    printf("ATSTR = %s \r\n",ATSTR);
    Uart2_SendStr(ATSTR);//发送链接到EMQX
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTCONN: 0,0,0");//返+QMTCONN: 0,0,0
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTCONN: 0,0,0");//返+QMTCONN: 0,0,0
    }
    Clear_Buffer();
		
		memset(ATSTR,0,BUFLEN);
    sprintf(ATSTR,"AT+QMTSUB=0,1,\"testtopic\",0 \r\n");
    printf("ATSTR = %s \r\n",ATSTR);
    Uart2_SendStr(ATSTR);//订阅到EMQX
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTSUB: 0,1,0,0");//返+QMTCONN: 0,0,0
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTSUB: 0,1,0,0");//返+QMTCONN: 0,0,0
    }
    Clear_Buffer();

}

void CSTX_4G_ALYIOTSenddata(void)//上发数据，上发的数据跟对应的插件有关系，用户需要注意插件然后对应数据即可
{
		CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
		DHT11_Read_TempAndHumidity();	//读取温湿度
		printf("温度:%d 湿度:%d \r\n",DHT11_Data.temp_int,DHT11_Data.humi_int);	 //打印温湿度
		memset(ATSTR,0,BUFLEN);
		Clear_Buffer1();	//发送命令之前清空之前的模块反馈的数据
   sprintf(ATSTR,"AT+QMTPUB=0,0,0,0,\"testtopic\"\r\n");//g_config_data.topicPost
		printf("ATSTR = %s \r\n",ATSTR);
		Uart2_SendStr(ATSTR);//发送命令 模块发送命令
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
		while(strx==NULL)
		{
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
				delay_ms(300);
				if(errcount>100)     //防止死循环跳出
				{
						errcount = 0;
						break;
				}
		}
		memset(ATSTR,0,BUFLEN);
		sprintf(ATSTR,"{\"msg\":\"temp\":%d,\"humi\":%d}",DHT11_Data.temp_int,DHT11_Data.humi_int);
		printf("ATSTR = %s \r\n",ATSTR);
		Uart2_SendStr(ATSTR);	//发送完毕命令接下来就进行判断反馈
		delay_ms(300);
		UART2_send_byte(0x1A); //发送结束符S
		strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返发送成功
		errcount=0;
		while(strx==NULL)
		{
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返发送成功
				delay_ms(100);
				if(errcount>100)     //超时退出死循环 表示服务器连接失败
				{
						errcount = 0;
						break;
				}
		}
		printf("温湿度数据发送成功!!\r\n");	 //打印温湿度
		CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
		printf(buf_uart2.buf);
//    Clear_Buffer();
}

void CSTX_4G_ALYIOTADC(void)//上传ADC值
{
		CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
		Clear_Buffer1();
		adcx1=Get_Adc_Average(ADC_Channel_10,10); //获取得到PC0的ADC1的值
		delay_ms(300);
//		adcx2=Get_Adc_Average(ADC_Channel_11,10);
		printf("ADC1原始数值：%d\r\n",adcx1); //打印原始采集的数据
		printf("***转换数值 ADC1：%.4f\r\n",adcx1*3.3f/4096); //打印转换后的数值	

		memset(ATSTR,0,BUFLEN);
		Clear_Buffer1();	//发送命令之前清空之前的模块反馈的数据
		sprintf(ATSTR,"AT+QMTPUB=0,0,0,0,\"testtopic\"\r\n");//g_config_data.topicPost
		printf("ATSTR = %s \r\n",ATSTR);
		Uart2_SendStr(ATSTR);//发送命令 模块发送命令
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
		while(strx==NULL)
		{
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
				delay_ms(300);
				if(errcount>100)     //防止死循环跳出
				{
						errcount = 0;
						break;
				}
		}
		memset(ATSTR,0,BUFLEN);

		sprintf(ATSTR,"{\"msg\":\"adcx\":%d,\"Voltage\":%.4f}",adcx1,adcx1*3.3f/4096);
		printf("ATSTR = %s \r\n",ATSTR);
		Uart2_SendStr(ATSTR);	//发送完毕命令接下来就进行判断反馈
		delay_ms(300);
		UART2_send_byte(0x1A); //发送结束符S
		strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返发送成功
		errcount=0;
		while(strx==NULL)
		{
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返发送成功
				delay_ms(100);
				if(errcount>100)     //超时退出死循环 表示服务器连接失败
				{
						errcount = 0;
						break;
				}
		}
		printf("ADC值上传成功！！！\r\n");
		CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
		printf(buf_uart2.buf);
//    Clear_Buffer();
}


void Start_GPS(void) //开启GPS并进行定位
{
		Uart2_SendStr("AT+QGPS=1\r\n");//查询激活状态
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返1
		while(strx==NULL)
		{
				delay_ms(300);
				strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");
		}
		Clear_Buffer();	
		
		while(res)//等待定位成功
		{
			Uart2_SendStr("AT+QGPSGNMEA=\"RMC\"\r\n");//查询激活状态
			delay_ms(300);
			strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//返1
			while(strx==NULL)
			{
					Clear_Buffer();	
					Uart2_SendStr("AT+QGPSGNMEA=\"RMC\"\r\n");//获取激活状态
					delay_ms(300);
					strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//返回1,表明注网成功
			}
			sprintf(GPRMCSTR,"%s",strx);
			printf("%s\r\n",GPRMCSTR);
			if(GPRMCSTR[17]=='A')
			{
					ClearRAM((u8 *)LongitudeStr,20);
					ClearRAM((u8 *)LatitudeStr,20);
					Get_GPS(LongitudeStr,LatitudeStr);//进行纠偏
					printf("GPS信息定位成功！！\r\n");
					printf("LongitudeStr=%s\r\n",LongitudeStr);
					printf("LatitudeStr=%s\r\n",LatitudeStr);
					
					strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返SEND OK
					while(strx==NULL)
					{
							strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返SEND OK
					}
					res=0; //定位成功，跳出循环
			}else{
					printf("正在定位GPS信息，请耐心等候!!\r\n");
					printf("注意：定位成功快慢与当前所处环境，GPS信号强弱有关!!!\r\n");
					res=1; //定位失败，继续等待
			}
			Clear_Buffer1();	//清空缓存，但不打印数据
		}
}


void Get_GPS_RMC(void)//上传GPS信息
{
		CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
		Uart2_SendStr("AT+QGPSGNMEA=\"RMC\"\r\n");//查询激活状态
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//返1
		while(strx==NULL)
		{
				Clear_Buffer();	
				Uart2_SendStr("AT+QGPSGNMEA=\"RMC\"\r\n");//获取激活状态
				delay_ms(300);
				strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");
		}
		sprintf(GPRMCSTR,"%s",strx);
		
		if(GPRMCSTR[17]=='A')
		{
				ClearRAM((u8 *)LongitudeStr,20);
				ClearRAM((u8 *)LatitudeStr,20);
				Get_GPS(LongitudeStr,LatitudeStr); //进行纠偏
				printf("LongitudeStr=%s\r\n",LongitudeStr);
				printf("LatitudeStr=%s\r\n",LatitudeStr);
				
				//将GPS信息上传至服务器
				memset(ATSTR,0,BUFLEN);
				Clear_Buffer1();	//发送命令之前清空之前的模块反馈的数据
				sprintf(ATSTR,"AT+QMTPUB=0,0,0,0,\"testtopic\"\r\n");//g_config_data.topicPost
				printf("ATSTR = %s \r\n",ATSTR);
				Uart2_SendStr(ATSTR);//发送命令 模块发送命令
				delay_ms(300);
				strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
				while(strx==NULL)
				{
						errcount++;
						strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
						delay_ms(300);
						if(errcount>100)     //防止死循环跳出
						{
								errcount = 0;
								break;
						}
				}
				memset(ATSTR,0,BUFLEN);
				sprintf(ATSTR,"{\"msg\":\"LongitudeStr\":%s,\"LatitudeStr\":%s}",LongitudeStr,LatitudeStr);
				printf("ATSTR = %s \r\n",ATSTR);
				Uart2_SendStr(ATSTR);	//发送完毕命令接下来就进行判断反馈
				delay_ms(300);
				UART2_send_byte(0x1A); //发送结束符
				strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返发送成功
				errcount=0;
				while(strx==NULL)
				{
						errcount++;
						strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返发送成功
						delay_ms(100);
						if(errcount>100)     //超时退出死循环 表示服务器连接失败
						{
								errcount = 0;
								break;
						}
				}
						printf("GPS信息上传成功！！！\r\n");
				}
		CSTX_4G_RECTCPData();//收数据,接收服务器下发的数据并打印到串口1进行显示
		printf(buf_uart2.buf);
//		Clear_Buffer();
}

/*************解析出经纬度数据*******************/	
void Get_GPS(char *LongitudeStr,char *LatitudeStr)//经纬度纠偏									
{	
		unsigned int i;
		char *strx;
    strx=strstr((const char*)buf_uart2.buf,(const char*)"A,"); //
    if(strx)
    {
	
        for(i=0;i<9;i++)
            LongLatidata.longitude[i]=strx[i+2];			//得到2603.9624 维度 
						//printf("\r\n测试一：%s\r\n",LongLatidata.longitude);
    }		    
    strx=strstr((const char*)buf_uart2.buf,(const char*)"N,");
    if(strx)
    {
        for(i=0;i<10;i++)
						LongLatidata.Latitude[i]=strx[i+2]; //得到11912.4105经度
		      //  printf("\r\n测试二：%s\r\n",LongLatidata.Latitude);
        for(i=0;i<3;i++)
            LongLatidata.Latitudess[i]=LongLatidata.Latitude[i]; //得到119
        for(i=3;i<10;i++)
            LongLatidata.Latitudedd[i-3]=LongLatidata.Latitude[i];//得到
				//经度
        tempdata[0]=(LongLatidata.Latitudess[0]-0x30)*100+(LongLatidata.Latitudess[1]-0x30)*10+(LongLatidata.Latitudess[2]-0x30)\
                    +((LongLatidata.Latitudedd[0]-0x30)*10+(LongLatidata.Latitudedd[1]-0x30)+(float)(LongLatidata.Latitudedd[3]-0x30)/10+\
                      (float)(LongLatidata.Latitudedd[4]-0x30)/100+(float)(LongLatidata.Latitudedd[5]-0x30)/1000+(float)(LongLatidata.Latitudedd[6]-0x30)/10000)/60.0f;//获取完整的数据

        for(i=0;i<2;i++)
						LongLatidata.longitudess[i]=LongLatidata.longitude[i];
        for(i=2;i<9;i++)
            LongLatidata.longitudedd[i-2]=LongLatidata.longitude[i];
				//纬度					
        tempdata[1]=(LongLatidata.longitudess[0]-0x30)*10+(LongLatidata.longitudess[1]-0x30)\
                    +((LongLatidata.longitudedd[0]-0x30)*10+(LongLatidata.longitudedd[1]-0x30)+(float)(LongLatidata.longitudedd[3]-0x30)/10+\
                      (float)(LongLatidata.longitudedd[4]-0x30)/100+(float)(LongLatidata.longitudedd[5]-0x30)/1000+(float)(LongLatidata.longitudedd[6]-0x30)/10000)/60.0f;//获取完整的数据
			sprintf(LongitudeStr,"%f",tempdata[0]);
			sprintf(LatitudeStr,"%f",tempdata[1]);
		}
}

char* Get_4GIMEI_NUM(void)
{
		Clear_Buffer();	
		memset(IMEINUMBER,0,BUFLEN);
		Uart2_SendStr("AT+CGSN=1\r\n");//查询激活状态
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGSN: \"");//如果反馈错误就表示没有定位好
		if(strx)	//没有反馈错误就表示有经纬度了 然后来进行显示 反馈得到LOC就表示有位置了
		{
				strncpy(IMEINUMBER,strx+8,15); //获取维度数据
//				Gui_DrawFont_GBK16(0,90,RED,WHITE, (u8*) IMEINUMBER);	//显示经度到液晶屏
				return IMEINUMBER;
		}
		return 0;
}






