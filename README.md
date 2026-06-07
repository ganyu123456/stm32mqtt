# STM32 MQTT IoT 数据采集固件

基于 STM32F103RE 的物联网数据采集固件，通过 Quectel EC800M 4G 模块接入 EMQX MQTT Broker，实时上传 GPS 定位、温湿度、ADC 采样数据，并支持服务器远程控制 LED 输出。本项目是 [02-traceiot](../02-traceiot) 云平台的配套嵌入式端固件。

---

## 功能概述

- **GPS 定位上报**：通过 EC800M 内置 GNSS，获取 GNRMC 报文并进行坐标纠偏，上传经纬度至 MQTT Broker
- **温湿度采集**：读取 DHT11 传感器数据，周期性上传
- **ADC 采集**：读取 PC0（ADC_Channel_10）模拟量，转换为电压值后上传
- **MQTT 双向通信**：订阅控制主题，接收服务器下发的 LED 控制指令（`LEDK`/`LEDG`）
- **LCD 显示**：SPI 接口 128×160 TFT 屏，显示系统状态、Logo 及中英文字符
- **SPI Flash 配置存储**：将服务器地址、MQTT 凭据、Topic 等配置持久化至外部 Flash
- **独立看门狗（IWDG）**：防止程序死循环，保障系统长期运行稳定
- **多串口调试**：UART1 连接 PC 输出调试日志，UART2 连接 4G 模块，UART3 备用 TTL

---

## 硬件平台

| 项目 | 规格 |
|------|------|
| 主控 MCU | STM32F103RE（Cortex-M3，512KB Flash，64KB SRAM） |
| 4G/GNSS 模块 | Quectel EC800M（含内置 GPS，AT 指令接口） |
| 温湿度传感器 | DHT11（单总线，接 PA11） |
| ADC 输入 | PC0（ADC1 通道 10，12-bit，参考电压 3.3V） |
| LCD 屏 | 128×160 SPI TFT（ST7735 兼容） |
| 外部 Flash | SPI NOR Flash（W25Qxx 系列） |
| 看门狗 | STM32 内置 IWDG |
| 开发工具 | Keil MDK-ARM（μVision5），ARM-ADS 工具链 |

### 引脚定义

| 外设 | 引脚 | 说明 |
|------|------|------|
| LED1 | PB5 | 系统运行指示 |
| LED2 | PB4 | 备用 |
| LED3 | PB3 | 服务器远程控制输出 |
| 4G PWRKEY | PC7 | EC800M 开机键 |
| 4G RESET | PC6 | EC800M 复位 |
| DHT11 DATA | PA11 | 温湿度单总线 |
| ADC IN | PC0 | 模拟量输入 |
| LCD SCL | PA5 | SPI 时钟 |
| LCD SDA | PA7 | SPI MOSI |
| LCD CS | PC4 | 片选 |
| LCD RS | PB1 | 命令/数据选择 |
| LCD RST | PB0 | 复位 |
| LCD BL | PC5 | 背光控制 |
| UART1 TX/RX | PA9/PA10 | 调试串口（连接 PC） |
| UART2 TX/RX | PA2/PA3 | 4G 模块通信 |
| UART3 TX/RX | PB10/PB11 | TTL 备用串口 |

---

## 目录结构

