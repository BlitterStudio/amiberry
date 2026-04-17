include(FindHelper)

if (USE_GPIOD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GPIOD)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBGPIOD REQUIRED libgpiod)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBGPIOD_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBGPIOD_LIBRARIES})
    # Extract major version for compile-time API selection (v1 vs v2)
    string(REGEX MATCH "^[0-9]+" GPIOD_VERSION_MAJOR "${LIBGPIOD_VERSION}")
    if (GPIOD_VERSION_MAJOR)
        target_compile_definitions(${PROJECT_NAME} PRIVATE GPIOD_VERSION_MAJOR=${GPIOD_VERSION_MAJOR})
        message(STATUS "libgpiod version: ${LIBGPIOD_VERSION} (major: ${GPIOD_VERSION_MAJOR})")
    endif ()
endif ()

if (USE_DBUS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_DBUS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    target_include_directories(${PROJECT_NAME} PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${DBUS_LIBRARIES})
endif ()

if (USE_IPC_SOCKET)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_IPC_SOCKET)
    message(STATUS "Unix socket IPC enabled")
endif ()

if (USE_OPENGL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENGL)
    if(IOS)
        # iOS: OpenGLES framework is linked via AMIBERRY_PLATFORM_LIBS in StandardProjectSettings.cmake
        target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GLES3)
    elseif (NOT ANDROID)
        if (USE_GLES)
            # IMGUI_IMPL_OPENGL_ES3 is needed here (not just on the imgui
            # target) because main_window.cpp includes imgui_impl_opengl3.h
            # directly to drive ImGui through the emulator's GL context when
            # the GUI shares its window — see issue #1974.
            target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GLES3 IMGUI_IMPL_OPENGL_ES3)
            target_link_libraries(${PROJECT_NAME} PRIVATE GLESv2 EGL)
        else()
            find_package(OpenGL REQUIRED)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${TARGET_LINK_LIBRARIES} OpenGL::GL)
        endif()
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE GLESv3 EGL)
    endif()
endif ()

if (USE_VULKAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_VULKAN)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include)
    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
    message(STATUS "Vulkan renderer enabled (experimental)")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "  Vulkan validation layers will be requested (Debug build)")
    endif()
endif ()

# iOS: SDL3's static lib exports a dependency on -lGLESv2 (Linux library name).
# On iOS, GLES is provided via -framework OpenGLES. Create an alias so the linker
# doesn't fail on the missing library name.
if(IOS)
    if(NOT TARGET GLESv2)
        add_library(GLESv2 INTERFACE IMPORTED)
        # OpenGLES framework is already linked via AMIBERRY_PLATFORM_LIBS
    endif()
endif()

# SDL3
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_SDL3)
message(STATUS "Using SDL3")

