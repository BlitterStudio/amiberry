cmake_minimum_required(VERSION 3.16)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree build. Please create a build directory outside of the amiberry source code and run \"cmake -S ${CMAKE_SOURCE_DIR} -B .\" from there")
endif ()

# Use libgpiod to control GPIO LEDs?
option(USE_GPIOD "Use GPIOD" OFF)
# Use DBUS to control the emulator?
option(USE_DBUS "Use DBus" OFF)
# Use OpenGL for rendering? NOTE: Not yet implemented
option(USE_OPENGL "Use OpenGL" OFF)
# Enable Link Time Optimization?
option(WITH_LTO "Enable Link Time Optimization" OFF)

# Use LibSerialPort for serial port emulation?
option(USE_LIBSERIALPORT "Use LibSerialPort" ON)
# Use LibENET for network emulation?
option(USE_LIBENET "Use LibENET" ON)
# Use PortMidi for MIDI emulation?
option(USE_PORTMIDI "Use PortMidi" ON)
# Use LibMPEG2 for MPEG2 decoding (CD32 FMV)?
option(USE_LIBMPEG2 "Use LibMPEG2" ON)

## Determine proper semantic version
set(VERSION_MAJOR "7")
set(VERSION_MINOR "0")
set(VERSION_PATCH "3")
set(VERSION_PRE_RELEASE "")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

project(amiberry
        VERSION ${VERSION}
        LANGUAGES C CXX
        DESCRIPTION "Optimized Amiga emulator for various platforms"
        HOMEPAGE_URL "https://amiberry.com"
)
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
