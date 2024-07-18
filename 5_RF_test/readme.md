
## 1️⃣ Help
This example is AT Slave and is only used for RF testing;

Connect to USART1 with a baud rate of 9600;

NOTE: Each AT cmd should end with `'\n'` ;

AT cmd help is as follows:

~~~
AT+<CMD>?        : Help on <CMD>
AT+<CMD>         : Run <CMD>
AT+<CMD>=<value> : Set the value
AT+<CMD>=?       : Get the value
AT+VER Get the FW version
AT+VL=<Level><CR>. Set the Verbose Level=[0:Off .. 3:High]
AT+LTIME Get the local time in UTC format
ATZ Trig a MCU reset
AT+TTONE Starts RF Tone test
AT+TRSSI Starts RF RSSI tone test
AT+TCONF=<Freq in Hz>:<Power in dBm>:<Lora Bandwidth <0 to 6>, or Rx FSK Bandwidth in Hz>:<Lora SF or FSK datarate (bps)>:<CodingRate 4/5, 4/6, 4/7, 4/8>:
         <Lna>:<PA Boost>:<Modulation 0:FSK, 1:Lora, 2:BPSK, 3:MSK>:<PayloadLen in Bytes>:<FskDeviation in Hz>:<LowDrOpt 0:off, 1:on, 2:Auto>:
         <BTproduct: 0:no Gaussian Filter Applied, 1:BT=0,3, 2:BT=0,5, 3:BT=0,7, 4:BT=1><CR>. Configure RF test
AT+TCONF=868000000:14:50000:50000:4/5:0:0:0:16:25000:2:3 /*FSK*/
AT+TCONF=868000000:14:10000:10000:4/5:0:0:3:16:25000:2:3 /*MSK*/
AT+TCONF=868000000:14:4:12:4/5:0:0:1:16:25000:2:3 /*LORA*/
AT+TTX=<PacketNb><CR>. Starts RF Tx test: Nb of packets sent
AT+TRX=<PacketNb><CR>. Starts RF Rx test: Nb of packets expected
AT+TTH=<Fstart>,<Fstop>,<Fdelta>,<PacketNb><CR>. Starts RF Tx hopping test from Fstart to Fstop in Hz or MHz, Fdelta in Hz
AT+TOFF Stops on-going RF test
~~~

## 2️⃣ AT examples
Each AT cmd should end with `'\n'` ;

|                          AT cmd                          |           Help           |
| :------------------------------------------------------: | :----------------------: |
|                            AT                            |       Sample test        |
|                           AT?                            |         Get help         |
|                         AT+VER?                          |       Get ver help       |
|                         AT+VER=?                         |       Run get ver        |
|                        AT+TCONF?                         |     Get config help      |
|                        AT+TCONF=?                        |      Run get config      |
|    AT+TCONF=868000000:22:4:12:4/5:0:0:1:16:25000:2:3     |     Modulation LORA      |
| AT+TCONF=868000000:22:10000:10000:4/5:0:0:3:16:25000:2:3 |      Modulation FSK      |
| AT+TCONF=868000000:22:50000:50000:4/5:0:0:0:16:25000:2:3 |      Modulation MSK      |
|                         AT+TTONE                         |    Start RF Tone test    |
|                         AT+TRSSI                         | Starts RF RSSI tone test |
|                        AT+TTX=10                         | Set the number of tests  |


### Start RF Tone test

Each AT cmd should end with `'\n'` ;

1. Use `AT+TCONF` to set modulation mode，Lora、FSK or MSK
2. Start the test with the `AT+TTONE` command
3. Use `AT+TTX=num` to set the number of tests

