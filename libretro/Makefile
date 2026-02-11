STATIC_LINKING := 0
ARGT := 

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	system_platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
	system_platform = win
endif

LIBRETRO_DIR := .
AMIBERRY_DIR := ..
LIBRETRO_COMMON ?= $(LIBRETRO_DIR)/libretro-common
LIBCO_DIR := $(LIBRETRO_DIR)/libco
PKG_CONFIG ?= pkg-config

TARGET_NAME := amiberry

ifneq ($(CROSS_COMPILE),)
   CC ?= $(CROSS_COMPILE)gcc
   CXX ?= $(CROSS_COMPILE)g++
   AR ?= $(CROSS_COMPILE)ar
   RANLIB ?= $(CROSS_COMPILE)ranlib
   STRIP ?= $(CROSS_COMPILE)strip
endif

# Detect architecture
ifeq ($(ARCH),)
   ARCH = $(shell uname -m)
endif

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(LIBRETRO_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else
   TARGET := $(TARGET_NAME)_libretro.dll
   CC ?= gcc
   CXX ?= g++
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(LIBRETRO_DIR)/link.T -Wl,--no-undefined
endif

LDFLAGS += $(fpic) $(SHARED)
FLAGS += $(fpic) 
FLAGS += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

ifeq ($(DEBUG), 1)
   FLAGS += -O0 -g
else
   FLAGS += -O3 -DNDEBUG
endif

# Add Amiberry defines
FLAGS += -DAMIBERRY -DLIBRETRO -D_FILE_OFFSET_BITS=64 -DAMIBERRY_DATADIR=\".\" -DAMIBERRY_LIBDIR=\".\"
FLAGS += -DAMIBERRY_VERSION=\"8.0.0\" -DAMIBERRY_VERSION_PRE_RELEASE=\"\"
FLAGS += -DAMIBERRY_VERSION_MAJOR=8 -DAMIBERRY_VERSION_MINOR=0 -DAMIBERRY_VERSION_PATCH=0
FLAGS += -DAMIBERRY_IS_PRE_RELEASE=0
FLAGS += -DAMIBERRY_BUILD_YEAR=$(shell date +%Y) -DAMIBERRY_BUILD_MONTH=$(shell date +%-m) -DAMIBERRY_BUILD_DAY=$(shell date +%-d)
FLAGS += -DAMIBERRY_BUILD_DATE=\"$(shell date +%Y-%m-%d)\"
FLAGS += -DOSDEP_GFX_PLATFORM_HEADER=\"libretro/gfx_platform_internal.h\"
FLAGS += -DOSDEP_AMIBERRY_PLATFORM_HEADER=\"libretro/amiberry_platform_internal.h\"
FLAGS += -DOSDEP_INPUT_PLATFORM_HEADER=\"libretro/input_platform_internal.h\"
FLAGS += -DOSDEP_GUI_HANDLING_HEADER=\"libretro/gui_handling_platform.h\"
FLAGS += -DCHD_FLAC_PLATFORM_HEADER=\"flac_platform_internal_libretro.h\"
FLAGS += -DSOUND_PLATFORM_HEADER=\"sound_platform_internal_libretro.h\"
FLAGS += -DWITH_MIDI

ifneq (,$(findstring aarch64,$(ARCH)))
   FLAGS += -DCPU_AARCH64
else ifneq (,$(findstring arm,$(ARCH)))
   FLAGS += -DCPU_ARM
else
   FLAGS += -DCPU_X86_64
endif

ifeq ($(platform), win)
   ifneq ($(shell command -v x86_64-w64-mingw32-gcc 2>/dev/null),)
      CC ?= x86_64-w64-mingw32-gcc
      CXX ?= x86_64-w64-mingw32-g++
   else ifneq ($(wildcard /mingw64/bin/gcc.exe),)
      CC ?= /mingw64/bin/gcc
      CXX ?= /mingw64/bin/g++
   endif
endif

# Ensure Windows target macros are set when using MinGW, even if platform isn't detected.
ifneq (,$(findstring mingw32,$(shell $(CXX) -dumpmachine 2>/dev/null)))
   FLAGS += -D_WIN32 -DWIN32 -D__MINGW32__ -D__MINGW64__
endif

# Include paths
INCFLAGS += -I$(AMIBERRY_DIR)/src
INCFLAGS += -I$(AMIBERRY_DIR)/src/include
INCFLAGS += -I$(AMIBERRY_DIR)/src/osdep/libretro
INCFLAGS += -I$(AMIBERRY_DIR)/src/osdep
INCFLAGS += -I$(AMIBERRY_DIR)/src/threaddep
INCFLAGS += -I$(AMIBERRY_DIR)/src/archivers
INCFLAGS += -I$(AMIBERRY_DIR)/external/floppybridge/src
INCFLAGS += -I$(LIBRETRO_DIR)
INCFLAGS += -I$(LIBCO_DIR)
INCFLAGS += -I$(LIBRETRO_COMMON)/include

ZLIB_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags zlib 2>/dev/null)
ZLIB_LIBS ?= $(shell $(PKG_CONFIG) --libs zlib 2>/dev/null)

