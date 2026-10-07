if(NOT DEFINED APP_BINARY)
    message(FATAL_ERROR "APP_BINARY is required")
endif()

# The Frameworks directory is the only valid runpath that should remain on
# the main executable. dylibbundler may create literal "@rpath/" LC_RPATH
# commands when -p @rpath/ is used, but "@rpath/" is not itself a useful
# filesystem search path and can result in duplicate LC_RPATH commands.
set(_frameworks_rpath "@executable_path/../Frameworks/")

# Derive the application bundle from the main executable so that the same
# cleanup can be applied to bundled Frameworks and plugins.
get_filename_component(_app_contents_dir "${APP_BINARY}" DIRECTORY)
get_filename_component(_app_macos_dir "${_app_contents_dir}" DIRECTORY)
get_filename_component(_app_bundle_dir "${_app_macos_dir}" DIRECTORY)

set(_frameworks_dir "${_app_contents_dir}/../Frameworks")
set(_plugins_dir "${_app_contents_dir}/../Resources/plugins")

# Remove every occurrence of a particular RPATH. Do not assume that there
# are only two or three entries. A dylib can contain an arbitrary number of
# LC_RPATH commands, and duplicate "@rpath/" entries cause recent dyld
# versions to reject the binary at launch.
function(_remove_all_rpaths binary rpath)
    while(1)
        execute_process(
            COMMAND install_name_tool -delete_rpath "${rpath}" "${binary}"
            RESULT_VARIABLE _delete_result
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(NOT _delete_result EQUAL 0)
            break()
        endif()
    endwhile()
endfunction()

# Build a list containing the main executable plus every bundled dylib and
# plugin. The @rpath/ cleanup must not be limited to the executable because
# dylibbundler can rewrite LC_RPATH commands in the libraries it processes.
set(_rpath_cleanup_files
    "${APP_BINARY}"
)

if(EXISTS "${_frameworks_dir}")
    file(GLOB _framework_dylibs "${_frameworks_dir}/*.dylib")
    list(APPEND _rpath_cleanup_files ${_framework_dylibs})
endif()

if(EXISTS "${_plugins_dir}")
    file(GLOB _plugin_dylibs "${_plugins_dir}/*.dylib")
    list(APPEND _rpath_cleanup_files ${_plugin_dylibs})
endif()

# Remove the invalid literal @rpath/ RPATH from every bundled Mach-O file.
# Also remove old Frameworks RPATH entries so the executable can be rebuilt
# with exactly one canonical Frameworks runpath below.
foreach(_binary IN LISTS _rpath_cleanup_files)
    if(NOT EXISTS "${_binary}")
        continue()
    endif()

    _remove_all_rpaths("${_binary}" "@rpath/")
    _remove_all_rpaths("${_binary}" "${_frameworks_rpath}")
endforeach()

# The main executable needs exactly one real Frameworks RPATH so that
# dependencies referenced as @rpath/<library> can be resolved from
# Contents/Frameworks.
execute_process(
    COMMAND install_name_tool -add_rpath "${_frameworks_rpath}" "${APP_BINARY}"
    RESULT_VARIABLE _add_result
    OUTPUT_VARIABLE _add_out
    ERROR_VARIABLE _add_err
)

if(NOT _add_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to add Frameworks RPATH for ${APP_BINARY}: "
        "${_add_out}${_add_err}")
endif()
