cmake_minimum_required(VERSION 3.16)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree build. Please create a build directory outside of the amiberry source code and run \"cmake -S ${CMAKE_SOURCE_DIR} -B .\" from there")
endif ()

set(VERSION 6.3.5)
project(amiberry
        VERSION ${VERSION}
        LANGUAGES C CXX
        DESCRIPTION "Optimized Amiga emulator for various platforms"
        HOMEPAGE_URL "https://amiberry.com"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Silence warnings about unused arguments
    set(CMAKE_C_FLAGS_RELEASE "-O3 -pipe -fno-pie")
    set(CMAKE_C_FLAGS_DEBUG "-Og -g -funwind-tables -DDEBUG -pipe -fno-pie")
else ()
    set(CMAKE_C_FLAGS_RELEASE "-O3 -pipe -fno-pie -no-pie")
    set(CMAKE_C_FLAGS_DEBUG "-Og -g -funwind-tables -DDEBUG -pipe -fno-pie -no-pie")
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

# Use libgpiod to control GPIO LEDs?
option(USE_GPIOD "Use GPIOD" OFF)
# Use DBUS to control the emulator?
option(USE_DBUS "Use DBus" OFF)
# Use OpenGL for rendering?
option(USE_OPENGL "Use OpenGL" OFF)
# Enable Link Time Optimization?
option(WITH_LTO "Enable Link Time Optimization" OFF)

# Set build type to "Release" if user did not specify any build type yet
# Other possible values: Debug, Release, RelWithDebInfo and MinSizeRel
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)

