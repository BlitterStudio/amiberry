ifeq ($(PLATFORM),)
	PLATFORM = rpi3
endif

ifeq ($(PLATFORM),rpi3)
	CPU_FLAGS += -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
	MORE_CFLAGS += -DCAPSLOCK_DEBIAN_WORKAROUND -DARMV6T2
	LDFLAGS += -lbcm_host
	DEFS += -DRASPBERRY
	HAVE_NEON = 1
	HAVE_DISPMANX = 1
	USE_PICASSO96 = 1
else ifeq ($(PLATFORM),rpi2)
	CPU_FLAGS += -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	MORE_CFLAGS += -DCAPSLOCK_DEBIAN_WORKAROUND -DARMV6T2 
	LDFLAGS += -lbcm_host
	DEFS += -DRASPBERRY
	HAVE_NEON = 1
	HAVE_DISPMANX = 1
	USE_PICASSO96 = 1
else ifeq ($(PLATFORM),rpi1)
	CPU_FLAGS += -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard
	MORE_CFLAGS += -DCAPSLOCK_DEBIAN_WORKAROUND
	LDFLAGS += -lbcm_host
	DEFS += -DRASPBERRY
	HAVE_DISPMANX = 1
else ifeq ($(PLATFORM),generic-sdl)
	# On Raspberry Pi uncomment below line or remove ARMV6T2 define.
	#CPU_FLAGS= -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	MORE_CFLAGS += -DARMV6T2
	HAVE_SDL_DISPLAY = 1
else ifeq ($(PLATFORM),gles)
	# For Raspberry Pi uncomment the two below lines
	#LDFLAGS += -lbcm_host
	#CPU_FLAGS= -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	MORE_CFLAGS += -DARMV6T2
	HAVE_GLES_DISPLAY = 1
	HAVE_NEON = 1
endif

NAME   = uae4arm
RM     = rm -f
CXX    = g++
STRIP  = strip

PROG   = $(NAME)

all: $(PROG)

#DEBUG=1
#TRACER=1

PANDORA=1
#GEN_PROFILE=1
USE_PROFILE=1

DEFAULT_CFLAGS = $(CFLAGS) -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT

MY_LDFLAGS = $(LDFLAGS)
MY_LDFLAGS += -lSDL -lpthread -lz -lSDL_image -lpng -lrt -lxml2 -lFLAC -lmpg123 -ldl
MY_LDFLAGS += -lSDL_ttf -lguichan_sdl -lguichan -L/opt/vc/lib 

MORE_CFLAGS += -I/usr/include/libxml2
MORE_CFLAGS += -DGP2X -DPANDORA -DARMV6_ASSEMBLY -DWITH_INGAME_WARNING
MORE_CFLAGS += -DCPU_arm -DUSE_SDL
MORE_CFLAGS += -DROM_PATH_PREFIX=\"./kickstarts/\" -DDATA_PREFIX=\"./data/\" -DSAVE_PREFIX=\"./savestates/\"

ifeq ($(USE_PICASSO96), 1)
	MORE_CFLAGS += -DPICASSO96
endif

ifeq ($(HAVE_NEON), 1)
	MORE_CFLAGS += -DUSE_ARMNEON
endif

MORE_CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
MORE_CFLAGS += -Isrc -Isrc/od-pandora -Isrc/td-sdl -Isrc/include 
MORE_CFLAGS += -Wno-unused -Wno-format -Wno-write-strings -Wno-multichar
MORE_CFLAGS += -fuse-ld=gold -fdiagnostics-color=auto
MORE_CFLAGS += -mstructure-size-boundary=32
MORE_CFLAGS += -falign-functions=32

ifndef DEBUG
MORE_CFLAGS += -Ofast -pipe -fsingle-precision-constant
MORE_CFLAGS += -fexceptions -fpermissive
else
MORE_CFLAGS += -g -DDEBUG -Wl,--export-dynamic

ifdef TRACER
TRACE_CFLAGS = -DTRACER -finstrument-functions -Wall -rdynamic
endif

endif

ifdef GEN_PROFILE
MORE_CFLAGS += -fprofile-generate=/media/MAINSD/pandora/test -fprofile-arcs
endif
ifdef USE_PROFILE
MORE_CFLAGS += -fprofile-use -fbranch-probabilities -fvpt -funroll-loops -fpeel-loops -ftracer -ftree-loop-distribute-patterns
endif

