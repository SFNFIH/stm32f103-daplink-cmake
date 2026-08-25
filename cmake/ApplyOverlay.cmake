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
