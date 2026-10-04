if(NOT DEFINED APP_BINARY)
    message(FATAL_ERROR "APP_BINARY is required")
endif()

# The bundled libraries use @rpath/<library>.
# The application resolves @rpath through this entry.
set(_frameworks_rpath "@executable_path/../Frameworks/")

# dylibbundler may add @rpath/ entries when -p @rpath/ is used.
#
# We don't need those on the executable because we provide the actual
# Frameworks directory as the rpath below:
#
#     @executable_path/../Frameworks
#
# Remove any @rpath/ entries left by dylibbundler.
#
# Run multiple times in case dylibbundler added duplicates.
execute_process(
    COMMAND install_name_tool -delete_rpath "@rpath/" "${APP_BINARY}"
    RESULT_VARIABLE _delete_rpath_result_1
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND install_name_tool -delete_rpath "@rpath/" "${APP_BINARY}"
    RESULT_VARIABLE _delete_rpath_result_2
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND install_name_tool -delete_rpath "@rpath/" "${APP_BINARY}"
    RESULT_VARIABLE _delete_rpath_result_3
    OUTPUT_QUIET
    ERROR_QUIET
)

# Remove any existing Frameworks rpath entries.
#
# This prevents duplicate @executable_path/../Frameworks entries when
# this script is run more than once.
execute_process(
    COMMAND install_name_tool -delete_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _delete_frameworks_result_1
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND install_name_tool -delete_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _delete_frameworks_result_2
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND install_name_tool -delete_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _delete_frameworks_result_3
    OUTPUT_QUIET
    ERROR_QUIET
)

# Add exactly one Frameworks rpath.
execute_process(
    COMMAND install_name_tool
        -add_rpath "${_frameworks_rpath}"
        "${APP_BINARY}"
    RESULT_VARIABLE _add_result
    OUTPUT_VARIABLE _add_out
    ERROR_VARIABLE _add_err
)

if(NOT _add_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to add Frameworks RPATH for ${APP_BINARY}: "
        "${_add_out}${_add_err}")
endif()
