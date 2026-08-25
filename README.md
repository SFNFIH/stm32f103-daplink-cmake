# STM32F103 DAPLink / ESP32 自动下载

基于 [ARMmbed/DAPLink](https://github.com/ARMmbed/DAPLink) 的 STM32F103xB 固件工程。  
一块板子、一份固件，上电读 **PB1** 在两种工作模式间切换。

| PB1 | 模式 | 用途 |
|-----|------|------|
| 悬空 / 高电平（内部上拉） | DAPLink | CMSIS-DAP 调试、虚拟串口、U 盘拖拽烧录 |
| 接 GND | ESP32 自动下载 | USB 串口 + DTR/RTS 控制 EN/IO0，配合 esptool |

> 模式只在上电时采样一次，切换模式请断电再上电。

---

## 功能概览

**DAPLink 模式**

- CMSIS-DAP（SWD）
- CDC 虚拟串口
- MSD 拖拽升级

**ESP32 模式**

- CDC 透传（USART2）
- 模拟经典 DevKit 自动下载电路：`RTS → EN`，`DTR → GPIO0`
- 连接 LED（PB6）常亮，便于识别当前模式

---

## 硬件

- MCU：**STM32F103CB**（128 KB Flash / 20 KB RAM）
- Flash 布局：Bootloader 48 KB + Interface

### 引脚一览

| 信号 | 引脚 | DAPLink | ESP32 模式 |
|------|------|---------|------------|
| MODE_SEL | PB1 | 模式选择（上拉） | 接 GND 进入本模式 |
| UART TX | PA2 | 目标串口 TX | → ESP32 RX0 |
| UART RX | PA3 | 目标串口 RX | ← ESP32 TX0 |
| nRESET / EN | PB0 | 目标复位 | → ESP32 EN |
| IO0 / BOOT | PB8 | — | → ESP32 GPIO0 |
| SWCLK | PB13 | SWD 时钟 | — |
| SWDIO 出 | PB14 | SWD 数据出 | — |
| SWDIO 入 | PB12 | SWD 数据入 | — |
| SWO | PA10 | 跟踪输出 | — |
| 连接 LED | PB6 | 连接指示 | ESP32 模式下常亮 |
| 状态 LED | PA9 | HID/CDC/MSC | 同左 |
| USB 连接 | PA15 | USB 拉高 | 同左 |

### ESP32 最小接线

```
STM32        ESP32
─────        ─────
PA2    →     RX0
PA3    ←     TX0
PB0    →     EN
PB8    →     GPIO0
PB1    →     GND   （选择 ESP32 模式）
GND    —     GND
```

---

## 快速开始

### 1. 环境

```bash
sudo apt install cmake ninja-build python3-venv python-is-python3 \
  gcc-arm-none-eabi binutils-arm-none-eabi
```

### 2. 获取源码

```bash
git clone --recurse-submodules https://github.com/SFNFIH/stm32f103-daplink-cmake.git
cd stm32f103-daplink-cmake
```

若已克隆未拉子模块：

```bash
git submodule update --init --recursive
```

### 3. 编译

```bash
chmod +x scripts/*.sh
./scripts/build.sh
```

或：

```bash
cmake -S . -B build
cmake --build build
```

产物目录：`build/firmware/`

| 文件 | 说明 |
|------|------|
| `stm32f103xb_bl_crc.bin` / `.hex` | Bootloader |
| `stm32f103xb_if_crc.bin` / `.hex` | 接口固件（含双模式） |

### 4. 烧录到 STM32

1. 用 ST-Link / ISP 将 bootloader 写到 `0x08000000`
2. 再写入 interface；之后可用 DAPLink U 盘方式升级 interface

### 5. 使用

**调试其它 MCU（DAPLink）**  
PB1 悬空 → 接 SWDIO / SWCLK / nRESET / GND → USB 插电脑。

**烧录 ESP32**  
PB1 接 GND 后上电 → 按上表接好 UART/EN/IO0 →：

```bash
esptool.py --port <串口> write_flash 0x1000 app.bin
```

串口名因系统而异（如 `/dev/ttyACM0`、`COM3`）。

---

## CMake 选项

```bash
# 只编接口固件
cmake -S . -B build -DDAPLINK_BUILD_BOOTLOADER=OFF

# 指定板级 interface 工程（拖拽算法绑定具体目标）
cmake -S . -B build -DDAPLINK_INTERFACE_PROJECT=stm32f103xb_stm32f103rb_if

# 使用 Make 代替 Ninja
cmake -S . -B build -DDAPLINK_CMAKE_GENERATOR=make
```

清理生成物：

```bash
cmake --build build --target daplink-clean
```

---

## 仓库结构

```
.
├── CMakeLists.txt                 # 顶层构建编排
├── README.md
├── overlay/stm32f103xb/           # 双模式补丁（构建前写入 DAPLink HIC）
│   ├── esp32_autoload.c / .h
│   ├── IO_Config.h
│   ├── gpio.c
│   └── uart.c
├── cmake/                         # venv、overlay、收集固件
├── scripts/                       # build / setup_venv / with_daplink_env
└── third_party/DAPLink/           # 官方源码（submodule，develop）
```

自定义逻辑集中在 `overlay/`，不直接改上游历史；每次构建会自动拷贝进 DAPLink 的 `stm32f103xb` HIC 目录。

---

## 说明

- 上游构建链：`progen` + `cmake_gcc_arm` + `arm-none-eabi-gcc`（建议 GCC 12+）
- Python 环境需 `setuptools<81`（`progen` 依赖 `pkg_resources`），脚本已处理
- DAPLink 协议：Apache-2.0  
- 本仓库：https://github.com/SFNFIH/stm32f103-daplink-cmake
