# Default platform is rpi3 / SDL2 / Dispmanx
ifeq ($(PLATFORM),)
	PLATFORM = rpi3-sdl2-dispmanx
endif

ifneq (,$(findstring rpi3,$(PLATFORM)))
    CFLAGS += -march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8
endif

ifneq (,$(findstring rpi2,$(PLATFORM)))
    CFLAGS += -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4
endif

ifneq (,$(findstring rpi1,$(PLATFORM)))
    CFLAGS += -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp
endif

#
# DispmanX Common flags for both SDL1 and SDL2 (RPI-specific)
#
DISPMANX_FLAGS = -DUSE_DISPMANX -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads 
DISPMANX_LDFLAGS = -lbcm_host -lvchiq_arm -L/opt/vc/lib -Wl,-rpath=/opt/vc/lib
CPPFLAGS=-MD -MT $@ -MF $(@:%.o=%.d)

#DEBUG=1
#GCC_PROFILE=1
#GEN_PROFILE=1
#USE_PROFILE=1
#SANITIZE=1

#
# SDL1 targets
#
ifeq ($(PLATFORM),rpi3)
    CPPFLAGS += ${DISPMANX_FLAGS} -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL1
    LDFLAGS += ${DISPMANX_LDFLAGS}
    HAVE_NEON = 1
    NAME  = amiberry-rpi3-sdl1
	
else ifeq ($(PLATFORM),rpi2)
    CPPFLAGS += ${DISPMANX_FLAGS} -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL1
    LDFLAGS += ${DISPMANX_LDFLAGS}
    HAVE_NEON = 1
    NAME  = amiberry-rpi2-sdl1
	
else ifeq ($(PLATFORM),rpi1)
    CPPFLAGS += ${DISPMANX_FLAGS} -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_SDL1
    LDFLAGS += ${DISPMANX_LDFLAGS}
    NAME  = amiberry-rpi1-sdl1

else ifeq ($(PLATFORM),android)
    CFLAGS += -mfpu=neon -mfloat-abi=soft
    DEFS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DANDROIDSDL -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL1
    ANDROID = 1
    HAVE_NEON = 1
    HAVE_SDL_DISPLAY = 1
    NAME  = amiberry

#
# SDL2 with DispmanX targets (RPI only)
#
else ifeq ($(PLATFORM),rpi3-sdl2-dispmanx)
USE_SDL2 = 1
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 ${DISPMANX_FLAGS}
    LDFLAGS += ${DISPMANX_LDFLAGS}
    HAVE_NEON = 1
    NAME  = amiberry-rpi3-sdl2-dispmanx

else ifeq ($(PLATFORM),rpi2-sdl2-dispmanx)
USE_SDL2 = 1
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 ${DISPMANX_FLAGS}
    LDFLAGS += ${DISPMANX_LDFLAGS}
    HAVE_NEON = 1
    NAME  = amiberry-rpi2-sdl2-dispmanx

else ifeq ($(PLATFORM),rpi1-sdl2-dispmanx)
USE_SDL2 = 1
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_SDL2 ${DISPMANX_FLAGS}
    LDFLAGS += ${DISPMANX_LDFLAGS}
    NAME  = amiberry-rpi1-sdl2-dispmanx

#
# SDL2 targets
#	
else ifeq ($(PLATFORM),rpi3-sdl2)
USE_SDL2 = 1
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2
    HAVE_NEON = 1
    NAME  = amiberry-rpi3-sdl2
	
else ifeq ($(PLATFORM),rpi2-sdl2)
USE_SDL2 = 1
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2
    HAVE_NEON = 1
    NAME  = amiberry-rpi2-sdl2
	
else ifeq ($(PLATFORM),rpi1-sdl2)
USE_SDL2 = 1
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_SDL2
    NAME  = amiberry-rpi1-sdl2

else ifeq ($(PLATFORM),orangepi-pc)
USE_SDL2 = 1
    CFLAGS += -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 -DMALI_GPU -DUSE_RENDER_THREAD
    HAVE_NEON = 1
    NAME  = amiberry-orangepi-pc
    ifdef DEBUG
	    # Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
	    # quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
        MORE_CFLAGS += -fomit-frame-pointer
    endif

