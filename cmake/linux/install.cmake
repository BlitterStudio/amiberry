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
    set(_bundle_sdl_soname_files "")

    # --- Helper: resolve real .so path + SONAME symlink from an imported target ---
    macro(_bundle_sdl_from_target _tgt)
        if(TARGET ${_tgt})
            list(APPEND _bundle_sdl_libs "$<TARGET_FILE:${_tgt}>")
            # Collect the SONAME symlink (e.g. libSDL3.so.0) for the dynamic linker.
            foreach(_cfg IN ITEMS RELEASE NOCONFIG "")
                if(_cfg)
                    get_target_property(_soname ${_tgt} IMPORTED_SONAME_${_cfg})
                    get_target_property(_loc    ${_tgt} IMPORTED_LOCATION_${_cfg})
                else()
                    get_target_property(_soname ${_tgt} IMPORTED_SONAME)
                    get_target_property(_loc    ${_tgt} IMPORTED_LOCATION)
                endif()
                if(_soname AND _loc)
                    break()
                endif()
            endforeach()
            if(_soname AND _loc)
                cmake_path(GET _loc PARENT_PATH _dir)
                set(_soname_path "${_dir}/${_soname}")
                if(EXISTS "${_soname_path}" AND NOT "${_soname_path}" STREQUAL "${_loc}")
                    list(APPEND _bundle_sdl_soname_files "${_soname_path}")
                endif()
            endif()
            unset(_soname)
            unset(_loc)
            unset(_dir)
            unset(_soname_path)
        endif()
    endmacro()

    # SDL3 main library — try target-based discovery first.
    if(TARGET SDL3::SDL3-shared)
        _bundle_sdl_from_target(SDL3::SDL3-shared)
    elseif(TARGET SDL3::SDL3)
        _bundle_sdl_from_target(SDL3::SDL3)
    endif()

    # SDL3_image library
    if(TARGET SDL3_image::SDL3_image-shared)
        _bundle_sdl_from_target(SDL3_image::SDL3_image-shared)
    elseif(TARGET SDL3_image::SDL3_image)
        _bundle_sdl_from_target(SDL3_image::SDL3_image)
    endif()

    # --- Fallback: find_library when no suitable CMake targets are available ---
    # This handles Docker images / custom SDL3 installs that provide CMake config
    # files but only create INTERFACE (non-file) imported targets.
    if(NOT _bundle_sdl_libs)
        message(STATUS "BUNDLE_SDL: No file-bearing SDL3 targets found — falling back to find_library")
        foreach(_lib_name IN ITEMS SDL3 SDL3_image)
            find_library(_found_${_lib_name} NAMES ${_lib_name} PATHS /usr/lib /usr/local/lib PATH_SUFFIXES ${CMAKE_LIBRARY_ARCHITECTURE})
            if(_found_${_lib_name})
                # Resolve any symlink to the real versioned file.
                get_filename_component(_real_path "${_found_${_lib_name}}" REALPATH)
                list(APPEND _bundle_sdl_libs "${_real_path}")
                message(STATUS "  Found ${_lib_name}: ${_real_path}")
                # Also grab the SONAME symlink (e.g. libSDL3.so.0).
                get_filename_component(_lib_dir "${_real_path}" DIRECTORY)
                file(GLOB _soname_candidates "${_lib_dir}/lib${_lib_name}.so.[0-9]*")
                foreach(_cand IN LISTS _soname_candidates)
                    if(IS_SYMLINK "${_cand}")
                        list(APPEND _bundle_sdl_soname_files "${_cand}")
                        message(STATUS "  Found SONAME symlink: ${_cand}")
                    endif()
                endforeach()
            else()
                message(WARNING "BUNDLE_SDL: could not find lib${_lib_name}.so via find_library")
            endif()
        endforeach()
    endif()

    if(_bundle_sdl_libs)
        install(FILES ${_bundle_sdl_libs}
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
        if(_bundle_sdl_soname_files)
            install(FILES ${_bundle_sdl_soname_files}
                    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
        endif()
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