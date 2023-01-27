# Specify "make PLATFORM=<platform>" to compile for a specific target.
# Check the supported list of platforms below for a full list

# Optional parameters
#DEBUG=1
#GCC_PROFILE=1
#GEN_PROFILE=1
#USE_PROFILE=1
#USE_LTO=1
#SANITIZE=1
#USE_GPIOD=1
#USE_OPENGL=1

#enable to compile on a version of GCC older than 8.0
#USE_OLDGCC=1

#
## Common options for all targets
#
SDL_CONFIG ?= sdl2-config
export SDL_CFLAGS := $(shell $(SDL_CONFIG) --cflags)
export SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)

CPPFLAGS = -MD -MT $@ -MF $(@:%.o=%.d) $(SDL_CFLAGS) -Iexternal/libguisan/include -Isrc -Isrc/osdep -Isrc/threaddep -Isrc/include -Isrc/archivers -Isrc/floppybridge -DAMIBERRY -D_FILE_OFFSET_BITS=64
CFLAGS=-pipe -Wno-shift-overflow -Wno-narrowing
USE_LD ?= gold
LDFLAGS = $(SDL_LDFLAGS) -lSDL2_image -lSDL2_ttf -lguisan -Lexternal/libguisan/lib
ifneq ($(strip $(USE_LD)),)
	LDFLAGS += -fuse-ld=$(USE_LD)
endif
LDFLAGS += -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -lpthread -lz -lpng -lrt -lFLAC -lmpg123 -ldl -lmpeg2convert -lmpeg2 -lstdc++fs

ifdef USE_OPENGL
	CFLAGS += -DUSE_OPENGL
	LDFLAGS += -lGL
endif

# Use libgpiod to control GPIO LEDs?
ifdef USE_GPIOD
	CFLAGS += -DUSE_GPIOD
	LDFLAGS += -lgpiod
endif

ifndef DEBUG
	CFLAGS += -O3
else
	CFLAGS += -g -rdynamic -funwind-tables -DDEBUG -Wl,--export-dynamic
endif

ifdef USE_OLDGCC
	CFLAGS += -DUSE_OLDGCC
endif

#Common flags for all 32bit targets
CPPFLAGS32=-DARMV6_ASSEMBLY -DARMV6T2

#Common flags for all 64bit targets
CPPFLAGS64=-DCPU_AARCH64

#Neon flags
NEON_FLAGS=-DUSE_ARMNEON -DARM_HAS_DIV

# Raspberry Pi 1 CPU flags
ifneq (,$(findstring rpi1,$(PLATFORM)))
	CPUFLAGS = -mcpu=arm1176jzf-s -mfpu=vfp
endif

# Raspberry Pi 2 CPU flags
ifneq (,$(findstring rpi2,$(PLATFORM)))
	CPUFLAGS = -mcpu=cortex-a7 -mfpu=neon-vfpv4
endif

# Raspberry Pi 3 CPU flags
ifneq (,$(findstring rpi3,$(PLATFORM)))
	CPUFLAGS = -mcpu=cortex-a53 -mfpu=neon-fp-armv8
endif

# Raspberry Pi 4 CPU flags
ifneq (,$(findstring rpi4,$(PLATFORM)))
	CPUFLAGS = -mcpu=cortex-a72 -mfpu=neon-fp-armv8
endif

# Mac OS X M1 CPU flags
ifneq (,$(findstring osx-m1,$(PLATFORM)))
	CPUFLAGS=-mcpu=apple-m1
endif
#
# DispmanX Common flags (RPI-specific)
#
DISPMANX_FLAGS = -DUSE_DISPMANX -I/opt/vc/include
DISPMANX_LDFLAGS = -lbcm_host -lvchiq_arm -L/opt/vc/lib -Wl,-rpath=/opt/vc/lib

#
# SDL2 with DispmanX targets (RPI only)
#
# Raspberry Pi 1/2/3/4 (SDL2, DispmanX)
ifeq ($(PLATFORM),$(filter $(PLATFORM),rpi1 rpi2 rpi3 rpi4))
	CPPFLAGS += $(CPPFLAGS32) $(DISPMANX_FLAGS)
	LDFLAGS += $(DISPMANX_LDFLAGS)
	ifeq ($(PLATFORM),$(filter $(PLATFORM),rpi2 rpi3 rpi4))
	   CPPFLAGS += $(NEON_FLAGS)
	   HAVE_NEON = 1
	endif    