```
03-stm32mqtt/
├── USER/                           # 用户主程序
│   ├── main.c                      # 主函数及初始化流程
│   ├── stm32f10x_it.c/.h           # 中断服务程序
│   ├── stm32f10x.h                 # STM32 外设寄存器定义
│   ├── stm32f10x_conf.h            # 外设库使能配置
│   ├── system_stm32f10x.c/.h       # 系统时钟初始化
│   └── CSTX.uvprojx                # Keil 工程文件
│
├── HARDWARE/                       # 硬件驱动层
│   ├── 4G/                         # EC800M 4G 模块驱动（AT指令、MQTT、GPS）
│   ├── DHT11/                      # DHT11 温湿度传感器驱动
│   ├── adc/                        # ADC 采集驱动
│   ├── flash/                      # SPI Flash 驱动及配置存储
│   ├── LCD/                        # TFT LCD 驱动、GUI、中英文字库
│   ├── LED/                        # LED 及 4G 模块电源控制
│   ├── spi/                        # SPI 总线驱动
│   ├── TIMER/                      # 定时器驱动（TIM2/3/4）
│   └── WDG/                        # IWDG 独立看门狗驱动
│
├── SYSTEM/                         # 系统基础层（正点原子库）
│   ├── delay/                      # 精确延时（delay_ms / delay_us）
│   ├── sys/                        # 系统初始化、NVIC 配置
│   └── usart/                      # 多串口驱动及缓冲区管理
│
├── CORE/                           # Cortex-M3 核心支持
│   ├── core_cm3.c/.h               # CMSIS 核心驱动
│   └── startup_stm32f10x_hd.s      # 大容量系列启动文件（HD，用于 F103RE）
│
├── STM32F10x_FWLib/                # ST 官方标准外设库
│   ├── inc/                        # 外设头文件
│   └── src/                        # 外设源文件
│
├── OUTPUT/                         # 编译输出（.hex/.axf/.map 等）
│   └── CSTX.hex                    # 可烧录固件
│
├── keilkilll.bat                   # 清除编译中间文件脚本
└── Quectel_ECx00U&EGx00U_Series_AT_Commands_Manual_V1.0.0.pdf
                                    # EC800M AT 指令参考手册
```

---

## 软件架构与启动流程

```
main()
  ├─ RCC1_Configuration()         使能 GPIO/USART/TIM4/SPI2 时钟
  ├─ delay_init()                  延时函数初始化
  ├─ NVIC_Configuration()          中断分组（2位抢占+2位响应）
  ├─ CSTX_4GCTR_Init()             EC800M 供电及开机
  ├─ uart_init(115200)             UART1 调试串口
  ├─ LED_Init() / LED_Run()        LED 初始化及跑马灯测试
  ├─ uart2_init(115200)            UART2 连接 EC800M
  ├─ uart3_init(115200)            UART3 TTL 串口
  ├─ Lcd_Init()                    LCD 初始化
  │   ├─ showimage(gImage_qq)      显示 Logo
  │   ├─ Num_Test() / Font_Test()  数码管及中英文字体测试
  ├─ Init_TIM2()                   TIM2 定时器初始化
  ├─ Adc_Init()                    ADC 初始化（PC0）
  ├─ CSTX_4G_Init()                EC800M 模块初始化
  │   ├─ ATE1                      AT 回显
  │   ├─ AT+CIMI                   检测 SIM 卡（需含"460"国内运营商标识）
  │   ├─ AT+CGSN                   获取 IMEI
  │   ├─ AT+CGATT?                 等待网络附着（GPRS 注册成功）
  │   └─ AT+CSQ                    查询信号质量
  ├─ Start_GPS()                   开启 GNSS 并等待首次定位（GNRMC 有效）
  ├─ CSTX_4G_RegALIYUNIOT()        连接 EMQX Broker，订阅控制主题
  └─ while(1)
      ├─ CSTX_4G_ALYIOTADC()       上传 ADC 值（每轮 1s）
      ├─ CSTX_4G_ALYIOTSenddata()  上传温湿度（每轮 1s）
      ├─ Get_GPS_RMC()             上传 GPS 坐标（每轮 1s）
      ├─ CSTX_4G_RECTCPData()      接收并处理服务器下发指令
      ├─ LCD 刷新状态显示
      └─ IWDG_Feed()               喂狗（防止复位）
```

---

## MQTT 通信协议

### 连接参数

| 参数 | 值 |
|------|----|
| Broker 地址 | `106.15.62.60` |
| 端口 | `1883` |
| Client ID | `cstx123456` |
| 用户名 | `admin` |
| 密码 | `public` |
| 订阅/发布 Topic | `testtopic` |
| QoS | 0 |

### 上行数据（设备→云端）

设备每秒轮流上传以下三种 JSON 报文：

**ADC 数据：**
```json
{"msg":"adcx":1234,"Voltage":0.9942}
```

**温湿度数据：**
```json
{"msg":"temp":25,"humi":60}
```

**GPS 坐标数据（已纠偏为十进制度）：**
```json
{"msg":"LongitudeStr":119.xxx,"LatitudeStr":26.xxx}
```

### 下行指令（云端→设备）

| 指令字符串 | 动作 |
|------------|------|
| `LEDK` | 点亮 PB3（LED3 低电平） |
| `LEDG` | 熄灭 PB3（LED3 高电平） |

