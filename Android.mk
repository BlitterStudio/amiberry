AMIBERRY_LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PATH := $(AMIBERRY_LOCAL_PATH)

include $(LOCAL_PATH)/external/libguisan/Android.mk
LOCAL_PATH := $(AMIBERRY_LOCAL_PATH)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/external/libxml2/Android.mk
LOCAL_PATH := $(AMIBERRY_LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := amiberry

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src \
                    $(LOCAL_PATH)/src/osdep \
                    $(LOCAL_PATH)/src/threaddep \
                    $(LOCAL_PATH)/src/include \
                    $(LOCAL_PATH)/src/archivers \
                    $(LOCAL_PATH)/external/libguisan/include \
                    $(SDL_PATH)/include \
                    $(LIBPNG_PATH) \

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_ARM_NEON := true
    LOCAL_CFLAGS := -DCPU_arm -DARM_HAS_DIV -DARMV6T2 -DARMV6_ASSEMBLY -DAMIBERRY -D_FILE_OFFSET_BITS=64 -DUSE_ARMNEON -DSTATIC_LIBXML
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_CFLAGS := -DCPU_AARCH64 -DAMIBERRY -D_FILE_OFFSET_BITS=64
endif

#LOCAL_CPPFLAGS := -std=gnu++14 -pipe -frename-registers 
LOCAL_CPPFLAGS := -std=c++17 -pipe -frename-registers \
                    -Wno-shift-overflow -Wno-narrowing

LOCAL_LDFLAGS += -fuse-ld=gold

# Add your application source files here...
LOCAL_SRC_FILES := src/akiko.cpp \
                    src/ar.cpp \
                    src/audio.cpp \
                    src/autoconf.cpp \
                    src/blitfunc.cpp \
                    src/blittable.cpp \
                    src/blitter.cpp \
                    src/blkdev.cpp \
                    src/blkdev_cdimage.cpp \
                    src/bsdsocket.cpp \
                    src/calc.cpp \
                    src/cdrom.cpp \
					src/cdtv.cpp \
					src/cdtvcr.cpp \
                    src/cfgfile.cpp \
                    src/cia.cpp \
					src/consolehook.cpp \
                    src/crc32.cpp \
                    src/custom.cpp \
                    src/debug.cpp \
					src/def_icons.cpp \
                    src/devices.cpp \
                    src/disk.cpp \
                    src/diskutil.cpp \
                    src/dlopen.cpp \
                    src/dongle.cpp \
                    src/drawing.cpp \
					src/driveclick.cpp \
                    src/events.cpp \
                    src/expansion.cpp \
                    src/fdi2raw.cpp \
                    src/filesys.cpp \
                    src/flashrom.cpp \
                    src/fpp.cpp \
                    src/fsdb.cpp \
                    src/fsusage.cpp \
                    src/gayle.cpp \
                    src/gfxboard.cpp \
                    src/gfxutil.cpp \
                    src/hardfile.cpp \
                    src/hrtmon.rom.cpp \
                    src/ide.cpp \
                    src/ini.cpp \
                    src/inputdevice.cpp \
                    src/inputrecord.cpp \
					src/isofs.cpp \
                    src/keybuf.cpp \
                    src/main.cpp \
                    src/memory.cpp \
                    src/native2amiga.cpp \
                    src/rommgr.cpp \
                    src/rtc.cpp \
                    src/savestate.cpp \
                    src/scp.cpp \
                    src/scsi.cpp \
					src/scsiemul.cpp \
                    src/scsitape.cpp \
                    src/statusline.cpp \
					src/tabletlibrary.cpp \
					src/tinyxml2.cpp \
                    src/traps.cpp \
					src/uaeexe.cpp \
                    src/uaelib.cpp \
                    src/uaenative.cpp \
                    src/uaeresource.cpp \
					src/uaeserial.cpp \
                    src/zfile.cpp \
                    src/zfile_archive.cpp \
                    src/archivers/7z/7zAlloc.c \
                    src/archivers/7z/7zArcIn.c \
                    src/archivers/7z/7zBuf.c \
                    src/archivers/7z/7zBuf2.c \
                    src/archivers/7z/7zCrc.c \
                    src/archivers/7z/7zCrcOpt.c \
                    src/archivers/7z/7zDec.c \
                    src/archivers/7z/7zFile.c \
                    src/archivers/7z/7zStream.c \
                    src/archivers/7z/Aes.c \
                    src/archivers/7z/AesOpt.c \
                    src/archivers/7z/Alloc.c \
                    src/archivers/7z/Bcj2.c \
                    src/archivers/7z/Bra.c \
                    src/archivers/7z/Bra86.c \
                    src/archivers/7z/BraIA64.c \
                    src/archivers/7z/CpuArch.c \
                    src/archivers/7z/Delta.c \
                    src/archivers/7z/LzFind.c \
                    src/archivers/7z/Lzma2Dec.c \
                    src/archivers/7z/Lzma2Enc.c \
                    src/archivers/7z/Lzma86Dec.c \
                    src/archivers/7z/Lzma86Enc.c \
                    src/archivers/7z/LzmaDec.c \
                    src/archivers/7z/LzmaEnc.c \
                    src/archivers/7z/LzmaLib.c \
                    src/archivers/7z/Ppmd7.c \
                    src/archivers/7z/Ppmd7Dec.c \
                    src/archivers/7z/Ppmd7Enc.c \
                    src/archivers/7z/Sha256.c \
                    src/archivers/7z/Sort.c \
                    src/archivers/7z/Xz.c \
                    src/archivers/7z/XzCrc64.c \
                    src/archivers/7z/XzCrc64Opt.c \
                    src/archivers/7z/XzDec.c \
                    src/archivers/7z/XzEnc.c \
                    src/archivers/7z/XzIn.c \
                    src/archivers/chd/avhuff.cpp \
                    src/archivers/chd/bitmap.cpp \
                    src/archivers/chd/cdrom.cpp \
                    src/archivers/chd/chd.cpp \
                    src/archivers/chd/chdcd.cpp \
                    src/archivers/chd/chdcodec.cpp \
                    src/archivers/chd/corealloc.cpp \
                    src/archivers/chd/corefile.cpp \
                    src/archivers/chd/corestr.cpp \
                    src/archivers/chd/flac.cpp \
                    src/archivers/chd/harddisk.cpp \
                    src/archivers/chd/hashing.cpp \
                    src/archivers/chd/huffman.cpp \
                    src/archivers/chd/md5.cpp \
                    src/archivers/chd/osdcore.cpp \
                    src/archivers/chd/osdlib_unix.cpp \
                    src/archivers/chd/osdsync.cpp \
                    src/archivers/chd/palette.cpp \
                    src/archivers/chd/posixdir.cpp \
                    src/archivers/chd/posixfile.cpp \
                    src/archivers/chd/posixptty.cpp \
                    src/archivers/chd/posixsocket.cpp \
                    src/archivers/chd/strconv.cpp \
                    src/archivers/chd/strformat.cpp \
                    src/archivers/chd/unicode.cpp \
                    src/archivers/chd/vecstream.cpp \
                    src/archivers/chd/utf8proc.c \
                    src/archivers/dms/crc_csum.cpp \
                    src/archivers/dms/getbits.cpp \
                    src/archivers/dms/maketbl.cpp \
                    src/archivers/dms/pfile.cpp \
                    src/archivers/dms/tables.cpp \
                    src/archivers/dms/u_deep.cpp \
                    src/archivers/dms/u_heavy.cpp \
                    src/archivers/dms/u_init.cpp \
                    src/archivers/dms/u_medium.cpp \
                    src/archivers/dms/u_quick.cpp \
                    src/archivers/dms/u_rle.cpp \
                    src/archivers/lha/crcio.cpp \
                    src/archivers/lha/dhuf.cpp \
                    src/archivers/lha/header.cpp \
                    src/archivers/lha/huf.cpp \
                    src/archivers/lha/larc.cpp \
                    src/archivers/lha/lhamaketbl.cpp \
                    src/archivers/lha/lharc.cpp \
                    src/archivers/lha/shuf.cpp \
                    src/archivers/lha/slide.cpp \
                    src/archivers/lha/uae_lha.cpp \
                    src/archivers/lha/util.cpp \
                    src/archivers/lzx/unlzx.cpp \
                    src/archivers/mp2/kjmp2.cpp \
                    src/archivers/wrp/warp.cpp \
                    src/archivers/zip/unzip.cpp \
                    src/caps/caps_amiberry.cpp \
                    src/machdep/support.cpp \
                    src/floppybridge/floppybridge_lib.cpp \
                    src/osdep/ahi_v1.cpp \
                    src/osdep/bsdsocket_host.cpp \
                    src/osdep/cda_play.cpp \
                    src/osdep/charset.cpp \
                    src/osdep/fsdb_host.cpp \
					src/osdep/clipboard.cpp \
                    src/osdep/amiberry_hardfile.cpp \
                    src/osdep/keyboard.cpp \
                    src/osdep/mp3decoder.cpp \
                    src/osdep/picasso96.cpp \
                    src/osdep/writelog.cpp \
                    src/osdep/amiberry.cpp \
                    src/osdep/ahi_v2.cpp \
                    src/osdep/amiberry_filesys.cpp \
                    src/osdep/amiberry_input.cpp \
                    src/osdep/amiberry_gfx.cpp \
                    src/osdep/amiberry_gui.cpp \
                    src/osdep/amiberry_mem.cpp \
					src/osdep/amiberry_serial.cpp \
                    src/osdep/amiberry_whdbooter.cpp \
                    src/osdep/sigsegv_handler.cpp \
					src/osdep/retroarch.cpp \
                    src/sounddep/sound.cpp \
                    src/threaddep/threading.cpp \
                    src/osdep/gui/SelectorEntry.cpp \
                    src/osdep/gui/ShowHelp.cpp \
                    src/osdep/gui/ShowMessage.cpp \
					src/osdep/gui/ShowDiskInfo.cpp \
                    src/osdep/gui/SelectFolder.cpp \
                    src/osdep/gui/SelectFile.cpp \
                    src/osdep/gui/CreateFilesysHardfile.cpp \
                    src/osdep/gui/EditFilesysVirtual.cpp \
                    src/osdep/gui/EditFilesysHardfile.cpp \
                    src/osdep/gui/EditFilesysHardDrive.cpp \
                    src/osdep/gui/PanelAbout.cpp \
                    src/osdep/gui/PanelPaths.cpp \
                    src/osdep/gui/PanelQuickstart.cpp \
                    src/osdep/gui/PanelConfig.cpp \
                    src/osdep/gui/PanelCPU.cpp \
                    src/osdep/gui/PanelChipset.cpp \
                    src/osdep/gui/PanelCustom.cpp \
                    src/osdep/gui/PanelROM.cpp \
                    src/osdep/gui/PanelRAM.cpp \
                    src/osdep/gui/PanelFloppy.cpp \
					src/osdep/gui/PanelExpansions.cpp \
                    src/osdep/gui/PanelHD.cpp \
                    src/osdep/gui/PanelRTG.cpp \
					src/osdep/gui/PanelHWInfo.cpp \
                    src/osdep/gui/PanelInput.cpp \
                    src/osdep/gui/PanelIOPorts.cpp \
                    src/osdep/gui/PanelDisplay.cpp \
                    src/osdep/gui/PanelSound.cpp \
                    src/osdep/gui/PanelDiskSwapper.cpp \
                    src/osdep/gui/PanelMisc.cpp \
                    src/osdep/gui/PanelPrio.cpp \
                    src/osdep/gui/PanelSavestate.cpp \
                    src/osdep/gui/main_window.cpp \
                    src/osdep/gui/Navigation.cpp \
                    src/osdep/gui/androidsdl_event.cpp \
                    src/osdep/gui/PanelOnScreen.cpp \
                    src/osdep/vkbd/vkbd.cpp

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES += src/osdep/aarch64_helper_min.s
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES += src/osdep/arm_helper.s
endif

LOCAL_SRC_FILES += src/newcpu.cpp \
                    src/newcpu_common.cpp \
                    src/readcpu.cpp \
                    src/cpudefs.cpp \
                    src/cpustbl.cpp \
                    src/cpuemu_0.cpp \
                    src/cpuemu_4.cpp \
                    src/cpuemu_11.cpp \
					src/cpuemu_13.cpp \
                    src/cpuemu_40.cpp \
                    src/cpuemu_44.cpp \
                    src/jit/compemu.cpp \
                    src/jit/compstbl.cpp \
                    src/jit/compemu_fpp.cpp \
                    src/jit/compemu_support.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_ttf SDL2_mixer mpg123 guisan serialport

LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -llog -lz

include $(BUILD_SHARED_LIBRARY)
