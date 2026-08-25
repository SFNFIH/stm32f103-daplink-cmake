# Ensure DAPLink Python venv exists with progen + pinned setuptools.
function(daplink_ensure_venv)
    set(options)
    set(oneValueArgs ROOT VENV PYTHON)
    set(multiValueArgs)
    cmake_parse_arguments(DEV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT EXISTS "${DEV_ROOT}/requirements.txt")
        message(FATAL_ERROR "Missing ${DEV_ROOT}/requirements.txt")
    endif()

    if(NOT EXISTS "${DEV_VENV}/bin/python")
        message(STATUS "Creating DAPLink venv at ${DEV_VENV}")
        execute_process(
            COMMAND "${DEV_PYTHON}" -m venv "${DEV_VENV}"
            RESULT_VARIABLE _rc
        )
        if(NOT _rc EQUAL 0)
            message(FATAL_ERROR "Failed to create venv (exit ${_rc})")
        endif()
    endif()

    set(_venv_python "${DEV_VENV}/bin/python")
    set(_req_stamp "${DEV_VENV}/.requirements.stamp")
    file(TIMESTAMP "${DEV_ROOT}/requirements.txt" _req_ts)

    set(_need_install TRUE)
    if(EXISTS "${_req_stamp}")
        file(READ "${_req_stamp}" _prev)
        string(STRIP "${_prev}" _prev)
        if(_prev STREQUAL "${_req_ts}")
            if(EXISTS "${DEV_VENV}/bin/progen")
                set(_need_install FALSE)
            endif()
        endif()
    endif()

    if(_need_install)
        message(STATUS "Installing DAPLink Python requirements (progen, etc.)")
        # setuptools>=81 removed pkg_resources; progen still imports it.
        execute_process(
            COMMAND "${_venv_python}" -m pip install -U "pip" "setuptools>=70,<81" "wheel"
            RESULT_VARIABLE _rc
        )
        if(NOT _rc EQUAL 0)
            message(FATAL_ERROR "pip bootstrap failed (exit ${_rc})")
        endif()
        execute_process(
            COMMAND "${_venv_python}" -m pip install -r "${DEV_ROOT}/requirements.txt"
            RESULT_VARIABLE _rc
        )
        if(NOT _rc EQUAL 0)
            message(FATAL_ERROR "pip install -r requirements.txt failed (exit ${_rc})")
        endif()
        file(WRITE "${_req_stamp}" "${_req_ts}\n")
    else()
        message(STATUS "DAPLink venv already prepared")
    endif()
endfunction()