if(ANDROID)
    include(FetchContent)

    # Android doesn't provide ALSA. Disable ALSA backends in bundled deps that try to detect it.
    set(SDL_ALSA OFF CACHE BOOL "Disable ALSA for Android" FORCE)
    set(SDL_ALSA_SHARED OFF CACHE BOOL "Disable ALSA shared loading for Android" FORCE)
    set(DEFAULT_OUTPUT_MODULE "" CACHE STRING "Disable mpg123 default output module selection" FORCE)

    # -------------------------------------------------------------------------
    # Android: build third-party deps from source via FetchContent (pinned tags)
    # -------------------------------------------------------------------------
    # Note: Desktop builds use system packages (see non-ANDROID branch below).
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

    # SDL3 / SDL3_image (built from source on Android)
    FetchContent_Declare(
        sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.2.8
    )
    FetchContent_Declare(
        sdl3_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG        release-3.2.4
    )

    # Zstd
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared lib" FORCE)
    set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static lib" FORCE)
    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG        v1.5.6
        SOURCE_SUBDIR  build/cmake
    )

    # FLAC
    set(BUILD_PROGRAMS OFF CACHE BOOL "Build and install programs" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build and install examples" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "Build and install doxygen documents" FORCE)
    set(INSTALL_MANPAGES OFF CACHE BOOL "Install MAN pages" FORCE)
    set(WITH_OGG OFF CACHE BOOL "ogg support" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
    FetchContent_Declare(
        flac
        GIT_REPOSITORY https://github.com/xiph/flac.git
        GIT_TAG        1.5.0
    )

    # mpg123 (only if enabled)
    if(USE_MPG123)
        set(BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
        set(BUILD_LIBOUT123 OFF CACHE BOOL "Build libout123" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
        FetchContent_Declare(
            mpg123
            # madebr/mpg123 is a mirror of the upstream SVN repo but does not publish git tags.
            # Pin to a specific commit for reproducible builds.
            GIT_REPOSITORY https://github.com/madebr/mpg123.git
            GIT_TAG        a06133928e6518bd65314c9cea12ccb5588703e9
        )
    endif()

    # libpng
    set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
    set(PNG_STATIC ON CACHE BOOL "Build static lib" FORCE)
    set(PNG_TESTS OFF CACHE BOOL "Build tests" FORCE)
    FetchContent_Declare(
        libpng
        GIT_REPOSITORY https://github.com/pnggroup/libpng.git
        GIT_TAG        v1.6.43
    )

    # mbedTLS (SSL backend for curl — Android NDK doesn't ship OpenSSL)
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
    set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
    set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        mbedtls
        GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
        GIT_TAG        v3.6.3
    )

    # libcurl
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_MBEDTLS ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_11_1
    )

    # nlohmann-json (header-only)
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
    )

    # Materialize mbedTLS first so curl can find it
    FetchContent_GetProperties(mbedtls)
    if(NOT mbedtls_POPULATED)
        FetchContent_Populate(mbedtls)
        add_subdirectory(${mbedtls_SOURCE_DIR} ${mbedtls_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    # Pre-set cache vars as FILE PATHS (not target names) so curl's
    # install(EXPORT CURLTargets) doesn't pull mbedtls targets into
    # the export set — file-path link deps don't need export entries.
    set(MBEDTLS_INCLUDE_DIR "${mbedtls_SOURCE_DIR}/include" CACHE PATH "" FORCE)
    set(MBEDTLS_LIBRARY "${mbedtls_BINARY_DIR}/library/libmbedtls.a" CACHE FILEPATH "" FORCE)
    set(MBEDX509_LIBRARY "${mbedtls_BINARY_DIR}/library/libmbedx509.a" CACHE FILEPATH "" FORCE)
    set(MBEDCRYPTO_LIBRARY "${mbedtls_BINARY_DIR}/library/libmbedcrypto.a" CACHE FILEPATH "" FORCE)

    # Materialize curl
    FetchContent_GetProperties(curl)
    if(NOT curl_POPULATED)
        FetchContent_Populate(curl)
        add_subdirectory(${curl_SOURCE_DIR} ${curl_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    # File-path linking skips automatic target dependency tracking,
    # so explicitly ensure mbedTLS is built before curl links against it.
    if(TARGET libcurl_static)
        add_dependencies(libcurl_static mbedtls mbedx509 mbedcrypto)
    elseif(TARGET libcurl)
        add_dependencies(libcurl mbedtls mbedx509 mbedcrypto)
    endif()

    # Materialize remaining deps (order matters: SDL first for *_image)
    if(USE_MPG123)
        FetchContent_MakeAvailable(sdl3 sdl3_image flac mpg123 libpng zstd nlohmann_json)
    else()
        FetchContent_MakeAvailable(sdl3 sdl3_image flac libpng zstd nlohmann_json)
    endif()

    # Make zstd discoverable for the rest of this file (later FindHelper/pkg-config logic).
    if(TARGET libzstd_static)
        set(ZSTD_FOUND TRUE)
        set(ZSTD_LIBRARIES libzstd_static)
        if(EXISTS "${zstd_SOURCE_DIR}/lib")
            set(ZSTD_INCLUDE_DIRS "${zstd_SOURCE_DIR}/lib")
        endif()
    elseif(TARGET zstd)
        set(ZSTD_FOUND TRUE)
        set(ZSTD_LIBRARIES zstd)
        if(EXISTS "${zstd_SOURCE_DIR}/lib")
            set(ZSTD_INCLUDE_DIRS "${zstd_SOURCE_DIR}/lib")
        endif()
    endif()

    # --- Android link hygiene ------------------------------------------------
    # Some upstream CMake projects can propagate a raw "SDL3" or "pthread"
    # library name via INTERFACE properties.
    function(amiberry_android_sanitize_target _tgt)
        if(NOT TARGET ${_tgt})
            return()
        endif()

        # If this is an ALIAS target (e.g. SDL3::SDL3), resolve to the real target.
        get_target_property(_aliased ${_tgt} ALIASED_TARGET)
        if(_aliased)
            set(_tgt ${_aliased})
        else()
            set(_tgt ${_tgt})
        endif()

        foreach(_prop IN ITEMS INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
            get_target_property(_val ${_tgt} ${_prop})
            if(_val)
                # Remove raw library names and common flag spellings.
                list(REMOVE_ITEM _val SDL3 SDL3_image pthread -lSDL3 -lSDL3_image -pthread)
                set_target_properties(${_tgt} PROPERTIES ${_prop} "${_val}")
            endif()
        endforeach()
    endfunction()

    # -------------------------------------------------------------------------

    # ZLIB is usually available in NDK
    find_package(ZLIB REQUIRED)

    # Link SDL3 + extensions
    # Use real CMake targets to avoid generating raw '-lSDL3' flags on Android.
    set(AMIBERRY_SDL_TARGET "")
    if(TARGET SDL3::SDL3)
        set(AMIBERRY_SDL_TARGET SDL3::SDL3)
    elseif(TARGET SDL3-static)
        set(AMIBERRY_SDL_TARGET SDL3-static)
    elseif(TARGET SDL3)
        set(AMIBERRY_SDL_TARGET SDL3)
    endif()
    if(AMIBERRY_SDL_TARGET STREQUAL "")
        message(FATAL_ERROR "SDL3 target not found (expected SDL3::SDL3, SDL3-static, or SDL3)")
    endif()

    # Resolve SDL3_image target (shared or static depending on BUILD_SHARED_LIBS)
    set(AMIBERRY_SDL_IMAGE_TARGET "")
    if(TARGET SDL3_image::SDL3_image)
        set(AMIBERRY_SDL_IMAGE_TARGET SDL3_image::SDL3_image)
    elseif(TARGET SDL3_image-static)
        set(AMIBERRY_SDL_IMAGE_TARGET SDL3_image-static)
    elseif(TARGET SDL3_image)
        set(AMIBERRY_SDL_IMAGE_TARGET SDL3_image)
    endif()

    # Resolve alias to real target for modification (target_link_libraries, sanitize)
    set(_SDL_IMAGE_REAL_TARGET ${AMIBERRY_SDL_IMAGE_TARGET})
    if(NOT _SDL_IMAGE_REAL_TARGET STREQUAL "")
        get_target_property(_aliased ${_SDL_IMAGE_REAL_TARGET} ALIASED_TARGET)
        if(_aliased)
            set(_SDL_IMAGE_REAL_TARGET ${_aliased})
        endif()
        target_link_libraries(${_SDL_IMAGE_REAL_TARGET} PRIVATE ${AMIBERRY_SDL_TARGET})
    endif()

    # Sanitize known SDL-related targets so they can't inject raw -lSDL3/-pthread
    amiberry_android_sanitize_target(${AMIBERRY_SDL_IMAGE_TARGET})
    amiberry_android_sanitize_target(${AMIBERRY_SDL_TARGET})

    target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_SDL_TARGET} ${AMIBERRY_SDL_IMAGE_TARGET})
    target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

    # Defensive: ensure we never add raw SDL3/pthread libraries or flags on Android.
    # Some transitive/legacy paths may still append plain library names or link options.
    foreach(_prop IN ITEMS LINK_LIBRARIES LINK_OPTIONS INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
        get_target_property(_amiberry_val ${PROJECT_NAME} ${_prop})
        if(_amiberry_val)
            list(REMOVE_ITEM _amiberry_val SDL3 pthread -lSDL3 -lSDL3_image -pthread)
            set_target_properties(${PROJECT_NAME} PROPERTIES ${_prop} "${_amiberry_val}")
        endif()
    endforeach()

    # mpg123 include path (subproject)
    if(USE_MPG123 AND EXISTS "${mpg123_SOURCE_DIR}/src/include")
        target_include_directories(${PROJECT_NAME} PRIVATE "${mpg123_SOURCE_DIR}/src/include")
    endif()
    # libpng (headers are exported by the target, but keep compatible include behavior)
    if(EXISTS "${libpng_SOURCE_DIR}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${libpng_SOURCE_DIR}")
    endif()
    # pnglibconf.h is generated into libpng's binary dir; include it so png.h can find it.
    if(EXISTS "${libpng_BINARY_DIR}/pnglibconf.h")
        target_include_directories(${PROJECT_NAME} PRIVATE "${libpng_BINARY_DIR}")
    endif()
    # zstd headers
    if(EXISTS "${zstd_SOURCE_DIR}/lib")
        target_include_directories(${PROJECT_NAME} PRIVATE "${zstd_SOURCE_DIR}/lib")
    endif()


    # --- PortMidi ---
    if(USE_PORTMIDI)
        # PortMidi decides whether to use ALSA based on the LINUX_DEFINES cache var.
        # On Android, ALSA is unavailable, so force PortMidi to build with PMNULL.
        set(LINUX_DEFINES "PMNULL" CACHE STRING "Disable PortMidi ALSA backend on Android" FORCE)

        # PortTime: PortMidi's "ptlinux" backend includes sys/timeb.h which is not available
        # on Android NDK. Force PortTime to use the null backend for Android builds.
        set(PT_BACKEND "ptnull" CACHE STRING "PortTime backend" FORCE)

        FetchContent_Declare(
            portmidi
            GIT_REPOSITORY https://github.com/PortMidi/portmidi.git
            GIT_TAG        v2.0.4
            # FetchContent may download a tarball snapshot. Don't rely on git being available
            # in <SOURCE_DIR>. Apply Android fixes via a deterministic CMake script.
            PATCH_COMMAND  ${CMAKE_COMMAND} -E echo "Fixing PortMidi for Android" &&
                           ${CMAKE_COMMAND} -DPORTMIDI_SOURCE_DIR=<SOURCE_DIR>
                                          -DAMIBERRY_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                                          -P "${CMAKE_SOURCE_DIR}/cmake/portmidi_fix_android.cmake"
         )
         FetchContent_MakeAvailable(portmidi)

        target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)

        # Amiberry includes porttime.h directly; ensure PortMidi's porttime headers are visible.
        if (EXISTS "${portmidi_SOURCE_DIR}/porttime/porttime.h")
            target_include_directories(${PROJECT_NAME} PRIVATE "${portmidi_SOURCE_DIR}/porttime")
        endif()

        # Amiberry includes portmidi.h directly; it's located in PortMidi's pm_common directory.
        if (EXISTS "${portmidi_SOURCE_DIR}/pm_common/portmidi.h")
            target_include_directories(${PROJECT_NAME} PRIVATE "${portmidi_SOURCE_DIR}/pm_common")
        endif()
    endif()

    # --- ENet ---
    if(USE_LIBENET)
        FetchContent_Declare(
            enet
            GIT_REPOSITORY https://github.com/lsalzman/enet
            GIT_TAG        v1.3.18
        )
        # ENet doesn't export a target in older versions, so we might need to be careful
        # But lsalzman/enet usually has CMake support.
        FetchContent_MakeAvailable(enet) 
        target_link_libraries(${PROJECT_NAME} PRIVATE enet)

        # Ensure the enet/enet.h header is visible when compiling amiberry on Android.
        if (EXISTS "${enet_SOURCE_DIR}/include/enet/enet.h")
            target_include_directories(${PROJECT_NAME} PRIVATE "${enet_SOURCE_DIR}/include")
        endif()
    endif()

    # --- LibSerialPort ---
    if(USE_LIBSERIALPORT)
        # Use a CMake-friendly fork
        FetchContent_Declare(
            libserialport
            GIT_REPOSITORY https://github.com/scottmudge/libserialport-cmake
            GIT_TAG        c82d28deb05185df8c4dafea96dd520a3c0db7c9
            PATCH_COMMAND  ${CMAKE_COMMAND} -E echo "Fixing libserialport for Android" &&
                           ${CMAKE_COMMAND} -DLIBSERIALPORT_SOURCE_DIR=<SOURCE_DIR>
                                          -P "${CMAKE_SOURCE_DIR}/cmake/libserialport_fix_android.cmake"
        )
        FetchContent_MakeAvailable(libserialport)

        # Link to the produced CMake target to avoid raw '-lserialport' flags.
        # scottmudge/libserialport-cmake defines project(serialport) and creates:
        #   - serialport-static
        #   - serialport-shared
        # Other libserialport setups may use different target names, so keep fallbacks.
        set(AMIBERRY_LIBSERIALPORT_TARGET "")
        if(TARGET serialport-static)
            set(AMIBERRY_LIBSERIALPORT_TARGET serialport-static)
        elseif(TARGET serialport)
            set(AMIBERRY_LIBSERIALPORT_TARGET serialport)
        elseif(TARGET serialport-shared)
            set(AMIBERRY_LIBSERIALPORT_TARGET serialport-shared)
        elseif(TARGET libserialport)
            set(AMIBERRY_LIBSERIALPORT_TARGET libserialport)
        elseif(TARGET libserialport-static)
            set(AMIBERRY_LIBSERIALPORT_TARGET libserialport-static)
        endif()

        if(AMIBERRY_LIBSERIALPORT_TARGET STREQUAL "")
            message(FATAL_ERROR "libserialport target was not created (tried: serialport-static, serialport-shared, serialport, libserialport, libserialport-static)")
        endif()

        target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_LIBSERIALPORT_TARGET})

        # The serialport target doesn't consistently propagate public include dirs across toolchains.
        # Ensure the header (<libserialport.h>) is visible when compiling amiberry on Android.
        if (EXISTS "${libserialport_SOURCE_DIR}/libserialport.h")
            target_include_directories(${PROJECT_NAME} PRIVATE "${libserialport_SOURCE_DIR}")
        elseif (EXISTS "${libserialport_SOURCE_DIR}/libserialport")
            target_include_directories(${PROJECT_NAME} PRIVATE "${libserialport_SOURCE_DIR}")
        endif()
    endif()
    
    # --- Existing dependencies --- 
elseif(IOS)
    # iOS: SDL3 and SDL3_image must be pre-built and findable via CMAKE_FIND_ROOT_PATH.
    # Most optional codecs and network libs are unavailable or unnecessary on iOS.
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_image CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG QUIET)
    if(PNG_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)
    endif()

    # nlohmann_json: header-only, fetch if not found
    find_package(nlohmann_json CONFIG QUIET)
    if(NOT nlohmann_json_FOUND)
        include(FetchContent)
        FetchContent_Declare(json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
            GIT_SHALLOW TRUE
        )
        set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(json)
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

    # No FLAC, mpg123, or CURL on iOS
    # - FLAC/mpg123: audio codec libs not needed (no CD audio ripping on mobile)
    # - CURL: self-update is disabled on iOS
else()
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_image CONFIG REQUIRED)
    find_package(FLAC REQUIRED)
    find_package(mpg123 REQUIRED)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(CURL REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)

    # mpg123 is available on desktop builds; enable mp3 decoding in code.
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_MPG123)
endif()

