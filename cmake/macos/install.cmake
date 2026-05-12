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

# Codesign all bundled code after dylibbundler/install_name_tool have finished
# mutating load commands.  Local builds use ad-hoc signing unless a real
# Developer ID identity is supplied.
set(MACOS_CODESIGN_IDENTITY "" CACHE STRING "Code signing identity for macOS ('-' for ad-hoc, or 'Developer ID Application: Name (TEAMID)')")
set(AMIBERRY_CODESIGN_IDENTITY "${MACOS_CODESIGN_IDENTITY}")
if(AMIBERRY_CODESIGN_IDENTITY STREQUAL "")
    set(AMIBERRY_CODESIGN_IDENTITY "-")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DAPP_BUNDLE=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
        -DAMIBERRY_CODESIGN_IDENTITY=${AMIBERRY_CODESIGN_IDENTITY}
        -DAMIBERRY_ENTITLEMENTS=${AMIBERRY_ENTITLEMENTS}
        -P ${CMAKE_SOURCE_DIR}/cmake/macos/codesign_bundle.cmake
    COMMENT "Codesigning Amiberry app bundle"
    VERBATIM)

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
