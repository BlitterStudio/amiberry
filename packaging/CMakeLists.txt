# Base package settings
if (NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif ()
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_IGNORE_FILES .git/ .github/ .claude/ android/ _CPack_Packages/)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_PACKAGE_NAME ${AMIBERRY_DISPLAY_NAME})
else()
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
endif()
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Include pre-release suffix in the package version if set.
# DEB uses ~ (sorts lower than release). RPM cannot have - or ~ in Version,
# so the pre-release tag goes into CPACK_RPM_PACKAGE_RELEASE instead.
if(VERSION_PRE_RELEASE)
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}~pre${VERSION_PRE_RELEASE}")
    set(CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_RPM_PACKAGE_RELEASE "0.pre${VERSION_PRE_RELEASE}")
else()
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()

set(CPACK_PACKAGE_VENDOR "BlitterStudio")
set(CPACK_PACKAGE_CONTACT "Dimitris Panokostas <midwan@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION
        "Optimized Amiga emulator for various ARM64, AMD64 and RISC-V platforms.
Amiberry is an optimized Amiga emulator for various platforms. It is based on
the latest WinUAE and supports various Amiga models, including the A4000T,
A4000D, A1200, A3000 and A600. Additionally, it includes several unique
features, such as support for WHDLoad titles, RetroArch integration, custom
Controller Mappings and more.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/packaging/Description.txt")

# macOS settings for DragNDrop generator
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
set(CPACK_DMG_VOLUME_NAME "${AMIBERRY_DISPLAY_NAME}")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/packaging/dmg/AppDMGBackground.tiff")
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/packaging/dmg/AppDMGSetup.scpt")

# Linux DEB settings - build dependency list conditionally based on enabled features
set(_deb_deps "libc6, libstdc++6, libsdl2-2.0-0, libsdl2-image-2.0-0, libsdl2-ttf-2.0-0, flac, libmpg123-0, libpng16-16, zlib1g")
if(USE_ZSTD)
    string(APPEND _deb_deps ", libzstd1")
endif()
if(USE_LIBSERIALPORT)
    string(APPEND _deb_deps ", libserialport0")
endif()
if(USE_PORTMIDI)
    string(APPEND _deb_deps ", libportmidi0")
endif()
if(USE_LIBENET)
    string(APPEND _deb_deps ", libenet7")
endif()
if(USE_LIBMPEG2)
    string(APPEND _deb_deps ", libmpeg2-4")
endif()
if(USE_UAENET_PCAP)
    string(APPEND _deb_deps ", libpcap0.8")
endif()
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${_deb_deps}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "riscv64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "otherosfs")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/linux/scripts/postinst"
        "${CMAKE_SOURCE_DIR}/packaging/linux/scripts/postrm")

# Linux RPM settings - build dependency list conditionally based on enabled features
set(_rpm_deps "glibc, libstdc++, SDL2, SDL2_ttf, SDL2_image, flac, mpg123, libpng, zlib")
if(USE_ZSTD)
    string(APPEND _rpm_deps ", zstd")
endif()
if(USE_LIBSERIALPORT)
    string(APPEND _rpm_deps ", libserialport")
endif()
if(USE_PORTMIDI)
    string(APPEND _rpm_deps ", portmidi")
endif()
if(USE_LIBENET)
    string(APPEND _rpm_deps ", enet")
endif()
if(USE_LIBMPEG2)
    string(APPEND _rpm_deps ", libmpeg2")
endif()
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_REQUIRES "${_rpm_deps}")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/linux/scripts/rpm-postinst.sh")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/linux/scripts/rpm-postrm.sh")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "armhf")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "riscv64")
else()
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
endif()

# Package resources
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/docs/README.md")

# Strip binaries
set(CPACK_STRIP_FILES TRUE)

# Generator selection per platform
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_GENERATOR TGZ DragNDrop)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CPACK_GENERATOR TGZ DEB RPM)
else ()
    set(CPACK_GENERATOR TGZ)
endif ()

include(CPack)
