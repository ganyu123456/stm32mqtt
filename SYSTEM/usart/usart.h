#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
	

#define BUFLEN 256      //数组缓存大小

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收

typedef struct _UART_BUF
{
    char buf [BUFLEN+1];               
    unsigned int index ;
}UART_BUF;	
	

//如果想串口中断接收，请不要注释以下宏定义
void uart_init(u32 bound);
void send_data_uart1(unsigned char Data);
void debug_put_word(unsigned char word);

void uart_init(u32 bound);
void uart2_init(u32 bound);
void uart3_init(u32 bound);
void UART1_send_byte(char data);
void UART2_send_byte(char data);
void UART3_send_byte(char data);
void Uart1_SendStr(char*SendBuf);
void Uart2_SendStr(char*SendBuf);
void Uart3_SendStr(char*SendBuf);
void ClearRAM(u8* ram,u32 n);
void Clear_Buffer_UART1(void);


extern UART_BUF buf_uart1;     //PC
extern UART_BUF buf_uart2;     //4G
extern UART_BUF buf_uart3;     //TTL

extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	

#endif