if (WITH_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

# Needed so that cmake uses our find modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        include_directories("/opt/homebrew/include")
        set(CMAKE_EXE_LINKER_FLAGS "-L/opt/homebrew/lib -framework IOKit -framework Foundation -liconv")
    else ()
        include_directories("/usr/local/include")
        set(CMAKE_EXE_LINKER_FLAGS "-L/usr/local/lib -framework IOKit -framework Foundation -liconv")
    endif ()
endif ()

# Define the list of source files
set(SOURCE_FILES
    src/a2065.cpp
    src/a2091.cpp
    src/akiko.cpp
    src/amax.cpp
    src/ar.cpp
    src/arcadia.cpp
    src/audio.cpp
    src/autoconf.cpp
    src/blitfunc.cpp
    src/blittable.cpp
    src/blitter.cpp
    src/blkdev.cpp
    src/blkdev_cdimage.cpp
    src/bsdsocket.cpp
    src/calc.cpp
    src/catweasel.cpp
    src/cd32_fmv.cpp
    src/cd32_fmv_genlock.cpp
    src/cdrom.cpp
    src/cdtv.cpp
    src/cdtvcr.cpp
    src/cfgfile.cpp
    src/cia.cpp
    src/consolehook.cpp
    src/cpuboard.cpp
    src/crc32.cpp
    src/custom.cpp
    src/debug.cpp
    src/debugmem.cpp
    src/def_icons.cpp
    src/devices.cpp
    src/disasm.cpp
    src/disk.cpp
    src/diskutil.cpp
    src/dlopen.cpp
    src/dongle.cpp
    src/draco.cpp
    src/drawing.cpp
    src/driveclick.cpp
    src/enforcer.cpp
    src/ethernet.cpp
    src/events.cpp
    src/expansion.cpp
    src/fdi2raw.cpp
    src/filesys.cpp
    src/flashrom.cpp
    src/fpp.cpp
    src/fpp_native.cpp
    src/framebufferboards.cpp
    src/fsdb.cpp
    src/fsusage.cpp
    src/gayle.cpp
    src/gfxboard.cpp
    src/gfxlib.cpp
    src/gfxutil.cpp
    src/hardfile.cpp
    src/hrtmon.rom.cpp
    src/ide.cpp
    src/idecontrollers.cpp
    src/identify.cpp
    src/ini.cpp
    src/inputdevice.cpp
    src/inputrecord.cpp
    src/isofs.cpp
    src/keybuf.cpp
    src/luascript.cpp
    src/main.cpp
    src/memory.cpp
    src/midiemu.cpp
    src/native2amiga.cpp
    src/ncr9x_scsi.cpp
    src/ncr_scsi.cpp
    src/parser.cpp
    src/pci.cpp
    src/rommgr.cpp
    src/rtc.cpp
    src/sampler.cpp
    src/sana2.cpp
    src/savestate.cpp
    src/scp.cpp
    src/scsi.cpp
    src/scsiemul.cpp
    src/scsitape.cpp
    src/slirp_uae.cpp
    src/sndboard.cpp
    src/specialmonitors.cpp
    src/statusline.cpp
    src/tabletlibrary.cpp
    src/test_card.cpp
    src/tinyxml2.cpp
    src/traps.cpp
    src/uaeexe.cpp
    src/uaelib.cpp
    src/uaenative.cpp
    src/uaeresource.cpp
    src/uaeserial.cpp
    src/vm.cpp
    src/x86.cpp
    src/zfile.cpp
    src/zfile_archive.cpp
    src/archivers/7z/7zAlloc.c
    src/archivers/7z/7zArcIn.c
    src/archivers/7z/7zBuf.c
    src/archivers/7z/7zBuf2.c
    src/archivers/7z/7zCrc.c
    src/archivers/7z/7zCrcOpt.c
    src/archivers/7z/7zDec.c
    src/archivers/7z/7zFile.c
    src/archivers/7z/7zStream.c
    src/archivers/7z/Aes.c
    src/archivers/7z/AesOpt.c
    src/archivers/7z/Alloc.c
    src/archivers/7z/Bcj2.c
    src/archivers/7z/Bra.c
    src/archivers/7z/Bra86.c
    src/archivers/7z/BraIA64.c
    src/archivers/7z/CpuArch.c
    src/archivers/7z/Delta.c
    src/archivers/7z/LzFind.c
    src/archivers/7z/Lzma2Dec.c
    src/archivers/7z/Lzma2Enc.c
    src/archivers/7z/Lzma86Dec.c
    src/archivers/7z/Lzma86Enc.c
    src/archivers/7z/LzmaDec.c
    src/archivers/7z/LzmaEnc.c
    src/archivers/7z/LzmaLib.c
    src/archivers/7z/Ppmd7.c
    src/archivers/7z/Ppmd7Dec.c
    src/archivers/7z/Ppmd7Enc.c
    src/archivers/7z/Sha256.c
    src/archivers/7z/Sort.c
    src/archivers/7z/Xz.c
    src/archivers/7z/XzCrc64.c
    src/archivers/7z/XzCrc64Opt.c
    src/archivers/7z/XzDec.c
    src/archivers/7z/XzEnc.c
    src/archivers/7z/XzIn.c
    src/archivers/chd/avhuff.cpp
    src/archivers/chd/bitmap.cpp
    src/archivers/chd/cdrom.cpp
    src/archivers/chd/chd.cpp
    src/archivers/chd/chdcd.cpp
    src/archivers/chd/chdcodec.cpp
    src/archivers/chd/corealloc.cpp
    src/archivers/chd/corefile.cpp
    src/archivers/chd/corestr.cpp
    src/archivers/chd/flac.cpp
    src/archivers/chd/harddisk.cpp
    src/archivers/chd/hashing.cpp
    src/archivers/chd/huffman.cpp
    src/archivers/chd/md5.cpp
    src/archivers/chd/osdcore.cpp
    src/archivers/chd/osdlib_unix.cpp
    src/archivers/chd/osdsync.cpp
    src/archivers/chd/palette.cpp
    src/archivers/chd/posixdir.cpp
    src/archivers/chd/posixfile.cpp
    src/archivers/chd/posixptty.cpp
    src/archivers/chd/posixsocket.cpp
    src/archivers/chd/strconv.cpp
    src/archivers/chd/strformat.cpp
    src/archivers/chd/unicode.cpp
    src/archivers/chd/vecstream.cpp
    src/archivers/chd/utf8proc.c
    src/archivers/dms/crc_csum.cpp
    src/archivers/dms/getbits.cpp
    src/archivers/dms/maketbl.cpp
    src/archivers/dms/pfile.cpp
    src/archivers/dms/tables.cpp
    src/archivers/dms/u_deep.cpp
    src/archivers/dms/u_heavy.cpp
    src/archivers/dms/u_init.cpp
    src/archivers/dms/u_medium.cpp
    src/archivers/dms/u_quick.cpp
    src/archivers/dms/u_rle.cpp
    src/archivers/lha/crcio.cpp
    src/archivers/lha/dhuf.cpp
    src/archivers/lha/header.cpp
    src/archivers/lha/huf.cpp
    src/archivers/lha/larc.cpp
    src/archivers/lha/lhamaketbl.cpp
    src/archivers/lha/lharc.cpp
    src/archivers/lha/shuf.cpp
    src/archivers/lha/shuf.cpp
    src/archivers/lha/slide.cpp
    src/archivers/lha/uae_lha.cpp
    src/archivers/lha/util.cpp
    src/archivers/lzx/unlzx.cpp
    src/archivers/mp2/kjmp2.cpp
    src/archivers/wrp/warp.cpp
    src/archivers/zip/unzip.cpp
    src/caps/caps_amiberry.cpp
    src/dsp3210/dsp_glue.cpp
    src/dsp3210/DSP3210_emulation.cpp
    src/machdep/support.cpp
    src/mame/a2410.cpp
    src/mame/tm34010/tms34010.cpp
    external/floppybridge/src/floppybridge_lib.cpp
    src/osdep/ahi_v1.cpp
    src/osdep/bsdsocket_host.cpp
    src/osdep/cda_play.cpp
    src/osdep/charset.cpp
    src/osdep/fsdb_host.cpp
    src/osdep/clipboard.cpp
    src/osdep/amiberry_hardfile.cpp
    src/osdep/keyboard.cpp
    src/osdep/midi.cpp
    src/osdep/mp3decoder.cpp
    src/osdep/picasso96.cpp
    src/osdep/writelog.cpp
    src/osdep/amiberry.cpp
    src/osdep/ahi_v2.cpp
    src/osdep/amiberry_dbus.cpp
    src/osdep/amiberry_filesys.cpp
    src/osdep/amiberry_input.cpp
    src/osdep/amiberry_gfx.cpp
    src/osdep/amiberry_gui.cpp
    src/osdep/amiberry_mem.cpp
    src/osdep/amiberry_serial.cpp
    src/osdep/amiberry_uaenet.cpp
    src/osdep/amiberry_whdbooter.cpp
    src/osdep/ioport.cpp
    src/osdep/sigsegv_handler.cpp
    src/osdep/socket.cpp
    src/osdep/retroarch.cpp
    src/osdep/vpar.cpp
    src/pcem/386.cpp
    src/pcem/386_common.cpp
    src/pcem/386_dynarec.cpp
    src/pcem/808x.cpp
    src/pcem/cpu.cpp
    src/pcem/dosbox/dbopl.cpp
    src/pcem/dma.cpp
    src/pcem/keyboard.cpp
    src/pcem/keyboard_at.cpp
    src/pcem/keyboard_at_draco.cpp
    src/pcem/mem.cpp
    src/pcem/mouse_ps2.cpp
    src/pcem/mouse_serial.cpp
    src/pcem/dosbox/nukedopl.cpp
    src/pcem/nvr.cpp
    src/pcem/pcemglue.cpp
    src/pcem/pcemrtc.cpp
    src/pcem/pic.cpp
    src/pcem/pit.cpp
    src/pcem/serial.cpp
    src/pcem/sound_cms.cpp
    src/pcem/sound_dbopl.cpp
    src/pcem/sound_mpu401_uart.cpp
    src/pcem/sound_opl.cpp
    src/pcem/sound_sb.cpp
    src/pcem/sound_sb_dsp.cpp
    src/pcem/sound_speaker.cpp
    src/pcem/timer.cpp
    src/pcem/vid_bt482_ramdac.cpp
    src/pcem/vid_cl5429.cpp
    src/pcem/vid_et4000.cpp
    src/pcem/vid_et4000w32.cpp
    src/pcem/vid_inmos.cpp
    src/pcem/vid_ncr.cpp
    src/pcem/vid_permedia2.cpp
    src/pcem/vid_s3.cpp
    src/pcem/vid_s3_virge.cpp
    src/pcem/vid_sc1502x_ramdac.cpp
    src/pcem/vid_sdac_ramdac.cpp
    src/pcem/vid_svga.cpp
    src/pcem/vid_svga_render.cpp
    src/pcem/vid_voodoo.cpp
    src/pcem/vid_voodoo_banshee.cpp
    src/pcem/vid_voodoo_banshee_blitter.cpp
    src/pcem/vid_voodoo_blitter.cpp
    src/pcem/vid_voodoo_display.cpp
    src/pcem/vid_voodoo_fb.cpp
    src/pcem/vid_voodoo_fifo.cpp
    src/pcem/vid_voodoo_reg.cpp
    src/pcem/vid_voodoo_render.cpp
    src/pcem/vid_voodoo_setup.cpp
    src/pcem/vid_voodoo_texture.cpp
    src/pcem/x86seg.cpp
    src/pcem/x87.cpp
    src/pcem/x87_timings.cpp
    src/ppc/ppc.cpp
    src/ppc/ppcd.cpp
    src/qemuvga/cirrus_vga.cpp
    src/qemuvga/es1370.cpp
    src/qemuvga/esp.cpp
    src/qemuvga/lsi53c710.cpp
    src/qemuvga/lsi53c895a.cpp
    src/qemuvga/ne2000.cpp
    src/qemuvga/qemu.cpp
    src/qemuvga/qemuuaeglue.cpp
    src/qemuvga/vga.cpp
    src/sounddep/sound.cpp
    src/threaddep/threading.cpp
    src/osdep/gui/ControllerMap.cpp
    src/osdep/gui/CreateFolder.cpp
    src/osdep/gui/SelectorEntry.cpp
    src/osdep/gui/ShowCustomFields.cpp
    src/osdep/gui/ShowHelp.cpp
    src/osdep/gui/ShowMessage.cpp
    src/osdep/gui/ShowDiskInfo.cpp
    src/osdep/gui/SelectFolder.cpp
    src/osdep/gui/SelectFile.cpp
    src/osdep/gui/CreateFilesysHardfile.cpp
    src/osdep/gui/EditFilesysVirtual.cpp
    src/osdep/gui/EditFilesysHardfile.cpp
    src/osdep/gui/EditFilesysHardDrive.cpp
    src/osdep/gui/EditTapeDrive.cpp
    src/osdep/gui/PanelAbout.cpp
    src/osdep/gui/PanelPaths.cpp
    src/osdep/gui/PanelQuickstart.cpp
    src/osdep/gui/PanelConfig.cpp
    src/osdep/gui/PanelCPU.cpp
    src/osdep/gui/PanelChipset.cpp
    src/osdep/gui/PanelCustom.cpp
    src/osdep/gui/PanelROM.cpp
    src/osdep/gui/PanelRAM.cpp
    src/osdep/gui/PanelFloppy.cpp
    src/osdep/gui/PanelExpansions.cpp
    src/osdep/gui/PanelHD.cpp
    src/osdep/gui/PanelRTG.cpp
    src/osdep/gui/PanelHWInfo.cpp
    src/osdep/gui/PanelInput.cpp
    src/osdep/gui/PanelIOPorts.cpp
    src/osdep/gui/PanelDisplay.cpp
    src/osdep/gui/PanelSound.cpp
    src/osdep/gui/PanelDiskSwapper.cpp
    src/osdep/gui/PanelMisc.cpp
    src/osdep/gui/PanelPrio.cpp
    src/osdep/gui/PanelSavestate.cpp
    src/osdep/gui/PanelVirtualKeyboard.cpp
    src/osdep/gui/PanelWHDLoad.cpp
	src/osdep/gui/PanelThemes.cpp
    src/osdep/gui/main_window.cpp
    src/osdep/gui/Navigation.cpp
    src/osdep/vkbd/vkbd.cpp
    src/newcpu.cpp
    src/newcpu_common.cpp
    src/readcpu.cpp
    src/cpudefs.cpp
    src/cpustbl.cpp
    src/cpummu.cpp
    src/cpummu30.cpp
    src/cpuemu_0.cpp
    src/cpuemu_11.cpp
    src/cpuemu_13.cpp
    src/cpuemu_20.cpp
    src/cpuemu_21.cpp
    src/cpuemu_22.cpp
    src/cpuemu_23.cpp
    src/cpuemu_24.cpp
    src/cpuemu_31.cpp
    src/cpuemu_32.cpp
    src/cpuemu_33.cpp
    src/cpuemu_34.cpp
    src/cpuemu_35.cpp
    src/cpuemu_40.cpp
    src/cpuemu_50.cpp
    src/jit/compemu.cpp
    src/jit/compstbl.cpp
    src/jit/compemu_fpp.cpp
    src/jit/compemu_support.cpp
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_EXECUTABLE_NAME "Amiberry"
        MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "data/amiberry.png"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.blitterstudio.amiberry"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.blitterstudio.amiberry"
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_NAME "Amiberry"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "(c) 2016-2024 Dimitris Panokostas"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE _FILE_OFFSET_BITS=64 -DAMIBERRY_VERSION="${PROJECT_VERSION}")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE CPU_AARCH64)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    target_compile_definitions(${PROJECT_NAME} PRIVATE CPU_arm ARMV6_ASSEMBLY ARMV6T2 USE_ARMNEON ARM_HAS_DIV)
endif ()

