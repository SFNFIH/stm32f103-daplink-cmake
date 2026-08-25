# Copy CRC'd firmware images from DAPLink projectfiles into FIRMWARE_OUT_DIR.
if(NOT DAPLINK_ROOT OR NOT FIRMWARE_OUT_DIR OR NOT DAPLINK_PROJECTS)
    message(FATAL_ERROR "CollectFirmware.cmake requires DAPLINK_ROOT, FIRMWARE_OUT_DIR, DAPLINK_PROJECTS")
endif()

file(MAKE_DIRECTORY "${FIRMWARE_OUT_DIR}")

# Accept comma- or semicolon-separated project lists from the parent build.
string(REPLACE "," ";" DAPLINK_PROJECTS "${DAPLINK_PROJECTS}")
string(REPLACE " " ";" DAPLINK_PROJECTS "${DAPLINK_PROJECTS}")

foreach(proj IN LISTS DAPLINK_PROJECTS)
    set(_build_dir "${DAPLINK_ROOT}/projectfiles/cmake_gcc_arm/${proj}/build")
    set(_copied FALSE)

    # Prefer post-build CRC artifacts produced by tools/post_build_script_gcc.py
    file(GLOB _crc_bins "${_build_dir}/${proj}_crc*.bin")
    file(GLOB _crc_hexs "${_build_dir}/${proj}_crc*.hex")

    foreach(f IN LISTS _crc_bins _crc_hexs)
        get_filename_component(_name "${f}" NAME)
        file(COPY "${f}" DESTINATION "${FIRMWARE_OUT_DIR}")
        message(STATUS "Collected ${FIRMWARE_OUT_DIR}/${_name}")
        set(_copied TRUE)
    endforeach()

    if(NOT _copied)
        foreach(ext IN ITEMS bin hex elf)
            set(_src "${_build_dir}/${proj}.${ext}")
            if(EXISTS "${_src}")
                file(COPY "${_src}" DESTINATION "${FIRMWARE_OUT_DIR}")
                message(STATUS "Collected ${FIRMWARE_OUT_DIR}/${proj}.${ext}")
                set(_copied TRUE)
            endif()
        endforeach()
    endif()

    if(NOT _copied)
        message(WARNING "No firmware artifacts found for ${proj} under ${_build_dir}")
    endif()
endforeach()

message(STATUS "Firmware output directory: ${FIRMWARE_OUT_DIR}")
