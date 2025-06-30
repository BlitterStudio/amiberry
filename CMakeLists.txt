cmake_minimum_required(VERSION 3.16)

project(amiberry
    VERSION 7.1.0
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
option(USE_LIBSERIALPORT "Use LibSerialPort for serial port emulation" ON)
option(USE_LIBENET       "Use LibENET for network emulation" ON)
option(USE_PORTMIDI      "Use PortMidi for MIDI emulation" ON)
option(USE_LIBMPEG2      "Use LibMPEG2 for CD32 FMV support" ON)
option(USE_UAENET_PCAP   "Use libpcap for uaenet (Linux/macOS)" ON)
option(USE_GPIOD         "Use GPIOD to control GPIO LEDs" OFF)
option(USE_DBUS          "Use DBus to control the emulator" OFF)
option(USE_OPENGL        "Use OpenGL for rendering (currently BROKEN)" OFF)
option(WITH_LTO          "Enable Link Time Optimization" OFF)

# Versioning
set(VERSION_MAJOR "7" CACHE STRING "Major version")
set(VERSION_MINOR "1" CACHE STRING "Minor version")
set(VERSION_PATCH "0" CACHE STRING "Patch version")
set(VERSION_PRE_RELEASE "" CACHE STRING "Pre-release version")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# Use PkgConfig only on FreeBSD
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
	# Required packages via pkg-config
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2 REQUIRED sdl2)
	pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
	pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
	pkg_check_modules(FLAC REQUIRED flac)
	pkg_check_modules(PNG REQUIRED libpng)

	include_directories(
    		${SDL2_INCLUDE_DIRS}
    		${SDL2_TTF_INCLUDE_DIRS}
    		${SDL2_IMAGE_INCLUDE_DIRS}
    		${FLAC_INCLUDE_DIRS}
    		${PNG_INCLUDE_DIRS}
	)

	link_libraries(
    	${SDL2_LIBRARIES}
    	${SDL2_TTF_LIBRARIES}
    	${SDL2_IMAGE_LIBRARIES}
    	${FLAC_LIBRARIES}
    	${PNG_LIBRARIES}
	)
endif()
# macOS-specific identifier (optional cosmetic)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PROJECT_NAME "Amiberry")
    set(CMAKE_PROJECT_NAME "Amiberry")
endif()

# Project identity
set(PROJECT_COMPANY_NAME "BlitterStudio")
set(PROJECT_COMPANY_NAMESPACE "com.blitterstudio")

# Global definitions
add_definitions(-DLIBICONV_PLUG)

# FreeBSD-specific linking and paths
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    link_directories(/usr/local/lib)

    find_library(LIBUSB_LIBRARY NAMES usb PATHS /usr/lib REQUIRED)
    if(NOT LIBUSB_LIBRARY)
        message(FATAL_ERROR "libusb not found in /usr/lib")
    endif()
endif()

# Install system and packaging
include(GNUInstallDirs)
include(cmake/StandardProjectSettings.cmake)
include(cmake/SourceFiles.cmake)
include(cmake/Dependencies.cmake)
add_subdirectory(packaging)

# Link libraries (FreeBSD needs correct order)
if(TARGET amiberry AND CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_libraries(amiberry PRIVATE ${LIBUSB_LIBRARY} serialport ${EXTRA_LIBS})
endif()
