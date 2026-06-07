/*----------------------------------------------------------------------------
  更新日志:
  2016-07-29 V1.0.0:初始版本
  ----------------------------------------------------------------------------*/
/******************************** SPI FLASH信息 ********************************
SPI FLASH种类很多, 但软件基本都兼容

1、W25X16信息：
页(Page) ------ 256Byte
扇区(Sector) -- 4KByte ------ (等于16页)
块(Block) ----- 64KByte ----- (等于16扇区)

W25X16芯片容量：2MB (16Mbit)
          页数：16*16*32 (2M/256)
        扇区数：16*32
          块数：32

2、读写操作：
读 ------------ 一次最大读一页(256B)
写 ------------ 页
擦出 ---------- 扇区、块、整个芯片

3、控制和状态寄存器命令(默认:0x00)
BIT位  7   6   5   4   3   2   1   0
      SPR  RV  TB  BP2 BP1 BP0 WEL BUSY
SPR:默认0,状态寄存器保护位,配合WP使用
TB,BP2,BP1,BP0:FLASH区域写保护设置
WEL:写使能锁定
BUSY:忙标记位(1,忙;0,空闲)
********************************************************************************/
/* 包含的头文件 --------------------------------------------------------------*/
#include "flash.h"
#include "spi.h"
#include "usart.h"
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERNUM "0000001"
#define PROTOTOCOL "UDP"
#define IPADDR "47.98.137.251"
#define PORTNUM "1001"
#define TELNUM "15922041115"
#define CACHESIZE 512

#define ClientId "a1QwT0hxqQ6"    //需要定义为用户自己的参数
#define Username   "DHT11"		//需要定义为用户自己的参数
#define Password   "zVq7I48SmNvL86Ae3MCGjy7QarA8BrQb"  //需要定义为用户自己的参数
#define Topic   "/a1QwT0hxqQ6/DHT11/user/EC20" //需要定义为用户自己的参数
#define TopicPost   "/sys/a1QwT0hxqQ6/DHT11/thing/event/property/post"
#define MQTTServerip      "47.98.137.251"							//需要定义为用户自己的参数
#define MQTTServerPort     "1883"				//需要定义为用户自己的参数
#define HearTime     1				//需要定义为用户自己的参数

#define USE_Flash_DATA_LENGTH 256              //长度
u8 spi_Buf[USE_Flash_DATA_LENGTH];

Flash_CONFIG_DATA g_config_data;


/************************************************
函数名称 ： Flash_WriteEnable
功    能 ： SPI_FLASH写使能，将WEL置位
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
static void Flash_WriteEnable(void)
{
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_WRITE_ENABLE);            //《写使能》指令
  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_WriteDisable
功    能 ： SPI_FLASH写禁止,将WEL清零
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
static void Flash_WriteDisable(void)
{
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_WRITE_DISABLE);           //《写失能》指令
  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_ReadSR
功    能 ： 读取Flash状态寄存器
参    数 ： 无
返 回 值 ： Byte --- 读取字节
作    者 ： strongerHuang
*************************************************/
uint8_t Flash_ReadSR(void)
{
  uint8_t data_tmp;
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_READ_STATUS_REG);         //《读状态寄存器》指令
  data_tmp = SPI_ReadByte();                     //读取一个字节
  SPI_CS_DISABLE;                                //失能器件
  return data_tmp;
}