if (USE_GPIOD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GPIOD)
    find_library(LIBGPIOD_LIBRARIES gpiod REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBGPIOD_LIBRARIES})
endif ()

if (USE_DBUS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_DBUS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    target_include_directories(${PROJECT_NAME} PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${DBUS_LIBRARIES})
endif ()

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image MODULE REQUIRED)
find_package(SDL2_ttf MODULE REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR})

find_package(FLAC REQUIRED)
find_package(mpg123 REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_library(LIBSERIALPORT_LIBRARIES serialport REQUIRED)
find_library(PORTMIDI_LIBRARIES portmidi REQUIRED)
find_library(LIBMPEG2 mpeg2 REQUIRED)

set(libmt32emu_SHARED FALSE)
add_subdirectory(external/mt32emu)
add_subdirectory(external/floppybridge)
add_subdirectory(external/capsimage)
add_subdirectory(external/libguisan)

# Add dependencies to ensure external libraries are built
add_dependencies(${PROJECT_NAME} mt32emu floppybridge capsimage guisan)

target_include_directories(${PROJECT_NAME} PRIVATE src src/osdep src/include src/threaddep src/archivers src/ppc/pearpc external/libguisan/include external/mt32emu/src external/floppybridge/src)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 SDL2_image SDL2_ttf guisan mt32emu ${DBUS_LIBRARIES} FLAC portmidi png MPG123::libmpg123 mpeg2convert mpeg2 serialport z pthread dl)