#
# SDL2 targets
#
# Raspberry Pi 1/2/3/4 (SDL2)
else ifeq ($(PLATFORM),$(filter $(PLATFORM),rpi1-sdl2 rpi2-sdl2 rpi3-sdl2 rpi4-sdl2))
	CPPFLAGS += $(CPPFLAGS32)
	ifeq ($(PLATFORM),$(filter $(PLATFORM), rpi2-sdl2 rpi3-sdl2 rpi4-sdl2))
	   CPPFLAGS += $(NEON_FLAGS)
	   HAVE_NEON = 1
	endif

# OrangePi (SDL2)
else ifeq ($(PLATFORM),orangepi-pc)
	CPUFLAGS = -mcpu=cortex-a7 -mfpu=neon-vfpv4
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS) -DUSE_RENDER_THREAD
	HAVE_NEON = 1
	ifdef DEBUG
		# Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
		# quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
		CFLAGS += -fomit-frame-pointer
	endif

# Odroid XU4 (SDL2)
else ifeq ($(PLATFORM),xu4)
	CPUFLAGS = -mcpu=cortex-a15 -mfpu=neon-vfpv4
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS) -DUSE_RENDER_THREAD
	HAVE_NEON = 1
	ifdef DEBUG
		# Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
		# quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
		CFLAGS += -fomit-frame-pointer
	endif

# Odroid C1 (SDL2)
else ifeq ($(PLATFORM),c1)
	CPUFLAGS = -mcpu=cortex-a5 -mfpu=neon-vfpv4
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS) -DUSE_RENDER_THREAD
	HAVE_NEON = 1
	ifdef DEBUG
		# Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
		# quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
		CFLAGS += -fomit-frame-pointer
	endif

# Odroid N1/N2, RockPro64 (SDL2 64-bit)
else ifeq ($(PLATFORM),n2)
	CPUFLAGS = -mcpu=cortex-a72
	CPPFLAGS += $(CPPFLAGS64) -DUSE_RENDER_THREAD
	AARCH64 = 1

# Raspberry Pi 3 (SDL2 64-bit)
else ifeq ($(PLATFORM),rpi3-64-sdl2)
	CPUFLAGS = -mcpu=cortex-a53
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# Raspberry Pi 4 (SDL2 64-bit)
else ifeq ($(PLATFORM),rpi4-64-sdl2)
	CPUFLAGS = -mcpu=cortex-a72+crc+simd+fp
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# Raspberry Pi 4 (SDL2 with OpenGLES 64-bit) - experimental
else ifeq ($(PLATFORM),rpi4-64-opengl)
	CPUFLAGS = -mcpu=cortex-a72+crc+simd+fp
	CPPFLAGS += $(CPPFLAGS64) -DUSE_OPENGL
	LDFLAGS += -lGL
	AARCH64 = 1

# Raspberry Pi 3 (SDL2 64-bit with DispmanX)
else ifeq ($(PLATFORM),rpi3-64-dmx)
	CPUFLAGS = -mcpu=cortex-a53
	CPPFLAGS += $(CPPFLAGS64) $(DISPMANX_FLAGS)
	LDFLAGS += $(DISPMANX_LDFLAGS)
	AARCH64 = 1

# Raspberry Pi 4 (SDL2 64-bit with DispmanX)
else ifeq ($(PLATFORM),rpi4-64-dmx)
	CPUFLAGS = -mcpu=cortex-a72+crc+simd+fp
	CPPFLAGS += $(CPPFLAGS64) $(DISPMANX_FLAGS)
	LDFLAGS += $(DISPMANX_LDFLAGS)
	AARCH64 = 1

# Vero 4k (SDL2)
else ifeq ($(PLATFORM),vero4k)
	CPUFLAGS = -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	CFLAGS += -ftree-vectorize -funsafe-math-optimizations
	CPPFLAGS += -I/opt/vero3/include $(CPPFLAGS32) $(NEON_FLAGS) -DUSE_RENDER_THREAD
	LDFLAGS += -L/opt/vero3/lib
	HAVE_NEON = 1