if (USE_ZSTD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_ZSTD)

    # On Android, zstd is provided via FetchContent above. Prefer its CMake target.
    if(ANDROID)
        if(TARGET libzstd_static)
            target_include_directories(${PROJECT_NAME} PRIVATE "${zstd_SOURCE_DIR}/lib")
            target_link_libraries(${PROJECT_NAME} PRIVATE libzstd_static)
        elseif(TARGET zstd)
            target_include_directories(${PROJECT_NAME} PRIVATE "${zstd_SOURCE_DIR}/lib")
            target_link_libraries(${PROJECT_NAME} PRIVATE zstd)
        else()
            message(STATUS "ZSTD enabled but zstd target was not created - CHD compressed disk images will not be supported")
        endif()
    else()
        find_helper(ZSTD libzstd zstd.h zstd)
        if(NOT ZSTD_FOUND)
            message(STATUS "ZSTD library not found - CHD compressed disk images will not be supported")
        else()
            target_include_directories(${PROJECT_NAME} PRIVATE ${ZSTD_INCLUDE_DIRS})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${ZSTD_LIBRARIES})
        endif()
    endif()
endif ()

# libserialport find/link fallback is for desktop builds.
# On Android, libserialport is built via FetchContent above and linked via the 'serialport' target;
# running FindHelper can inject a raw '-lserialport' link flag, which fails on the NDK.
if (USE_LIBSERIALPORT AND NOT ANDROID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LIBSERIALPORT)
    find_helper(LIBSERIALPORT libserialport libserialport.h serialport)
    if(TARGET serialport)
        target_link_libraries(${PROJECT_NAME} PRIVATE serialport)
    elseif(LIBSERIALPORT_FOUND AND LIBSERIALPORT_LIBRARIES)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBSERIALPORT_LIBRARIES})
    else()
        message(STATUS "LibSerialPort enabled but library was not found")
    endif()