if (USE_OPENGL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENGL)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${TARGET_LINK_LIBRARIES} GLEW OpenGL::GL)
endif ()

include(GNUInstallDirs)
add_definitions(
    -DAMIBERRY_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}"
    -DAMIBERRY_LIBDIR="${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_NAME}"
)

# Install the executable
install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/controllers
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/controllers)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/data)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/roms
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/roms)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/whdboot
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/whdboot)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:capsimage>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/$<TARGET_FILE_NAME:capsimage>)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:floppybridge>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/$<TARGET_FILE_NAME:floppybridge>)
     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND dylibbundler -od -b -x $<TARGET_FILE:${PROJECT_NAME}> -d $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/ -p @executable_path/../Frameworks/)

    if (NOT "${CMAKE_GENERATOR}" MATCHES "Xcode")
        install(FILES $<TARGET_FILE:capsimage>
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/)
        install(FILES $<TARGET_FILE:floppybridge>
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/)

        # This one contains the gamecontrollersdb.txt file
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
        # This one contains the data files
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
        # This one contains the AROS kickstart files
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
        # This one contains the whdboot files
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
            DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources)
    endif ()
else()
    install(FILES $<TARGET_FILE:capsimage>
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
    install(FILES $<TARGET_FILE:floppybridge>
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
    # This one contains the gamecontrollersdb.txt file
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
    # This one contains the data files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
    # This one contains the AROS kickstart files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
    # This one contains the plugins
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/plugins/abr
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
    # This one contains the whdboot files
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
    # Install desktop file
    install(FILES ${CMAKE_SOURCE_DIR}/data/Amiberry.desktop
        DESTINATION share/applications)
    # Install icon
    install(FILES ${CMAKE_SOURCE_DIR}/data/amiberry.png
        DESTINATION share/icons/hicolor/512x512/apps)
    # Install Debian files
    install(FILES ${CMAKE_SOURCE_DIR}/debian/changelog.gz
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(FILES ${CMAKE_SOURCE_DIR}/debian/copyright
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(FILES ${CMAKE_SOURCE_DIR}/docs/amiberry.1.gz
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

# Base package settings
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_IGNORE_FILES .git/ .github/ .vscode/ _CPack_Packages/)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR "BlitterStudio")

# macOS settings for DragNDrop generator
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
set(CPACK_DMG_VOLUME_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/macos/AppDMGBackground.tiff")
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/macos/AppDMGSetup.scpt")

# Linux DEB settings
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.27), libstdc++6 (>= 8.3.0), libsdl2-2.0-0, libsdl2-image-2.0-0, libsdl2-ttf-2.0-0, flac, libmpg123-0, libpng16-16, zlib1g, libserialport0, libportmidi0, libmpeg2-4") # Adjust dependencies as needed
set(CPACK_DEBIAN_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
endif()
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE_URL})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dimitris Panokostas <midwan@gmail.com>") # Required
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
        "Optimized Amiga emulator for various ARM, ARM64, AMD64 and RISC-V platforms.
Amiberry is an optimized Amiga emulator for various platforms. It is based on
the latest WinUAE and supports various Amiga models, including the A4000T,
A4000D, A1200, A3000 and A600. Additionally, it includes several unique
features, such as support for WHDLoad titles, RetroArch integration, custom
Controller Mappings and more.")
set(CPACK_DEBIAN_PACKAGE_SECTION "otherosfs")

# Package resources
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/docs/README.md")

# Strip binaries
set(CPACK_STRIP_FILES "YES")

# Generator selection per platform
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_GENERATOR TGZ DragNDrop)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CPACK_GENERATOR TGZ DEB)
else ()
    set(CPACK_GENERATOR TGZ)
endif ()

include(CPack)