# Amlogic S905/S905X/S912 (AMLGXBB/AMLGXL/AMLGXM) e.g. Khadas VIM1/2 / S905X2 (AMLG12A) & S922X/A311D (AMLG12B) e.g. Khadas VIM3 - 32-bit userspace
else ifneq (,$(findstring AMLG,$(PLATFORM)))
	CPUFLAGS = -mfloat-abi=hard -mfpu=neon-fp-armv8
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS)
	HAVE_NEON = 1

	ifneq (,$(findstring AMLG12,$(PLATFORM)))
	  ifneq (,$(findstring AMLG12B,$(PLATFORM)))
		CPUFLAGS = -mcpu=cortex-a73
	  else
		CPUFLAGS = -mcpu=cortex-a53
	  endif
	else ifneq (,$(findstring AMLGX,$(PLATFORM)))
	  CPUFLAGS = -mcpu=cortex-a53
	  CPPFLAGS += -DUSE_RENDER_THREAD
	endif

# Amlogic S905D3/S905X3/S905Y3 (AMLSM1) e.g. HardKernel ODroid C4 & Khadas VIM3L (SDL2 64-bit)
else ifneq (,$(findstring AMLSM1,$(PLATFORM)))
	CPUFLAGS = -mcpu=cortex-a55
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# Odroid Go Advance target (SDL2, 64-bit)
else ifeq ($(PLATFORM),oga)
	CPUFLAGS = -mcpu=cortex-a35
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# OS X (SDL2, 64-bit, M1)
else ifeq ($(PLATFORM),osx-m1)
	LDFLAGS = -L/usr/local/lib external/libguisan/dylib/libguisan.dylib -lSDL2_image -lSDL2_ttf -lpng -liconv -lz -lFLAC -L/opt/homebrew/lib/ -lmpg123 -lmpeg2 -lmpeg2convert $(SDL_LDFLAGS) -framework IOKit -framework Foundation
	CPPFLAGS = -MD -MT $@ -MF $(@:%.o=%.d) $(SDL_CFLAGS) -I/opt/homebrew/include -Iexternal/libguisan/include -Isrc -Isrc/osdep -Isrc/threaddep -Isrc/include -Isrc/archivers -DAMIBERRY -D_FILE_OFFSET_BITS=64 -DCPU_AARCH64 $(SDL_CFLAGS) 
	CXX=/usr/bin/c++
#	DEBUG=1
	APPBUNDLE=1

# OS X (SDL2, 64-bit, x86-64)
else ifeq ($(PLATFORM),osx-x86)
	LDFLAGS = -L/usr/local/lib external/libguisan/dylib/libguisan.dylib -lSDL2_image -lSDL2_ttf -lpng -liconv -lz -lFLAC -L/opt/homebrew/lib/ -lmpg123 -lmpeg2 -lmpeg2convert $(SDL_LDFLAGS) -framework IOKit -framework Foundation
	CPPFLAGS = -MD -MT $@ -MF $(@:%.o=%.d) $(SDL_CFLAGS) -I/opt/homebrew/include -Iexternal/libguisan/include -Isrc -Isrc/osdep -Isrc/threaddep -Isrc/include -Isrc/archivers -DAMIBERRY -D_FILE_OFFSET_BITS=64 $(SDL_CFLAGS) 
	CXX=/usr/bin/c++
#	DEBUG=1
	APPBUNDLE=1

# Generic aarch64 target defaulting to Cortex A53 CPU (SDL2, 64-bit)
else ifeq ($(PLATFORM),a64)
	CPUFLAGS ?= -mcpu=cortex-a53
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# Generic EXPERIMENTAL x86-64 target
else ifeq ($(PLATFORM),x86-64)
	CPPFLAGS += -DUSE_RENDER_THREAD

