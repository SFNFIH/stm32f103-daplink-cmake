# STM32F103 DAPLink / ESP32 自动下载

基于 [ARMmbed/DAPLink](https://github.com/ARMmbed/DAPLink) 的 STM32F103xB 固件。  
两种功能**不能同时用**，因此目标侧做成**同一套 6 针排针**（含电源）；上电读 **PB1** 决定当前功能。

| PB1（板上跳线，不在 6 针里） | 模式 |
|------------------------------|------|
| 悬空 / 高（内部上拉） | DAPLink：SWD 调试 |
| 接 GND | ESP32：串口自动下载（esptool） |

换模式请断电再上电。

---

## 统一 6 针排针

两种模式共用同一排针、同一焊盘：

| 针号 | 丝印 | STM32 | DAPLink 模式 | ESP32 模式 |
|------|------|-------|--------------|------------|
| 1 | 3V3 | 3V3 | 电源 | 电源 |
| 2 | GND | GND | 地 | 地 |
| 3 | DIO | **PB14** | SWDIO | **UART RX（软件串口）** |
| 4 | CLK/TX | **PA2** | SWCLK | **UART TX（软件串口）** |
| 5 | RST | **PB0** | nRESET | EN（GPIO / RTS） |
| 6 | IO0 | **PA3** | UART RX（CDC，硬件 USART2） | **IO0（GPIO / DTR）** |

说明：

- ESP32 烧录串口为 **GPIO 位带软件 UART**（不是硬件 USART）；IO0/EN 为普通 GPIO。  
- **IO0 与 RX 已对调**：IO0=PA3，RX=PB14。  
- DAP 模式下 PA2 作 SWCLK，故目标串口仅 RX（PA3）可用。  
- ESP32 模式：DTR→IO0、RTS→EN；PB6 连接灯常亮。

```
        6-Pin Header
   ┌─────────────────┐
   │ 1 3V3    2 GND  │
   │ 3 DIO    4 CLK  │  DIO=SWDIO/软RX  CLK=SWCLK/软TX
   │ 5 RST    6 IO0  │  RST=nRESET/EN   IO0=CDC_RX/ESP_IO0
   └─────────────────┘
```

---

## 接线示例

**烧录 ESP32（PB1→GND 后上电）**

| 6 针 | ESP32 |
|------|-------|
| 3V3 | 3V3 |
| GND | GND |
| DIO (PB14) | TX0（模组输出） |
| CLK/TX (PA2) | RX0（模组输入） |
| RST (PB0) | EN |
| IO0 (PA3) | GPIO0 |

```bash
esptool.py --port <串口> write_flash 0x1000 app.bin
```

**调试其它 MCU（PB1 悬空）**

| 6 针 | 目标 |
|------|------|
| 3V3 | VTref / 3V3（按需） |
| GND | GND |
| DIO | SWDIO |
| CLK/TX | SWCLK |
| RST | nRESET |
| IO0 | 目标 UART TX（可选日志） |

---

## 编译

```bash
sudo apt install cmake ninja-build python3-venv python-is-python3 \
  gcc-arm-none-eabi binutils-arm-none-eabi

git clone --recurse-submodules https://github.com/SFNFIH/stm32f103-daplink-cmake.git
cd stm32f103-daplink-cmake
./scripts/build.sh
```

固件：`build/firmware/stm32f103xb_bl_crc.*`、`stm32f103xb_if_crc.*`  
芯片：**STM32F103CB**（Bootloader 48KB + Interface）。

```bash
cmake -S . -B build -DDAPLINK_BUILD_BOOTLOADER=OFF   # 仅接口固件
cmake --build build --target daplink-clean           # 清理
```

---

## 仓库结构

```
overlay/stm32f103xb/     # 统一 6 针 + 双模式补丁（构建时写入 DAPLink）
third_party/DAPLink/     # 官方源码 submodule（develop）
CMakeLists.txt / cmake/ / scripts/
```

上游 DAPLink：Apache-2.0  
本仓库：https://github.com/SFNFIH/stm32f103-daplink-cmake
