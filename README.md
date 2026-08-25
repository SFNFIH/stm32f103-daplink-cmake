# STM32F103 DAPLink + ESP32 自动下载（CMake）

基于官方开源固件 [ARMmbed/DAPLink](https://github.com/ARMmbed/DAPLink) 的 **STM32F103xB** 接口芯片（HIC）工程。  
上电时根据 **MODE_SEL** 引脚电平，在两种功能间切换：

| MODE_SEL (PB1) | 功能 |
|----------------|------|
| 高电平 / 悬空（内部上拉） | **DAPLink**：CMSIS-DAP 调试 + CDC 串口 + U 盘 |
| 接 GND | **ESP32 自动下载**：CDC 串口 + DTR/RTS 控制 EN/IO0（兼容 esptool） |

顶层 CMake：准备 Python/progen → 应用 `overlay/` → 编译 → 收集固件。

## 依赖

```bash
sudo apt install cmake ninja-build python3-venv python-is-python3 \
  gcc-arm-none-eabi binutils-arm-none-eabi
```

```bash
git clone --recurse-submodules https://github.com/SFNFIH/stm32f103-daplink-cmake.git
cd stm32f103-daplink-cmake
```

## 快速构建

```bash
chmod +x scripts/*.sh
./scripts/build.sh
# 或
cmake -S . -B build && cmake --build build
```

固件在 `build/firmware/`：`stm32f103xb_bl_crc.*`、`stm32f103xb_if_crc.*`。

## 双模式接线

### 共用

| 功能 | 引脚 |
|------|------|
| USB | STM32 USB DM/DP |
| MODE_SEL | **PB1**（上拉；接下拉/GND = ESP32 模式） |
| UART TX → 目标 RX | **PA2** |
| UART RX ← 目标 TX | **PA3** |
| GND | 共地 |

### DAPLink 模式（PB1 悬空/高）

| 功能 | 引脚 |
|------|------|
| SWCLK | PB13 |
| SWDIO 出 / 入 | PB14 / PB12 |
| nRESET | PB0 |
| SWO | PA10 |

### ESP32 自动下载模式（PB1→GND）

| 功能 | 引脚 | 接 ESP32 |
|------|------|----------|
| EN | **PB0** | EN / CHIP_PU |
| IO0 (BOOT) | **PB8** | GPIO0 |
| TX | PA2 | RX0 |
| RX | PA3 | TX0 |

控制逻辑与经典 DevKit 自动下载电路一致（给 esptool 用）：

- USB CDC **RTS 有效** → EN 拉低  
- USB CDC **DTR 有效** → IO0 拉低  

ESP32 模式下上电后 **连接 LED（PB6）常亮**，便于区分模式。

烧录示例：

```bash
esptool.py --port /dev/ttyACM0 write_flash 0x1000 firmware.bin
```

（具体端口名以系统枚举为准。）

## 常用 CMake 选项

```bash
cmake -S . -B build -DDAPLINK_BUILD_BOOTLOADER=OFF
cmake -S . -B build -DDAPLINK_INTERFACE_PROJECT=stm32f103xb_stm32f103rb_if
cmake -S . -B build -DDAPLINK_CMAKE_GENERATOR=make
```

## 目录结构

```
.
├── CMakeLists.txt
├── overlay/stm32f103xb/     # 双模式补丁（构建前拷入 DAPLink HIC）
│   ├── esp32_autoload.c/.h
│   ├── IO_Config.h
│   ├── gpio.c
│   └── uart.c
├── cmake/
├── scripts/
└── third_party/DAPLink/     # 官方源码 submodule（develop）
```

## 烧录提示

1. 用 ST-Link / ISP 将 bootloader 烧到 `0x08000000`，再烧 interface。  
2. **换模式需重新上电**（仅上电时采样 PB1）。  
3. 芯片：**STM32F103CB**（128KB Flash），Bootloader 48KB + Interface。

上游 DAPLink：Apache-2.0。
