# Remove macOS Finder/resource metadata sidecars from the staged CPack tree
# before libarchive writes package payloads. Clearable extended attributes are
# stripped too; the DEB generator is also forced to gnutar so sticky platform
# attributes are not emitted as PAX headers.
if(NOT DEFINED CPACK_TEMPORARY_INSTALL_DIRECTORY
        OR NOT EXISTS "${CPACK_TEMPORARY_INSTALL_DIRECTORY}")
    message(STATUS "Amiberry package metadata scrub skipped: no CPack staging directory")
    return()
endif()

set(_amiberry_stage "${CPACK_TEMPORARY_INSTALL_DIRECTORY}")
set(_amiberry_apple_metadata_names
    ".DS_Store"
    "__MACOSX"
    ".AppleDouble"
    ".LSOverride"
    ".Spotlight-V100"
    ".Trashes"
    ".fseventsd"
)

file(GLOB_RECURSE _amiberry_stage_entries
        LIST_DIRECTORIES true
        "${_amiberry_stage}/*")

foreach(_amiberry_entry IN LISTS _amiberry_stage_entries)
    get_filename_component(_amiberry_name "${_amiberry_entry}" NAME)
    list(FIND _amiberry_apple_metadata_names "${_amiberry_name}" _amiberry_metadata_index)

    if(NOT _amiberry_metadata_index EQUAL -1
            OR _amiberry_name MATCHES "^\\._")
        if(IS_DIRECTORY "${_amiberry_entry}")
            file(REMOVE_RECURSE "${_amiberry_entry}")
        else()
            file(REMOVE "${_amiberry_entry}")
        endif()
    endif()
endforeach()

if(APPLE)
    find_program(_amiberry_xattr xattr)
    if(_amiberry_xattr)
        execute_process(
                COMMAND "${_amiberry_xattr}" -cr "${_amiberry_stage}"
                RESULT_VARIABLE _amiberry_xattr_result
                ERROR_VARIABLE _amiberry_xattr_error
                OUTPUT_QUIET)
        if(NOT _amiberry_xattr_result EQUAL 0)
            message(WARNING
                    "Failed to strip macOS extended attributes from package staging tree: "
                    "${_amiberry_xattr_error}")
        endif()
    endif()
endif()