/************************************************
函数名称 ： Flash_WriteSR
功    能 ： 写Flash状态寄存器
参    数 ： SR --- 写状态寄存器命令
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_WriteSR(uint8_t SR)
{
  Flash_WriteEnable();                          //写使能
  SPI_WriteByte(Flash_WRITE_STATUS_REG);        //《写状态寄存器》指令
  SPI_WriteByte(SR);                             //写入一个字节
  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_ReadNByte
功    能 ： 从ReadAddr地址开始连续读取Flash的nByte
参    数 ： pBuffer ---- 数据存储区首地址
            ReadAddr --- 要读取Flash Flash的首地址地址
            nByte ------ 要读取的字节数(最大65535B = 64K 块)
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_ReadNByte(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t nByte)
{
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_READ_DATA);               //《读数据》指令
  SPI_WriteByte((uint8_t)((ReadAddr)>>16));      //发送24bit地址
  SPI_WriteByte((uint8_t)((ReadAddr)>>8));
  SPI_WriteByte((uint8_t)ReadAddr);

  while(nByte--)                                 //循环读数
  {
    *pBuffer = SPI_ReadByte();
    pBuffer++;
  }

  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_FastReadNByte
功    能 ： 从ReadAddr地址开始连续快速读取Flash的nByte
参    数 ： pBuffer ---- 数据存储区首地址
            ReadAddr --- 要读取Flash Flash的首地址地址
            nByte ------ 要读取的字节数(最大65535B = 64K 块)
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_FastReadNByte(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t nByte)
{
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_FAST_READ);               //《快读数据》指令
  SPI_WriteByte((uint8_t)((ReadAddr)>>16));      //发送24bit地址
  SPI_WriteByte((uint8_t)((ReadAddr)>>8));
  SPI_WriteByte((uint8_t)ReadAddr);
  SPI_WriteByte(0xFF);                           //等待8个时钟

  while(nByte--)                                 //循环读数
  {
    *pBuffer = SPI_ReadByte();
    pBuffer++;
  }

  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_WritePage
功    能 ： 在Flash内写入少于1页(256个字节)的数据
参    数 ： pBuffer ----- 写入数据区首地址
            WriteAddr --- 要写入Flash的地址
            nByte ------- 要写入的字节数(最大1页)
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t nByte)
{
  Flash_WriteEnable();                          //写使能

  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_WRITE_PAGE);              //《页编程》指令
  SPI_WriteByte((uint8_t)((WriteAddr)>>16));     //发送24bit地址
  SPI_WriteByte((uint8_t)((WriteAddr)>>8));
  SPI_WriteByte((uint8_t)WriteAddr);

  while (nByte--)
  {
    SPI_WriteByte(*pBuffer);
    pBuffer++;
  }

  SPI_CS_DISABLE;
  Flash_WaitForNoBusy();                        //等待空闲（等待写入结束）
}

/************************************************
函数名称 ： Flash_WriteNoCheck
功    能 ： 无检验写Flash
            必须确保所写的地址范围内的数据全部为0xFF,否则在非0xFF处写入的数据将失败!
            具有自动换页功能
            在指定地址开始写入指定长度的数据,但是要确保地址不越界!
参    数 ： pBuffer ----- 写入数据区首地址
            WriteAddr --- 要写入Flash的地址
            nByte ------- 要写入的字节数
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_WriteNoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t nByte)
{
  uint16_t PageRemain = 256 - WriteAddr%256;     //单页剩余可写的字节数

  if(nByte <= PageRemain)
    PageRemain = nByte;                          //不大于256个字节

  while(1)
  {
    Flash_WritePage(pBuffer, WriteAddr, PageRemain);
    if(nByte == PageRemain)                      //写入结束
      break;
    else                                         //写入未结束
    {
      pBuffer += PageRemain;                     //下一页写入数据
      WriteAddr += PageRemain;                   //下一页写入数据地址
      nByte -= PageRemain;                       //待写入字节数递减

      if(nByte > 256)
        PageRemain = 256;                        //待写入1页(256字节)的数据
      else
        PageRemain = nByte;                      //待写入少于1页(256字节)的数据
    }
  }
}


/************************************************
函数名称 ： Flash_WriteNByte
功    能 ： 从ReadAddr地址开始连续写入nByte到Flash中
参    数 ： pBuffer ----- 写入数据区首地址
            WriteAddr --- 要写入Flash的地址
            nByte ------- 要写入的字节数(最大65535B = 64K 块)
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_WriteNByte(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t nByte)
{
  static uint8_t SectorBuf[4096];                //扇区buf
  uint32_t SecPos;                               //扇区位置
  uint16_t SecOff;                               //扇区偏移
  uint16_t SecRemain;                            //剩余扇区
  uint16_t i;

  SecPos = WriteAddr/4096;                       //地址所在扇区(0~511)
  SecOff = WriteAddr%4096;                       //地址所在扇区的偏移
  SecRemain = 4096-SecOff;                       //地址所在扇区剩余字节数(扇区大小4096B=4KB)

  if(nByte <= SecRemain)
    SecRemain = nByte;                           //写入数据大小 < 剩余空间大小 (即剩余空间够保存这些数据)

  while(1)
  {
    /* 第1步·校验 */
    Flash_ReadNByte(SectorBuf, SecPos*4096, 4096);        //读出整个扇区的内容
    for(i=0; i<SecRemain; i++)                             //校验数据,是否需要擦除
    {
      if(SectorBuf[SecOff + i] != 0xFF)                    //存储数据不为0xFF 则需要擦除
        break;
    }
    if(i < SecRemain)                                      //需要擦除
    {
      Flash_EraseSector(SecPos);                          //擦除该扇区
      for(i=0; i<SecRemain; i++)                           //保存写入的数据(第1次时，是写入那扇区后面剩余的空间)
      {
        SectorBuf[SecOff + i] = pBuffer[i];
      }
      Flash_WriteNoCheck(SectorBuf, SecPos*4096, 4096);   //写入整个扇区（扇区 = 老数据 + 新写入数据）
    }
    else
      Flash_WriteNoCheck(pBuffer, WriteAddr, SecRemain);  //不需要擦除,直接写入扇区剩余空间

    if(nByte == SecRemain)                       //写入结束
    {
      Flash_WriteDisable();                     //写失能, 退出写
      break;
    }
    else                                         //写入未结束
    {
      SecPos++;                                  //扇区地址增1
      SecOff = 0;                                //偏移位置归零
      pBuffer += SecRemain;                      //指针偏移
      WriteAddr += SecRemain;                    //写地址偏移
      nByte -= SecRemain;                        //待写入字节数递减
      if(nByte > 4096)
        SecRemain = 4096;                        //待写入1扇区(4096字节)的数据
      else
        SecRemain = nByte;                       //待写入少于1扇区(4096字节)的数据
    }
  }
}

