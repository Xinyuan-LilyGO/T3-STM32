
# Ra-08 LoRa 通信示例

**中文** | [English](README.md)

基于 Q443-T3-STM32 开发板，通过串口 AT 指令控制 Ra-08（ASR6601）模块进行 LoRa 无线通信。

参考文档：
- ai-thinker：https://docs.ai-thinker.com/Ra-08/
- 指令说明：[AT 指令说明](./指令说明.txt)

## 快速开始

串口连接开发板（波特率 9600），上电后控制台输出可用命令列表，直接输入 AT 指令操作。

### 两板对发示例

测试设备：
![](./image/device.png)

#### LilyGo-T3 设置步骤 (发送端)

- 下载 [examples/Ra-08](https://github.com/Xinyuan-LilyGO/T3-STM32/tree/master/examples/Ra-08/MDK-ARM/Ra-08) 程序
- 通过串口发送如下 AT 命令，串口波特率设置为 9600
~~~
AT+CTXADDRSET=12        # 目标地址设为 12（对方本地地址）
AT+CADDRSET=13          # 本地地址设为 13
AT+CTX=920800000,7,2,2,22,0
~~~

进入透传模式后，串口输入的内容会直接发出。输入 `+++\r\n` 退出透传模式。

#### Ra-08 设置步骤 (接收端)

- 下载 [ra-08_lora_transparent_at_20221118_lpuart_addr](https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/docs/_media_old/ra-08_lora_transparent_at_20221118_lpuart_addr.zip) 程序
![](./image/download_cn.png)
- 通过串口发送如下 AT 命令，串口波特率设置为 9600
~~~
- AT+CADDRSET=12        # 本地地址设为 12（匹配发送端目标地址）
- AT+CRXS=920800000,7,2,2,0
~~~
收到数据后以字符串格式打印输出。

