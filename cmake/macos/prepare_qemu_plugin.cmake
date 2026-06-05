if(NOT DEFINED QEMU_UAE_PLUGIN_INPUT OR QEMU_UAE_PLUGIN_INPUT STREQUAL "")
    message(FATAL_ERROR "QEMU_UAE_PLUGIN_INPUT is required")
endif()

if(NOT DEFINED QEMU_UAE_PLUGIN_OUTPUT OR QEMU_UAE_PLUGIN_OUTPUT STREQUAL "")
    message(FATAL_ERROR "QEMU_UAE_PLUGIN_OUTPUT is required")
endif()

get_filename_component(_qemu_uae_output_dir "${QEMU_UAE_PLUGIN_OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_qemu_uae_output_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QEMU_UAE_PLUGIN_INPUT}"
        "${QEMU_UAE_PLUGIN_OUTPUT}"
    RESULT_VARIABLE _qemu_uae_copy_result
    OUTPUT_VARIABLE _qemu_uae_copy_out
    ERROR_VARIABLE _qemu_uae_copy_err
)

if(NOT _qemu_uae_copy_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to copy QEMU-UAE plugin '${QEMU_UAE_PLUGIN_INPUT}' to "
        "'${QEMU_UAE_PLUGIN_OUTPUT}':\n"
        "${_qemu_uae_copy_out}${_qemu_uae_copy_err}"
    )
endif()

if(DEFINED QEMU_UAE_PLUGIN_ARCH AND NOT QEMU_UAE_PLUGIN_ARCH STREQUAL "")
    execute_process(
        COMMAND lipo -info "${QEMU_UAE_PLUGIN_OUTPUT}"
        RESULT_VARIABLE _qemu_uae_lipo_info_result
        OUTPUT_VARIABLE _qemu_uae_lipo_info_out
        ERROR_VARIABLE _qemu_uae_lipo_info_err
    )

    if(NOT _qemu_uae_lipo_info_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to inspect QEMU-UAE plugin architecture '${QEMU_UAE_PLUGIN_OUTPUT}':\n"
            "${_qemu_uae_lipo_info_out}${_qemu_uae_lipo_info_err}"
        )
    endif()

    if(_qemu_uae_lipo_info_out MATCHES "Architectures in the fat file")
        if(NOT _qemu_uae_lipo_info_out MATCHES "(^| )${QEMU_UAE_PLUGIN_ARCH}( |$)")
            message(FATAL_ERROR
                "QEMU-UAE plugin '${QEMU_UAE_PLUGIN_OUTPUT}' does not contain "
                "architecture '${QEMU_UAE_PLUGIN_ARCH}': ${_qemu_uae_lipo_info_out}"
            )
        endif()

        set(_qemu_uae_thin_output "${QEMU_UAE_PLUGIN_OUTPUT}.thin")
        execute_process(
            COMMAND lipo "${QEMU_UAE_PLUGIN_OUTPUT}"
                -thin "${QEMU_UAE_PLUGIN_ARCH}"
                -output "${_qemu_uae_thin_output}"
            RESULT_VARIABLE _qemu_uae_lipo_thin_result
            OUTPUT_VARIABLE _qemu_uae_lipo_thin_out
            ERROR_VARIABLE _qemu_uae_lipo_thin_err
        )

        if(NOT _qemu_uae_lipo_thin_result EQUAL 0)
            message(FATAL_ERROR
                "Failed to thin QEMU-UAE plugin '${QEMU_UAE_PLUGIN_OUTPUT}' to "
                "'${QEMU_UAE_PLUGIN_ARCH}':\n"
                "${_qemu_uae_lipo_thin_out}${_qemu_uae_lipo_thin_err}"
            )
        endif()

        file(RENAME "${_qemu_uae_thin_output}" "${QEMU_UAE_PLUGIN_OUTPUT}")
    elseif(NOT _qemu_uae_lipo_info_out MATCHES "architecture: ${QEMU_UAE_PLUGIN_ARCH}")
        message(FATAL_ERROR
            "QEMU-UAE plugin '${QEMU_UAE_PLUGIN_OUTPUT}' has unexpected "
            "architecture for '${QEMU_UAE_PLUGIN_ARCH}': ${_qemu_uae_lipo_info_out}"
        )
    endif()
endif()
