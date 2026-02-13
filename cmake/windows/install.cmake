# Copy dirs to the build directory so we can debug locally
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/controllers
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/controllers)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/data)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/roms
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/roms)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/whdboot
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/whdboot)

# Copy plugin DLLs next to the executable
install(FILES $<TARGET_FILE:capsimage>
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES $<TARGET_FILE:floppybridge>
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# Bundle vcpkg runtime DLLs into the install directory using applocal.ps1
# (same mechanism vcpkg uses at build time to copy DLLs next to the executable).
# The x_vcpkg_install_local_dependencies function gates on "windows|uwp|xbox" platform
# and excludes "mingw-dynamic", so we invoke applocal.ps1 directly.
cmake_policy(SET CMP0087 NEW)

z_vcpkg_set_powershell_path()

install(CODE "
    set(_applocal \"${Z_VCPKG_TOOLCHAIN_DIR}/msbuild/applocal.ps1\")
    set(_target \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>\")
    set(_installed_bin \"${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin\")
    if(EXISTS \"\${_applocal}\")
        message(STATUS \"Installing vcpkg runtime DLLs for ${PROJECT_NAME}...\")
        execute_process(
            COMMAND \"${Z_VCPKG_POWERSHELL_PATH}\" -noprofile -executionpolicy Bypass
                -file \"\${_applocal}\"
                -targetBinary \"\${_target}\"
                -installedDir \"\${_installed_bin}\"
                -OutVariable out
        )
    else()
        message(WARNING \"vcpkg applocal.ps1 not found, runtime DLLs not bundled\")
    endif()
")

# Bundle MinGW runtime DLLs (not managed by vcpkg)
get_filename_component(_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

install(CODE "
    foreach(_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(EXISTS \"${_MINGW_BIN_DIR}/\${_dll}\")
            message(STATUS \"Installing MinGW runtime: \${_dll}\")
            file(INSTALL \"${_MINGW_BIN_DIR}/\${_dll}\"
                 DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
        endif()
    endforeach()
")