# RK3288 e.g. Asus Tinker Board
# RK3328 e.g. PINE64 Rock64 
# RK3399 e.g. PINE64 RockPro64 
# RK3326 e.g. Odroid Go Advance - 32-bit userspace
else ifneq (,$(findstring RK,$(PLATFORM)))
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS)
	HAVE_NEON = 1

	ifneq (,$(findstring RK33,$(PLATFORM)))
	  CPUFLAGS = -mfloat-abi=hard -mfpu=neon-fp-armv8
	  ifneq (,$(findstring RK3399,$(PLATFORM)))
		CPUFLAGS += -mcpu=cortex-a72
	  else ifneq (,$(findstring RK3328,$(PLATFORM)))
		CPUFLAGS += -mcpu=cortex-a53
		CPPFLAGS += -DUSE_RENDER_THREAD
	  else ifneq (,$(findstring RK3326,$(PLATFORM)))
		CPUFLAGS += -mcpu=cortex-a35
		CPPFLAGS += -DUSE_RENDER_THREAD
	  endif
	else ifneq (,$(findstring RK3288,$(PLATFORM)))
	  CPUFLAGS = -mcpu=cortex-a17 -mfloat-abi=hard -mfpu=neon-vfpv4
	  CPPFLAGS += -DUSE_RENDER_THREAD
	endif

# sun8i Allwinner H2+ / H3 like Orange PI, Nano PI, Banana PI, Tritium, AlphaCore2, MPCORE-HUB
else ifeq ($(PLATFORM),sun8i)
	CPUFLAGS = -mcpu=cortex-a7 -mfpu=neon-vfpv4
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS) -DUSE_RENDER_THREAD
	HAVE_NEON = 1
	ifdef DEBUG
		# Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
		# quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
		CFLAGS += -fomit-frame-pointer
	endif

# LePotato Libre Computer
else ifeq ($(PLATFORM),lePotato)
   CPUFLAGS = -mcpu=cortex-a53 -mabi=lp64
   CPPFLAGS += $(CPPFLAGS64)
   AARCH64 = 1

# Nvidia Jetson Nano (SDL2 64-bit)
else ifeq ($(PLATFORM),jetson-nano)
	CPUFLAGS = -mcpu=cortex-a57
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# La Frite Libre Computer
else ifeq ($(PLATFORM),mali-drm-gles2-sdl2)
	CPUFLAGS = -mcpu=cortex-a53
	CPPFLAGS += $(CPPFLAGS64)
	AARCH64 = 1

# Generic Cortex-A9 32-bit
else ifeq ($(PLATFORM),s812)
	CPUFLAGS = -mcpu=cortex-a9 -mfpu=neon-vfpv3 -mfloat-abi=hard
	CPPFLAGS += $(CPPFLAGS32) $(NEON_FLAGS) -DUSE_RENDER_THREAD
	HAVE_NEON = 1 

else
$(error Unknown platform:$(PLATFORM))
endif

RM     = rm -f
AS     ?= as
CC     ?= gcc
CXX    ?= g++
STRIP  ?= strip
PROG   = amiberry

#
# SDL2 options
#
all: guisan $(PROG)

export CFLAGS := $(CPUFLAGS) $(CFLAGS) $(EXTRA_CFLAGS)
export CXXFLAGS = $(CFLAGS) -std=gnu++17
export CPPFLAGS

ifdef GCC_PROFILE
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ifdef GEN_PROFILE
	CFLAGS += -fprofile-generate -fprofile-arcs -fvpt
	LDFLAGS += -lgcov
endif

ifdef USE_PROFILE
	CFLAGS += -fprofile-use -fprofile-correction -fbranch-probabilities -fvpt
	LDFLAGS += -lgcov
endif

ifdef SANITIZE
	export LDFLAGS := -lasan $(LDFLAGS)
	CFLAGS += -fsanitize=leak -fsanitize-recover=address
endif

