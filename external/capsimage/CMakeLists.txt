# CMakeList.txt
# CAPSImage library + public headers only

cmake_minimum_required(VERSION 3.10)

# read CAPSImage version from header file
set(CAPSLIB_VERSION_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsLibVersion.h")

function(capsimage_read_define out_var define_name)
    file(STRINGS "${CAPSLIB_VERSION_HEADER}" line
        REGEX "^[ \t]*#[ \t]*define[ \t]+${define_name}[ \t]+[0-9]+")

    if(NOT line)
        message(FATAL_ERROR "Missing ${define_name} in ${CAPSLIB_VERSION_HEADER}")
    endif()

    string(REGEX REPLACE
        "^.*${define_name}[ \t]+([0-9]+).*$"
        "\\1"
        value
        "${line}")

    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

capsimage_read_define(CAPSIMAGE_VERSION_MAJOR CAPS_LIB_RELEASE)
capsimage_read_define(CAPSIMAGE_VERSION_MINOR CAPS_LIB_REVISION)
set(CAPSIMAGE_VERSION_PATCH 0)

set(CAPSIMAGE_VERSION
    "${CAPSIMAGE_VERSION_MAJOR}.${CAPSIMAGE_VERSION_MINOR}.${CAPSIMAGE_VERSION_PATCH}")


if(APPLE)
    option(CAPSIMAGE_MACOS_UNIVERSAL
        "Build macOS universal binary for x86_64 and arm64"
        OFF)

    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.5"
        CACHE STRING "macOS deployment target" FORCE)

    if(CAPSIMAGE_MACOS_UNIVERSAL)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64"
            CACHE STRING "macOS architectures" FORCE)

        message(STATUS "Configuring macOS Universal Build: ${CMAKE_OSX_ARCHITECTURES}")
    else()
        message(STATUS "Configuring macOS native architecture build")
    endif()
endif()

