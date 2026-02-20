include(FindHelper)

if (USE_GPIOD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GPIOD)
    find_library(LIBGPIOD_LIBRARIES gpiod REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBGPIOD_LIBRARIES})
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
    if (NOT ANDROID)
        find_package(OpenGL REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${TARGET_LINK_LIBRARIES} OpenGL::GL)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE GLESv3 EGL)
    endif()
endif ()

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

    # SDL2 / SDL2_image
    FetchContent_Declare(
        sdl2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.30.10
    )
    FetchContent_Declare(
        sdl2_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG        release-2.8.2
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

    # Materialize deps (order matters a bit: SDL2 first for *_image)
    if(USE_MPG123)
        FetchContent_MakeAvailable(sdl2 sdl2_image flac mpg123 libpng zstd)
    else()
        FetchContent_MakeAvailable(sdl2 sdl2_image flac libpng zstd)
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
    # Some upstream CMake projects can propagate a raw "SDL2" or "pthread"
    # library name via INTERFACE properties.
    # On Android this becomes "-lSDL2" / "-pthread" and breaks the link.
    function(amiberry_android_sanitize_target _tgt)
        if(NOT TARGET ${_tgt})
            return()
        endif()

        # If this is an ALIAS target (e.g. SDL2::SDL2), resolve to the real target.
        get_target_property(_aliased ${_tgt} ALIASED_TARGET)
        if(_aliased)
            set(_tgt ${_aliased})
        else()
            set(_tgt ${_tgt})
        endif()

        foreach(_prop IN ITEMS INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
            get_target_property(_val ${_tgt} ${_prop})
            if(_val)
                # Remove both "SDL2" (library name) and the common flag spellings.
                list(REMOVE_ITEM _val SDL2 pthread -lSDL2 -pthread)
                set_target_properties(${_tgt} PROPERTIES ${_prop} "${_val}")
            endif()
        endforeach()
    endfunction()

    # -------------------------------------------------------------------------

    # We use Prefabs for SDL2, but might need to fetch others or rely on system
    # However, on Android these are usually not present in the system image for NDK

    # ZLIB is usually available in NDK
    find_package(ZLIB REQUIRED)

    # Link SDL2 + extensions
    # Use real CMake targets to avoid generating raw '-lSDL2' flags on Android.
    set(AMIBERRY_SDL2_TARGET "")
    if(TARGET SDL2::SDL2)
        set(AMIBERRY_SDL2_TARGET SDL2::SDL2)
    elseif(TARGET SDL2-static)
        set(AMIBERRY_SDL2_TARGET SDL2-static)
    elseif(TARGET SDL2)
        set(AMIBERRY_SDL2_TARGET SDL2)
    endif()

    if(AMIBERRY_SDL2_TARGET STREQUAL "")
        message(FATAL_ERROR "SDL2 target not found (expected SDL2::SDL2, SDL2-static, or SDL2)")
    endif()

    # Ensure SDL extension libs depend on the chosen SDL2 target (not a raw "SDL2" name)
    if(TARGET SDL2_image)
        target_link_libraries(SDL2_image PRIVATE ${AMIBERRY_SDL2_TARGET})
    endif()

    # Compatibility shim: some deps still emit a raw "SDL2" item that becomes -lSDL2.
    # Provide a CMake target named "SDL2" that aliases the real SDL2 target so the
    # dependency resolves via target linking (no -lSDL2 lookup).
    # Note: CMake doesn't allow ALIAS -> ALIAS, so resolve to the underlying target.
    if(NOT TARGET SDL2)
        set(_amiberry_sdl2_real_target "${AMIBERRY_SDL2_TARGET}")
        if(TARGET ${_amiberry_sdl2_real_target})
            get_target_property(_amiberry_sdl2_aliased ${_amiberry_sdl2_real_target} ALIASED_TARGET)
            if(_amiberry_sdl2_aliased)
                set(_amiberry_sdl2_real_target ${_amiberry_sdl2_aliased})
            endif()
        endif()
        add_library(SDL2 ALIAS ${_amiberry_sdl2_real_target})
        unset(_amiberry_sdl2_real_target)
        unset(_amiberry_sdl2_aliased)
    endif()

    # Sanitize known SDL-related targets so they can't inject raw -lSDL2/-pthread
    amiberry_android_sanitize_target(SDL2_image)
    amiberry_android_sanitize_target(${AMIBERRY_SDL2_TARGET})

    target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_SDL2_TARGET} SDL2_image)

    # Defensive: ensure we never add raw SDL2/pthread libraries or flags on Android.
    # Some transitive/legacy paths may still append plain library names or link options.
    foreach(_prop IN ITEMS LINK_LIBRARIES LINK_OPTIONS INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
        get_target_property(_amiberry_val ${PROJECT_NAME} ${_prop})
        if(_amiberry_val)
            list(REMOVE_ITEM _amiberry_val SDL2 pthread -lSDL2 -pthread)
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
else()
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG QUIET)
    if(NOT SDL2_image_FOUND)
        find_package(SDL2_image MODULE REQUIRED)
    endif()
    find_package(FLAC REQUIRED)
    find_package(mpg123 REQUIRED)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)

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