INCFLAGS += $(ZLIB_CFLAGS)
LDFLAGS += $(ZLIB_LIBS)

ifeq ($(platform), unix)
LDFLAGS += -ldl -lm -pthread
endif

# Source files - Core Amiga Emulation
SOURCES_CORE := $(AMIBERRY_DIR)/src/a2065.cpp \
               $(AMIBERRY_DIR)/src/a2091.cpp \
               $(AMIBERRY_DIR)/src/akiko.cpp \
               $(AMIBERRY_DIR)/src/amax.cpp \
               $(AMIBERRY_DIR)/src/ar.cpp \
               $(AMIBERRY_DIR)/src/arcadia.cpp \
               $(AMIBERRY_DIR)/src/audio.cpp \
               $(AMIBERRY_DIR)/src/autoconf.cpp \
               $(AMIBERRY_DIR)/src/blitfunc.cpp \
               $(AMIBERRY_DIR)/src/blittable.cpp \
               $(AMIBERRY_DIR)/src/blitter.cpp \
               $(AMIBERRY_DIR)/src/blkdev.cpp \
               $(AMIBERRY_DIR)/src/blkdev_cdimage.cpp \
               $(AMIBERRY_DIR)/src/bsdsocket.cpp \
               $(AMIBERRY_DIR)/src/calc.cpp \
               $(AMIBERRY_DIR)/src/catweasel.cpp \
               $(AMIBERRY_DIR)/src/cd32_fmv.cpp \
               $(AMIBERRY_DIR)/src/cd32_fmv_genlock.cpp \
               $(AMIBERRY_DIR)/src/cdrom.cpp \
               $(AMIBERRY_DIR)/src/cdtv.cpp \
               $(AMIBERRY_DIR)/src/cdtvcr.cpp \
               $(AMIBERRY_DIR)/src/cfgfile.cpp \
               $(AMIBERRY_DIR)/src/cia.cpp \
               $(AMIBERRY_DIR)/src/consolehook.cpp \
               $(AMIBERRY_DIR)/src/cpuboard.cpp \
               $(AMIBERRY_DIR)/src/crc32.cpp \
               $(AMIBERRY_DIR)/src/cpu_thread.cpp \
               $(AMIBERRY_DIR)/src/custom.cpp \
               $(AMIBERRY_DIR)/src/debug.cpp \
               $(AMIBERRY_DIR)/src/debugmem.cpp \
               $(AMIBERRY_DIR)/src/def_icons.cpp \
               $(AMIBERRY_DIR)/src/devices.cpp \
               $(AMIBERRY_DIR)/src/disasm.cpp \
               $(AMIBERRY_DIR)/src/disk.cpp \
               $(AMIBERRY_DIR)/src/diskutil.cpp \
               $(AMIBERRY_DIR)/src/dlopen.cpp \
               $(AMIBERRY_DIR)/src/dongle.cpp \
               $(AMIBERRY_DIR)/src/draco.cpp \
               $(AMIBERRY_DIR)/src/drawing.cpp \
               $(AMIBERRY_DIR)/src/driveclick.cpp \
               $(AMIBERRY_DIR)/src/enforcer.cpp \
               $(AMIBERRY_DIR)/src/ethernet.cpp \
               $(AMIBERRY_DIR)/src/events.cpp \
               $(AMIBERRY_DIR)/src/expansion.cpp \
               $(AMIBERRY_DIR)/src/fdi2raw.cpp \
               $(AMIBERRY_DIR)/src/filesys.cpp \
               $(AMIBERRY_DIR)/src/flashrom.cpp \
               $(AMIBERRY_DIR)/src/fpp.cpp \
               $(AMIBERRY_DIR)/src/fpp_native.cpp \
               $(AMIBERRY_DIR)/src/framebufferboards.cpp \
               $(AMIBERRY_DIR)/src/fsdb.cpp \
               $(AMIBERRY_DIR)/src/fsusage.cpp \
               $(AMIBERRY_DIR)/src/gayle.cpp \
               $(AMIBERRY_DIR)/src/gfxboard.cpp \
               $(AMIBERRY_DIR)/src/gfxlib.cpp \
               $(AMIBERRY_DIR)/src/gfxutil.cpp \
               $(AMIBERRY_DIR)/src/hardfile.cpp \
               $(AMIBERRY_DIR)/src/hrtmon.rom.cpp \
               $(AMIBERRY_DIR)/src/ide.cpp \
               $(AMIBERRY_DIR)/src/idecontrollers.cpp \
               $(AMIBERRY_DIR)/src/identify.cpp \
               $(AMIBERRY_DIR)/src/ini.cpp \
               $(AMIBERRY_DIR)/src/inputdevice.cpp \
               $(AMIBERRY_DIR)/src/inputrecord.cpp \
               $(AMIBERRY_DIR)/src/isofs.cpp \
               $(AMIBERRY_DIR)/src/keybuf.cpp \
               $(AMIBERRY_DIR)/src/luascript.cpp \
               $(AMIBERRY_DIR)/src/main.cpp \
               $(AMIBERRY_DIR)/src/memory.cpp \
               $(AMIBERRY_DIR)/src/mos6502.cpp \
               $(AMIBERRY_DIR)/src/native2amiga.cpp \
               $(AMIBERRY_DIR)/src/ncr9x_scsi.cpp \
               $(AMIBERRY_DIR)/src/ncr_scsi.cpp \
               $(AMIBERRY_DIR)/src/parser.cpp \
               $(AMIBERRY_DIR)/src/pci.cpp \
               $(AMIBERRY_DIR)/src/rommgr.cpp \
               $(AMIBERRY_DIR)/src/rtc.cpp \
               $(AMIBERRY_DIR)/src/sampler.cpp \
               $(AMIBERRY_DIR)/src/sana2.cpp \
               $(AMIBERRY_DIR)/src/savestate.cpp \
               $(AMIBERRY_DIR)/src/scp.cpp \
               $(AMIBERRY_DIR)/src/scsi.cpp \
               $(AMIBERRY_DIR)/src/scsiemul.cpp \
               $(AMIBERRY_DIR)/src/scsitape.cpp \
               $(AMIBERRY_DIR)/src/slirp_uae.cpp \
               $(AMIBERRY_DIR)/src/sndboard.cpp \
               $(AMIBERRY_DIR)/src/specialmonitors.cpp \
               $(AMIBERRY_DIR)/src/statusline.cpp \
               $(AMIBERRY_DIR)/src/tabletlibrary.cpp \
               $(AMIBERRY_DIR)/src/test_card.cpp \
               $(AMIBERRY_DIR)/src/tinyxml2.cpp \
               $(AMIBERRY_DIR)/src/traps.cpp \
               $(AMIBERRY_DIR)/src/uaeexe.cpp \
               $(AMIBERRY_DIR)/src/uaelib.cpp \
               $(AMIBERRY_DIR)/src/uaenative.cpp \
               $(AMIBERRY_DIR)/src/uaeresource.cpp \
               $(AMIBERRY_DIR)/src/uaeserial.cpp \
               $(AMIBERRY_DIR)/src/vm.cpp \
               $(AMIBERRY_DIR)/src/x86.cpp \
               $(AMIBERRY_DIR)/src/zfile.cpp \
               $(AMIBERRY_DIR)/src/zfile_archive.cpp