endif ()

if (USE_PORTMIDI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_PORTMIDI)
    find_helper(PORTMIDI portmidi portmidi.h portmidi)
    if(TARGET portmidi)
        target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)
    elseif(PORTMIDI_FOUND AND PORTMIDI_LIBRARIES)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PORTMIDI_LIBRARIES})
    else()
        message(STATUS "PortMidi enabled but library was not found")
    endif()
endif ()

if (USE_LIBMPEG2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LIBMPEG2)
    find_helper(LIBMPEG2_CONVERT libmpeg2convert mpeg2convert.h mpeg2convert)
    find_helper(LIBMPEG2 libmpeg2 mpeg2.h mpeg2)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBMPEG2_LIBRARIES} ${LIBMPEG2_CONVERT_LIBRARIES})
endif ()

if (USE_LIBENET)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LIBENET)
    find_helper(LIBENET libenet enet/enet.h enet)
    if(NOT LIBENET_FOUND)
        message(STATUS "LibENET library not found - network emulation will not be supported")
    else()
        target_include_directories(${PROJECT_NAME} PRIVATE ${LIBENET_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBENET_LIBRARIES})
    endif()
endif ()

if (USE_PCEM)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_PCEM)
endif ()

# Add libpcap for uaenet (Linux/macOS)
if (USE_UAENET_PCAP)
    find_path(PCAP_INCLUDE_DIR pcap.h)
    find_library(PCAP_LIBRARY pcap)
    if (PCAP_INCLUDE_DIR AND PCAP_LIBRARY)
        message(STATUS "Found libpcap: ${PCAP_LIBRARY}")
        target_include_directories(${PROJECT_NAME} PRIVATE ${PCAP_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PCAP_LIBRARY})
        target_compile_definitions(${PROJECT_NAME} PRIVATE WITH_UAENET_PCAP)
    else()
        message(FATAL_ERROR "libpcap not found. Please install libpcap-dev (Linux) or brew install libpcap (macOS)")
    endif()