# SDL include dirs: FetchContent builds may not provide SDL2::SDL2.
set(SDL2_INCLUDE_DIRS "")
if(TARGET SDL2::SDL2)
    get_target_property(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
elseif(TARGET SDL2-static)
    get_target_property(SDL2_INCLUDE_DIRS SDL2-static INTERFACE_INCLUDE_DIRECTORIES)
elseif(TARGET SDL2)
    get_target_property(SDL2_INCLUDE_DIRS SDL2 INTERFACE_INCLUDE_DIRECTORIES)
endif()

if(SDL2_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR})
else()
    # Keep previous behavior if detection fails; include dirs are often already set by targets.
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_IMAGE_INCLUDE_DIR})
endif()

if (USE_IMGUI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_IMGUI)
endif()

set(libmt32emu_SHARED FALSE)
add_subdirectory(external/mt32emu)
add_subdirectory(external/floppybridge)
add_subdirectory(external/capsimage)

# Prefer imported targets from CONFIG mode (vcpkg), fall back to MODULE variables
if(TARGET SDL2_image::SDL2_image)
    set(_SDL2_IMAGE_LIB SDL2_image::SDL2_image)
elseif(SDL2_IMAGE_LIBRARIES)
    set(_SDL2_IMAGE_LIB ${SDL2_IMAGE_LIBRARIES})
else()
    set(_SDL2_IMAGE_LIB SDL2_image)
endif()

set(AMIBERRY_LIBS
        mt32emu
        ZLIB::ZLIB
        ${CMAKE_DL_LIBS}
        ${_SDL2_IMAGE_LIB}
)

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

# Do not add SDL2 as a raw library name; Android must link SDL via the CMake target above.
if(ANDROID)
    list(REMOVE_ITEM AMIBERRY_LIBS SDL2 pthread)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_LIBS})

if (USE_IMGUI)
    if(USE_OPENGL)
        set(IMGUI_USE_OPENGL ON CACHE BOOL "Build ImGui OpenGL3 backend" FORCE)
    endif()
    add_subdirectory(external/imgui)
    target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
endif()

# capsimage and floppybridge are plugins (not linked into amiberry) but are
# copied by post-build commands. Explicit dependencies ensure they are built.
add_dependencies(${PROJECT_NAME} floppybridge capsimage)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(UTIL_LIBRARY util)
    if(UTIL_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${UTIL_LIBRARY})
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE rt)
endif ()

# Platform system libraries must come AFTER all other dependencies so that
# static libs (enet, etc.) can resolve their system library references.
if(AMIBERRY_PLATFORM_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_PLATFORM_LIBS})
endif()