# OSDep files
SOURCES_OSDEP := $(LIBRETRO_DIR)/libretro.cpp \
                $(LIBRETRO_DIR)/libretro_midi.cpp \
                $(LIBRETRO_DIR)/libretro_stubs.cpp \
                $(LIBRETRO_DIR)/sdl_stub.cpp \
                $(LIBRETRO_DIR)/libretro_gui_stubs.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_gfx.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_input.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_mem.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_serial.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_uaenet.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_hardfile.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_whdbooter.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_filesys.cpp \
                $(AMIBERRY_DIR)/src/osdep/cda_play.cpp \
                $(AMIBERRY_DIR)/src/osdep/charset.cpp \
                $(AMIBERRY_DIR)/src/osdep/clipboard.cpp \
                $(AMIBERRY_DIR)/src/osdep/dpi_handler.cpp \
                $(AMIBERRY_DIR)/src/osdep/libretro/input_platform.cpp \
                $(AMIBERRY_DIR)/src/osdep/libretro/posixemu_vfs.cpp \
                $(AMIBERRY_DIR)/src/osdep/libretro/stdioemu_vfs.cpp \
                $(AMIBERRY_DIR)/src/osdep/ioport.cpp \
                $(AMIBERRY_DIR)/src/osdep/keyboard.cpp \
                $(AMIBERRY_DIR)/src/osdep/libretro/mp3decoder.cpp \
                $(AMIBERRY_DIR)/src/osdep/registry.cpp \
                $(AMIBERRY_DIR)/src/osdep/retroarch.cpp \
                $(AMIBERRY_DIR)/src/osdep/vpar.cpp \
                $(AMIBERRY_DIR)/src/osdep/libretro/writelog.cpp \
                $(AMIBERRY_DIR)/src/osdep/ahi_v1.cpp \
                $(AMIBERRY_DIR)/src/osdep/ahi_v2.cpp \
                $(AMIBERRY_DIR)/src/osdep/bsdsocket_host.cpp \
                $(AMIBERRY_DIR)/src/osdep/fsdb_host.cpp \
                $(AMIBERRY_DIR)/src/osdep/picasso96.cpp \
                $(AMIBERRY_DIR)/src/osdep/sigsegv_handler.cpp \
                $(AMIBERRY_DIR)/src/osdep/socket.cpp \
                $(AMIBERRY_DIR)/src/sounddep/sound.cpp \
                $(AMIBERRY_DIR)/src/threaddep/threading.cpp

