/* 定义防止递归包含 ----------------------------------------------------------*/
#ifndef _Flash_H
#define _Flash_H

typedef unsigned char  u8;
typedef unsigned short u16;

/* 包含的头文件 --------------------------------------------------------------*/
#include "stm32f10x.h"


/* 宏定义 --------------------------------------------------------------------*/

/* 指令表 */
#define Flash_WRITE_ENABLE       0x06                     //写使能
#define Flash_WRITE_DISABLE      0x04                     //写失能
#define Flash_READ_STATUS_REG    0x05                     //读状态寄存器
#define Flash_WRITE_STATUS_REG   0x01                     //写状态寄存器

#define Flash_READ_DATA          0x03                     //读数据
#define Flash_FAST_READ          0x0B                     //快读数据
#define Flash_FAST_READ_DUAL     0x3B                     //快读数据(双数据线输出)
#define Flash_WRITE_PAGE         0x02                     //页编程
#define Flash_ERASE_BLOCK        0xD8                     //擦除块
#define Flash_ERASE_SECTOR       0x20                     //擦除扇区
#define Flash_ERASE_CHIP         0xC7                     //擦除芯片
#define Flash_POWER_DOWN         0xB9                     //掉电
#define Flash_RELEASE_POWER_DOWN 0xAB                     //释放掉电
#define Flash_DEVICE_ID          0x90                     //设备ID
#define Flash_JEDEC_ID           0x9F                     //Jedec ID

/**/
#define Flash_FILED_LENGTH 64					   //段长度

typedef struct _Flash_CONFIG_DATA
{
    char serialnum[Flash_FILED_LENGTH+1];          //系列号
    char protocoltype[Flash_FILED_LENGTH+1];       //协议类型
    char ipaddr[Flash_FILED_LENGTH+1];             //IP地址
    char portnum[Flash_FILED_LENGTH+1];            //端口
    char telnum[Flash_FILED_LENGTH+1];             //电话号码
		char clientid[Flash_FILED_LENGTH+1];             //产品ProductKey
		char username[Flash_FILED_LENGTH+1];             //设备名称DeviceName
		char password[Flash_FILED_LENGTH+1];             //设备秘钥DeviceSecret
		char topic[Flash_FILED_LENGTH+1];             //订阅Topic
		char topicPost[Flash_FILED_LENGTH+1];             //上传数据的Topic
		char mqttserverip[Flash_FILED_LENGTH+1];             //MQTT服务器IP地址
		char mqttserverport[Flash_FILED_LENGTH+1];             //MQTT服务器端口号
		int heartime;                       //心跳时间
    int bufsize;            						//缓存大小

}Flash_CONFIG_DATA;
/**/

/* 函数申明 ------------------------------------------------------------------*/

/**/
extern int parser_config_data(char *data);
void Flash_Clean(void);
extern unsigned char GetComma(unsigned char num,char *str);

extern Flash_CONFIG_DATA g_config_data;
/**/

void Flash_Initializes(void);
uint8_t SPI_WriteReadByte(uint8_t TxData);

static void Flash_WriteEnable(void);                      //写使能
static void Flash_WriteDisable(void);                     //写失能
uint8_t Flash_ReadSR(void);                               //读状态寄存器
void Flash_WriteSR(uint8_t SR);                           //写状态寄存器
void Flash_ReadNByte(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t nByte);         //读取n字节数据
void Flash_FastReadNByte(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t nByte);     //快读n字节数据
void Flash_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t nByte);        //写入n(<256字节)数据

void Flash_WriteNoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t nByte);     //写入n字节数据(无校验)
void Flash_WriteNByte(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t nByte);       //写入n字节数据
void Flash_WaitForNoBusy(void);                           //等待不忙

void Flash_EraseBlock(uint32_t BlockAddr);                //擦除块
void Flash_EraseSector(uint32_t SectorAddr);              //擦除扇区
void Flash_EraseChip(void);                               //擦除整个芯片
void Flash_PowerDown(void);                               //掉电模式
void Flash_WAKEUP(void);                                  //唤醒器件
uint16_t Flash_ReadID(void);                              //读取Flash ID
uint32_t Flash_ReadJEDEC_ID(void);                        //读取JEDEC_ID


#endif /* _Flash_H */

/**** Copyright (C)2016 strongerHuang. All Rights Reserved **** END OF FILE ****/
