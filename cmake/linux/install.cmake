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

install(FILES $<TARGET_FILE:capsimage>
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
install(FILES $<TARGET_FILE:floppybridge>
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

# Bundle SDL3 shared libraries for distros that don't ship them (e.g. Debian Bookworm).
# The .so files are resolved from the imported CMake targets created by find_package(SDL3).
if(BUNDLE_SDL)
    # Collect SDL3 and SDL3_image shared library paths from their imported targets.
    set(_bundle_sdl_libs "")

    # SDL3 main library
    if(TARGET SDL3::SDL3-shared)
        list(APPEND _bundle_sdl_libs "$<TARGET_FILE:SDL3::SDL3-shared>")
    elseif(TARGET SDL3::SDL3)
        list(APPEND _bundle_sdl_libs "$<TARGET_FILE:SDL3::SDL3>")
    endif()

    # SDL3_image library
    if(TARGET SDL3_image::SDL3_image-shared)
        list(APPEND _bundle_sdl_libs "$<TARGET_FILE:SDL3_image::SDL3_image-shared>")
    elseif(TARGET SDL3_image::SDL3_image)
        list(APPEND _bundle_sdl_libs "$<TARGET_FILE:SDL3_image::SDL3_image>")
    endif()

    if(_bundle_sdl_libs)
        install(FILES ${_bundle_sdl_libs}
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

        # Also install the versioned symlinks (e.g. libSDL3.so.0) so the
        # dynamic linker finds the correct SONAME at runtime.
        foreach(_sdl_tgt IN ITEMS SDL3::SDL3-shared SDL3::SDL3 SDL3_image::SDL3_image-shared SDL3_image::SDL3_image)
            if(TARGET ${_sdl_tgt})
                get_target_property(_sdl_soname ${_sdl_tgt} IMPORTED_SONAME_RELEASE)
                if(NOT _sdl_soname)
                    get_target_property(_sdl_soname ${_sdl_tgt} IMPORTED_SONAME_NOCONFIG)
                endif()
                if(NOT _sdl_soname)
                    get_target_property(_sdl_soname ${_sdl_tgt} IMPORTED_SONAME)
                endif()
                if(_sdl_soname)
                    get_target_property(_sdl_loc ${_sdl_tgt} IMPORTED_LOCATION_RELEASE)
                    if(NOT _sdl_loc)
                        get_target_property(_sdl_loc ${_sdl_tgt} IMPORTED_LOCATION_NOCONFIG)
                    endif()
                    if(NOT _sdl_loc)
                        get_target_property(_sdl_loc ${_sdl_tgt} IMPORTED_LOCATION)
                    endif()
                    if(_sdl_loc)
                        cmake_path(GET _sdl_loc PARENT_PATH _sdl_dir)
                        set(_sdl_soname_path "${_sdl_dir}/${_sdl_soname}")
                        if(EXISTS "${_sdl_soname_path}" AND NOT "${_sdl_soname_path}" STREQUAL "${_sdl_loc}")
                            install(FILES "${_sdl_soname_path}"
                                    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
                        endif()
                    endif()
                endif()
            endif()
        endforeach()

        message(STATUS "BUNDLE_SDL: SDL3 shared libraries will be installed to ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")
    else()
        message(WARNING "BUNDLE_SDL is ON but no SDL3 shared library targets were found")
    endif()
endif()

# This one contains the gamecontrollersdb.txt file
install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# This one contains the data files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# This one contains the AROS kickstart files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# This one contains the whdboot files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# Install desktop file
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/Amiberry.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# Install icons at multiple sizes for proper desktop integration
foreach(_size 64 128 256 512)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/icons/${_size}x${_size}/amiberry.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps)
endforeach()

install(FILES ${CMAKE_SOURCE_DIR}/debian/changelog.gz
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES ${CMAKE_SOURCE_DIR}/debian/copyright
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/man/amiberry.1.gz
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/Amiberry.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/mime/amiberry.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)