if(NOT DEFINED APP_BINARY)
    message(FATAL_ERROR "APP_BINARY is required")
endif()

set(_frameworks_rpath "@executable_path/../Frameworks/")

execute_process(
    COMMAND install_name_tool -delete_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _delete_result_1
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND install_name_tool -delete_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _delete_result_2
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND install_name_tool -add_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _add_result
    OUTPUT_VARIABLE _add_out
    ERROR_VARIABLE _add_err
)

if(NOT _add_result EQUAL 0)
    message(FATAL_ERROR "Failed to add Frameworks RPATH for ${APP_BINARY}: ${_add_out}${_add_err}")
endif()
