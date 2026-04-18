cmake_minimum_required(VERSION 3.16)

project(amiberry
    VERSION 8.1.5
    LANGUAGES C CXX
    DESCRIPTION "Optimized Amiga emulator for various platforms"
    HOMEPAGE_URL "https://amiberry.com"
)

# Prevent in-tree builds
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-tree builds are not supported. Use a separate build directory.")
endif()

# Set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Feature options
option(USE_PCEM          "Use PCem for hardware emulation (no support for RISC-V)" ON)
option(USE_LIBSERIALPORT "Use LibSerialPort for serial port emulation" ON)
option(USE_LIBENET       "Use LibENET for network emulation" ON)
option(USE_PORTMIDI      "Use PortMidi for MIDI emulation" ON)
option(USE_LIBMPEG2      "Use LibMPEG2 for CD32 FMV support" ON)
option(USE_UAENET_PCAP   "Use libpcap for uaenet (Linux/macOS)" ON)
option(USE_UAENET_TAP    "Use TAP backend for uaenet (Linux only)" ON)
option(USE_ZSTD 	     "Use Zstandard for CHD compressed disk images" ON)
option(USE_GPIOD         "Use GPIOD to control GPIO LEDs" OFF)
option(USE_DBUS          "Use DBus to control the emulator" OFF)
option(USE_OPENGL        "Use OpenGL for rendering" ON)
option(USE_GLES          "Use OpenGL ES instead of desktop OpenGL (for embedded Linux without GLX)" OFF)
option(USE_VULKAN        "Use Vulkan for rendering (experimental, mutually exclusive with USE_OPENGL)" OFF)
option(USE_IPC_SOCKET    "Use Unix socket for IPC control (cross-platform)" ON)
option(WITH_LTO          "Enable Link Time Optimization" OFF)
option(WITH_OPTIMIZE     "Enable GCC native CPU optimizations" OFF)
option(BUNDLE_SDL        "Bundle SDL3 shared libraries into the package (for distros without SDL3)" OFF)

# Automatically disable PCem on architectures without a PCem backend
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "riscv"
   OR CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "loongarch")
    if(USE_PCEM)
        message(STATUS "Detected ${CMAKE_SYSTEM_PROCESSOR} architecture. Disabling USE_PCEM (no backend available).")
    else()
        message(STATUS "Detected ${CMAKE_SYSTEM_PROCESSOR} architecture. USE_PCEM already disabled.")
    endif()
    set(USE_PCEM OFF CACHE BOOL "Use PCem for hardware emulation (no backend for this architecture)" FORCE)
endif()

if(ANDROID)
    set(USE_UAENET_PCAP OFF CACHE BOOL "Use libpcap for uaenet (Linux/macOS)" FORCE)
    set(USE_UAENET_TAP OFF CACHE BOOL "TAP backend not supported on Android" FORCE)
    set(USE_DBUS OFF CACHE BOOL "Use DBus to control the emulator" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "Use Unix socket for IPC control" FORCE)
    set(USE_GPIOD OFF CACHE BOOL "Use GPIOD to control GPIO LEDs" FORCE)
    # Enabled features for Android
    set(USE_LIBMPEG2 OFF CACHE BOOL "Use LibMPEG2 for CD32 FMV support" FORCE)
    set(USE_MPG123 OFF CACHE BOOL "Disable mpg123 on Android" FORCE)
    # Android 15+ (API 35) requires 16KB page size alignment for Play Store
    add_link_options("-Wl,-z,max-page-size=16384")
endif()

# iOS detection: CMAKE_SYSTEM_NAME is "iOS" when using -DCMAKE_SYSTEM_NAME=iOS
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS TRUE)
    set(USE_UAENET_PCAP OFF CACHE BOOL "pcap not available on iOS" FORCE)
    set(USE_UAENET_TAP OFF CACHE BOOL "TAP backend not available on iOS" FORCE)
    set(USE_DBUS OFF CACHE BOOL "DBus not available on iOS" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "IPC socket not available on iOS" FORCE)
    set(USE_GPIOD OFF CACHE BOOL "GPIO not available on iOS" FORCE)
    set(USE_LIBSERIALPORT OFF CACHE BOOL "Serial port not available on iOS" FORCE)
    set(USE_PCEM OFF CACHE BOOL "PCem x86 emulation not available on iOS" FORCE)
    set(USE_LIBMPEG2 OFF CACHE BOOL "LibMPEG2 not available on iOS" FORCE)
    set(USE_VULKAN OFF CACHE BOOL "Vulkan not available on iOS (use Metal via SDL)" FORCE)
    set(USE_PORTMIDI OFF CACHE BOOL "PortMidi not available on iOS" FORCE)
    set(USE_LIBENET OFF CACHE BOOL "LibENET not available on iOS" FORCE)
    set(USE_ZSTD OFF CACHE BOOL "ZSTD not available on iOS (for now)" FORCE)
    # iOS requires OpenGL ES, not desktop OpenGL
    set(USE_OPENGL ON CACHE BOOL "Use OpenGL ES for rendering on iOS" FORCE)
    set(USE_GLES ON CACHE BOOL "Force OpenGL ES on iOS" FORCE)
    message(STATUS "iOS build enabled: sandbox features disabled")
