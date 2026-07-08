
# Ra-08 LoRa 通信示例

**中文** | [English](README.md)

基于 Q443-T3-STM32 开发板，通过串口 AT 指令控制 Ra-08（ASR6601）模块进行 LoRa 无线通信。

参考文档：https://docs.ai-thinker.com/Ra-08/

## 快速开始

串口连接开发板（波特率 115200），上电后控制台输出可用命令列表，直接输入 AT 指令操作。

### 两板对发示例

**发送端**

```
AT+CTXADDRSET=12        # 目标地址设为 12（对方本地地址）
AT+CADDRSET=13          # 本地地址设为 13
AT+CTX=433000000,7,2,2,22,0
```

进入透传模式后，串口输入的内容会直接发出。输入 `+++\r\n` 退出透传模式。

**接收端**

```
AT+CADDRSET=12          # 本地地址设为 12（匹配发送端目标地址）
AT+CRXS=433000000,7,2,2,0
```

收到数据后以字符串格式打印输出。

---

## AT 指令说明

### `AT+CTX` — 发送（透传模式）

```
AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>,<iqconverted>
```

配置发射参数并启动发送，进入透传模式（提示符 `>`，发送 `+++\r\n` 退出）。

### `AT+CRX` — 接收（HEX 格式）

```
AT+CRX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<iqconverted>
```

### `AT+CRXS` — 接收（字符串格式）

```
AT+CRXS=<freq>,<data_rate>,<bandwidth>,<code_rate>,<iqconverted>
```

### `AT+CADDRSET` — 设置本地地址

```
AT+CADDRSET=<local_addr>
```

无符号 16 位整数，默认为 0。

### `AT+CTXADDRSET` — 设置目标地址

```
AT+CTXADDRSET=<target_addr>
```

无符号 16 位整数，默认为 1。发送时数据包目标地址必须与接收端本地地址一致。

### `AT+CSLEEP` — 睡眠模式

```
AT+CSLEEP=<sleep_mode>    # 0：热启动唤醒，1：冷启动唤醒
```

---

## 参数取值表

### `data_rate`（扩频因子）

| 值 | SF |
|----|----|
| 0  | SF12 |
| 1  | SF11 |
| 2  | SF10 |
| 3  | SF9  |
| 4  | SF8  |
| 5  | SF7  |
| 6  | SF6  |
| 7  | SF5  |

### `bandwidth`（带宽）

| 值 | 带宽 |
|----|------|
| 0  | 125 KHz |
| 1  | 250 KHz |
| 2  | 500 KHz |
| 3  | 62.5 KHz |
| 4  | 41.67 KHz |
| 5  | 31.25 KHz |
| 6  | 20.83 KHz |
| 7  | 15.63 KHz |
| 8  | 10.42 KHz |
| 9  | 7.81 KHz |

### `code_rate`（编码率）

| 值 | 编码率 |
|----|--------|
| 1  | 4/5 |
| 2  | 4/6 |
| 3  | 4/7 |
| 4  | 4/8 |

### `pwr`（发射功率）

0 ~ 22 dBm

### `iqconverted`（IQ 转换）

0：关闭，1：开启