C_OBJS= \
	src/archivers/7z/7zAlloc.o \
	src/archivers/7z/7zArcIn.o \
	src/archivers/7z/7zBuf.o \
	src/archivers/7z/7zBuf2.o \
	src/archivers/7z/7zCrc.o \
	src/archivers/7z/7zCrcOpt.o \
	src/archivers/7z/7zDec.o \
	src/archivers/7z/7zFile.o \
	src/archivers/7z/7zStream.o \
	src/archivers/7z/Aes.o \
	src/archivers/7z/AesOpt.o \
	src/archivers/7z/Alloc.o \
	src/archivers/7z/Bcj2.o \
	src/archivers/7z/Bra.o \
	src/archivers/7z/Bra86.o \
	src/archivers/7z/BraIA64.o \
	src/archivers/7z/CpuArch.o \
	src/archivers/7z/Delta.o \
	src/archivers/7z/LzFind.o \
	src/archivers/7z/Lzma2Dec.o \
	src/archivers/7z/Lzma2Enc.o \
	src/archivers/7z/Lzma86Dec.o \
	src/archivers/7z/Lzma86Enc.o \
	src/archivers/7z/LzmaDec.o \
	src/archivers/7z/LzmaEnc.o \
	src/archivers/7z/LzmaLib.o \
	src/archivers/7z/Ppmd7.o \
	src/archivers/7z/Ppmd7Dec.o \
	src/archivers/7z/Ppmd7Enc.o \
	src/archivers/7z/Sha256.o \
	src/archivers/7z/Sort.o \
	src/archivers/7z/Xz.o \
	src/archivers/7z/XzCrc64.o \
	src/archivers/7z/XzCrc64Opt.o \
	src/archivers/7z/XzDec.o \
	src/archivers/7z/XzEnc.o \
	src/archivers/7z/XzIn.o \
	src/archivers/chd/utf8proc.o