---

## GPS 坐标处理

1. 通过 `AT+QGPS=1` 开启 EC800M 内置 GNSS
2. 循环发送 `AT+QGPSGNMEA="RMC"` 获取 `$GNRMC` 报文
3. 判断报文第 17 位为 `'A'`（有效定位）后开始解析
4. 将 NMEA ddmm.mmmm 格式转换为十进制度（dd + mm/60），消除度分格式误差
5. 纠偏后的经纬度作为浮点字符串封装进 JSON 上传

---

## Flash 配置存储

`Flash_CONFIG_DATA` 结构体（存储于外部 SPI NOR Flash）包含以下字段：

| 字段 | 说明 |
|------|------|
| `serialnum` | 设备序列号 |
| `ipaddr` / `portnum` | TCP 服务器地址和端口 |
| `mqttserverip` / `mqttserverport` | MQTT Broker 地址和端口 |
| `clientid` | MQTT Client ID |
| `username` / `password` | MQTT 认证凭据 |
| `topic` / `topicPost` | 订阅/发布 Topic |
| `heartime` | 心跳间隔（秒） |

---

## 开发环境与编译

### 工具要求

- **Keil MDK-ARM** v5.x（μVision5）
- **芯片支持包**：Keil.STM32F1xx_DFP.1.1.0（可从 Pack Installer 安装）
- **下载器**：ST-Link V2 或 J-Link（支持 SWD 接口）

### 编译步骤

1. 双击 `USER/CSTX.uvprojx` 打开 Keil 工程
2. 点击 **Build**（F7）编译，无报错后在 `OUTPUT/` 目录生成 `CSTX.hex`
3. 点击 **Download**（F8）或使用 ST-Link Utility 将 `CSTX.hex` 烧录至目标板

### 清理编译中间文件

```bat
keilkilll.bat
```

---

## 串口调试

| 串口 | 波特率 | 用途 |
|------|--------|------|
| UART1 | 115200 | 连接 PC，输出 AT 指令交互日志、传感器数据、GPS 信息 |
| UART2 | 115200 | 连接 EC800M 4G 模块（内部使用，不对外暴露） |
| UART3 | 115200 | 备用 TTL，可扩展其他设备 |

推荐使用串口助手（如 XCOM、SecureCRT）以 115200/8N1 打开 UART1 查看运行日志。

典型日志输出：
```
############ http://www.csgsm.com/ ############
start init EC800M
****单片机和模块连接成功*****
CIMI:460xxxxxxxxxx，CGSN:86952305xxxxxxx
信号质量是:+CSQ: 23,0  注意：信号最大值是31
正在定位GPS信息，请耐心等候!!
GPS信息定位成功！！
温度:25 湿度:60
ADC1原始数值：1234
GPS信息上传成功！！！
```

---

## 与 TraceIoT 云平台对接

本固件与 [02-traceiot](../02-traceiot) 物联网平台配套使用：

1. 部署 02-traceiot（含 EMQX Broker）后，确认 Broker 地址与固件中 `CSTX_4G_RegALIYUNIOT()` 的 IP 一致
2. 在 TraceIoT 管理后台注册设备，设备编码填写固件使用的 IMEI 或 Client ID
3. 固件通过 `testtopic` 发布数据，后端 `GpsMqttWorker` 订阅后写入 Redis（实时位置）和 InfluxDB（历史轨迹）
4. 在 TraceIoT 实时地图页面即可查看设备位置

> **注意：** 代码中 MQTT Broker IP `106.15.62.60` 为开发测试地址，正式部署时需修改 `CSTX_4G_RegALIYUNIOT()` 函数中的 IP 为实际 EMQX 服务器地址。

---

## 参考文档

- `Quectel_ECx00U&EGx00U_Series_AT_Commands_Manual_V1.0.0_Preliminary_20210309.pdf`  
  EC800M AT 指令完整手册，含 MQTT（QMTOPEN/QMTCONN/QMTPUB/QMTSUB）和 GPS（QGPS/QGPSGNMEA）指令说明
- [STM32F103RE 数据手册](https://www.st.com/en/microcontrollers-microprocessors/stm32f103re.html)
- [Keil MDK 官方文档](https://www.keil.com/support/man/docs/uv4/)
