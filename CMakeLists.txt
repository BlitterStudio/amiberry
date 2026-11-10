cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW) # Normalize install() DESTINATION paths
endif()

# Before project(), which otherwise creates it empty
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json")
# Deployment target: must be set before project() so it is part of the
# initial compiler flags of every target, including the dependencies built
# from source (SDL3, SDL3_image, libpng, FLAC); setting it later no longer
# changes -m*-version-min. iOS defaults to 16.0, or 17.0 with the native
# UI, which targets the oldest systems with Swift Observation (iOS 17.0 /
# macOS 14.0). macOS without the native UI keeps the platform default.
option(AMIBERRY_NATIVE_APPLE_UI "Build the native Apple UI layer (Swift) into the app; macOS and iOS only" OFF)
# The built-in ImGui GUI and its overlays (the on-screen keyboard, the
# overlay frame): the native Apple apps have their own, and the Android
# app's launcher, menus, dialogs and on-screen keyboard replace it there;
# every other build has it.
option(AMIBERRY_NO_IMGUI "Build without the built-in ImGui GUI; the platform's own UI takes its place" OFF)
if(AMIBERRY_NATIVE_APPLE_UI OR AMIBERRY_NO_IMGUI)
    set(AMIBERRY_IMGUI OFF)
else()
    set(AMIBERRY_IMGUI ON)