ifdef GEN_PROFILE
MORE_CFLAGS += -fprofile-generate=/media/MAINSD/pandora/test -fprofile-arcs
endif
ifdef USE_PROFILE
MORE_CFLAGS += -fprofile-use -fbranch-probabilities -fvpt -funroll-loops -fpeel-loops -ftracer -ftree-loop-distribute-patterns
endif

MY_CFLAGS  = $(MORE_CFLAGS) $(DEFAULT_CFLAGS)

OBJS =	\
	src/akiko.o \
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
	src/cdrom.o \
	src/cfgfile.o \
	src/cia.o \
	src/crc32.o \
	src/custom.o \
	src/disk.o \
	src/diskutil.o \
	src/drawing.o \
	src/events.o \
	src/expansion.o \
	src/filesys.o \
	src/fpp.o \
	src/fsdb.o \
	src/fsdb_unix.o \
	src/fsusage.o \
	src/gfxutil.o \
	src/hardfile.o \
	src/inputdevice.o \
	src/keybuf.o \
	src/main.o \
	src/memory.o \
	src/native2amiga.o \
	src/rommgr.o \
	src/savestate.o \
	src/statusline.o \
	src/traps.o \
	src/uaelib.o \
	src/uaeresource.o \
	src/zfile.o \
	src/zfile_archive.o \
	src/archivers/7z/Archive/7z/7zAlloc.o \
	src/archivers/7z/Archive/7z/7zDecode.o \
	src/archivers/7z/Archive/7z/7zExtract.o \
	src/archivers/7z/Archive/7z/7zHeader.o \
	src/archivers/7z/Archive/7z/7zIn.o \
	src/archivers/7z/Archive/7z/7zItem.o \
	src/archivers/7z/7zBuf.o \
	src/archivers/7z/7zCrc.o \
	src/archivers/7z/7zStream.o \
	src/archivers/7z/Bcj2.o \
	src/archivers/7z/Bra.o \
	src/archivers/7z/Bra86.o \
	src/archivers/7z/LzmaDec.o \
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
	src/archivers/wrp/warp.o \
	src/archivers/zip/unzip.o \
	src/md-pandora/support.o \
	src/od-pandora/bsdsocket_host.o \
	src/od-pandora/cda_play.o \
	src/od-pandora/charset.o \
	src/od-pandora/fsdb_host.o \
	src/od-pandora/hardfile_pandora.o \
	src/od-pandora/keyboard.o \
	src/od-pandora/mp3decoder.o \
	src/od-pandora/writelog.o \
	src/od-pandora/pandora.o \
	src/od-pandora/pandora_filesys.o \
	src/od-pandora/pandora_input.o \
	src/od-pandora/pandora_gui.o \
	src/od-pandora/pandora_rp9.o \
	src/od-pandora/pandora_mem.o \
	src/od-pandora/sigsegv_handler.o \
	src/od-pandora/menu/menu_config.o \
	src/sd-sdl/sound_sdl_new.o \
	src/od-pandora/gui/UaeRadioButton.o \
	src/od-pandora/gui/UaeDropDown.o \
	src/od-pandora/gui/UaeCheckBox.o \
	src/od-pandora/gui/UaeListBox.o \
	src/od-pandora/gui/InGameMessage.o \
	src/od-pandora/gui/SelectorEntry.o \
	src/od-pandora/gui/ShowMessage.o \
	src/od-pandora/gui/SelectFolder.o \
	src/od-pandora/gui/SelectFile.o \
	src/od-pandora/gui/CreateFilesysHardfile.o \
	src/od-pandora/gui/EditFilesysVirtual.o \
	src/od-pandora/gui/EditFilesysHardfile.o \
	src/od-pandora/gui/PanelPaths.o \
	src/od-pandora/gui/PanelConfig.o \
	src/od-pandora/gui/PanelCPU.o \
	src/od-pandora/gui/PanelChipset.o \
	src/od-pandora/gui/PanelROM.o \
	src/od-pandora/gui/PanelRAM.o \
	src/od-pandora/gui/PanelFloppy.o \
	src/od-pandora/gui/PanelHD.o \
	src/od-pandora/gui/PanelDisplay.o \
	src/od-pandora/gui/PanelSound.o \
	src/od-pandora/gui/PanelInput.o \
	src/od-pandora/gui/PanelMisc.o \
	src/od-pandora/gui/PanelSavestate.o \
	src/od-pandora/gui/main_window.o \
	src/od-pandora/gui/Navigation.o