# libretro-common (VFS helpers)
SOURCES_LIBRETRO_COMMON := $(LIBRETRO_COMMON)/file/file_path.c \
                          $(LIBRETRO_COMMON)/file/file_path_io.c \
                          $(LIBRETRO_COMMON)/file/retro_dirent.c \
                          $(LIBRETRO_COMMON)/streams/file_stream.c \
                          $(LIBRETRO_COMMON)/string/stdstring.c \
                          $(LIBRETRO_COMMON)/compat/compat_strl.c \
                          $(LIBRETRO_COMMON)/vfs/vfs_implementation.c

# CPU Emulation
SOURCES_CPU := $(AMIBERRY_DIR)/src/newcpu.cpp \
              $(AMIBERRY_DIR)/src/newcpu_common.cpp \
              $(AMIBERRY_DIR)/src/readcpu.cpp \
              $(AMIBERRY_DIR)/src/cpudefs.cpp \
              $(AMIBERRY_DIR)/src/cpustbl.cpp \
              $(AMIBERRY_DIR)/src/cpummu.cpp \
              $(AMIBERRY_DIR)/src/cpummu30.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_0.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_11.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_13.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_20.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_21.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_22.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_23.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_24.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_31.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_32.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_33.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_34.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_35.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_40.cpp \
              $(AMIBERRY_DIR)/src/cpuemu_50.cpp

