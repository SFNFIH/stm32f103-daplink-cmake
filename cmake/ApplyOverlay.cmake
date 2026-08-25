# Copy dual-mode overlay sources into the DAPLink STM32F103 HIC tree.
if(NOT DAPLINK_ROOT OR NOT OVERLAY_DIR)
    message(FATAL_ERROR "ApplyOverlay.cmake requires DAPLINK_ROOT and OVERLAY_DIR")
endif()

set(_dst "${DAPLINK_ROOT}/source/hic_hal/stm32/stm32f103xb")
if(NOT EXISTS "${_dst}")
    message(FATAL_ERROR "DAPLink HIC path missing: ${_dst}")
endif()

file(GLOB _overlay_files "${OVERLAY_DIR}/*")
foreach(f IN LISTS _overlay_files)
    get_filename_component(_name "${f}" NAME)
    file(COPY "${f}" DESTINATION "${_dst}")
    message(STATUS "Overlay applied: ${_name} -> ${_dst}")
endforeach()

# Drop obsolete soft-UART sources if a previous build left them behind.
foreach(_stale IN ITEMS esp32_soft_uart.c esp32_soft_uart.h)
    if(EXISTS "${_dst}/${_stale}")
        file(REMOVE "${_dst}/${_stale}")
        message(STATUS "Removed stale overlay file: ${_stale}")
    endif()
endforeach()
