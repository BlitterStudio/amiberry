cmake_minimum_required(VERSION 3.16)

# Project setup
project(amiberry
        VERSION 7.1.0
        LANGUAGES C CXX
        DESCRIPTION "Optimized Amiga emulator for various platforms"
        HOMEPAGE_URL "https://amiberry.com"
)

# Set CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Prevent in-tree build (recommended for out-of-source builds)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree build. Please create a build directory outside of the amiberry source code and run \"cmake -S ${CMAKE_SOURCE_DIR} -B .\" from there")
endif ()

# --- Feature Options ---
# Emulation features
option(USE_LIBSERIALPORT "Use LibSerialPort for serial port emulation" ON)
option(USE_LIBENET "Use LibENET for network emulation" ON)
option(USE_PORTMIDI "Use PortMidi for MIDI emulation" ON)
option(USE_LIBMPEG2 "Use LibMPEG2 for MPEG2 decoding (CD32 FMV)" ON)
option(USE_UAENET_PCAP "Use libpcap for uaenet (Linux/macOS)" ON)

# Platform/Integration features
option(USE_GPIOD "Use GPIOD to control GPIO LEDs" OFF)
option(USE_DBUS "Use DBus to control the emulator" OFF)
option(USE_OPENGL "Use OpenGL for rendering (currently BROKEN)" OFF)

# Build options
option(WITH_LTO "Enable Link Time Optimization" OFF)

# --- Versioning ---
set(VERSION_MAJOR "7" CACHE STRING "Major version")
set(VERSION_MINOR "1" CACHE STRING "Minor version")
set(VERSION_PATCH "0" CACHE STRING "Patch version")
set(VERSION_PRE_RELEASE "" CACHE STRING "Pre-release version")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# Platform-specific settings
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PROJECT_NAME "Amiberry")
    set(CMAKE_PROJECT_NAME "Amiberry")
endif ()
set(PROJECT_COMPANY_NAME "BlitterStudio")
set(PROJECT_COMPANY_NAMESPACE "com.blitterstudio")  # Reverse domain name notation

include(GNUInstallDirs)

include(cmake/StandardProjectSettings.cmake)
include(cmake/SourceFiles.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(packaging)
