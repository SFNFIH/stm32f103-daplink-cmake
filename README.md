# STM32F103 DAPLink / ESP32 自动下载

基于 [ARMmbed/DAPLink](https://github.com/ARMmbed/DAPLink) 的 STM32F103xB 固件。  
目标侧 **同一套 6 针**；用 **ADG936（推荐）或 ADG904** 在 SWD 与硬件 USART 之间切换。  
上电读 **PB1** 决定模式，并驱动 **PB7 (MUX_CTRL)**。

| PB1 MODE_SEL | 模式 | PB7 MUX_CTRL |
|--------------|------|--------------|
| 悬空 / 高 | DAPLink（SWD） | 高 → 接通 SWDIO/SWCLK |
| 接 GND | ESP32 自动下载 | 低 → 接通 USART TX/RX |

---

## 统一 6 针

| 针 | 丝印 | 去向 |
|----|------|------|
| 1 | 3V3 | 电源 |
| 2 | GND | 地 |
| 3 | DIO | → 模拟开关 → SWDIO **或** USART_RX |
| 4 | CLK/TX | → 模拟开关 → SWCLK **或** USART_TX |
| 5 | RST | **PB0**（nRESET / EN，直连） |
| 6 | IO0 | **PB8**（仅 ESP32 用；DAP 可悬空） |

### STM32 引脚

| 功能 | GPIO |
|------|------|
| MODE_SEL | PB1（输入上拉） |
| MUX_CTRL | PB7（输出，接 ADG 控制脚） |
| SWDIO | PB14 |
| SWCLK | PB13 |
| USART2 TX | PA2 |
| USART2 RX | PA3 |
| nRESET / EN | PB0 |
| IO0 | PB8 |
| 连接 LED | PB6（ESP 模式常亮） |

ESP32 模式串口为 **硬件 USART2**，可稳定使用较高烧录波特率（如 460800 / 921600，视布线而定）。

---

## 模拟开关接线（ADG936 推荐）

ADG936 为双路 SPDT，一路切 DIO，一路切 CLK/TX：

```
                    ADG936
                 ┌──────────┐
  Header DIO ───│ S1A       │
                │     D1 ───│←── Header 针3
  PB14 SWDIO ───│ S1B       │      (公共端接排针)
                │           │
  Header 概念上由 D1 出到针3；S1A/S1B 为两路输入。
  （按 ADG936 数据手册：Dx 为公共端，SxA/SxB 为被选端）

  PA3  USART_RX ── S1A
  PB14 SWDIO    ── S1B
  D1 ────────────── 针3 DIO

  PA2  USART_TX ── S2A
  PB13 SWCLK    ── S2B
  D2 ────────────── 针4 CLK/TX

  CTRL ──────────── PB7 (MUX_CTRL)
  VDD / GND        3V3 / GND
  EN 接高（常开）
```

逻辑（与固件一致）：

| MUX_CTRL | 针3 | 针4 |
|----------|-----|-----|
| 高 (DAP) | PB14 SWDIO | PB13 SWCLK |
| 低 (ESP) | PA3 RX | PA2 TX |

若 CTRL 极性与芯片封装定义相反，可对调 SxA/SxB，或改 `IO_Config.h` 里 `MUX_CTRL_*_LEVEL`。

### 使用 ADG904（SP4T）时

用两片 ADG904（或一片多路）分别切换针3、针4；`A0/A1` 编码中仅用两态时，把 **A0 接 PB7**，A1 接地，只在两路输入间切换即可（其余输入悬空或接地）。

---

## 接线示例

**ESP32（PB1→GND）**

| 6 针 | ESP32 |
|------|-------|
| 3V3 | 3V3 |
| GND | GND |
| DIO | TX0 |
| CLK/TX | RX0 |
| RST | EN |
| IO0 | GPIO0 |

```bash
esptool.py -b 460800 --port <串口> write_flash 0x1000 app.bin
```

**DAPLink（PB1 悬空）**

| 6 针 | 目标 |
|------|------|
| 3V3 | 3V3 / VTref |
| GND | GND |
| DIO | SWDIO |
| CLK/TX | SWCLK |
| RST | nRESET |
| IO0 | — |

---

## 编译

```bash
sudo apt install cmake ninja-build python3-venv python-is-python3 \
  gcc-arm-none-eabi binutils-arm-none-eabi

git clone --recurse-submodules https://github.com/SFNFIH/stm32f103-daplink-cmake.git
cd stm32f103-daplink-cmake
./scripts/build.sh
```

产物：`build/firmware/stm32f103xb_*_crc.bin`  
MCU：**STM32F103CB**（Bootloader 48KB + Interface）。

---

## 说明

- 切换模式：改 PB1 后 **重新上电**（同时切换固件逻辑与 ADG 通道）。  
- 也可把 ADG `CTRL` 直接并到 MODE_SEL 跳线网络；固件仍会驱动 PB7，建议 CTRL 只接 PB7，MODE_SEL 只接跳线。  
- 上游 DAPLink：Apache-2.0  
- 仓库：https://github.com/SFNFIH/stm32f103-daplink-cmake