endif()

# Add TAP backend for uaenet (Linux only, no library dependencies)
if (USE_UAENET_TAP)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_definitions(${PROJECT_NAME} PRIVATE WITH_UAENET_TAP)
        message(STATUS "TAP uaenet backend enabled")
    else()
        message(STATUS "TAP uaenet backend not available (Linux only)")
    endif()
endif()

# SDL3 include dirs: FetchContent builds may not provide SDL3::SDL3.
set(_SDL_INCLUDE_DIRS "")
if(TARGET SDL3::SDL3)
    get_target_property(_SDL_INCLUDE_DIRS SDL3::SDL3 INTERFACE_INCLUDE_DIRECTORIES)
elseif(TARGET SDL3-static)
    get_target_property(_SDL_INCLUDE_DIRS SDL3-static INTERFACE_INCLUDE_DIRECTORIES)
elseif(TARGET SDL3)
    get_target_property(_SDL_INCLUDE_DIRS SDL3 INTERFACE_INCLUDE_DIRECTORIES)
endif()
if(_SDL_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${_SDL_INCLUDE_DIRS})
endif()

# SDL3_image include dirs: FetchContent builds need the include path for <SDL3_image/SDL_image.h>.
if(ANDROID AND DEFINED sdl3_image_SOURCE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE "${sdl3_image_SOURCE_DIR}/include")
endif()

