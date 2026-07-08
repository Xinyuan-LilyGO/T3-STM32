# Ra-08 AT Command Reference

## Firmware Boot Message

```
/************************************************************************
*************************************************************************
*                         ASR6601 LoRa Test                              *
*           fix RF switch with ANT_SW_CTRL and IO47(C15)                 *
* version:v0.0.4(Oct  6 2022-19:44:33)                                   *
* Available commands are:                                                 *
* AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>                *
* AT+CRX=<freq>,<data_rate>,<bandwidth>,<code_rate>                      *
* AT+CRXS=<freq>,<data_rate>,<bandwidth>,<code_rate>                     *
*************************************************************************
************************************************************************/
```

---

## AT Commands

### 1. `AT+CTX` — Transmit

**Syntax:** `AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>,<iqconverted>`

Configures TX parameters and starts transmission. Enters transparent transmission mode (serial passthrough). The prompt `>` indicates passthrough is active. Send `+++\r\n` to exit passthrough mode.

| Parameter | Description |
|-----------|-------------|
| `freq` | Transmit frequency (Hz) |
| `data_rate` | Spreading factor (see table below) |
| `bandwidth` | Channel bandwidth (see table below) |
| `code_rate` | Coding rate (see table below) |
| `pwr` | Transmit power, range: 0–22 |
| `iqconverted` | IQ inversion: `0` = disabled, `1` = enabled |

---

### 2. `AT+CRX` — Receive (HEX output)

**Syntax:** `AT+CRX=<freq>,<data_rate>,<bandwidth>,<code_rate>`

Configures RX parameters and starts receiving. Received data is printed in HEX format.

| Parameter | Description |
|-----------|-------------|
| `freq` | Receive frequency (Hz) |
| `data_rate` | Spreading factor (see table below) |
| `bandwidth` | Channel bandwidth (see table below) |
| `code_rate` | Coding rate (see table below) |
| `iqconverted` | IQ inversion: `0` = disabled, `1` = enabled |

---

### 3. `AT+CRXS` — Receive (String output)

**Syntax:** `AT+CRXS=<freq>,<data_rate>,<bandwidth>,<code_rate>`

Configures RX parameters and starts receiving. Received data is printed as a string.

| Parameter | Description |
|-----------|-------------|
| `freq` | Receive frequency (Hz) |
| `data_rate` | Spreading factor (see table below) |
| `bandwidth` | Channel bandwidth (see table below) |
| `code_rate` | Coding rate (see table below) |
| `iqconverted` | IQ inversion: `0` = disabled, `1` = enabled |

---

### 4. `AT+CADDRSET` — Set Local Address

**Syntax:** `AT+CADDRSET=<local_addr>`

Sets the local device address. Address is an unsigned 16-bit integer. Default: `0`.

---

### 5. `AT+CTXADDRSET` — Set Target Address

**Syntax:** `AT+CTXADDRSET=<target_addr>`

Sets the destination address for transmitted data. Address is an unsigned 16-bit integer. Default: `1`.

---

### 6. `AT+CSLEEP` — Enter Sleep Mode

**Syntax:** `AT+CSLEEP=<sleep_mode>`

Puts the device into sleep mode. Wake-up is triggered by a UART interrupt.

| Value | Description |
|-------|-------------|
| `0` | Warm start (retains state) |
| `1` | Cold start (full reset on wake) |

---

## Parameter Tables

### Spreading Factor (`data_rate`)

| Value | SF |
|-------|----|
| `0` | SF12 |
| `1` | SF11 |
| `2` | SF10 |
| `3` | SF9 |
| `4` | SF8 |
| `5` | SF7 |
| `6` | SF6 |
| `7` | SF5 |

### Bandwidth (`bandwidth`)

| Value | Bandwidth |
|-------|-----------|
| `0` | 125 KHz |
| `1` | 250 KHz |
| `2` | 500 KHz |
| `3` | 62.5 KHz |
| `4` | 41.67 KHz |
| `5` | 31.25 KHz |
| `6` | 20.83 KHz |
| `7` | 15.63 KHz |
| `8` | 10.42 KHz |
| `9` | 7.81 KHz |

### Coding Rate (`code_rate`)

| Value | Coding Rate |
|-------|-------------|
| `1` | 4/5 |
| `2` | 4/6 |
| `3` | 4/7 |
| `4` | 4/8 |
