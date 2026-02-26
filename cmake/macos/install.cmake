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
        COMMAND dylibbundler -od -b -x $<TARGET_FILE:${PROJECT_NAME}> -d $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/ -p @executable_path/../Frameworks/)

# Codesign all bundled code with the same team identity
# This removes the need for com.apple.security.cs.disable-library-validation
set(MACOS_CODESIGN_IDENTITY "" CACHE STRING "Code signing identity for macOS (e.g. 'Developer ID Application: Name (TEAMID)')")
if(MACOS_CODESIGN_IDENTITY)
    # Sign plugins
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND codesign --force --sign "${MACOS_CODESIGN_IDENTITY}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/$<TARGET_FILE_NAME:capsimage>
        COMMENT "Codesigning capsimage plugin")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND codesign --force --sign "${MACOS_CODESIGN_IDENTITY}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/$<TARGET_FILE_NAME:floppybridge>
        COMMENT "Codesigning floppybridge plugin")
    # Sign all bundled frameworks
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND bash -c "find '$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/' -name '*.dylib' -exec codesign --force --sign '${MACOS_CODESIGN_IDENTITY}' {} \\;"
        COMMENT "Codesigning bundled frameworks")
endif()

if (NOT "${CMAKE_GENERATOR}" MATCHES "Xcode")
    install(FILES $<TARGET_FILE:capsimage>
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/)
    install(FILES $<TARGET_FILE:floppybridge>
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/plugins/)

    # This one contains the gamecontrollersdb.txt file
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
    # This one contains the data files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
    # This one contains the AROS kickstart files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
    # This one contains the whdboot files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
endif ()