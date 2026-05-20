cmake_minimum_required(VERSION 3.16)

# Enforce modern CMake behavior for find_package and policy resolution
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW) # MSVC runtime selection
endif()
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW) # Normalize install() DESTINATION paths
endif()

project(amiberry
    VERSION 8.1.6
    LANGUAGES C CXX
    DESCRIPTION "Optimized Amiga emulator for various platforms"
    HOMEPAGE_URL "https://amiberry.com"
)

# Enforce C++17 explicitly across all toolchains
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# IDE & Tooling support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable PIC for all targets" FORCE)

# Prevent in-tree builds
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-tree builds are not supported. Use a separate build directory.")
endif()

# Set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(CMakeDependentOption)

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
option(USE_JIT           "Enable 68K JIT recompiler (x86_64 and AArch64)" ON)
option(WITH_LTO          "Enable Link Time Optimization" OFF)
option(WITH_OPTIMIZE     "Enable GCC native CPU optimizations" OFF)
option(BUNDLE_SDL        "Bundle SDL3 shared libraries into the package (for distros without SDL3)" OFF)

# Handle dependent options gracefully
cmake_dependent_option(USE_GLES "Force OpenGL ES on target" ON "USE_OPENGL" OFF)
if(USE_VULKAN AND USE_OPENGL)
    message(WARNING "USE_VULKAN and USE_OPENGL are mutually exclusive. Disabling USE_OPENGL.")
    set(USE_OPENGL OFF CACHE BOOL "Use OpenGL for rendering" FORCE)
endif()

# Automatically disable PCem on architectures without a PCem backend
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCH_LOWER)
if(ARCH_LOWER MATCHES "riscv|loongarch")
    if(USE_PCEM)
        message(STATUS "Detected ${CMAKE_SYSTEM_PROCESSOR} architecture. Disabling USE_PCEM (no backend available).")
    endif()
    set(USE_PCEM OFF CACHE BOOL "Use PCem for hardware emulation (no backend for this architecture)" FORCE)
endif()

# Platform-specific feature gating
if(ANDROID)
    set(USE_UAENET_PCAP OFF CACHE BOOL "Use libpcap for uaenet (Linux/macOS)" FORCE)
    set(USE_UAENET_TAP OFF CACHE BOOL "TAP backend not supported on Android" FORCE)
    set(USE_DBUS OFF CACHE BOOL "Use DBus to control the emulator" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "Use Unix socket for IPC control" FORCE)
    set(USE_GPIOD OFF CACHE BOOL "Use GPIOD to control GPIO LEDs" FORCE)
    set(USE_LIBMPEG2 OFF CACHE BOOL "Use LibMPEG2 for CD32 FMV support" FORCE)
    set(USE_MPG123 OFF CACHE BOOL "Disable mpg123 on Android" FORCE)
    # Android 15+ (API 35) requires 16KB page size alignment for Play Store
    add_link_options("-Wl,-z,max-page-size=16384")
elseif(IOS)
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
    set(USE_OPENGL ON CACHE BOOL "Use OpenGL ES for rendering on iOS" FORCE)
    set(USE_GLES ON CACHE BOOL "Force OpenGL ES on iOS" FORCE)
    message(STATUS "iOS build enabled: sandbox features disabled")
elseif(WIN32)
    set(USE_GPIOD OFF CACHE BOOL "GPIO not available on Windows" FORCE)
    set(USE_DBUS OFF CACHE BOOL "DBus not available on Windows" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "Unix sockets not available on Windows" FORCE)
    set(USE_UAENET_PCAP OFF CACHE BOOL "Disable pcap on Windows initially" FORCE)
    set(USE_LIBMPEG2 OFF CACHE BOOL "LibMPEG2 not available in vcpkg" FORCE)
    set(USE_LIBSERIALPORT OFF CACHE BOOL "LibSerialPort not available in vcpkg for MinGW" FORCE)
    set(USE_PORTMIDI OFF CACHE BOOL "PortMidi not available in vcpkg for MinGW" FORCE)

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(USE_MPG123 OFF CACHE BOOL "mpg123 vcpkg port broken on Windows ARM64 mingw" FORCE)
        message(STATUS "Windows ARM64 detected: mpg123 disabled")
    endif()
elseif(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

# Pre-release version suffix
set(VERSION_PRE_RELEASE "" CACHE STRING "Pre-release version")
if(VERSION_PRE_RELEASE)
    set(PROJECT_VERSION "${PROJECT_VERSION}-${VERSION_PRE_RELEASE}")
endif()

# Project identity & metadata
set(AMIBERRY_DISPLAY_NAME "Amiberry")
set(PROJECT_COMPANY_NAME "BlitterStudio")
set(PROJECT_COMPANY_NAMESPACE "com.blitterstudio")
file(WRITE "${CMAKE_BINARY_DIR}/amiberry-product-version.txt" "${PROJECT_VERSION}\n")

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
set(AMIBERRY_PACKAGE_METADATA_EXCLUDES
    PATTERN ".DS_Store" EXCLUDE
    PATTERN "._*" EXCLUDE
    PATTERN "__MACOSX" EXCLUDE
    PATTERN ".AppleDouble" EXCLUDE
    PATTERN ".LSOverride" EXCLUDE
    PATTERN ".Spotlight-V100" EXCLUDE
    PATTERN ".Trashes" EXCLUDE
    PATTERN ".fseventsd" EXCLUDE
)
include(cmake/StandardProjectSettings.cmake)
include(cmake/SourceFiles.cmake)
include(cmake/Dependencies.cmake)

# Install rules may need dependency targets, such as SDL3 and SDL3_image,
# so include them after dependency discovery.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    include(cmake/linux/install.cmake)
elseif (IOS)
    include(cmake/ios/install.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(cmake/macos/install.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/windows/install.cmake)
endif ()

include(packaging/CMakeLists.txt)