else ifeq ($(PLATFORM),xu4)
USE_SDL2 = 1
    CFLAGS += -mcpu=cortex-a15.cortex-a7 -mtune=cortex-a15.cortex-a7 -mfpu=neon-vfpv4
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 -DMALI_GPU -DUSE_RENDER_THREAD -DFASTERCYCLES
    HAVE_NEON = 1
    NAME  = amiberry-xu4
    ifdef DEBUG
	    # Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
	    # quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
        MORE_CFLAGS += -fomit-frame-pointer
    endif

else ifeq ($(PLATFORM),c1)
USE_SDL2 = 1
    CFLAGS += -march=armv7-a -mcpu=cortex-a5 -mtune=cortex-a5 -mfpu=neon-vfpv4
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 -DMALI_GPU -DUSE_RENDER_THREAD -DFASTERCYCLES
    HAVE_NEON = 1
    NAME  = amiberry-c1
    ifdef DEBUG
	    # Otherwise we'll get compilation errors, check https://tls.mbed.org/kb/development/arm-thumb-error-r7-cannot-be-used-in-asm-here
	    # quote: The assembly code in bn_mul.h is optimized for the ARM platform and uses some registers, including r7 to efficiently do an operation. GCC also uses r7 as the frame pointer under ARM Thumb assembly.
        MORE_CFLAGS += -fomit-frame-pointer
    endif

else ifeq ($(PLATFORM),n1)
USE_SDL2 = 1
AARCH64 = 1
    #CFLAGS += -march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8
    CPPFLAGS += -D_FILE_OFFSET_BITS=64 -DUSE_SDL2 -DMALI_GPU -DUSE_RENDER_THREAD -DFASTERCYCLES
    #HAVE_NEON = 1
    NAME  = amiberry-n1

else ifeq ($(PLATFORM),vero4k)
USE_SDL2 = 1
    CFLAGS += -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -ftree-vectorize -funsafe-math-optimizations
    CPPFLAGS += -I/opt/vero3/include -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 -DMALI_GPU -DUSE_RENDER_THREAD -DFASTERCYCLES
    LDFLAGS += -L/opt/vero3/lib
    HAVE_NEON = 1
    NAME  = amiberry-vero4k

else ifeq ($(PLATFORM),tinker)
USE_SDL2 = 1
    CFLAGS += -march=armv7-a -mtune=cortex-a17 -mfpu=neon-vfpv4
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 -DFASTERCYCLES -DUSE_RENDER_THREAD -DMALI_GPU
    HAVE_NEON = 1
    NAME  = amiberry-tinker

else ifeq ($(PLATFORM),rockpro64)
USE_SDL2 = 1
    CFLAGS += -march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8
    CPPFLAGS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2 -DMALI_GPU -DUSE_RENDER_THREAD -DFASTERCYCLES
    HAVE_NEON = 1
    NAME  = amiberry-rockpro64
	
else ifeq ($(PLATFORM),android-sdl2)
USE_SDL2 = 1
    CFLAGS += -mfpu=neon -mfloat-abi=soft
    DEFS += -DARMV6_ASSEMBLY -D_FILE_OFFSET_BITS=64 -DANDROIDSDL -DARMV6T2 -DUSE_ARMNEON -DARM_HAS_DIV -DUSE_SDL2
    ANDROID = 1
    HAVE_NEON = 1
    HAVE_SDL_DISPLAY = 1
    NAME  = amiberry
endif

RM     = rm -f
CC     ?= gcc
CXX    ?= g++
STRIP  ?= strip
PROG   = $(NAME)

#
# SDL1 options
#
ifndef USE_SDL2
all: $(PROG)

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

export CPPFLAGS += $(SDL_CFLAGS)
LDFLAGS += $(SDL_LDFLAGS) -lSDL_image -lSDL_ttf -lguichan_sdl -lguichan
endif