# ImGui is always enabled
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_IMGUI)

set(libmt32emu_SHARED FALSE)
add_subdirectory(external/mt32emu)
if(NOT IOS)
    add_subdirectory(external/floppybridge)
endif()
add_subdirectory(external/capsimage)

# Prefer imported targets from CONFIG mode (vcpkg), fall back to MODULE variables.
# On Android, SDL3_image is already linked via the FetchContent target (line ~187).
if(ANDROID)
    set(_SDL_IMAGE_LIB "")
elseif(TARGET SDL3_image::SDL3_image)
    set(_SDL_IMAGE_LIB SDL3_image::SDL3_image)
elseif(SDL3_IMAGE_LIBRARIES)
    set(_SDL_IMAGE_LIB ${SDL3_IMAGE_LIBRARIES})
else()
    set(_SDL_IMAGE_LIB SDL3_image)
endif()

set(AMIBERRY_LIBS
        mt32emu
        ZLIB::ZLIB
        nlohmann_json::nlohmann_json
        ${CMAKE_DL_LIBS}
        ${_SDL_IMAGE_LIB}
)

# CURL is used for self-update (not available on iOS)
if(TARGET CURL::libcurl)
    list(APPEND AMIBERRY_LIBS CURL::libcurl)
    target_compile_definitions(${PROJECT_NAME} PRIVATE AMIBERRY_HAS_CURL)
