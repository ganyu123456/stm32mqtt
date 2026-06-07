/* 包含的头文件 --------------------------------------------------------------*/
#include "spi.h"


/************************************************
函数名称 ： SPI_GPIO_Configuration
功    能 ： SPI引脚配置
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void SPI_GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* 使能APB2时钟 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  /* 使能APB1时钟 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4|RCC_APB1Periph_SPI2, ENABLE);

  /* SPI: CS推挽输出 */
  GPIO_InitStructure.GPIO_Pin = PIN_SPI_CS;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(PORT_SPI_CS, &GPIO_InitStructure);

  /* SPI: SCK, MOSI复用输出 */
  GPIO_InitStructure.GPIO_Pin = PIN_SPI_SCK | PIN_SPI_MOSI;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(PORT_SPI_SCK, &GPIO_InitStructure);

  /* SPI: MISO浮动输入 */
  GPIO_InitStructure.GPIO_Pin = PIN_SPI_MISO;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(PORT_SPI_MISO, &GPIO_InitStructure);
}

/************************************************
函数名称 ： SPI_Configuration
功    能 ： SPI配置
            SPI为主模式，时钟线平时为低，上升沿采集数据
            8位数据格式，软件控制片选，数据高位在前
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void SPI_Configuration(void)
{
  SPI_InitTypeDef  SPI_InitStructure;

  /* SPI 初始化定义 */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //SPI设置为双线双向全双工
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                      //设置为主 SPI
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                  //SPI发送接收 8 位帧结构
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                        //时钟悬空高
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                       //数据捕获于第二个时钟沿
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                          //软件控制 NSS 信号
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; //波特率预分频值为2
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                 //数据传输从 MSB 位开始
  SPI_InitStructure.SPI_CRCPolynomial = 7;                           //定义了用于 CRC值计算的多项式
  SPI_Init(SPI2, &SPI_InitStructure);

  SPI_Cmd(SPI2, ENABLE);                                             //使能 SPI2
}

/************************************************
函数名称 ： SPI_Initializes
功    能 ： SPI初始化
参    数 ： 无
返 回 值 ： 无
作    者 ： strongerHuang
*************************************************/
void SPI_Initializes(void)
{
  SPI_GPIO_Configuration();
  SPI_Configuration();
}

/************************************************
函数名称 ： SPI_WriteByte
功    能 ： SPI写一字节数据
参    数 ： TxData --- 发送的字节数据
返 回 值 ： 读回来的字节数据
作    者 ： strongerHuang
*************************************************/
uint8_t SPI_WriteByte(uint8_t TxData)
{
  while((SPI2->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);
  SPI2->DR = TxData;

  while((SPI2->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  return SPI2->DR;
}

/************************************************
函数名称 ： SPI_ReadByte
功    能 ： SPI读一字节数据
参    数 ： 无
返 回 值 ： 读回来的字节数据
作    者 ： strongerHuang
*************************************************/
uint8_t SPI_ReadByte(void)
{
  while((SPI2->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);
  SPI2->DR = 0xFF;

  while((SPI2->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  return SPI2->DR;
}


/**** Copyright (C)2016 strongerHuang. All Rights Reserved **** END OF FILE ****/