#
# SDL2 options
#
ifdef USE_SDL2
all: guisan $(PROG)
export SDL_CFLAGS := $(shell sdl2-config --cflags)
export SDL_LDFLAGS := $(shell sdl2-config --libs)

CPPFLAGS += $(SDL_CFLAGS) -Iguisan-dev/include
LDFLAGS += $(SDL_LDFLAGS) -lSDL2_image -lSDL2_ttf -lguisan -Lguisan-dev/lib
endif

#
# Common options
#
DEFS = $(XML_CFLAGS) -DAMIBERRY
CPPFLAGS += -Isrc -Isrc/osdep -Isrc/threaddep -Isrc/include -Isrc/archivers $(DEFS)
XML_CFLAGS := $(shell xml2-config --cflags )
LDFLAGS += -flto -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed

ifndef DEBUG
    CFLAGS += -Ofast -frename-registers -falign-functions=16
else
    CFLAGS += -g -rdynamic -funwind-tables -mapcs-frame -DDEBUG -Wl,--export-dynamic
endif

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
    LDFLAGS += -lasan
    CFLAGS += -fsanitize=leak -fsanitize-recover=address
endif

LDFLAGS += -lpthread -lz -lpng -lrt -lxml2 -lFLAC -lmpg123 -ldl -lmpeg2convert -lmpeg2

ASFLAGS += $(CFLAGS) -falign-functions=16

export CFLAGS +=  -pipe -Wno-shift-overflow -Wno-narrowing $(EXTRA_CFLAGS)
export CXXFLAGS += $(CFLAGS) -std=gnu++14 -fpermissive

C_OBJS= \
	src/archivers/7z/BraIA64.o \
	src/archivers/7z/Delta.o \
	src/archivers/7z/Sha256.o \
	src/archivers/7z/XzCrc64.o \
	src/archivers/7z/XzDec.o

OBJS =	\
	src/akiko.o \
	src/ar.o \
	src/aros.rom.o \
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
	src/cfgfile.o \
	src/cia.o \
	src/crc32.o \
	src/custom.o \
	src/def_icons.o \
	src/devices.o \
	src/disk.o \
	src/diskutil.o \
	src/dlopen.o \
	src/drawing.o \
	src/events.o \
	src/expansion.o \
	src/fdi2raw.o \
	src/filesys.o \
	src/flashrom.o \
	src/fpp.o \
	src/fsdb.o \
	src/fsdb_unix.o \
	src/fsusage.o \
	src/gayle.o \
	src/gfxboard.o \
	src/gfxutil.o \
	src/hardfile.o \
	src/hrtmon.rom.o \
	src/ide.o \
	src/inputdevice.o \
	src/keybuf.o \
	src/main.o \
	src/memory.o \
	src/native2amiga.o \
	src/rommgr.o \
	src/rtc.o \
	src/savestate.o \
	src/scsi.o \
	src/statusline.o \
	src/traps.o \
	src/uaelib.o \
	src/uaeresource.o \
	src/zfile.o \
	src/zfile_archive.o \
	src/archivers/7z/7zAlloc.o \
	src/archivers/7z/7zBuf.o \
	src/archivers/7z/7zCrc.o \
	src/archivers/7z/7zCrcOpt.o \
	src/archivers/7z/7zDec.o \
	src/archivers/7z/7zIn.o \
	src/archivers/7z/7zStream.o \
	src/archivers/7z/Bcj2.o \
	src/archivers/7z/Bra.o \
	src/archivers/7z/Bra86.o \
	src/archivers/7z/LzmaDec.o \
	src/archivers/7z/Lzma2Dec.o \
	src/archivers/7z/Xz.o \
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
	src/caps/caps_win32.o \
	src/machdep/support.o \
	src/osdep/bsdsocket_host.o \
	src/osdep/cda_play.o \
	src/osdep/charset.o \
	src/osdep/fsdb_host.o \
	src/osdep/amiberry_hardfile.o \
	src/osdep/keyboard.o \
	src/osdep/mp3decoder.o \
	src/osdep/picasso96.o \
	src/osdep/writelog.o \
	src/osdep/amiberry.o \
	src/osdep/amiberry_filesys.o \
	src/osdep/amiberry_input.o \
	src/osdep/amiberry_gfx.o \
	src/osdep/amiberry_gui.o \
	src/osdep/amiberry_rp9.o \
	src/osdep/amiberry_mem.o \
	src/osdep/amiberry_whdbooter.o \
	src/osdep/sigsegv_handler.o \
	src/sounddep/sound.o \
	src/osdep/gui/UaeRadioButton.o \
	src/osdep/gui/UaeDropDown.o \
	src/osdep/gui/UaeCheckBox.o \
	src/osdep/gui/UaeListBox.o \
	src/osdep/gui/InGameMessage.o \
	src/osdep/gui/SelectorEntry.o \
	src/osdep/gui/ShowHelp.o \
	src/osdep/gui/ShowMessage.o \
	src/osdep/gui/SelectFolder.o \
	src/osdep/gui/SelectFile.o \
	src/osdep/gui/CreateFilesysHardfile.o \
	src/osdep/gui/EditFilesysVirtual.o \
	src/osdep/gui/EditFilesysHardfile.o \
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
	src/osdep/gui/PanelHD.o \
	src/osdep/gui/PanelInput.o \
	src/osdep/gui/PanelDisplay.o \
	src/osdep/gui/PanelSound.o \
	src/osdep/gui/PanelMisc.o \
	src/osdep/gui/PanelSavestate.o \
	src/osdep/gui/main_window.o \
	src/osdep/gui/Navigation.o