elseif(CURL_FOUND)
    list(APPEND AMIBERRY_LIBS ${CURL_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE AMIBERRY_HAS_CURL)
endif()

if (NOT ANDROID AND NOT WIN32)
    list(APPEND AMIBERRY_LIBS pthread)
endif()

if(TARGET FLAC)
    list(APPEND AMIBERRY_LIBS FLAC)
elseif(FLAC_FOUND)
     # Fallback if FLAC_FOUND is set but target not visible (old FindFLAC)
     list(APPEND AMIBERRY_LIBS ${FLAC_LIBRARIES})
endif()

if(TARGET PNG::PNG)
    list(APPEND AMIBERRY_LIBS PNG::PNG)
elseif(TARGET png_static)
    list(APPEND AMIBERRY_LIBS png_static)
elseif(TARGET png)
    list(APPEND AMIBERRY_LIBS png)
elseif(PNG_FOUND)
    list(APPEND AMIBERRY_LIBS ${PNG_LIBRARIES})
endif()

if(TARGET MPG123::libmpg123)
    list(APPEND AMIBERRY_LIBS MPG123::libmpg123)
elseif(TARGET libmpg123)
    list(APPEND AMIBERRY_LIBS libmpg123)
elseif(MPG123_FOUND)
    list(APPEND AMIBERRY_LIBS ${MPG123_LIBRARIES})
endif()

if(TARGET libzstd_static)
    list(APPEND AMIBERRY_LIBS libzstd_static)
elseif(TARGET zstd)
    list(APPEND AMIBERRY_LIBS zstd)
elseif(ZSTD_FOUND)
    list(APPEND AMIBERRY_LIBS ${ZSTD_LIBRARIES})
endif()

if(ANDROID)
    list(APPEND AMIBERRY_LIBS log android)
endif()

# Do not add SDL as a raw library name; Android must link SDL via the CMake target above.
if(ANDROID)
    list(REMOVE_ITEM AMIBERRY_LIBS SDL3 pthread)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_LIBS})

