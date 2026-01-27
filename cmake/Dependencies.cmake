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

if (USE_OPENGL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENGL)
    if (NOT ANDROID)
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${TARGET_LINK_LIBRARIES} GLEW OpenGL::GL)
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

    # SDL2 / SDL2_image / SDL2_ttf
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
    FetchContent_Declare(
        sdl2_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG        release-2.22.0
    )

    # Zstd
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared lib" FORCE)
    set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static lib" FORCE)
    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG        v1.5.6
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

    # mpg123
    set(BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
    set(BUILD_LIBOUT123 OFF CACHE BOOL "Build libout123" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
    FetchContent_Declare(
        mpg123
        # madebr/mpg123 is a mirror of the upstream SVN repo but does not publish git tags.
        # Pin to an existing ref so FetchContent can check out reliably.
        GIT_REPOSITORY https://github.com/madebr/mpg123.git
        GIT_TAG        master
    )

    # libpng
    set(PNG_SHARED OFF CACHE BOOL "Build shared lib" FORCE)
    set(PNG_STATIC ON CACHE BOOL "Build static lib" FORCE)
    set(PNG_TESTS OFF CACHE BOOL "Build tests" FORCE)
    FetchContent_Declare(
        libpng
        GIT_REPOSITORY https://github.com/pnggroup/libpng.git
        GIT_TAG        v1.6.43
    )

    # Materialize deps (order matters a bit: SDL2 first for *_image/_ttf)
    FetchContent_MakeAvailable(sdl2 sdl2_image sdl2_ttf flac mpg123 libpng zstd)

    # We use Prefabs for SDL2, but might need to fetch others or rely on system
    # However, on Android these are usually not present in the system image for NDK

    # ZLIB is usually available in NDK
    find_package(ZLIB REQUIRED)

    # Link SDL2 + extensions
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 SDL2_image SDL2_ttf)

    # mpg123 include path (subproject)
    if(EXISTS "${mpg123_SOURCE_DIR}/src/include")
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

        FetchContent_Declare(
            portmidi
            GIT_REPOSITORY https://github.com/PortMidi/portmidi.git
            GIT_TAG        v2.0.4
            PATCH_COMMAND  ${CMAKE_COMMAND} -E echo "Patching PortMidi for Android (bzero -> memset)" &&
                           ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> git apply --ignore-space-change --ignore-whitespace
                               "${CMAKE_SOURCE_DIR}/cmake/patches/portmidi-android-bzero.patch"
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
            GIT_TAG        master
        )
        FetchContent_MakeAvailable(libserialport)
        target_link_libraries(${PROJECT_NAME} PRIVATE serialport)

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
    find_package(SDL2_image MODULE REQUIRED)
    find_package(SDL2_ttf MODULE REQUIRED)
    find_package(FLAC REQUIRED)
    find_package(mpg123 REQUIRED)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)
endif()

if (USE_ZSTD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_ZSTD)
    find_helper(ZSTD libzstd zstd.h zstd)
    if(NOT ZSTD_FOUND)
        message(STATUS "ZSTD library not found - CHD compressed disk images will not be supported")
    else()
        target_include_directories(${PROJECT_NAME} PRIVATE ${ZSTD_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ZSTD_LIBRARIES})
    endif()
endif ()

if (USE_LIBSERIALPORT)
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

get_target_property(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR})

set(libmt32emu_SHARED FALSE)
add_subdirectory(external/mt32emu)
add_subdirectory(external/floppybridge)
add_subdirectory(external/capsimage)
add_subdirectory(external/libguisan)

target_include_directories(guisan PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR})

set(AMIBERRY_LIBS
        guisan
        mt32emu
        z
        dl
)
if (NOT ANDROID)
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

if(TARGET png_static)
    list(APPEND AMIBERRY_LIBS png_static)
elseif(TARGET png)
    list(APPEND AMIBERRY_LIBS png)
    list(APPEND AMIBERRY_LIBS ${PNG_LIBRARIES})
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

target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_LIBS})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(UTIL_LIBRARY util)
    if(UTIL_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${UTIL_LIBRARY})
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE rt)
endif ()

# Add dependencies to ensure external libraries are built
add_dependencies(${PROJECT_NAME} mt32emu floppybridge capsimage guisan)
