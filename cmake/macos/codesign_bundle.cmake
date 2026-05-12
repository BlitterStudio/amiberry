if(NOT DEFINED APP_BUNDLE OR APP_BUNDLE STREQUAL "")
    message(FATAL_ERROR "APP_BUNDLE is required")
endif()

if(NOT EXISTS "${APP_BUNDLE}/Contents")
    message(FATAL_ERROR "APP_BUNDLE does not look like a macOS app bundle: ${APP_BUNDLE}")
endif()

if(NOT DEFINED CODESIGN_EXECUTABLE OR CODESIGN_EXECUTABLE STREQUAL "")
    find_program(CODESIGN_EXECUTABLE codesign)
    if(NOT CODESIGN_EXECUTABLE)
        message(FATAL_ERROR "codesign executable not found")
    endif()
endif()

if(NOT DEFINED AMIBERRY_CODESIGN_IDENTITY OR AMIBERRY_CODESIGN_IDENTITY STREQUAL "")
    set(AMIBERRY_CODESIGN_IDENTITY "-")
endif()

set(_amiberry_codesign_base_args
    --force
    --sign "${AMIBERRY_CODESIGN_IDENTITY}"
)

if(DEFINED AMIBERRY_CODESIGN_OPTIONS AND NOT AMIBERRY_CODESIGN_OPTIONS STREQUAL "")
    list(APPEND _amiberry_codesign_base_args --options "${AMIBERRY_CODESIGN_OPTIONS}")
endif()

function(_amiberry_codesign _path _description)
    execute_process(
        COMMAND "${CODESIGN_EXECUTABLE}" ${_amiberry_codesign_base_args} "${_path}"
        RESULT_VARIABLE _amiberry_codesign_result
        OUTPUT_VARIABLE _amiberry_codesign_stdout
        ERROR_VARIABLE _amiberry_codesign_stderr
    )

    if(NOT _amiberry_codesign_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to codesign ${_description} '${_path}':\n"
            "${_amiberry_codesign_stdout}${_amiberry_codesign_stderr}"
        )
    endif()
endfunction()

set(_amiberry_nested_code)
foreach(_amiberry_nested_dir IN ITEMS
        "${APP_BUNDLE}/Contents/Frameworks"
        "${APP_BUNDLE}/Contents/Resources/plugins")
    if(EXISTS "${_amiberry_nested_dir}")
        file(GLOB_RECURSE _amiberry_dir_code
            LIST_DIRECTORIES false
            "${_amiberry_nested_dir}/*.dylib"
            "${_amiberry_nested_dir}/*.so"
        )
        list(APPEND _amiberry_nested_code ${_amiberry_dir_code})
    endif()
endforeach()

if(_amiberry_nested_code)
    list(REMOVE_DUPLICATES _amiberry_nested_code)
    list(SORT _amiberry_nested_code)
endif()

foreach(_amiberry_code IN LISTS _amiberry_nested_code)
    if(NOT IS_SYMLINK "${_amiberry_code}")
        _amiberry_codesign("${_amiberry_code}" "nested code")
    endif()
endforeach()

set(_amiberry_bundle_codesign_args ${_amiberry_codesign_base_args})
if(DEFINED AMIBERRY_ENTITLEMENTS AND NOT AMIBERRY_ENTITLEMENTS STREQUAL "")
    if(NOT EXISTS "${AMIBERRY_ENTITLEMENTS}")
        message(FATAL_ERROR "AMIBERRY_ENTITLEMENTS does not exist: ${AMIBERRY_ENTITLEMENTS}")
    endif()
    list(APPEND _amiberry_bundle_codesign_args --entitlements "${AMIBERRY_ENTITLEMENTS}")
endif()

execute_process(
    COMMAND "${CODESIGN_EXECUTABLE}" ${_amiberry_bundle_codesign_args} "${APP_BUNDLE}"
    RESULT_VARIABLE _amiberry_bundle_codesign_result
    OUTPUT_VARIABLE _amiberry_bundle_codesign_stdout
    ERROR_VARIABLE _amiberry_bundle_codesign_stderr
)

if(NOT _amiberry_bundle_codesign_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to codesign app bundle '${APP_BUNDLE}':\n"
        "${_amiberry_bundle_codesign_stdout}${_amiberry_bundle_codesign_stderr}"
    )
endif()

execute_process(
    COMMAND "${CODESIGN_EXECUTABLE}" --verify --deep --strict "${APP_BUNDLE}"
    RESULT_VARIABLE _amiberry_verify_result
    OUTPUT_VARIABLE _amiberry_verify_stdout
    ERROR_VARIABLE _amiberry_verify_stderr
)

if(NOT _amiberry_verify_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to verify app bundle signature '${APP_BUNDLE}':\n"
        "${_amiberry_verify_stdout}${_amiberry_verify_stderr}"
    )
endif()