endif()
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    if(DEFINED CMAKE_SYSTEM_NAME AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
        if(AMIBERRY_NATIVE_APPLE_UI)
            set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0")
        else()
            set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0")
        endif()
    elseif(AMIBERRY_NATIVE_APPLE_UI)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    endif()
endif()

project(amiberry
    VERSION 8.5.0
    LANGUAGES C CXX
    DESCRIPTION "Optimized Amiga emulator for various platforms"
    HOMEPAGE_URL "https://amiberry.com"
)

include(cmake/HostArchitecture.cmake)

# Enforce C++17 explicitly across all toolchains
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable PIC for all targets")

# Prevent in-tree builds
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-tree builds are not supported. Use a separate build directory.")
endif()

# Set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(CMakeDependentOption)

# iOS detection: CMAKE_SYSTEM_NAME is "iOS" when using -DCMAKE_SYSTEM_NAME=iOS.
# Keep this explicit because many build files gate iOS-only features on IOS.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS TRUE)
endif()

# Feature options
option(USE_86BOX        "Use 86Box for x86 bridgeboard and RTG hardware emulation" ON)
option(USE_PPC "Enable PowerPC accelerator board support" ON)
option(USE_QEMU_PPC "Enable QEMU-UAE PowerPC plugin support" ON)
option(USE_LIBSERIALPORT "Use LibSerialPort for serial port emulation" ON)
option(USE_LIBENET       "Use LibENET for network emulation" ON)
option(USE_PORTMIDI      "Use PortMidi for MIDI emulation" ON)
option(USE_LIBMPEG2      "Use LibMPEG2 for CD32 FMV support" ON)
option(USE_MPG123        "Use mpg123 for MP3 decoding" ON)
option(USE_VIDEOGRAB     "Use SDL3 genlock video and camera input" ON)
option(USE_FFMPEG        "Use FFmpeg 5.1+ media decoding" ON)
option(USE_WEBP          "Use libwebp for still and animated WebP decoding" ON)
option(USE_UAENET_PCAP   "Use libpcap for uaenet (Linux/macOS)" ON)
option(USE_UAENET_TAP    "Use TAP backend for uaenet (Linux only)" ON)
option(USE_ZSTD 	     "Use Zstandard for CHD compressed disk images" ON)
option(USE_GPIOD         "Use GPIOD to control GPIO LEDs" OFF)
option(USE_OPENGL        "Use OpenGL for rendering" ON)
cmake_dependent_option(USE_GLES "Use OpenGL ES instead of desktop OpenGL (for embedded Linux without GLX)" OFF "USE_OPENGL" OFF)
option(USE_VULKAN        "Use Vulkan for rendering (experimental, mutually exclusive with USE_OPENGL)" OFF)
option(USE_IPC_SOCKET    "Use a local socket for IPC control (Unix socket; named pipe on Windows)" ON)
option(USE_JIT           "Enable 68K JIT recompiler (x86_64 and AArch64)" ON)
option(WITH_LTO          "Enable Link Time Optimization" OFF)
option(WITH_PGO_GENERATE "Generate profile data for Profile-Guided Optimization" OFF)
option(WITH_PGO_USE      "Use profile data for Profile-Guided Optimization" OFF)
option(WITH_OPTIMIZE     "Enable native CPU optimizations" OFF)
option(WITH_STRIP        "Strip symbols from the Release binary" ON)
option(BUNDLE_SDL        "Bundle SDL3 shared libraries into the package (for distros without SDL3)" OFF)
# A build without the built-in GUI has no ImGui, which the Vulkan
# renderer's overlay needs.
if(NOT AMIBERRY_IMGUI AND USE_VULKAN)
    message(FATAL_ERROR "This build has no ImGui (AMIBERRY_NATIVE_APPLE_UI or AMIBERRY_NO_IMGUI), which the Vulkan renderer's overlay needs: build it with USE_VULKAN=OFF")
endif()
if(AMIBERRY_NO_IMGUI AND NOT ANDROID)
    message(FATAL_ERROR "AMIBERRY_NO_IMGUI needs the Android app's UI; other platforms build the built-in GUI (the Apple apps use AMIBERRY_NATIVE_APPLE_UI)")
endif()
if(AMIBERRY_NATIVE_APPLE_UI AND NOT APPLE)
    message(FATAL_ERROR "AMIBERRY_NATIVE_APPLE_UI requires an Apple platform (macOS or iOS)")
endif()

# Deployment target applied to the app target under the Xcode generator.
set(AMIBERRY_IOS_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}")
set(PGO_PROFILE_DIR "" CACHE PATH "Directory containing Profile-Guided Optimization data")
set(QEMU_UAE_PLUGIN "" CACHE FILEPATH "Path to a prebuilt qemu-uae plugin artifact to bundle")
set(QEMU_UAE_PLUGIN_DIR "" CACHE PATH "Directory containing the QEMU-UAE plugin artifact and any sidecar runtime files")
if(NOT QEMU_UAE_PLUGIN_DIR AND DEFINED ENV{QEMU_UAE_PLUGIN_DIR})
    set(QEMU_UAE_PLUGIN_DIR "$ENV{QEMU_UAE_PLUGIN_DIR}" CACHE PATH "Directory containing the QEMU-UAE plugin artifact and any sidecar runtime files" FORCE)
endif()

# Vulkan and OpenGL are mutually exclusive rendering backends.
if(USE_VULKAN AND USE_OPENGL)
    message(WARNING "USE_VULKAN and USE_OPENGL are mutually exclusive. Disabling USE_OPENGL.")
    set(USE_OPENGL OFF CACHE BOOL "Use OpenGL for rendering" FORCE)
endif()

# PCem was replaced by 86Box: warn about the removed option instead of
# silently aliasing it to the new one.
if(DEFINED USE_PCEM)
    message(WARNING "USE_PCEM was removed: Amiberry uses 86Box for the x86 bridgeboards and RTG boards now. Set USE_86BOX instead; this USE_PCEM value is ignored.")
endif()

# 86Box's x86 recompiler for the bridgeboard PCs. It has one backend per
# binary, so multi-architecture builds (macOS universal) and other
# architectures (RISC-V, LoongArch) keep the interpreter.
set(AMIBERRY_86BOX_TARGET_ARCHES "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(AMIBERRY_86BOX_TARGET_ARCHES "${CMAKE_OSX_ARCHITECTURES}")
endif()
set(AMIBERRY_86BOX_DYNAREC_BACKEND "")
list(LENGTH AMIBERRY_86BOX_TARGET_ARCHES AMIBERRY_86BOX_TARGET_ARCH_COUNT)
if(AMIBERRY_86BOX_TARGET_ARCH_COUNT EQUAL 1)
    if(AMIBERRY_86BOX_TARGET_ARCHES MATCHES "^(aarch64|arm64)$")
        set(AMIBERRY_86BOX_DYNAREC_BACKEND arm64)
    elseif(AMIBERRY_86BOX_TARGET_ARCHES MATCHES "^(x86_64|amd64|AMD64)$")
        set(AMIBERRY_86BOX_DYNAREC_BACKEND x86-64)
    endif()
endif()
cmake_dependent_option(USE_86BOX_DYNAREC "Build the 86Box x86 recompiler for the bridgeboard PCs (single-arch x86-64/arm64 binaries)" ON "USE_86BOX" OFF)
if(USE_86BOX_DYNAREC AND NOT AMIBERRY_86BOX_DYNAREC_BACKEND)
    message(STATUS "The 86Box x86 recompiler has no backend for ${AMIBERRY_86BOX_TARGET_ARCHES}; the bridgeboards use the interpreter")
    set(USE_86BOX_DYNAREC OFF CACHE BOOL "Build the 86Box x86 recompiler for the bridgeboard PCs" FORCE)
endif()

# Platform-specific feature gating
if(ANDROID)
    set(USE_VIDEOGRAB OFF CACHE BOOL "Genlock video input is not supported on Android yet" FORCE)
    set(USE_FFMPEG OFF CACHE BOOL "FFmpeg media decoding is not supported on Android yet" FORCE)
    set(USE_WEBP OFF CACHE BOOL "WebP media decoding is not supported on Android yet" FORCE)
    set(USE_UAENET_PCAP OFF CACHE BOOL "Use libpcap for uaenet (Linux/macOS)" FORCE)
    set(USE_UAENET_TAP OFF CACHE BOOL "TAP backend not supported on Android" FORCE)
    set(USE_IPC_SOCKET OFF CACHE BOOL "Use Unix socket for IPC control" FORCE)
    set(USE_GPIOD OFF CACHE BOOL "Use GPIOD to control GPIO LEDs" FORCE)
    set(USE_LIBMPEG2 OFF CACHE BOOL "Use LibMPEG2 for CD32 FMV support" FORCE)
    set(USE_MPG123 OFF CACHE BOOL "Disable mpg123 on Android" FORCE)
    set(USE_PPC OFF CACHE BOOL "PPC accelerator support requires the QEMU-UAE plugin on Android" FORCE)
    set(USE_QEMU_PPC OFF CACHE BOOL "QEMU-UAE PPC plugin loading is not supported on Android yet" FORCE)
    # Android 15+ (API 35) requires 16KB page size alignment for Play Store
    add_link_options("-Wl,-z,max-page-size=16384")
elseif(IOS)
    set(USE_VIDEOGRAB OFF CACHE BOOL "Genlock video input is not supported on iOS yet" FORCE)
    set(USE_FFMPEG OFF CACHE BOOL "FFmpeg media decoding is not supported on iOS yet" FORCE)
    set(USE_WEBP OFF CACHE BOOL "WebP media decoding is not supported on iOS yet" FORCE)
    set(USE_UAENET_PCAP OFF CACHE BOOL "pcap not available on iOS" FORCE)
    set(USE_UAENET_TAP OFF CACHE BOOL "TAP backend not available on iOS" FORCE)
    # A simulator app is a host process, so the host's IPC tools can drive
    # it; test builds only, never for a device or a release.
    option(AMIBERRY_IOS_SIMULATOR_IPC "Serve the IPC socket in an iOS simulator test build" OFF)
    if(AMIBERRY_IOS_SIMULATOR_IPC AND NOT CMAKE_OSX_SYSROOT MATCHES "simulator")
        message(FATAL_ERROR "AMIBERRY_IOS_SIMULATOR_IPC is for simulator builds (-DCMAKE_OSX_SYSROOT=iphonesimulator)")
    endif()
    if(AMIBERRY_IOS_SIMULATOR_IPC)
        # Also over the OFF an earlier configure of this build forced.
        set(USE_IPC_SOCKET ON CACHE BOOL "IPC socket for the iOS simulator test build" FORCE)
    else()
        set(USE_IPC_SOCKET OFF CACHE BOOL "IPC socket not available on iOS" FORCE)
    endif()
    set(USE_GPIOD OFF CACHE BOOL "GPIO not available on iOS" FORCE)
    set(USE_LIBSERIALPORT OFF CACHE BOOL "Serial port not available on iOS" FORCE)
    set(USE_86BOX OFF CACHE BOOL "86Box x86 emulation not available on iOS" FORCE)
    set(USE_PPC OFF CACHE BOOL "PPC accelerator support is not available on iOS yet" FORCE)
    set(USE_QEMU_PPC OFF CACHE BOOL "Dynamic QEMU-UAE PPC plugins are not available on iOS" FORCE)
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

if(QEMU_UAE_PLUGIN)
    if(NOT USE_PPC OR NOT USE_QEMU_PPC)
        message(FATAL_ERROR "QEMU_UAE_PLUGIN requires USE_PPC=ON and USE_QEMU_PPC=ON")
    endif()
    get_filename_component(QEMU_UAE_PLUGIN "${QEMU_UAE_PLUGIN}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
    if(NOT EXISTS "${QEMU_UAE_PLUGIN}")
        message(FATAL_ERROR "QEMU_UAE_PLUGIN does not exist: ${QEMU_UAE_PLUGIN}")
    endif()
    set(QEMU_UAE_PLUGIN "${QEMU_UAE_PLUGIN}" CACHE FILEPATH "Path to a prebuilt qemu-uae plugin artifact to bundle" FORCE)
    if(QEMU_UAE_PLUGIN_DIR)
        get_filename_component(QEMU_UAE_PLUGIN_DIR "${QEMU_UAE_PLUGIN_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        if(NOT IS_DIRECTORY "${QEMU_UAE_PLUGIN_DIR}")
            message(FATAL_ERROR "QEMU_UAE_PLUGIN_DIR is not a directory: ${QEMU_UAE_PLUGIN_DIR}")
        endif()
        set(QEMU_UAE_PLUGIN_DIR "${QEMU_UAE_PLUGIN_DIR}" CACHE PATH "Directory containing the QEMU-UAE plugin artifact and any sidecar runtime files" FORCE)
    endif()
    message(STATUS "Bundling QEMU-UAE PPC plugin: ${QEMU_UAE_PLUGIN}")
endif()

# macOS and iOS helpers use Objective-C++ (Cocoa windows, NSURLSession).
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
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
    # FFmpeg brings its own OpenSSL, which the App Store's export compliance
    # counts as non-exempt encryption; the store build goes without genlock
    # video files and the ZZ9000 SDK's media codecs.
    set(USE_FFMPEG OFF CACHE BOOL "FFmpeg is left out of Mac App Store builds" FORCE)
    # Only the system's encryption is left (curl's HTTPS): exempt.
    set(AMIBERRY_MACOS_STORE_PLIST_KEYS "<key>ITSAppUsesNonExemptEncryption</key>
        <false/>")
    message(STATUS "Mac App Store build enabled: sandbox entitlements, serial/IPC/FFmpeg disabled")
endif()

# Pre-release version suffix
set(VERSION_PRE_RELEASE "" CACHE STRING "Pre-release version")
set(AMIBERRY_PRODUCT_VERSION "${PROJECT_VERSION}")
if(VERSION_PRE_RELEASE)
    set(AMIBERRY_PRODUCT_VERSION "${PROJECT_VERSION}-${VERSION_PRE_RELEASE}")
endif()

# Project identity & metadata
set(AMIBERRY_DISPLAY_NAME "Amiberry")
set(PROJECT_COMPANY_NAME "BlitterStudio")
set(PROJECT_COMPANY_NAMESPACE "com.blitterstudio")
file(WRITE "${CMAKE_BINARY_DIR}/amiberry-product-version.txt" "${AMIBERRY_PRODUCT_VERSION}\n")

# Which catalogue distribution the Apple licences screen shows. The source
# offer itself is generated at build time by cmake/SourceOffer.cmake, so it
# always names the revision this binary was actually built from.
if(IOS)
    set(AMIBERRY_DISTRIBUTION "ios")
elseif(MACOS_APP_STORE)
    set(AMIBERRY_DISTRIBUTION "macos-store")
elseif(APPLE)
    set(AMIBERRY_DISTRIBUTION "macos")
else()
    set(AMIBERRY_DISTRIBUTION "other")
endif()

# FreeBSD-specific paths and libraries
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND AMIBERRY_PLATFORM_LINK_DIRS /usr/local/lib)
    find_library(LIBUSB_LIBRARY NAMES usb PATHS /usr/lib REQUIRED)
endif()

# Install system and packaging
# Haiku packages install into /boot/system using the platform's own
# hierarchy: data files under data/, documentation under documentation/,
# man pages under documentation/man (GNUInstallDirs would otherwise use
# the GNU share/ layout everywhere). Seed the cache before the include so
# GNUInstallDirs adopts these instead of its GNU defaults.
if (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    set(CMAKE_INSTALL_DATADIR "data" CACHE PATH "Object read-only architecture-independent data")
    set(CMAKE_INSTALL_DOCDIR "documentation/packages/${PROJECT_NAME}" CACHE PATH "Documentation root")
    set(CMAKE_INSTALL_MANDIR "documentation/man" CACHE PATH "Man documentation root")
endif ()
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

# Native Apple UI layer (Swift). Included after dependency discovery so the
# Swift static library links into the fully-populated amiberry target.
if(AMIBERRY_NATIVE_APPLE_UI)
    include(cmake/AppleNativeUI.cmake)
endif()

# Install rules may need dependency targets, such as SDL3 and SDL3_image,
# so include them after dependency discovery.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    include(cmake/linux/install.cmake)
elseif (IOS)
    include(cmake/ios/install.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(cmake/macos/install.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(cmake/windows/install.cmake)
endif ()

include(packaging/CMakeLists.txt)