SOURCES_JIT := $(AMIBERRY_DIR)/src/jit/compemu.cpp \
               $(AMIBERRY_DIR)/src/jit/compemu_fpp.cpp \
               $(AMIBERRY_DIR)/src/jit/compemu_support.cpp \
               $(AMIBERRY_DIR)/src/jit/compstbl.cpp

# Disable JIT for 64-bit x86 libretro builds (addressing 32-bit pointer checks).
ifneq (,$(findstring x86_64,$(ARCH)))
   FLAGS += -DLIBRETRO_NO_JIT
   SOURCES_JIT :=
endif
ifneq (,$(findstring amd64,$(ARCH)))
   FLAGS += -DLIBRETRO_NO_JIT
   SOURCES_JIT :=
endif

SOURCES_PPC := $(AMIBERRY_DIR)/src/ppc/ppc.cpp \
              $(AMIBERRY_DIR)/src/ppc/ppcd.cpp

SOURCES_QEMUVGA := $(AMIBERRY_DIR)/src/qemuvga/qemu.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/qemuuaeglue.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/vga.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/cirrus_vga.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/ne2000.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/es1370.cpp

# Archivers
SOURCES_ARCHIVE := $(AMIBERRY_DIR)/src/archivers/dms/crc_csum.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/getbits.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/maketbl.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/pfile.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/tables.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/u_deep.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/u_heavy.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/u_init.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/u_medium.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/u_quick.cpp \
                  $(AMIBERRY_DIR)/src/archivers/dms/u_rle.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/crcio.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/dhuf.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/header.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/huf.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/larc.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/lhamaketbl.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/lharc.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/shuf.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/slide.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/uae_lha.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lha/util.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lzx/unlzx.cpp \
                  $(AMIBERRY_DIR)/src/archivers/mp2/kjmp2.cpp \
                  $(AMIBERRY_DIR)/src/archivers/wrp/warp.cpp \
                  $(AMIBERRY_DIR)/src/archivers/zip/unzip.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/avhuff.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/bitmap.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/cdrom.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/chd.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/chdcd.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/chdcodec.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/corealloc.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/corefile.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/corestr.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/flac.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/harddisk.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/hashing.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/huffman.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/md5.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/osdcore.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/osdlib_unix.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/osdsync.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/palette.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/posixdir.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/posixfile.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/posixptty.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/posixsocket.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/strconv.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/strformat.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/unicode.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/vecstream.cpp

SOURCES_7Z := $(AMIBERRY_DIR)/src/archivers/7z/7zAlloc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zArcIn.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zBuf.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zBuf2.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zCrc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zCrcOpt.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zDec.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zFile.c \
             $(AMIBERRY_DIR)/src/archivers/7z/7zStream.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Aes.c \
             $(AMIBERRY_DIR)/src/archivers/7z/AesOpt.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Alloc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Bcj2.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Bra.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Bra86.c \
             $(AMIBERRY_DIR)/src/archivers/7z/BraIA64.c \
             $(AMIBERRY_DIR)/src/archivers/7z/CpuArch.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Delta.c \
             $(AMIBERRY_DIR)/src/archivers/7z/LzFind.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Lzma2Dec.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Lzma2Enc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Lzma86Dec.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Lzma86Enc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/LzmaDec.c \
             $(AMIBERRY_DIR)/src/archivers/7z/LzmaEnc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/LzmaLib.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Ppmd7.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Ppmd7Dec.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Ppmd7Enc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Sha256.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Sort.c \
             $(AMIBERRY_DIR)/src/archivers/7z/Xz.c \
             $(AMIBERRY_DIR)/src/archivers/7z/XzCrc64.c \
             $(AMIBERRY_DIR)/src/archivers/7z/XzCrc64Opt.c \
             $(AMIBERRY_DIR)/src/archivers/7z/XzDec.c \
             $(AMIBERRY_DIR)/src/archivers/7z/XzEnc.c \
             $(AMIBERRY_DIR)/src/archivers/7z/XzIn.c \
             $(AMIBERRY_DIR)/src/archivers/chd/utf8proc.c