# Keep the project name stable because it is used for install/export paths.
project(CAPSImage VERSION ${CAPSIMAGE_VERSION} LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

#include(FetchContent)
#set(FMT_INSTALL OFF CACHE INTERNAL "Disable fmt install")

#FetchContent_Declare(
#    fmt
#    GIT_REPOSITORY https://github.com/fmtlib/fmt
#    GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281 # 10.2.1
#)
#FetchContent_MakeAvailable(fmt)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

set(CAPSIMAGE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Codec/CTRawCodec.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Codec/CTRawCodecDecompressor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Codec/DiskEncoding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/BaseFile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/BitBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/CRC.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/DiskFile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/MemoryFile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CapsAPI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CapsFDCEmulator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CapsFormatMFM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CapsImage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CapsImageStd.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CAPSImg.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/CapsLoader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/DiskImage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/DiskImageFactory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/StreamCueImage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/StreamImage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg/stdafx.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Compatibility/Posix/posixfix.cpp"
)

set(CAPSIMAGE_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsAPI.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsFDC.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsForm.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsLib.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsLibAll.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/CapsLibVersion.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LibIPF/ComLib.h"
)

# -----------------------------------------------------------------------------
# Target: runtime library
# -----------------------------------------------------------------------------

add_library(CAPSImage SHARED ${CAPSIMAGE_SOURCES})
add_library(CAPSImage::CAPSImage ALIAS CAPSImage)

set_target_properties(CAPSImage PROPERTIES
    VERSION     "${PROJECT_VERSION}"
    SOVERSION   "${PROJECT_VERSION_MAJOR}"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    OUTPUT_NAME CAPSImage
)

target_include_directories(CAPSImage
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Core"
        "${CMAKE_CURRENT_SOURCE_DIR}/Compatibility/Posix"
        "${CMAKE_CURRENT_SOURCE_DIR}/Compatibility/GCC4x"
        "${CMAKE_CURRENT_SOURCE_DIR}/Codec"
        "${CMAKE_CURRENT_SOURCE_DIR}/CAPSImg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Device"
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LibIPF>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/CAPSImage>"
)

#target_link_libraries(CAPSImage PRIVATE fmt::fmt)

# Static libgcc/libstdc++ helps distributing binaries across older Linux distros.
if(NOT APPLE)
    target_link_options(CAPSImage PRIVATE -static-libgcc -static-libstdc++)
endif()

# -----------------------------------------------------------------------------
# Target: public headers / development usage
# -----------------------------------------------------------------------------

add_library(CAPSImageHeaders INTERFACE)
add_library(CAPSImage::Headers ALIAS CAPSImageHeaders)

#target_sources(CAPSImageHeaders INTERFACE ${CAPSIMAGE_PUBLIC_HEADERS})

target_include_directories(CAPSImageHeaders INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LibIPF>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/CAPSImage>"
)

# Optional convenience target: consumers can link one target and get both the
# shared library and the public include path.
add_library(CAPSImageDev INTERFACE)
add_library(CAPSImage::Dev ALIAS CAPSImageDev)
target_link_libraries(CAPSImageDev INTERFACE CAPSImage::CAPSImage CAPSImage::Headers)

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------

message(STATUS "Library install dir: ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "Header install dir:  ${CMAKE_INSTALL_INCLUDEDIR}/CAPSImage")

install(TARGETS CAPSImage
    EXPORT CAPSImageTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
)

install(TARGETS CAPSImageHeaders CAPSImageDev
    EXPORT CAPSImageTargets
)

install(FILES ${CAPSIMAGE_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CAPSImage
    COMPONENT Development
)

install(EXPORT CAPSImageTargets
    NAMESPACE CAPSImage::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CAPSImage
    COMPONENT Development
)

if(NOT ANDROID AND UNIX AND NOT APPLE)
    # Backwards-compatible lowercase .so symlink.
    install(CODE [[
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E create_symlink libCAPSImage.so libcapsimage.so
            WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        )
    ]] COMPONENT Runtime)
endif()

# -----------------------------------------------------------------------------
# CPack
# -----------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(CPACK_GENERATOR FREEBSD)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR TGZ DEB RPM)
endif()

set(CPACK_PACKAGE_NAME "capsimage")
set(CPACK_PACKAGE_CONTACT "info@kryoflux.com")
set(CPACK_PACKAGE_VENDOR "KryoFlux Products & Services Ltd.")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://www.kryoflux.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CAPSImage shared library")
set(CPACK_PACKAGE_DESCRIPTION "CAPSImage shared library\nCopyright (c) 2010-2026 KryoFlux Products & Services Ltd")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_STRIP_FILES ON)

set(CPACK_COMPONENTS_ALL Runtime Development)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_MONOLITHIC_INSTALL OFF)

set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "CAPSImage runtime library")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "CAPSImage shared library runtime files")
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "CAPSImage development headers")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "CAPSImage public headers and CMake export files")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS Runtime)

# DEB component packages: capsimage + capsimage-dev
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "capsimage")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "capsimage-dev")
set(CPACK_DEBIAN_RUNTIME_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_DEVELOPMENT_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS "capsimage (= ${PROJECT_VERSION})")

# RPM component packages: capsimage + capsimage-devel
set(CPACK_RPM_RUNTIME_PACKAGE_NAME "capsimage")
set(CPACK_RPM_DEVELOPMENT_PACKAGE_NAME "capsimage-devel")
set(CPACK_RPM_RUNTIME_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_DEVELOPMENT_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "non-free binary")
set(CPACK_RPM_RUNTIME_PACKAGE_GROUP "System/Libraries")
set(CPACK_RPM_DEVELOPMENT_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_RUNTIME_PACKAGE_PROVIDES "libCAPSImage.so.5")
set(CPACK_RPM_DEVELOPMENT_PACKAGE_REQUIRES "capsimage = ${PROJECT_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}")
set(CPACK_RPM_PACKAGE_RELEASE 1)

# FreeBSD PKG setup
set(CPACK_FREEBSD_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

include(CPack)
