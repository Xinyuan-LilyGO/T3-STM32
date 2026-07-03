## T-STM32

- In receive mode, `PB2` pin is at a `high` level and `PB6` pin is at a `low` level.
- In transmission mode, `PB2` pin is at a `low` level and `PB6` pin is at a `high` level.

~~~
*****  RX *****  
PB2 --- 1
PB6 --- 0

*****  TX *****  
PB2 --- 0
PB6 --- 1
~~~

## Ra-08

接收 Ra-08 发送的数据
- 1、Ra-08 下载串口透传程序 lora_transparent_lpuart_ADDR.bin
- 2、设置 Ra-08 参数为：
    - AT+CADDRSET=13
    - AT+CTXADDRSET=12
    - AT+CTX=433000000,7,2,2,22,0
- 3、然后在 Ra-08 串口发送的数据，会被示例程序接收到

## 参考资料
- Ra-08(H)模组透传固件、源码以及指令说明：[点击下载](https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/docs/_media_old/ra-08_lora_transparent_at_20221118_lpuart_addr.zip)
- Ra-08(H)模组/开发板透传固件使用说明：[点击查看](https://aithinker.blog.csdn.net/article/details/127969603?spm=1001.2014.3001.5502)
- Ra-08(H)模组固件烧录工具：[点击下载](https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/docs/_media_old/firmware_update_tool.zip)
- [Ai-Thinker-LoRaWAN-Ra-08](https://github.com/Ai-Thinker-Open/Ai-Thinker-LoRaWAN-Ra-08)