ifdef PANDORA
OBJS += src/od-pandora/gui/sdltruetypefont.o
endif

ifeq ($(HAVE_DISPMANX), 1)
OBJS += src/od-rasp/rasp_gfx.o
endif

ifeq ($(HAVE_SDL_DISPLAY), 1)
OBJS += src/od-pandora/pandora_gfx.o
endif

ifeq ($(HAVE_GLES_DISPLAY), 1)
OBJS += src/od-gles/gl.o
OBJS += src/od-gles/gl_platform.o
OBJS += src/od-gles/gles_gfx.o
MORE_CFLAGS += -DHAVE_GLES
MY_LDFLAGS += -lEGL -lGLESv1_CM
endif

ifeq ($(USE_PICASSO96), 1)
OBJS += src/od-pandora/picasso96.o
endif

ifeq ($(HAVE_NEON), 1)
OBJS += src/od-pandora/neon_helper.o
endif

OBJS += src/newcpu.o
OBJS += src/readcpu.o
OBJS += src/cpudefs.o
OBJS += src/cpustbl.o
OBJS += src/cpuemu_0.o
OBJS += src/cpuemu_4.o
OBJS += src/cpuemu_11.o
OBJS += src/jit/compemu.o
OBJS += src/jit/compstbl.o
OBJS += src/jit/compemu_fpp.o
OBJS += src/jit/compemu_support.o

src/od-pandora/neon_helper.o: src/od-pandora/neon_helper.s
	$(CXX) $(CPU_FLAGS) -falign-functions=32 -Wall -o src/od-pandora/neon_helper.o -c src/od-pandora/neon_helper.s

src/trace.o: src/trace.c
	$(CC) $(MORE_CFLAGS) -c src/trace.c -o src/trace.o

.cpp.o:
	$(CXX) $(MY_CFLAGS) $(TRACE_CFLAGS) -c $< -o $@

.cpp.s:
	$(CXX) $(MY_CFLAGS) -S -c $< -o $@

$(PROG): $(OBJS) src/trace.o
ifndef DEBUG
	$(CXX) $(MY_CFLAGS) -o $(PROG) $(OBJS) $(MY_LDFLAGS)
	$(STRIP) $(PROG)
else
	$(CXX) $(MY_CFLAGS) -o $(PROG) $(OBJS) src/trace.o $(MY_LDFLAGS)
endif

ASMS = \
	src/audio.s \
	src/autoconf.s \
	src/blitfunc.s \
	src/blitter.s \
	src/cia.s \
	src/custom.s \
	src/disk.s \
	src/drawing.s \
	src/events.s \
	src/expansion.s \
	src/filesys.s \
	src/fpp.s \
	src/fsdb.s \
	src/fsdb_unix.s \
	src/fsusage.s \
	src/gfxutil.s \
	src/hardfile.s \
	src/inputdevice.s \
	src/keybuf.s \
	src/main.s \
	src/memory.s \
	src/native2amiga.s \
	src/traps.s \
	src/uaelib.s \
	src/uaeresource.s \
	src/zfile.s \
	src/zfile_archive.s \
	src/md-pandora/support.s \
	src/od-pandora/picasso96.s \
	src/od-pandora/pandora.s \
	src/od-pandora/pandora_gfx.s \
	src/od-pandora/pandora_mem.s \
	src/od-pandora/sigsegv_handler.s \
	src/sd-sdl/sound_sdl_new.s \
	src/newcpu.s \
	src/readcpu.s \
	src/cpudefs.s \
	src/cpustbl.s \
	src/cpuemu_0.s \
	src/cpuemu_4.s \
	src/cpuemu_11.s \
	src/jit/compemu.s \
	src/jit/compemu_fpp.s \
	src/jit/compstbl.s \
	src/jit/compemu_support.s

genasm: $(ASMS)


clean:
	$(RM) $(PROG) $(OBJS) $(ASMS)

delasm:
	$(RM) $(ASMS)
