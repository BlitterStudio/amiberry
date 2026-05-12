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

# Gather all dependencies with dylibbundler
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND dylibbundler -od -b -x $<TARGET_FILE:${PROJECT_NAME}> -d $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/ -p @executable_path/../Frameworks/ -s /usr/local/lib)

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