# ImGui is always enabled
if(USE_OPENGL)
    set(IMGUI_USE_OPENGL ON CACHE BOOL "Build ImGui OpenGL3 backend" FORCE)
endif()
if(USE_VULKAN)
    set(IMGUI_USE_VULKAN ON CACHE BOOL "Build ImGui Vulkan backend" FORCE)
endif()
set(IMGUI_USE_SDL3 ON CACHE BOOL "Build ImGui SDL3 backend" FORCE)
add_subdirectory(external/imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)

# Native File Dialog Extended (NFD) — OS-native open/save/folder dialogs.
# Excluded on Android (own Kotlin UI) and Haiku (unsupported by NFD).
# Gracefully skipped when the submodule is not initialized (plain git clone
# without --recursive, or GitHub source archives), or when Linux build deps
# (dbus-1) are missing — falls back to the built-in ImGuiFileDialog.
set(NFD_AVAILABLE OFF)
if(NOT ANDROID AND NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Haiku"
        AND EXISTS "${CMAKE_SOURCE_DIR}/external/nativefiledialog-extended/CMakeLists.txt")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(NFD_DBUS dbus-1)
            pkg_check_modules(NFD_WAYLAND QUIET wayland-client)
        endif()
        if(NOT NFD_DBUS_FOUND)
            message(STATUS "NFD: dbus-1 not found — native file dialogs disabled (install libdbus-1-dev)")
        else()
            set(NFD_AVAILABLE ON)
            if(NOT NFD_WAYLAND_FOUND)
                set(NFD_WAYLAND OFF CACHE BOOL "Wayland not available" FORCE)
            endif()
        endif()
    else()
        set(NFD_AVAILABLE ON)
    endif()
elseif(NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    message(STATUS "NFD submodule not found — native file dialogs disabled (use git submodule update --init --recursive)")
endif()
if(NFD_AVAILABLE)
    set(NFD_PORTAL ON CACHE BOOL "Use xdg-desktop-portal on Linux (avoids GTK dep)" FORCE)
    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(NFD_INSTALL OFF CACHE BOOL "" FORCE)
    set(NFD_USE_ALLOWEDCONTENTTYPES_IF_AVAILABLE OFF CACHE BOOL "Use legacy allowedFileTypes for broader macOS compat" FORCE)
    set(NFD_OVERRIDE_RECENT_WITH_DEFAULT ON CACHE BOOL "Always open in the specified default directory, not the last-used one" FORCE)
    set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(external/nativefiledialog-extended)
    set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")
    target_link_libraries(${PROJECT_NAME} PRIVATE nfd)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_NFD)
endif()

# capsimage and floppybridge are plugins (not linked into amiberry) but are
# copied by post-build commands. Explicit dependencies ensure they are built.
if(IOS)
    add_dependencies(${PROJECT_NAME} capsimage)
else()
    add_dependencies(${PROJECT_NAME} floppybridge capsimage)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(UTIL_LIBRARY util)
    if(UTIL_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${UTIL_LIBRARY})
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE rt)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    target_link_libraries(${PROJECT_NAME} PRIVATE network iconv bsd)
endif()

# Platform system libraries must come AFTER all other dependencies so that
# static libs (enet, etc.) can resolve their system library references.
if(AMIBERRY_PLATFORM_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_PLATFORM_LIBS})
endif()