endif()

# Vulkan and OpenGL are mutually exclusive rendering backends
# (checked after platform blocks so iOS/Android can force-disable Vulkan first)
if(USE_VULKAN AND USE_OPENGL)
    message(FATAL_ERROR "USE_VULKAN and USE_OPENGL are mutually exclusive. Please enable only one.")
endif()

if(WIN32)
    set(USE_GPIOD OFF CACHE BOOL "GPIO not available on Windows" FORCE)
    set(USE_DBUS OFF CACHE BOOL "DBus not available on Windows" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "Unix sockets not available on Windows" FORCE)
    set(USE_UAENET_PCAP OFF CACHE BOOL "Disable pcap on Windows initially" FORCE)
    set(USE_LIBMPEG2 OFF CACHE BOOL "LibMPEG2 not available in vcpkg" FORCE)
    set(USE_LIBSERIALPORT OFF CACHE BOOL "LibSerialPort not available in vcpkg for MinGW" FORCE)
    set(USE_PORTMIDI OFF CACHE BOOL "PortMidi not available in vcpkg for MinGW" FORCE)
endif()

# TAP backend is Linux-only (not macOS, FreeBSD, Windows, or Android)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(USE_UAENET_TAP OFF CACHE BOOL "TAP backend only available on Linux" FORCE)
endif()

# macOS helpers use Objective-C++ for Cocoa window activation.
if(CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT IOS)
    enable_language(OBJCXX)
endif()

# Mac App Store: enforce sandbox-compatible feature set
if(MACOS_APP_STORE)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
        message(FATAL_ERROR "MACOS_APP_STORE can only be used on macOS builds")
    endif()
    add_compile_definitions(MACOS_APP_STORE)
    set(USE_LIBSERIALPORT OFF CACHE BOOL "Serial port not available in App Store sandbox" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "IPC socket not available in App Store sandbox" FORCE)
    message(STATUS "Mac App Store build enabled: sandbox entitlements, serial/IPC disabled")
endif()

# Pre-release version suffix (project version is set in project() above)
set(VERSION_PRE_RELEASE "" CACHE STRING "Pre-release version")

# Project identity
set(AMIBERRY_DISPLAY_NAME "Amiberry")
set(PROJECT_COMPANY_NAME "BlitterStudio")
set(PROJECT_COMPANY_NAMESPACE "com.blitterstudio")

# FreeBSD-specific paths and libraries
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND AMIBERRY_PLATFORM_LINK_DIRS /usr/local/lib)

    find_library(LIBUSB_LIBRARY NAMES usb PATHS /usr/lib REQUIRED)
    if(NOT LIBUSB_LIBRARY)
        message(FATAL_ERROR "libusb not found in /usr/lib")
    endif()
endif()

# Install system and packaging
include(GNUInstallDirs)
if (WIN32)
    # Windows apps conventionally live directly under their install folder,
    # not in a Unix-style bin/ subdirectory.
    set(CMAKE_INSTALL_BINDIR ".")
    set(CMAKE_INSTALL_LIBDIR ".")
endif()
include(cmake/StandardProjectSettings.cmake)
include(cmake/SourceFiles.cmake)
include(cmake/Dependencies.cmake)

# Install rules may need dependency targets, such as SDL3 and SDL3_image,
# so include them after dependency discovery.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(cmake/linux/install.cmake)
elseif (IOS)
    include(cmake/ios/install.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(cmake/macos/install.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/windows/install.cmake)
endif ()

include(packaging/CMakeLists.txt)

# Link libraries (FreeBSD needs correct order)
if(TARGET amiberry AND CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_options(amiberry PRIVATE -L/usr/local/lib)
    # Apply FreeBSD pkg-config include/link dirs
    if(FREEBSD_INCLUDE_DIRS)
        target_include_directories(amiberry PRIVATE ${FREEBSD_INCLUDE_DIRS})
    endif()
    if(FREEBSD_LIBRARIES)
        target_link_libraries(amiberry PRIVATE ${FREEBSD_LIBRARIES})
    endif()

    # forkpty() lives in libutil on FreeBSD
    target_link_libraries(amiberry PRIVATE util)

    # libiconv_* symbols come from ports/converters/libiconv
    find_library(LIBICONV_LIBRARY NAMES iconv PATHS /usr/local/lib REQUIRED)
    target_link_libraries(amiberry PRIVATE ${LIBICONV_LIBRARY})

    # existing libs
    target_link_libraries(amiberry PRIVATE ${LIBUSB_LIBRARY} serialport ${EXTRA_LIBS})
endif()
