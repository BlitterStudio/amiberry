add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/controllers
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/controllers)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/data)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/roms
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/roms)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/whdboot
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/whdboot)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:capsimage>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/$<TARGET_FILE_NAME:capsimage>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:floppybridge>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/$<TARGET_FILE_NAME:floppybridge>)

set(_amiberry_dylibbundler_search_args
        -s /usr/local/lib
        -s /usr/local/opt/glib/lib
        -s /usr/local/opt/gettext/lib
        -s /usr/local/opt/pcre2/lib
        -s /opt/homebrew/lib
        -s /opt/homebrew/opt/glib/lib
        -s /opt/homebrew/opt/gettext/lib
        -s /opt/homebrew/opt/pcre2/lib)

if(QEMU_UAE_PLUGIN)
    get_filename_component(_amiberry_qemu_uae_plugin_name "${QEMU_UAE_PLUGIN}" NAME)
    set(_amiberry_qemu_uae_plugin_output
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/${_amiberry_qemu_uae_plugin_name}")

    set(_amiberry_qemu_uae_plugin_arches ${CMAKE_OSX_ARCHITECTURES})
    if(NOT _amiberry_qemu_uae_plugin_arches)
        set(_amiberry_qemu_uae_plugin_arches "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    list(LENGTH _amiberry_qemu_uae_plugin_arches _amiberry_qemu_uae_plugin_arch_count)
    if(_amiberry_qemu_uae_plugin_arch_count EQUAL 1)
        list(GET _amiberry_qemu_uae_plugin_arches 0 _amiberry_qemu_uae_plugin_arch)
        if(_amiberry_qemu_uae_plugin_arch STREQUAL "aarch64")
            set(_amiberry_qemu_uae_plugin_arch "arm64")
        endif()
    else()
        set(_amiberry_qemu_uae_plugin_arch "")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-DQEMU_UAE_PLUGIN_INPUT=${QEMU_UAE_PLUGIN}"
                "-DQEMU_UAE_PLUGIN_OUTPUT=${_amiberry_qemu_uae_plugin_output}"
                "-DQEMU_UAE_PLUGIN_ARCH=${_amiberry_qemu_uae_plugin_arch}"
                -P "${CMAKE_SOURCE_DIR}/cmake/macos/prepare_qemu_plugin.cmake"
            COMMENT "Preparing QEMU-UAE PPC plugin"
            VERBATIM)
endif()

# Gather all dependencies with dylibbundler
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND dylibbundler -od -b -x $<TARGET_FILE:${PROJECT_NAME}> -d $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/ -p @executable_path/../Frameworks/ ${_amiberry_dylibbundler_search_args})

if(QEMU_UAE_PLUGIN)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND dylibbundler -of -cd -b -x "${_amiberry_qemu_uae_plugin_output}" -d $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/ -p @executable_path/../Frameworks/ ${_amiberry_dylibbundler_search_args}
            COMMENT "Bundling QEMU-UAE PPC plugin dependencies"
            VERBATIM)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DAPP_BINARY=$<TARGET_FILE:${PROJECT_NAME}> -P ${CMAKE_SOURCE_DIR}/cmake/macos/dedupe_frameworks_rpath.cmake)

# Codesign local/intermediate bundles after dylibbundler and install_name_tool
# have finished mutating load commands. CI re-signs the final universal bundle
# with the Developer ID identity before notarization.
option(MACOS_CODESIGN_BUNDLE
    "Codesign local/intermediate macOS app bundles after copying dependencies"
    ON)
string(CONCAT _amiberry_codesign_identity_help
    "Code signing identity for local/intermediate macOS bundles "
    "(empty for non-App Store ad-hoc, '-' for explicit ad-hoc, "
    "or 'Developer ID Application: Name (TEAMID)')")
set(MACOS_CODESIGN_IDENTITY "" CACHE STRING "${_amiberry_codesign_identity_help}")
string(CONCAT _amiberry_codesign_options_help
    "Additional codesign --options value for local/intermediate macOS bundles (for example: runtime,hard)")
set(MACOS_CODESIGN_OPTIONS "" CACHE STRING "${_amiberry_codesign_options_help}")
if(MACOS_CODESIGN_BUNDLE)
    find_program(CODESIGN_EXECUTABLE codesign)
    if(NOT CODESIGN_EXECUTABLE)
        message(FATAL_ERROR
            "codesign executable not found; "
            "set MACOS_CODESIGN_BUNDLE=OFF to skip macOS bundle signing"
        )
    endif()

    if(MACOS_APP_STORE AND MACOS_CODESIGN_IDENTITY STREQUAL "")
        message(FATAL_ERROR
            "MACOS_APP_STORE builds require MACOS_CODESIGN_IDENTITY; "
            "ad-hoc signing must be requested explicitly with '-'"
        )
    endif()

    set(_amiberry_codesign_identity "${MACOS_CODESIGN_IDENTITY}")
    if(_amiberry_codesign_identity STREQUAL "")
        set(_amiberry_codesign_identity "-")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DAPP_BUNDLE=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            "-DCODESIGN_EXECUTABLE=${CODESIGN_EXECUTABLE}"
            "-DAMIBERRY_CODESIGN_IDENTITY=${_amiberry_codesign_identity}"
            "-DAMIBERRY_CODESIGN_OPTIONS=${MACOS_CODESIGN_OPTIONS}"
            "-DAMIBERRY_ENTITLEMENTS=${AMIBERRY_ENTITLEMENTS}"
            -P "${CMAKE_SOURCE_DIR}/cmake/macos/codesign_bundle.cmake"
        COMMENT "Codesigning Amiberry app bundle"
        VERBATIM)
endif()

if (NOT "${CMAKE_GENERATOR}" MATCHES "Xcode")
    install(FILES $<TARGET_FILE:capsimage>
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/)
    install(FILES $<TARGET_FILE:floppybridge>
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/)
    if(QEMU_UAE_PLUGIN)
        install(FILES "${QEMU_UAE_PLUGIN}"
                DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/)
    endif()

    # This one contains the gamecontrollersdb.txt file
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources
            ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
    # This one contains the data files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources
            ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
    # This one contains the AROS kickstart files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources
            ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
    # This one contains the whdboot files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources
            ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
endif ()