# Slirp
SOURCES_SLIRP := $(AMIBERRY_DIR)/src/slirp/bootp.cpp \
                $(AMIBERRY_DIR)/src/slirp/cksum.cpp \
                $(AMIBERRY_DIR)/src/slirp/if.cpp \
                $(AMIBERRY_DIR)/src/slirp/ip_icmp.cpp \
                $(AMIBERRY_DIR)/src/slirp/ip_input.cpp \
                $(AMIBERRY_DIR)/src/slirp/ip_output.cpp \
                $(AMIBERRY_DIR)/src/slirp/mbuf.cpp \
                $(AMIBERRY_DIR)/src/slirp/misc.cpp \
                $(AMIBERRY_DIR)/src/slirp/sbuf.cpp \
                $(AMIBERRY_DIR)/src/slirp/slirp.cpp \
                $(AMIBERRY_DIR)/src/slirp/slirpdebug.cpp \
                $(AMIBERRY_DIR)/src/slirp/socket.cpp \
                $(AMIBERRY_DIR)/src/slirp/tcp_input.cpp \
                $(AMIBERRY_DIR)/src/slirp/tcp_output.cpp \
                $(AMIBERRY_DIR)/src/slirp/tcp_subr.cpp \
                $(AMIBERRY_DIR)/src/slirp/tcp_timer.cpp \
                $(AMIBERRY_DIR)/src/slirp/tftp.cpp \
                $(AMIBERRY_DIR)/src/slirp/udp.cpp

# PCem
SOURCES_PCEM := $(AMIBERRY_DIR)/src/pcem/386.cpp \
               $(AMIBERRY_DIR)/src/pcem/386_common.cpp \
               $(AMIBERRY_DIR)/src/pcem/386_dynarec.cpp \
               $(AMIBERRY_DIR)/src/pcem/808x.cpp \
               $(AMIBERRY_DIR)/src/pcem/cpu.cpp \
               $(AMIBERRY_DIR)/src/pcem/dosbox/dbopl.cpp \
               $(AMIBERRY_DIR)/src/pcem/dma.cpp \
               $(AMIBERRY_DIR)/src/pcem/keyboard.cpp \
               $(AMIBERRY_DIR)/src/pcem/keyboard_at.cpp \
               $(AMIBERRY_DIR)/src/pcem/keyboard_at_draco.cpp \
               $(AMIBERRY_DIR)/src/pcem/mem.cpp \
               $(AMIBERRY_DIR)/src/pcem/mouse_ps2.cpp \
               $(AMIBERRY_DIR)/src/pcem/mouse_serial.cpp \
               $(AMIBERRY_DIR)/src/pcem/dosbox/nukedopl.cpp \
               $(AMIBERRY_DIR)/src/pcem/nvr.cpp \
               $(AMIBERRY_DIR)/src/pcem/pcemglue.cpp \
               $(AMIBERRY_DIR)/src/pcem/pcemrtc.cpp \
               $(AMIBERRY_DIR)/src/pcem/pic.cpp \
               $(AMIBERRY_DIR)/src/pcem/pit.cpp \
               $(AMIBERRY_DIR)/src/pcem/serial.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_cms.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_dbopl.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_mpu401_uart.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_opl.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_sb.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_sb_dsp.cpp \
               $(AMIBERRY_DIR)/src/pcem/sound_speaker.cpp \
               $(AMIBERRY_DIR)/src/pcem/timer.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_bt482_ramdac.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_cl5429.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_et4000.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_et4000w32.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_inmos.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_mga.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_ncr.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_permedia2.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_s3.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_s3_virge.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_sc1502x_ramdac.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_sdac_ramdac.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_svga.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_svga_render.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_tvp3026_ramdac.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_banshee.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_banshee_blitter.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_blitter.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_display.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_fb.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_fifo.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_reg.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_render.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_setup.cpp \
               $(AMIBERRY_DIR)/src/pcem/vid_voodoo_texture.cpp \
               $(AMIBERRY_DIR)/src/pcem/x86seg.cpp \
               $(AMIBERRY_DIR)/src/pcem/x87.cpp \
               $(AMIBERRY_DIR)/src/pcem/x87_timings.cpp

