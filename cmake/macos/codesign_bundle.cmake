if(NOT DEFINED APP_BUNDLE OR APP_BUNDLE STREQUAL "")
    message(FATAL_ERROR "APP_BUNDLE is required")
endif()

if(NOT DEFINED AMIBERRY_CODESIGN_IDENTITY OR AMIBERRY_CODESIGN_IDENTITY STREQUAL "")
    set(AMIBERRY_CODESIGN_IDENTITY "-")
endif()

set(_amiberry_dylibs)
foreach(_amiberry_dir IN ITEMS
        "${APP_BUNDLE}/Contents/Frameworks"
        "${APP_BUNDLE}/Contents/Resources/plugins")
    if(EXISTS "${_amiberry_dir}")
        file(GLOB _amiberry_dir_dylibs
                LIST_DIRECTORIES false
                "${_amiberry_dir}/*.dylib")
        list(APPEND _amiberry_dylibs ${_amiberry_dir_dylibs})
    endif()
endforeach()

foreach(_amiberry_dylib IN LISTS _amiberry_dylibs)
    execute_process(
            COMMAND codesign --force --sign "${AMIBERRY_CODESIGN_IDENTITY}" "${_amiberry_dylib}"
            RESULT_VARIABLE _amiberry_codesign_result
            OUTPUT_VARIABLE _amiberry_codesign_stdout
            ERROR_VARIABLE _amiberry_codesign_stderr)
    if(NOT _amiberry_codesign_result EQUAL 0)
        message(FATAL_ERROR
                "Failed to codesign ${_amiberry_dylib}:\n"
                "${_amiberry_codesign_stdout}${_amiberry_codesign_stderr}")
    endif()
endforeach()

set(_amiberry_app_codesign_args --force --sign "${AMIBERRY_CODESIGN_IDENTITY}")
if(DEFINED AMIBERRY_ENTITLEMENTS AND EXISTS "${AMIBERRY_ENTITLEMENTS}")
    list(APPEND _amiberry_app_codesign_args --entitlements "${AMIBERRY_ENTITLEMENTS}")
endif()
list(APPEND _amiberry_app_codesign_args "${APP_BUNDLE}")

execute_process(
        COMMAND codesign ${_amiberry_app_codesign_args}
        RESULT_VARIABLE _amiberry_codesign_result
        OUTPUT_VARIABLE _amiberry_codesign_stdout
        ERROR_VARIABLE _amiberry_codesign_stderr)
if(NOT _amiberry_codesign_result EQUAL 0)
    message(FATAL_ERROR
            "Failed to codesign ${APP_BUNDLE}:\n"
            "${_amiberry_codesign_stdout}${_amiberry_codesign_stderr}")
endif()
