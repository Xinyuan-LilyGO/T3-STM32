# Ra-08 LoRa Communication Example

[中文](README_CN.md) | **English**

Based on the Q443-T3-STM32 board, controls the Ra-08 (ASR6601) LoRa module via serial AT commands.

Reference: https://docs.ai-thinker.com/Ra-08/

## Quick Start

Connect the board via serial (115200 baud). On power-up, the console prints the available commands. Type AT commands directly to operate.

### Two-board Communication Example

**Transmitter**

```
AT+CTXADDRSET=12        # target address = 12 (receiver's local address)
AT+CADDRSET=13          # local address = 13
AT+CTX=433000000,7,2,2,22,0
```

After entering transparent transmission mode (prompt `>`), serial input is sent directly. Type `+++\r\n` to exit.

**Receiver**

```
AT+CADDRSET=12          # local address = 12 (matches transmitter's target address)
AT+CRXS=433000000,7,2,2,0
```

Received data is printed as a string.

---

## AT Command Reference

### `AT+CTX` — Transmit (transparent mode)

```
AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>,<iqconverted>
```

Configures TX parameters, starts transmission, and enters transparent mode.

### `AT+CRX` — Receive (HEX output)

```
AT+CRX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<iqconverted>
```

### `AT+CRXS` — Receive (string output)

```
AT+CRXS=<freq>,<data_rate>,<bandwidth>,<code_rate>,<iqconverted>
```

### `AT+CADDRSET` — Set local address

```
AT+CADDRSET=<local_addr>
```

Unsigned 16-bit integer, default 0.

### `AT+CTXADDRSET` — Set target address

```
AT+CTXADDRSET=<target_addr>
```

Unsigned 16-bit integer, default 1. Must match the receiver's local address.

### `AT+CSLEEP` — Sleep mode

```
AT+CSLEEP=<sleep_mode>    # 0: warm start wake-up, 1: cold start wake-up
```

---

## Parameter Reference

### `data_rate` (Spreading Factor)

| Value | SF |
|-------|----|
| 0 | SF12 |
| 1 | SF11 |
| 2 | SF10 |
| 3 | SF9 |
| 4 | SF8 |
| 5 | SF7 |
| 6 | SF6 |
| 7 | SF5 |

### `bandwidth`

| Value | Bandwidth |
|-------|-----------|
| 0 | 125 KHz |
| 1 | 250 KHz |
| 2 | 500 KHz |
| 3 | 62.5 KHz |
| 4 | 41.67 KHz |
| 5 | 31.25 KHz |
| 6 | 20.83 KHz |
| 7 | 15.63 KHz |
| 8 | 10.42 KHz |
| 9 | 7.81 KHz |

### `code_rate` (Coding Rate)

| Value | Rate |
|-------|------|
| 1 | 4/5 |
| 2 | 4/6 |
| 3 | 4/7 |
| 4 | 4/8 |

### `pwr` (TX Power)

0 ~ 22 dBm

### `iqconverted` (IQ Inversion)

0: disabled, 1: enabled
