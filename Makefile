ifeq ($(PLATFORM),)
	PLATFORM = rpi3
endif

ifeq ($(PLATFORM),rpi3)
	CPU_FLAGS += -std=gnu++14 -march=armv8-a -mfpu=neon-fp-armv8 -mfloat-abi=hard
	MORE_CFLAGS += -DARMV6T2 -DUSE_ARMNEON
else ifeq ($(PLATFORM),rpi2)
	CPU_FLAGS += -std=gnu++14 -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard
	MORE_CFLAGS += -DARMV6T2 -DUSE_ARMNEON
else ifeq ($(PLATFORM),rpi1)
	CPU_FLAGS += -std=gnu++14 -march=armv6zk -mfpu=vfp -mfloat-abi=hard
endif

NAME  = amiberry-sdl2
RM      = rm -f
CC      = gcc
CXX    = g++
STRIP  = strip

PROG   = $(NAME)

all: guisan $(PROG)

guisan:
	$(MAKE) -C src/guisan

#DEBUG=1

SDL_CFLAGS = `sdl2-config --cflags --libs`

DEFS += `xml2-config --cflags`
DEFS += -DARMV6_ASSEMBLY -DAMIBERRY -DCPU_arm
DEFS += -DCAPSLOCK_DEBIAN_WORKAROUND
DEFS += -DROM_PATH_PREFIX=\"./\" -DDATA_PREFIX=\"./data/\" -DSAVE_PREFIX=\"./saves/\"
DEFS += -DUSE_SDL

MORE_CFLAGS += -Isrc -Isrc/osdep -Isrc/threaddep -Isrc/include -Isrc/guisan/include
MORE_CFLAGS += -Wno-unused -Wno-format -DGCCCONSTFUNC="__attribute__((const))"
MORE_CFLAGS += -fexceptions -fpermissive

LDFLAGS += -lpthread -lm -lz -lpng -lrt -lxml2 -lFLAC -lmpg123 -ldl
LDFLAGS += -lSDL2 -lSDL2_image -lSDL2_ttf -lguisan -L/opt/vc/lib -Lsrc/guisan/lib

ifndef DEBUG
MORE_CFLAGS += -Ofast -pipe -Wno-write-strings 
else
MORE_CFLAGS += -g -DDEBUG -Wl,--export-dynamic
MORE_CFLAGS += -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
LDFLAGS += -ltcmalloc -lprofiler

endif

ASFLAGS += $(CPU_FLAGS)

CXXFLAGS += $(SDL_CFLAGS) $(CPU_FLAGS) $(DEFS) $(MORE_CFLAGS)

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
	src/archivers/7z/7zAlloc.o \
	src/archivers/7z/7zDecode.o \
	src/archivers/7z/7zExtract.o \
	src/archivers/7z/7zHeader.o \
	src/archivers/7z/7zIn.o \
	src/archivers/7z/7zItem.o \
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
	src/osdep/bsdsocket_host.o \
	src/osdep/cda_play.o \
	src/osdep/charset.o \
	src/osdep/fsdb_host.o \
	src/osdep/hardfile_amiberry.o \
	src/osdep/keyboard_amiberry.o \
	src/osdep/mp3decoder.o \
	src/osdep/writelog.o \
	src/osdep/amiberry.o \
	src/osdep/amiberry_filesys.o \
	src/osdep/amiberry_input.o \
	src/osdep/amiberry_gfx.o \
	src/osdep/amiberry_gui.o \
	src/osdep/amiberry_rp9.o \
	src/osdep/amiberry_mem.o \
	src/osdep/sigsegv_handler.o \
	src/osdep/menu/menu_config.o \
	src/sounddep/sound_sdl_new.o \
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
	
OBJS += src/osdep/picasso96.o

ifeq ($(PLATFORM),rpi1)
	OBJS += src/osdep/arm_helper.o
else
	OBJS += src/osdep/neon_helper.o
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
	$(CXX) $(CPU_FLAGS) -Wall -o src/osdep/neon_helper.o -c src/osdep/neon_helper.s

src/osdep/arm_helper.o: src/osdep/arm_helper.s
	$(CXX) $(CPU_FLAGS) -Wall -o src/osdep/arm_helper.o -c src/osdep/arm_helper.s
	
$(PROG): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS)
ifndef DEBUG
	$(STRIP) $(PROG)
endif

clean:
	$(RM) $(PROG) $(OBJS)
