#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

typedef struct TIME2_T
{
    unsigned int    time_overflow_1s;
    unsigned int    time_overflow_100ms;
    unsigned char   flag;
    unsigned int    send_gprs_position;
}TIME2_T;
extern void Delay(unsigned long nCount);
extern void delay_GSM(unsigned int i);
extern void delay_1ms(unsigned int i);
extern void Init_TIM2(void);


extern void clean_time2_flags(void);
extern unsigned char is_enable_send_gprs_position(void);

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM4_Int_Init(u16 arr,u16 psc);
#endif
