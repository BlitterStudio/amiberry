cmake_minimum_required(VERSION 3.16)

project(amiberry
    VERSION 8.0.0
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
option(USE_IPC_SOCKET    "Use Unix socket for IPC control (cross-platform)" ON)
option(WITH_LTO          "Enable Link Time Optimization" OFF)
option(WITH_OPTIMIZE     "Enable GCC native CPU optimizations" OFF)
option(USE_IMGUI         "Use ImGui for the GUI" ON)

# Automatically disable PCem on RISC-V targets
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "riscv")
    if(USE_PCEM)
        message(STATUS "Detected RISC-V architecture (${CMAKE_SYSTEM_PROCESSOR}). Disabling USE_PCEM.")
    else()
        message(STATUS "Detected RISC-V architecture (${CMAKE_SYSTEM_PROCESSOR}). USE_PCEM already disabled.")
    endif()
    set(USE_PCEM OFF CACHE BOOL "Use PCem for hardware emulation (no support for RISC-V)" FORCE)
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

# Pre-release version suffix (project version is set in project() above)
set(VERSION_PRE_RELEASE "23" CACHE STRING "Pre-release version")

# Use PkgConfig only on FreeBSD
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
	# Required packages via pkg-config
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2 REQUIRED sdl2)
	pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
	pkg_check_modules(FLAC REQUIRED flac)
	pkg_check_modules(PNG REQUIRED libpng)
  pkg_check_modules(SERIALPORT REQUIRED libserialport)

	# Store FreeBSD include/link info for target-specific application in SourceFiles.cmake
	set(FREEBSD_INCLUDE_DIRS
    	${SDL2_INCLUDE_DIRS}
    	${SDL2_IMAGE_INCLUDE_DIRS}
    	${FLAC_INCLUDE_DIRS}
    	${PNG_INCLUDE_DIRS}
	)
	set(FREEBSD_LIBRARIES
    	${SDL2_LIBRARIES}
    	${SDL2_IMAGE_LIBRARIES}
    	${FLAC_LIBRARIES}
    	${PNG_LIBRARIES}
	)
endif()

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
include(cmake/StandardProjectSettings.cmake)
include(cmake/SourceFiles.cmake)
include(cmake/Dependencies.cmake)
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