/************************************************
函数名称 ： Flash_WaitForNoBusy
功    能 ： 等待不忙
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_WaitForNoBusy(void)
{
  while((Flash_ReadSR()&0x01)==0x01);           //等待BUSY位清空
}

/************************************************
函数名称 ： Flash_EraseBlock
功    能 ： 擦除块
            擦除块需要一定时间
参    数 ： BlockAddr --- 块地址 0~31
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_EraseBlock(uint32_t BlockAddr)
{
  BlockAddr *= 65536;                            //块首地址
  Flash_WriteEnable();                          //写使能
  Flash_WaitForNoBusy();
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_ERASE_BLOCK);             //《擦除块》指令
  SPI_WriteByte((uint8_t)((BlockAddr)>>16));     //擦除地址
  SPI_WriteByte((uint8_t)((BlockAddr)>>8));
  SPI_WriteByte((uint8_t)BlockAddr);
  SPI_CS_DISABLE;

  Flash_WaitForNoBusy();                        //等待擦除完成
}

/************************************************
函数名称 ： Flash_EraseSector
功    能 ： 擦除扇区
参    数 ： SectorAddr --- 扇区地址 0~511
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_EraseSector(uint32_t SectorAddr)
{
  SectorAddr *= 4096;                            //扇区首地址
  Flash_WriteEnable();                          //写使能
  Flash_WaitForNoBusy();
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_ERASE_SECTOR);            //《擦除扇区》指令
  SPI_WriteByte((uint8_t)((SectorAddr)>>16));    //擦除地址
  SPI_WriteByte((uint8_t)((SectorAddr)>>8));
  SPI_WriteByte((uint8_t)SectorAddr);
  SPI_CS_DISABLE;

  Flash_WaitForNoBusy();                        //等待擦除完成
}

/************************************************
函数名称 ： Flash_EraseChip
功    能 ： 擦除整个芯片(整片擦除时间较长)
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_EraseChip(void)
{
  Flash_WriteEnable();                          //写使能
  Flash_WaitForNoBusy();
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_ERASE_CHIP);              //《擦除芯片》指令
  SPI_CS_DISABLE;

  Flash_WaitForNoBusy();                        //等待芯片擦除结束
}

/************************************************
函数名称 ： Flash_PowerDown
功    能 ： 进入掉电模式
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_PowerDown(void)
{
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_POWER_DOWN);              //《掉电》指令
  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_WAKEUP
功    能 ： 掉电唤醒
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void Flash_WAKEUP(void)
{
  SPI_CS_ENABLE;                                 //使能器件
  SPI_WriteByte(Flash_RELEASE_POWER_DOWN);      //《掉电唤醒》指令
  SPI_CS_DISABLE;                                //失能器件
}

/************************************************
函数名称 ： Flash_ReadID
功    能 ： 读取芯片ID Flash的ID(W25X16: EF14）
参    数 ： 无
返 回 值 ： ID --- 16位ID号
作    者 ： strongerHuang
*************************************************/
uint16_t Flash_ReadID(void)
{
  uint16_t ID = 0;
  SPI_CS_ENABLE;                                 //使能器件

  SPI_WriteByte(Flash_DEVICE_ID);               //《设备ID》指令
  SPI_WriteByte(0x00);
  SPI_WriteByte(0x00);
  SPI_WriteByte(0x00);

  ID |= SPI_ReadByte()<<8;                       //读取ID
  ID |= SPI_ReadByte();
  SPI_CS_DISABLE;                                //失能器件
  return ID;
}

