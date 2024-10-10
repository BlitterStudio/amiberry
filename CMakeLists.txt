cmake_minimum_required(VERSION 3.16)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree build. Please create a build directory outside of the amiberry source code and run \"cmake -S ${CMAKE_SOURCE_DIR} -B .\" from there")
endif ()

# Use libgpiod to control GPIO LEDs?
option(USE_GPIOD "Use GPIOD" OFF)
# Use DBUS to control the emulator?
option(USE_DBUS "Use DBus" OFF)
# Use OpenGL for rendering?
option(USE_OPENGL "Use OpenGL" OFF)
# Enable Link Time Optimization?
option(WITH_LTO "Enable Link Time Optimization" OFF)

## Determine proper semantic version
set(VERSION_MAJOR "7")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(PROJECT_NAME "Amiberry")
else ()
    set(PROJECT_NAME "amiberry")
endif ()

project(${PROJECT_NAME}
        VERSION ${VERSION}
        LANGUAGES C CXX
        DESCRIPTION "Optimized Amiga emulator for various platforms"
        HOMEPAGE_URL "https://amiberry.com"
)
set(PROJECT_COMPANY_NAME "BlitterStudio")
set(PROJECT_COMPANY_NAMESPACE "com.blitterstudio")  # Reverse domain name notation

include(GNUInstallDirs)

include(cmake/StandardProjectSettings.cmake)
include(cmake/SourceFiles.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(packaging)
