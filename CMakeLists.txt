cmake_minimum_required(VERSION 3.16)

# Host-side orchestration: build official ARMmbed DAPLink for STM32F103xB HIC.
project(stm32f103-daplink LANGUAGES NONE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DaplinkStm32)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
set(DAPLINK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/DAPLink"
    CACHE PATH "Path to ARMmbed DAPLink sources")

set(DAPLINK_BOOTLOADER_PROJECT "stm32f103xb_bl"
    CACHE STRING "DAPLink bootloader progen project name")

set(DAPLINK_INTERFACE_PROJECT "stm32f103xb_if"
    CACHE STRING "DAPLink interface progen project (default: generic all-family probe)")

set_property(CACHE DAPLINK_INTERFACE_PROJECT PROPERTY STRINGS
    stm32f103xb_if
    stm32f103xb_stm32f103rb_if
    stm32f103xb_stm32f072rb_if
    stm32f103xb_stm32f401re_if
    stm32f103xb_stm32f411re_if
    stm32f103xb_stm32f429zi_if
    stm32f103xb_stm32f746zg_if
    stm32f103xb_stm32l476rg_if
    stm32f103xb_stm32f334r8_if
    stm32f103xb_stm32f207zg_if
    stm32f103xb_ublox_evk_odin_w2_if
)

option(DAPLINK_BUILD_BOOTLOADER "Build STM32F103 DAPLink bootloader" ON)
option(DAPLINK_BUILD_INTERFACE  "Build STM32F103 DAPLink interface firmware" ON)
option(DAPLINK_SETUP_VENV       "Create/update Python venv under DAPLink on configure" ON)

set(DAPLINK_TOOL "cmake_gcc_arm" CACHE STRING "progen toolchain id")
set(DAPLINK_CMAKE_GENERATOR "ninja" CACHE STRING "CMake generator passed to progen (ninja|make)")

# ---------------------------------------------------------------------------
# Tool checks
# ---------------------------------------------------------------------------
find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_NONE_EABI_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(NINJA_PROGRAM ninja)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(DAPLINK_CMAKE_GENERATOR STREQUAL "ninja" AND NOT NINJA_PROGRAM)
    message(FATAL_ERROR "ninja not found; install ninja-build or set -DDAPLINK_CMAKE_GENERATOR=make")
endif()

if(NOT EXISTS "${DAPLINK_ROOT}/projects.yaml")
    message(FATAL_ERROR
        "DAPLink sources not found at:\n  ${DAPLINK_ROOT}\n"
        "Clone https://github.com/ARMmbed/DAPLink (develop recommended) into third_party/DAPLink")
endif()

message(STATUS "arm-none-eabi-gcc: ${ARM_NONE_EABI_GCC}")
execute_process(
    COMMAND "${ARM_NONE_EABI_GCC}" -dumpversion
    OUTPUT_VARIABLE ARM_GCC_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "ARM GCC version: ${ARM_GCC_VERSION}")
message(STATUS "DAPLink root: ${DAPLINK_ROOT}")
message(STATUS "Interface project: ${DAPLINK_INTERFACE_PROJECT}")

# ---------------------------------------------------------------------------
# Python venv + progen
# ---------------------------------------------------------------------------
set(DAPLINK_VENV "${DAPLINK_ROOT}/venv")
set(DAPLINK_VENV_PYTHON "${DAPLINK_VENV}/bin/python")
set(DAPLINK_PROGEN "${DAPLINK_VENV}/bin/progen")

if(DAPLINK_SETUP_VENV)
    daplink_ensure_venv(
        ROOT "${DAPLINK_ROOT}"
        VENV "${DAPLINK_VENV}"
        PYTHON "${Python3_EXECUTABLE}"
    )
endif()

if(NOT EXISTS "${DAPLINK_PROGEN}")
    message(FATAL_ERROR
        "progen not found at ${DAPLINK_PROGEN}. "
        "Re-configure with -DDAPLINK_SETUP_VENV=ON or run scripts/setup_venv.sh")
endif()

# ---------------------------------------------------------------------------
# Project list
# ---------------------------------------------------------------------------
set(FIRMWARE_OUT_DIR "${CMAKE_BINARY_DIR}/firmware")
file(MAKE_DIRECTORY "${FIRMWARE_OUT_DIR}")

set(_daplink_projects "")
if(DAPLINK_BUILD_BOOTLOADER)
    list(APPEND _daplink_projects "${DAPLINK_BOOTLOADER_PROJECT}")
endif()
if(DAPLINK_BUILD_INTERFACE)
    list(APPEND _daplink_projects "${DAPLINK_INTERFACE_PROJECT}")
endif()
if(_daplink_projects STREQUAL "")
    message(FATAL_ERROR "Enable DAPLINK_BUILD_BOOTLOADER and/or DAPLINK_BUILD_INTERFACE")
endif()

set(_progen_p_args "")
foreach(_p IN LISTS _daplink_projects)
    list(APPEND _progen_p_args -p "${_p}")
endforeach()

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
# Wrap progen so DAPLink pre/post scripts (`#!/usr/bin/env python`) use the venv.
set(_WITH_ENV "${CMAKE_CURRENT_SOURCE_DIR}/scripts/with_daplink_env.sh")

# Pass projects as a comma-separated string for CollectFirmware.cmake (script mode).
string(REPLACE ";" "," _daplink_projects_csv "${_daplink_projects}")

add_custom_target(daplink-generate
    COMMAND "${_WITH_ENV}"
            "${DAPLINK_VENV_PYTHON}" "${DAPLINK_PROGEN}"
            generate
            -t "${DAPLINK_TOOL}"
            -o "generator=${DAPLINK_CMAKE_GENERATOR}"
            ${_progen_p_args}
    WORKING_DIRECTORY "${DAPLINK_ROOT}"
    COMMENT "Generate DAPLink CMake/${DAPLINK_CMAKE_GENERATOR} projects"
    VERBATIM
)

add_custom_target(daplink-firmware ALL
    COMMAND "${_WITH_ENV}"
            "${DAPLINK_VENV_PYTHON}" "${DAPLINK_PROGEN}"
            generate
            -t "${DAPLINK_TOOL}"
            -o "generator=${DAPLINK_CMAKE_GENERATOR}"
            ${_progen_p_args}
            -b
    COMMAND "${CMAKE_COMMAND}"
            "-DDAPLINK_ROOT=${DAPLINK_ROOT}"
            "-DFIRMWARE_OUT_DIR=${FIRMWARE_OUT_DIR}"
            "-DDAPLINK_PROJECTS=${_daplink_projects_csv}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CollectFirmware.cmake"
    WORKING_DIRECTORY "${DAPLINK_ROOT}"
    COMMENT "Build STM32F103 DAPLink firmware and collect binaries"
    VERBATIM
    USES_TERMINAL
)

add_custom_target(daplink-clean
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${DAPLINK_ROOT}/projectfiles/${DAPLINK_TOOL}"
            "${FIRMWARE_OUT_DIR}"
    COMMENT "Remove generated DAPLink projectfiles and collected firmware"
    VERBATIM
)

add_custom_target(firmware DEPENDS daplink-firmware)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_info.txt.in"
    "${CMAKE_BINARY_DIR}/build_info.txt"
    @ONLY
)

message(STATUS "-----------------------------------------------------------")
message(STATUS "Configure done. Build with:")
message(STATUS "  cmake --build ${CMAKE_BINARY_DIR}")
message(STATUS "Firmware collected under:")
message(STATUS "  ${FIRMWARE_OUT_DIR}")
message(STATUS "-----------------------------------------------------------")