/************************************************
函数名称 ： Flash_ReadJEDEC_ID
功    能 ： 读取芯片JEDEC_ID
参    数 ： 无
返 回 值 ： ID --- 24位ID号
作    者 ： strongerHuang
*************************************************/
uint32_t Flash_ReadJEDEC_ID(void)
{
  uint32_t ID = 0;
  SPI_CS_ENABLE;                                 //使能器件

  SPI_WriteByte(Flash_JEDEC_ID);                //《JEDEC_ID》指令

  ID |= SPI_ReadByte()<<16;                      //读取ID
  ID |= SPI_ReadByte()<<8;
  ID |= SPI_ReadByte();
  SPI_CS_DISABLE;                                //失能器件
  return ID;
}


/**** Copyright (C)2016 strongerHuang. All Rights Reserved **** END OF FILE ****/

/*分割数据*/
unsigned char GetComma(unsigned char num,char *str)
{
    unsigned char i,j = 0;
    int len=strlen(str);  	//总长度
    for(i = 0;i < len;i ++)
    {
        if(str[i] == '*')
            j++;
        if(j == num)
            return i + 1;
        if(j> 256)         //如果超时
            return 0;
    }

    return 0;
}

/*配置数据解析*/
int parser_config_data(char *data)
{
    int str_st, str_end, len;
    char line[USE_Flash_DATA_LENGTH];          //line 原始数据
    char des_line[Flash_FILED_LENGTH];         //seg str
    char *pt=NULL;

    memset(line,0,USE_Flash_DATA_LENGTH);
    memset(des_line,0,Flash_FILED_LENGTH);



    memset(g_config_data.serialnum,0,Flash_FILED_LENGTH);		 //设备号
    memset(g_config_data.protocoltype,0,Flash_FILED_LENGTH);	 //协议
    memset(g_config_data.ipaddr,0,Flash_FILED_LENGTH);			 //IP地址
    memset(g_config_data.portnum,0,Flash_FILED_LENGTH);		 //端口号
    memset(g_config_data.telnum,0,Flash_FILED_LENGTH);			 //电话号码
		memset(g_config_data.clientid,0,Flash_FILED_LENGTH);	 //协议
    memset(g_config_data.username,0,Flash_FILED_LENGTH);			 //IP地址
    memset(g_config_data.password,0,Flash_FILED_LENGTH);		 //端口号
    memset(g_config_data.topic,0,Flash_FILED_LENGTH);			 //电话号码
		memset(g_config_data.topicPost,0,Flash_FILED_LENGTH);			 //电话号码
		memset(g_config_data.mqttserverip,0,Flash_FILED_LENGTH);			 //电话号码
		memset(g_config_data.mqttserverport,0,Flash_FILED_LENGTH);			 //电话号码
		g_config_data.heartime=0;
    g_config_data.bufsize = 0;									 //缓存大小

    if(strlen(data) < 20)										 //如果大小少于二十个字节
    {
        return 1;
    }

    pt = strstr(data,"$");										 //查询$符号
    if(!pt)
    {
        return 1;
    }


    sprintf(line,"%s",data);									 //原始数据
    printf("line");
    printf(line);
    printf("\r\n");
    //$CONFIG,chance001,TCP,120.32.68.174,1001,1383838438,4096,


//    str_st = GetComma(1,line);									 //获取第一个逗号
//    str_end = GetComma(2,line);									 //获取第二个逗号
//    len = str_end - str_st;

//    memset(des_line,0,Flash_FILED_LENGTH);
//    strncpy(des_line, &line[str_st], len-1);  					//得到第一个和第二个逗号间的数据，序列号

//    if(strlen(des_line)==0)
//    {
//        return 1;
//    }

//    sprintf(g_config_data.serialnum,"%s",des_line);

//    printf("**************************\r\n");
//    printf("CE1:");												//打印序列号
//    printf(g_config_data.serialnum);
//    printf("\r\n");

    str_st = GetComma(1,line);
    str_end = GetComma(2,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取productKey
//    if(strlen(des_line)!=3)
//    {
//        return 1;
//    }
    sprintf(g_config_data.clientid,"%s",des_line);

    printf("CE1:");												//打印productKey
    printf(g_config_data.clientid);
    printf("\r\n");

    str_st = GetComma(2,line);
    str_end = GetComma(3,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取deviceName

    sprintf(g_config_data.username,"%s",des_line);

    printf("CE2:");												//打印deviceName
    printf("%s\r\n",g_config_data.username);

    str_st = GetComma(3,line);
    str_end = GetComma(4,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取deviceSecret
    if(strlen(des_line)==0)
    {
        return 1;
    }
    sprintf(g_config_data.password,"%s",des_line);

    printf("CE3:");
    printf(g_config_data.password);									//打印deviceSecret
    printf("\r\n");

    str_st = GetComma(4,line);
    str_end = GetComma(5,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取topic
    if(strlen(des_line)==0)
    {
        return 1;
    }
    sprintf(g_config_data.topic,"%s",des_line);

    printf("CE4:");												//打印topic
    printf(g_config_data.topic);
    printf("\r\n");

		str_st = GetComma(5,line);
    str_end = GetComma(6,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取topicpost
    if(strlen(des_line)==0)
    {
        return 1;
    }
    sprintf(g_config_data.topicPost,"%s",des_line);

    printf("CE5:");												//打印topicpost
    printf(g_config_data.topicPost);
    printf("\r\n");
		
		str_st = GetComma(6,line);
    str_end = GetComma(7,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取humi
    if(strlen(des_line)==0)
    {
        return 1;
    }
    sprintf(g_config_data.mqttserverip,"%s",des_line);

    printf("CE6:");												//打印humi,
    printf(g_config_data.mqttserverip);
    printf("\r\n");
		
		str_st = GetComma(7,line);
    str_end = GetComma(8,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//temp
    if(strlen(des_line)==0)
    {
        return 1;
    }
    sprintf(g_config_data.mqttserverport,"%s",des_line);

    printf("CE7:");												//打印temp,端口号
    printf(g_config_data.mqttserverport);
    printf("\r\n");
		
		str_st = GetComma(8,line);
    str_end = GetComma(9,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//获取led
    if(strlen(des_line)==0)
    {
        return 1;
    }
    //sprintf(g_config_data.heartime,"%s",des_line);
		g_config_data.heartime = atoi(des_line);				   //转换为int类型
    printf("CE8:");												//打印led
    printf("%d",g_config_data.heartime);
    printf("\r\n");
		
    str_st = GetComma(9,line);
    str_end = GetComma(10,line);

    len = str_end - str_st;
    memset(des_line,0,Flash_FILED_LENGTH);

    strncpy(des_line, &line[str_st], len-1);  					//缓存大小
    if(strlen(des_line)==0)
    {
        return 1;
    }
    printf("CE9:");
    printf(des_line);
    printf("\r\n");
    printf("**************************\r\n");

    if(strlen(des_line) > 0)
        g_config_data.bufsize = atoi(des_line);				   //转换为int类型
    return 0;
}

/*清空Flash数据*/
void Flash_Clean(void)
{
    u8 i;
    u8 spi_Buf[USE_Flash_DATA_LENGTH];

    //填充缓冲
    for(i=0;i<USE_Flash_DATA_LENGTH-1;i++)
        spi_Buf[i]=0;


    //写
    printf("Write\r\n");
		Flash_WriteNByte(spi_Buf, 0, USE_Flash_DATA_LENGTH-1);   //从地址0，连续写入6字节数据(ABCDEF)

    //清缓冲
    for(i=0;i<255;i++)
        spi_Buf[i]=0;

    //读
    printf("Read\r\n");
    Flash_ReadNByte(spi_Buf,0,USE_Flash_DATA_LENGTH-1);

    for(i=0;i<254;i++)
    {

    }
}

