# Base package settings
if (NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif ()
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_IGNORE_FILES .git/ .github/ .vscode/ _CPack_Packages/)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR "BlitterStudio")
set(CPACK_PACKAGE_CONTACT "Dimitris Panokostas <midwan@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION
        "Optimized Amiga emulator for various ARM, ARM64, AMD64 and RISC-V platforms.
Amiberry is an optimized Amiga emulator for various platforms. It is based on
the latest WinUAE and supports various Amiga models, including the A4000T,
A4000D, A1200, A3000 and A600. Additionally, it includes several unique
features, such as support for WHDLoad titles, RetroArch integration, custom
Controller Mappings and more.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/packaging/Description.txt")

# macOS settings for DragNDrop generator
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
set(CPACK_DMG_VOLUME_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/packaging/dmg/AppDMGBackground.tiff")
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/packaging/dmg/AppDMGSetup.scpt")

# Linux DEB settings
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libsdl2-2.0-0, libsdl2-image-2.0-0, libsdl2-ttf-2.0-0, flac, libmpg123-0, libpng16-16, zlib1g, libzstd1, libserialport0, libportmidi0, libenet7, libmpeg2-4, libpcap0.8") # Adjust dependencies as needed
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "otherosfs")

# Linux RPM settings
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_REQUIRES "glibc, libstdc++, SDL2, SDL2_ttf SDL2_image, flac, mpg123, libpng, libserialport, portmidi, enet, libmpeg2")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "armhf")
else()
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
endif()

# Package resources
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/docs/README.md")

# Strip binaries
set(CPACK_STRIP_FILES "YES")

# Generator selection per platform
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_GENERATOR TGZ DragNDrop)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CPACK_GENERATOR TGZ DEB RPM)
else ()
    set(CPACK_GENERATOR TGZ)
endif ()

include(CPack)
