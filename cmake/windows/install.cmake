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

# Copy plugin DLLs to plugins/ subdirectory in build dir for local debugging
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:CAPSImage>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:floppybridge>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/)
if(QEMU_UAE_PLUGIN)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${QEMU_UAE_PLUGIN}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/)
    if(QEMU_UAE_PLUGIN_DIR)
        file(GLOB _QEMU_UAE_PLUGIN_SIDECARS CONFIGURE_DEPENDS "${QEMU_UAE_PLUGIN_DIR}/*.dll")
        list(REMOVE_ITEM _QEMU_UAE_PLUGIN_SIDECARS "${QEMU_UAE_PLUGIN}")
        if(_QEMU_UAE_PLUGIN_SIDECARS)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${_QEMU_UAE_PLUGIN_SIDECARS}
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
        endif()
    endif()
endif()

# Install plugin DLLs to plugins/ subdirectory
install(FILES $<TARGET_FILE:CAPSImage>
        DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins)
install(FILES $<TARGET_FILE:floppybridge>
        DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins)
if(QEMU_UAE_PLUGIN)
    install(FILES "${QEMU_UAE_PLUGIN}"
            DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins)
    if(_QEMU_UAE_PLUGIN_SIDECARS)
        install(FILES ${_QEMU_UAE_PLUGIN_SIDECARS}
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        ${AMIBERRY_PACKAGE_METADATA_EXCLUDES})

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

# Bundle the llvm-mingw runtime DLLs (not managed by vcpkg). Amiberry on
# Windows is built exclusively with llvm-mingw (clang + lld + libc++); the
# legacy MSYS2 / MinGW-w64 GCC build was retired together with PR #2026, so
# we only need the libc++ / libunwind / libwinpthread runtime here.
get_filename_component(_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(_MINGW_RUNTIME_DLLS libc++.dll libunwind.dll libwinpthread-1.dll)

install(CODE "
    foreach(_dll ${_MINGW_RUNTIME_DLLS})
        if(EXISTS \"${_MINGW_BIN_DIR}/\${_dll}\")
            message(STATUS \"Installing llvm-mingw runtime: \${_dll}\")
            file(INSTALL \"${_MINGW_BIN_DIR}/\${_dll}\"
                 DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
        endif()
    endforeach()
")
