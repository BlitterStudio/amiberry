ifeq ($(PLATFORM),)
	PLATFORM = rpi3
endif

ifeq ($(PLATFORM),rpi3)
	CPU_FLAGS += -march=armv8-a -mfpu=neon-fp-armv8 -mfloat-abi=hard
	MORE_CFLAGS += -DRASPBERRY -DCAPSLOCK_DEBIAN_WORKAROUND -DARMV6T2 -DUSE_ARMNEON
	LDFLAGS += -lbcm_host
	HAVE_DISPMANX = 1
else ifeq ($(PLATFORM),rpi2)
	CPU_FLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard
	MORE_CFLAGS += -DRASPBERRY -DCAPSLOCK_DEBIAN_WORKAROUND -DARMV6T2 -DUSE_ARMNEON
	LDFLAGS += -lbcm_host
	HAVE_DISPMANX = 1
else ifeq ($(PLATFORM),rpi1)
	CPU_FLAGS += -march=armv6zk -mfpu=vfp -mfloat-abi=hard
	MORE_CFLAGS += -DRASPBERRY -DCAPSLOCK_DEBIAN_WORKAROUND
	LDFLAGS += -lbcm_host
	HAVE_DISPMANX = 1
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
#USE_PROFILE=1

DEFAULT_CFLAGS = $(CFLAGS) `sdl-config --cflags`

MY_LDFLAGS = $(LDFLAGS)
MY_LDFLAGS += -lSDL -lpthread -lz -lSDL_image -lpng -lrt -lxml2 -lFLAC -lmpg123 -ldl
MY_LDFLAGS += -lSDL_ttf -lguichan_sdl -lguichan -L/opt/vc/lib 

MORE_CFLAGS += -I/usr/include/libxml2
MORE_CFLAGS += -DPANDORA -DARMV6_ASSEMBLY -DWITH_INGAME_WARNING
MORE_CFLAGS += -DCPU_arm -DUSE_SDL
MORE_CFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads
MORE_CFLAGS += -Isrc -Isrc/osdep -Isrc/td-sdl -Isrc/include 
MORE_CFLAGS += -Wno-unused -Wno-format -Wno-write-strings -Wno-multichar
#MORE_CFLAGS += -fuse-ld=gold
MORE_CFLAGS += -fdiagnostics-color=auto
MORE_CFLAGS += -mstructure-size-boundary=32
MORE_CFLAGS += -falign-functions=32

TRACE_CFLAGS = 

ifndef DEBUG
MORE_CFLAGS += -Ofast -pipe
MORE_CFLAGS += -fweb -frename-registers
MORE_CFLAGS += -fipa-pta -fgcse-las -funroll-loops -ftracer -funswitch-loops
else
MORE_CFLAGS += -g -DDEBUG -Wl,--export-dynamic

ifdef TRACER
TRACE_CFLAGS = -DTRACER -finstrument-functions -Wall -rdynamic
endif

endif

ifdef GEN_PROFILE
MORE_CFLAGS += -fprofile-generate=/media/MAINSD/pandora/test -fprofile-arcs -fvpt
endif
ifdef USE_PROFILE
MORE_CFLAGS += -fprofile-use -fbranch-probabilities -fvpt
endif

MY_CFLAGS  = $(CPU_FLAGS) $(MORE_CFLAGS) $(DEFAULT_CFLAGS)

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
	src/gfxboard.o \
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
	src/machdep/support.o \
	src/osdep/neon_helper.o \
	src/osdep/bsdsocket_host.o \
	src/osdep/cda_play.o \
	src/osdep/charset.o \
	src/osdep/fsdb_host.o \
	src/osdep/hardfile_pandora.o \
	src/osdep/keyboard.o \
	src/osdep/mp3decoder.o \
	src/osdep/picasso96.o \
	src/osdep/writelog.o \
	src/osdep/pandora.o \
	src/osdep/pandora_filesys.o \
	src/osdep/pandora_input.o \
	src/osdep/pandora_gui.o \
	src/osdep/pandora_rp9.o \
	src/osdep/pandora_mem.o \
	src/osdep/sigsegv_handler.o \
	src/osdep/menu/menu_config.o \
	src/sounddep/sound.o \
	src/osdep/gui/UaeRadioButton.o \
	src/osdep/gui/UaeDropDown.o \
	src/osdep/gui/UaeCheckBox.o \
	src/osdep/gui/UaeListBox.o \
	src/osdep/gui/InGameMessage.o \
	src/osdep/gui/SelectorEntry.o \
	src/osdep/gui/ShowMessage.o \
	src/osdep/gui/SelectFolder.o \
	src/osdep/gui/SelectFile.o \
	src/osdep/gui/CreateFilesysHardfile.o \
	src/osdep/gui/EditFilesysVirtual.o \
	src/osdep/gui/EditFilesysHardfile.o \
	src/osdep/gui/PanelPaths.o \
	src/osdep/gui/PanelConfig.o \
	src/osdep/gui/PanelCPU.o \
	src/osdep/gui/PanelChipset.o \
	src/osdep/gui/PanelROM.o \
	src/osdep/gui/PanelRAM.o \
	src/osdep/gui/PanelFloppy.o \
	src/osdep/gui/PanelHD.o \
	src/osdep/gui/PanelDisplay.o \
	src/osdep/gui/PanelSound.o \
	src/osdep/gui/PanelInput.o \
	src/osdep/gui/PanelMisc.o \
	src/osdep/gui/PanelSavestate.o \
	src/osdep/gui/main_window.o \
	src/osdep/gui/Navigation.o
ifdef PANDORA
OBJS += src/osdep/gui/sdltruetypefont.o
endif

ifdef DEBUG
OBJS += src/trace.o
endif

ifeq ($(HAVE_DISPMANX), 1)
OBJS += src/osdep/rasp_gfx.o
MORE_CFLAGS += -DHAVE_DISPMANX
endif

ifeq ($(HAVE_SDL_DISPLAY), 1)
OBJS += src/osdep/pandora_gfx.o
endif

OBJS += src/newcpu.o
OBJS += src/newcpu_common.o
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

src/osdep/neon_helper.o: src/osdep/neon_helper.s
	$(CXX) $(CPU_FLAGS) -falign-functions=32 -Wall -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s

src/trace.o: src/trace.c
	$(CC) $(MORE_CFLAGS) -std=c11 -c src/trace.c -o src/trace.o

.cpp.o:
	$(CXX) $(MY_CFLAGS) $(TRACE_CFLAGS) -c $< -o $@

.cpp.s:
	$(CXX) $(MY_CFLAGS) -S -c $< -o $@

$(PROG): $(OBJS)
ifndef DEBUG
	$(CXX) $(MY_CFLAGS) -o $(PROG) $(OBJS) $(MY_LDFLAGS)
	$(STRIP) $(PROG)
else
	$(CXX) $(MY_CFLAGS) -o $(PROG) $(OBJS) $(MY_LDFLAGS)
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
	src/machdep/support.s \
	src/osdep/picasso96.s \
	src/osdep/pandora.s \
	src/osdep/pandora_gfx.s \
	src/osdep/pandora_mem.s \
	src/osdep/sigsegv_handler.s \
	src/sounddep/sound.s \
	src/newcpu.s \
	src/newcpu_common.s \
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