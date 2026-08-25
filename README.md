# STM32F103 DAPLink（CMake）

基于官方开源固件 [ARMmbed/DAPLink](https://github.com/ARMmbed/DAPLink) 的 **STM32F103xB** 接口芯片（HIC）工程。  
顶层 CMake 负责：准备 Python/progen 环境 → 生成官方工程 → 用 `arm-none-eabi-gcc` 编译 → 收集 `.bin/.hex`。

默认构建：

| 产物 | progen 工程 | 说明 |
|------|-------------|------|
| Bootloader | `stm32f103xb_bl` | 48 KB，位于 Flash 前部 |
| Interface | `stm32f103xb_if` | 通用探针（含多 family flash 算法） |

## 依赖

- CMake ≥ 3.16
- Python 3 + `venv`
- `arm-none-eabi-gcc`（建议 12.2+ / 13.x；本仓库使用 DAPLink **develop** 以兼容新 GCC）
- `ninja`（或改用 `-DDAPLINK_CMAKE_GENERATOR=make`）

Ubuntu 示例：

```bash
sudo apt install cmake ninja-build python3-venv python-is-python3 \
  gcc-arm-none-eabi binutils-arm-none-eabi
```

首次克隆后拉取 DAPLink 子模块：

```bash
git submodule update --init --recursive
```

## 快速构建

```bash
chmod +x scripts/*.sh
./scripts/build.sh
```

或手动：

```bash
cmake -S . -B build
cmake --build build
```

固件输出目录：`build/firmware/`

常见文件名：

- `stm32f103xb_bl_crc.bin` / `.hex`
- `stm32f103xb_if_crc.bin` / `.hex`

## 常用 CMake 选项

```bash
# 只编接口固件
cmake -S . -B build -DDAPLINK_BUILD_BOOTLOADER=OFF

# 指定目标板接口固件（拖拽烧录算法绑定具体 MCU）
cmake -S . -B build -DDAPLINK_INTERFACE_PROJECT=stm32f103xb_stm32f103rb_if

# 使用 Unix Makefiles 而非 Ninja
cmake -S . -B build -DDAPLINK_CMAKE_GENERATOR=make
```

可选接口工程还包括：`stm32f103xb_stm32f401re_if`、`stm32f103xb_stm32f411re_if`、`stm32f103xb_stm32l476rg_if` 等（见 `third_party/DAPLink/projects.yaml`）。

清理生成物：

```bash
cmake --build build --target daplink-clean
```

## 默认引脚（官方 stm32f103xb HIC）

定义见 `third_party/DAPLink/source/hic_hal/stm32/stm32f103xb/IO_Config.h`：

| 功能 | 引脚 |
|------|------|
| SWCLK / TCK | PB13 |
| SWDIO 输出 / TMS | PB14 |
| SWDIO 输入 | PB12 |
| SWO / TDO | PA10 |
| nRESET | PB0 |
| UART RX | PA2 |
| UART TX | PA3 |
| 连接 LED | PB6 |
| 状态 LED（HID/CDC/MSC） | PA9 |
| USB 连接控制 | PA15 |

芯片要求：**STM32F103CB**（128 KB Flash / 20 KB RAM）或同系列兼容封装。Flash 布局：Bootloader 48 KB + Interface。

## 烧录提示

1. 首次可用 ST-Link / 串口 ISP 等把 **bootloader** 烧到 `0x08000000`。
2. 之后可通过 DAPLink 的 U 盘升级方式更新 **interface** 固件（或继续用外部调试器整片烧录）。
3. 接目标板 SWD：`SWDIO/SWCLK/GND/(3V3)/nRESET`，USB 连 PC 后应枚举为 CMSIS-DAP + 虚拟串口 + 磁盘。

## 目录结构

```
.
├── CMakeLists.txt              # 顶层编排
├── cmake/                      # venv、收集固件等模块
├── scripts/build.sh            # 一键构建
├── scripts/setup_venv.sh       # 仅准备 Python 环境
└── third_party/DAPLink/        # 官方 DAPLink 源码（develop）
```

## 说明

- DAPLink 官方构建系统是 **progen + CMake/Make**；本工程不改写其源码结构，只做可复现的 STM32F103 封装。
- Python 依赖里需要 `setuptools<81`（`progen` 仍使用 `pkg_resources`），CMake/`setup_venv.sh` 已自动处理。
- 上游仓库：https://github.com/ARMmbed/DAPLink ，协议 Apache-2.0。