OBJS = \
	src/akiko.o \
	src/ar.o \
	src/audio.o \
	src/autoconf.o \
	src/blitfunc.o \
	src/blittable.o \
	src/blitter.o \
	src/blkdev.o \
	src/blkdev_cdimage.o \
	src/bsdsocket.o \
	src/calc.o \
	src/cd32_fmv.o \
	src/cd32_fmv_genlock.o \
	src/cdrom.o \
	src/cdtv.o \
	src/cdtvcr.o \
	src/cfgfile.o \
	src/cia.o \
	src/consolehook.o \
	src/crc32.o \
	src/custom.o \
	src/debug.o \
	src/def_icons.o \
	src/devices.o \
	src/disk.o \
	src/diskutil.o \
	src/dlopen.o \
	src/dongle.o \
	src/drawing.o \
	src/driveclick.o \
	src/events.o \
	src/expansion.o \
	src/fdi2raw.o \
	src/filesys.o \
	src/flashrom.o \
	src/fpp.o \
	src/fsdb.o \
	src/fsusage.o \
	src/gayle.o \
	src/gfxboard.o \
	src/gfxutil.o \
	src/hardfile.o \
	src/hrtmon.rom.o \
	src/ide.o \
	src/ini.o \
	src/inputdevice.o \
	src/inputrecord.o \
	src/isofs.o \
	src/keybuf.o \
	src/main.o \
	src/memory.o \
	src/native2amiga.o \
	src/parser.o \
	src/rommgr.o \
	src/rtc.o \
	src/sampler.o \
	src/savestate.o \
	src/scp.o \
	src/scsi.o \
	src/scsiemul.o \
	src/scsitape.o \
	src/serial_win32.o \
	src/statusline.o \
	src/tabletlibrary.o \
	src/tinyxml2.o \
	src/traps.o \
	src/uaeexe.o \
	src/uaelib.o \
	src/uaeserial.o \
	src/uaenative.o \
	src/uaeresource.o \
	src/zfile.o \
	src/zfile_archive.o \
	src/archivers/chd/avhuff.o \
	src/archivers/chd/bitmap.o \
	src/archivers/chd/cdrom.o \
	src/archivers/chd/chd.o \
	src/archivers/chd/chdcd.o \
	src/archivers/chd/chdcodec.o \
	src/archivers/chd/corealloc.o \
	src/archivers/chd/corefile.o \
	src/archivers/chd/corestr.o \
	src/archivers/chd/flac.o \
	src/archivers/chd/harddisk.o \
	src/archivers/chd/hashing.o \
	src/archivers/chd/huffman.o \
	src/archivers/chd/md5.o \
	src/archivers/chd/osdcore.o \
	src/archivers/chd/osdlib_unix.o \
	src/archivers/chd/osdsync.o \
	src/archivers/chd/palette.o \
	src/archivers/chd/posixdir.o \
	src/archivers/chd/posixfile.o \
	src/archivers/chd/posixptty.o \
	src/archivers/chd/posixsocket.o \
	src/archivers/chd/strconv.o \
	src/archivers/chd/strformat.o \
	src/archivers/chd/unicode.o \
	src/archivers/chd/vecstream.o \
	src/archivers/dms/crc_csum.o \
	src/archivers/dms/getbits.o \
	src/archivers/dms/maketbl.o \
	src/archivers/dms/pfile.o \
	src/archivers/dms/tables.o \
	src/archivers/dms/u_deep.o \
	src/archivers/dms/u_heavy.o \
	src/archivers/dms/u_init.o \
	src/archivers/dms/u_medium.o \
	src/archivers/dms/u_quick.o \
	src/archivers/dms/u_rle.o \
	src/archivers/lha/crcio.o \
	src/archivers/lha/dhuf.o \
	src/archivers/lha/header.o \
	src/archivers/lha/huf.o \
	src/archivers/lha/larc.o \
	src/archivers/lha/lhamaketbl.o \
	src/archivers/lha/lharc.o \
	src/archivers/lha/shuf.o \
	src/archivers/lha/slide.o \
	src/archivers/lha/uae_lha.o \
	src/archivers/lha/util.o \
	src/archivers/lzx/unlzx.o \
	src/archivers/mp2/kjmp2.o \
	src/archivers/wrp/warp.o \
	src/archivers/zip/unzip.o \
	src/caps/caps_amiberry.o \
	src/machdep/support.o \
	src/osdep/ahi_v1.o \
	src/osdep/bsdsocket_host.o \
	src/osdep/cda_play.o \
	src/osdep/charset.o \
	src/osdep/fsdb_host.o \
	src/osdep/clipboard.o \
	src/osdep/amiberry_hardfile.o \
	src/osdep/keyboard.o \
	src/osdep/mp3decoder.o \
	src/osdep/picasso96.o \
	src/osdep/writelog.o \
	src/osdep/amiberry.o \
	src/osdep/ahi_v2.o \
	src/osdep/amiberry_filesys.o \
	src/osdep/amiberry_input.o \
	src/osdep/amiberry_gfx.o \
	src/osdep/amiberry_gui.o \
	src/osdep/amiberry_mem.o \
	src/osdep/amiberry_whdbooter.o \
	src/osdep/sigsegv_handler.o \
	src/osdep/retroarch.o \
	src/sounddep/sound.o \
	src/threaddep/threading.o \
	src/osdep/gui/ControllerMap.o \
	src/osdep/gui/SelectorEntry.o \
	src/osdep/gui/ShowHelp.o \
	src/osdep/gui/ShowMessage.o \
	src/osdep/gui/ShowDiskInfo.o \
	src/osdep/gui/SelectFolder.o \
	src/osdep/gui/SelectFile.o \
	src/osdep/gui/CreateFilesysHardfile.o \
	src/osdep/gui/EditFilesysVirtual.o \
	src/osdep/gui/EditFilesysHardfile.o \
	src/osdep/gui/EditFilesysHardDrive.o \
	src/osdep/gui/PanelAbout.o \
	src/osdep/gui/PanelPaths.o \
	src/osdep/gui/PanelQuickstart.o \
	src/osdep/gui/PanelConfig.o \
	src/osdep/gui/PanelCPU.o \
	src/osdep/gui/PanelChipset.o \
	src/osdep/gui/PanelCustom.o \
	src/osdep/gui/PanelROM.o \
	src/osdep/gui/PanelRAM.o \
	src/osdep/gui/PanelFloppy.o \
	src/osdep/gui/PanelExpansions.o \
	src/osdep/gui/PanelHD.o \
	src/osdep/gui/PanelRTG.o \
	src/osdep/gui/PanelHWInfo.o \
	src/osdep/gui/PanelInput.o \
	src/osdep/gui/PanelIOPorts.o \
	src/osdep/gui/PanelDisplay.o \
	src/osdep/gui/PanelSound.o \
	src/osdep/gui/PanelDiskSwapper.o \
	src/osdep/gui/PanelMisc.o \
	src/osdep/gui/PanelPrio.o \
	src/osdep/gui/PanelSavestate.o \
	src/osdep/gui/PanelVirtualKeyboard.o \
	src/osdep/gui/main_window.o \
	src/osdep/gui/Navigation.o \
	src/osdep/vkbd/vkbd.o