# Miscellaneous hardware / support
SOURCES_MISC := $(AMIBERRY_DIR)/src/caps/caps_amiberry.cpp \
               $(AMIBERRY_DIR)/src/dsp3210/dsp_glue.cpp \
               $(AMIBERRY_DIR)/src/dsp3210/DSP3210_emulation.cpp \
               $(AMIBERRY_DIR)/src/kbmcu/8048/co8048.cpp \
               $(AMIBERRY_DIR)/src/kbmcu/keyboard_mcu_6500_1.cpp \
               $(AMIBERRY_DIR)/src/kbmcu/keyboard_mcu_6805.cpp \
               $(AMIBERRY_DIR)/src/kbmcu/keyboard_mcu_d8039hlc.cpp \
               $(AMIBERRY_DIR)/src/kbmcu/m6805/m68_ops.cpp \
               $(AMIBERRY_DIR)/src/kbmcu/m6805/m68emu.cpp \
               $(AMIBERRY_DIR)/src/machdep/support.cpp \
               $(AMIBERRY_DIR)/src/mame/a2410.cpp \
               $(AMIBERRY_DIR)/src/mame/tm34010/tms34010.cpp \
               $(AMIBERRY_DIR)/external/floppybridge/src/floppybridge_lib.cpp

SOURCES_CXX := $(SOURCES_CORE) $(SOURCES_OSDEP) $(SOURCES_CPU) $(SOURCES_JIT) $(SOURCES_PPC) $(SOURCES_QEMUVGA) $(SOURCES_ARCHIVE) $(SOURCES_SLIRP) $(SOURCES_MISC)

ifeq ($(USE_PCEM),1)
SOURCES_CXX += $(SOURCES_PCEM)
endif

# Optional MT-32/CM-32L emulation via MUNT
# Note: WITH_MIDIEMU is already defined in sysconfig.h;
# WITH_MT32EMU_SOURCES tells stubs they are not needed.
ifeq ($(WITH_MT32EMU),1)
FLAGS += -DWITH_MT32EMU_SOURCES
MT32EMU_DIR = $(AMIBERRY_DIR)/external/mt32emu/src
SOURCES_MT32EMU = \
    $(MT32EMU_DIR)/Analog.cpp \
    $(MT32EMU_DIR)/BReverbModel.cpp \
    $(MT32EMU_DIR)/Display.cpp \
    $(MT32EMU_DIR)/File.cpp \
    $(MT32EMU_DIR)/FileStream.cpp \
    $(MT32EMU_DIR)/LA32FloatWaveGenerator.cpp \
    $(MT32EMU_DIR)/LA32Ramp.cpp \
    $(MT32EMU_DIR)/LA32WaveGenerator.cpp \
    $(MT32EMU_DIR)/MidiStreamParser.cpp \
    $(MT32EMU_DIR)/Part.cpp \
    $(MT32EMU_DIR)/Partial.cpp \
    $(MT32EMU_DIR)/PartialManager.cpp \
    $(MT32EMU_DIR)/Poly.cpp \
    $(MT32EMU_DIR)/ROMInfo.cpp \
    $(MT32EMU_DIR)/SampleRateConverter.cpp \
    $(MT32EMU_DIR)/Synth.cpp \
    $(MT32EMU_DIR)/Tables.cpp \
    $(MT32EMU_DIR)/TVA.cpp \
    $(MT32EMU_DIR)/TVF.cpp \
    $(MT32EMU_DIR)/TVP.cpp \
    $(MT32EMU_DIR)/VersionTagging.cpp \
    $(MT32EMU_DIR)/sha1/sha1.cpp \
    $(MT32EMU_DIR)/c_interface/c_interface.cpp
INCFLAGS += -I$(MT32EMU_DIR)
SOURCES_CXX += $(SOURCES_MT32EMU) $(AMIBERRY_DIR)/src/midiemu.cpp
endif
SOURCES_C := $(SOURCES_7Z) $(LIBCO_DIR)/libco.c $(SOURCES_LIBRETRO_COMMON)

OBJECTS := $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(INCFLAGS) $(FLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(INCFLAGS) $(FLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(INCFLAGS) $(FLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean all