ifndef USE_SDL2
OBJS += src/osdep/gui/sdltruetypefont.o
endif
	
ifeq ($(ANDROID), 1)
OBJS += src/osdep/gui/androidsdl_event.o \
	src/osdep/gui/PanelOnScreen.o
endif

# disable NEON helpers for AARCH64
ifndef AARCH64
ifdef HAVE_NEON
OBJS += src/osdep/neon_helper.o
src/osdep/neon_helper.o: src/osdep/neon_helper.s
	$(CXX) $(CFLAGS) -Wall -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s
else
OBJS += src/osdep/arm_helper.o
src/osdep/arm_helper.o: src/osdep/arm_helper.s
	$(CXX) $(CFLAGS) -Wall -o src/osdep/arm_helper.o -c src/osdep/arm_helper.s
endif
endif

OBJS += src/newcpu.o \
	src/newcpu_common.o \
	src/readcpu.o \
	src/cpudefs.o \
	src/cpustbl.o \
	src/cpuemu_0.o \
	src/cpuemu_4.o \
	src/cpuemu_11.o \
	src/cpuemu_40.o \
	src/cpuemu_44.o \
	src/jit/compemu.o \
	src/jit/compstbl.o \
	src/jit/compemu_fpp.o \
	src/jit/compemu_support.o

DEPS = $(OBJS:%.o=%.d) $(C_OBJS:%.o=%.d)

$(PROG): $(OBJS) $(C_OBJS)
	$(CXX) -o $(PROG) $(OBJS) $(C_OBJS) $(LDFLAGS)
ifndef DEBUG
# want to keep a copy of the binary before stripping? Then enable the below line
#	cp $(PROG) $(PROG)-debug
	$(STRIP) $(PROG)
endif

clean:
	$(RM) $(PROG) $(PROG)-debug $(C_OBJS) $(OBJS) $(ASMS) $(DEPS)
	$(MAKE) -C guisan-dev clean

cleanprofile:
	$(RM) $(OBJS:%.o=%.gcda)
	
bootrom:
	od -v -t xC -w8 src/filesys |tail -n +5 | sed -e "s,^.......,," -e "s,[0123456789abcdefABCDEF][0123456789abcdefABCDEF],db(0x&);,g" > src/filesys_bootrom.cpp
	touch src/filesys.cpp

guisan:
	$(MAKE) -C guisan-dev
	
-include $(DEPS)