ifeq ($(ANDROID), 1)
OBJS += src/osdep/gui/androidsdl_event.o \
	src/osdep/gui/PanelOnScreen.o
endif

USE_JIT=1

ifdef AARCH64
OBJS += src/osdep/aarch64_helper.o
src/osdep/aarch64_helper.o: src/osdep/aarch64_helper.s
	$(AS) $(CPUFLAGS) -o src/osdep/aarch64_helper.o -c src/osdep/aarch64_helper.s
else ifeq ($(PLATFORM),$(filter $(PLATFORM),rpi1 rpi1-sdl2))
OBJS += src/osdep/arm_helper.o
src/osdep/arm_helper.o: src/osdep/arm_helper.s
	$(AS) $(CPUFLAGS) -o src/osdep/arm_helper.o -c src/osdep/arm_helper.s
else ifeq ($(PLATFORM),$(filter $(PLATFORM),osx-m1))
	USE_JIT = 0
OBJS += src/osdep/aarch64_helper_osx.o
else ifeq ($(PLATFORM),$(filter $(PLATFORM),osx-x86))
	USE_JIT = 0
else ifeq ($(PLATFORM),$(filter $(PLATFORM),x86-64))
	USE_JIT = 0
else
OBJS += src/osdep/neon_helper.o
src/osdep/neon_helper.o: src/osdep/neon_helper.s
	$(AS) $(CPUFLAGS) -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s
endif

OBJS += src/newcpu.o \
	src/newcpu_common.o \
	src/readcpu.o \
	src/cpudefs.o \
	src/cpustbl.o \
	src/cpuemu_0.o \
	src/cpuemu_4.o \
	src/cpuemu_11.o \
	src/cpuemu_13.o \
	src/cpuemu_40.o \
	src/cpuemu_44.o

ifeq ($(USE_JIT),1)
OBJS += src/jit/compemu.o \
	src/jit/compstbl.o \
	src/jit/compemu_fpp.o \
	src/jit/compemu_support.o
endif

OBJS += src/floppybridge/ArduinoFloppyBridge.o \
		src/floppybridge/ArduinoInterface.o \
		src/floppybridge/CommonBridgeTemplate.o \
		src/floppybridge/floppybridge_lib.o \
		src/floppybridge/ftdi.o \
		src/floppybridge/GreaseWeazleBridge.o \
		src/floppybridge/GreaseWeazleInterface.o \
		src/floppybridge/pll.o \
		src/floppybridge/RotationExtractor.o \
		src/floppybridge/SerialIO.o \
		src/floppybridge/SuperCardProBridge.o \
		src/floppybridge/SuperCardProInterface.o \
		src/floppybridge/FloppyBridge.o

DEPS = $(OBJS:%.o=%.d) $(C_OBJS:%.o=%.d)

$(PROG): $(OBJS) $(C_OBJS)
	$(CXX) -o $(PROG) $(OBJS) $(C_OBJS) $(LDFLAGS)
ifndef DEBUG
# want to keep a copy of the binary before stripping? Then enable the below line
#	cp $(PROG) $(PROG)-debug
	$(STRIP) $(PROG)
endif
ifdef	APPBUNDLE
	sh make-bundle.sh
endif

clean:
	$(RM) $(PROG) $(PROG)-debug $(C_OBJS) $(OBJS) $(ASMS) $(DEPS)
	$(MAKE) -C external/libguisan clean

cleanprofile:
	$(RM) $(OBJS:%.o=%.gcda)
	$(MAKE) -C external/libguisan cleanprofile
	
guisan:
	$(MAKE) -C external/libguisan

gencpu:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o gencpu src/cpudefs.cpp src/gencpu.cpp src/readcpu.cpp src/osdep/charset.cpp

capsimg:
	cd external/capsimg && ./bootstrap && ./configure && $(MAKE)
	cp external/capsimg/capsimg.so ./

-include $(DEPS)
