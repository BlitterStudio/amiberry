PREFIX	=/usr
$(shell ./link_pandora_dirs.sh)

#SDL_BASE = $(PREFIX)/bin/
SDL_BASE = 

NAME   = uae4arm
O      = o
RM     = rm -f
#CC     = gcc
#CXX    = g++
#STRIP  = strip
#AS     = as

PROG   = $(NAME)

all: $(PROG)

PANDORA=1

DEFAULT_CFLAGS = `$(SDL_BASE)sdl-config --cflags`
LDFLAGS = -lSDL -lpthread  -lz -lSDL_image -lpng -lrt

MORE_CFLAGS += -DGP2X -DPANDORA -DDOUBLEBUFFER -DARMV6_ASSEMBLY -DUSE_ARMNEON
MORE_CFLAGS += -DSUPPORT_THREADS -DUAE_FILESYS_THREADS -DNO_MAIN_IN_MAIN_C -DFILESYS -DAUTOCONFIG -DSAVESTATE
MORE_CFLAGS += -DDONT_PARSE_CMDLINE
#MORE_CFLAGS += -DWITH_LOGGING

MORE_CFLAGS += -DJIT -DCPU_arm -DARM_ASSEMBLY

MORE_CFLAGS += -Isrc -Isrc/gp2x -Isrc/menu -Isrc/include -Isrc/gp2x/menu -fomit-frame-pointer -Wno-unused -Wno-format -DUSE_SDL -DGCCCONSTFUNC="__attribute__((const))" -DUSE_UNDERSCORE -DUNALIGNED_PROFITABLE -DOPTIMIZED_FLAGS
LDFLAGS +=  -lSDL_ttf -lguichan_sdl -lguichan
MORE_CFLAGS += -fexceptions


MORE_CFLAGS += -DROM_PATH_PREFIX=\"./\" -DDATA_PREFIX=\"./data/\" -DSAVE_PREFIX=\"./saves/\"

MORE_CFLAGS += -msoft-float -ffast-math
ifndef DEBUG
MORE_CFLAGS += -O2
MORE_CFLAGS += -fstrict-aliasing -mstructure-size-boundary=32 -fexpensive-optimizations
MORE_CFLAGS += -fweb -frename-registers -fomit-frame-pointer
#MORE_CFLAGS += -falign-functions=32 -falign-loops -falign-labels -falign-jumps
MORE_CFLAGS += -falign-functions=32
MORE_CFLAGS += -finline -finline-functions -fno-builtin
#MORE_CFLAGS += -S
else
MORE_CFLAGS += -ggdb
endif

ASFLAGS += -mfloat-abi=soft

CFLAGS  = $(DEFAULT_CFLAGS) $(MORE_CFLAGS)
CFLAGS+= -DCPUEMU_0 -DCPUEMU_5 -DFPUEMU

OBJS =	\
	src/audio.o \
	src/autoconf.o \
	src/blitfunc.o \
	src/blittable.o \
	src/blitter.o \
	src/cfgfile.o \
	src/cia.o \
	src/crc32.o \
	src/custom.o \
	src/disk.o \
	src/drawing.o \
	src/ersatz.o \
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
	src/missing.o \
	src/native2amiga.o \
	src/savestate.o \
	src/scsi-none.o \
	src/traps.o \
	src/unzip.o \
	src/zfile.o \
	src/machdep/support.o \
	src/osdep/neon_helper.o \
	src/osdep/fsdb_host.o \
	src/osdep/joystick.o \
	src/osdep/keyboard.o \
	src/osdep/inputmode.o \
	src/osdep/writelog.o \
	src/osdep/pandora.o \
	src/osdep/pandora_filesys.o \
	src/osdep/pandora_gui.o \
	src/osdep/pandora_gfx.o \
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

OBJS += src/newcpu.o
OBJS += src/readcpu.o
OBJS += src/cpudefs.o
OBJS += src/cpustbl.o
OBJS += src/cpuemu_0.o
OBJS += src/cpuemu_5.o
OBJS += src/compemu.o
OBJS += src/compemu_fpp.o
OBJS += src/compstbl.o
OBJS += src/compemu_support.o

CPPFLAGS  = $(CFLAGS)

src/osdep/neon_helper.o: src/osdep/neon_helper.s
	$(CXX) -O3 -pipe -falign-functions=32 -march=armv7-a -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS)
ifndef DEBUG
	$(STRIP) $(PROG)
endif

clean:
	$(RM) $(PROG) $(OBJS)
