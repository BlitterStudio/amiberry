# Building the core into another program rather than loading it as a shared
# library:
#   STATIC_LINKING=1        build amiberry_libretro.a, not a shared core
#                           (TARGET=<path> names the file)
#   OBJDIR=<dir>            put the objects in <dir>, not next to their
#                           sources, so one tree can build several setups
#   EXTRA_SOURCES=<files>   the program's own .cpp and .c files, built with
#                           the core's flags so they can use its headers
#   NLOHMANN_JSON_INCLUDE=<dir>  the folder holding nlohmann/json.hpp,
#                           instead of downloading it into deps/
#   CPPFLAGS, CFLAGS, CXXFLAGS   added to every compile
#   NET_CFLAGS              added to the network sources only (SLiRP,
#                           slirp_uae.cpp, bsdsocket_host.cpp), e.g. to
#                           send their socket calls to the host's own stack
# For hosts without some of what the core expects (bare metal):
#   NO_JIT=1                leave the JIT out
#   NO_STD_THREAD=1         no std::thread: CHD's work queues run their
#                           work on the thread that queues it
#   NO_HOST_SIGNALS=1       no fault or SIGTERM handlers
#   NO_CHD_SPECIAL_FILES=1  CHD's file layer opens every name as a file
#                           (no "socket.", "domain." or pty names)
# Such a program can also make the calls in libretro/amiberry_embed.h.

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
LIBRETRO_INFO := $(TARGET_NAME)_libretro.info

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

# Clang leaves ASan runtime references unresolved in shared libraries so the
# frontend executable can provide the runtime. Keep strict symbol checking for
# normal builds, but do not make sanitizer builds reject those references.
ifneq ($(SANITIZE), 1)
   NO_UNDEFINED := -Wl,--no-undefined
endif

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   CHD_OSDLIB := $(AMIBERRY_DIR)/src/archivers/chd/osdlib_unix.cpp
   CHD_FILEIMP := $(AMIBERRY_DIR)/src/archivers/chd/posixfile.cpp
   CHD_DIR := $(AMIBERRY_DIR)/src/archivers/chd/posixdir.cpp
   CHD_PTTY := $(AMIBERRY_DIR)/src/archivers/chd/posixptty.cpp
   CHD_SOCKET := $(AMIBERRY_DIR)/src/archivers/chd/posixsocket.cpp
   ifeq ($(DEBUG), 1)
   SHARED := -shared $(NO_UNDEFINED) -Wl,-Bsymbolic-functions
   else
   SHARED := -shared $(NO_UNDEFINED) -Wl,--version-script=$(LIBRETRO_DIR)/link.T -Wl,-Bsymbolic-functions
   endif
else ifneq (,$(findstring android,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_android.so
   fpic := -fPIC
   CHD_OSDLIB := $(AMIBERRY_DIR)/src/archivers/chd/osdlib_unix.cpp
   CHD_FILEIMP := $(AMIBERRY_DIR)/src/archivers/chd/posixfile.cpp
   CHD_DIR := $(AMIBERRY_DIR)/src/archivers/chd/posixdir.cpp
   CHD_PTTY := $(AMIBERRY_DIR)/src/archivers/chd/posixptty.cpp
   CHD_SOCKET := $(AMIBERRY_DIR)/src/archivers/chd/posixsocket.cpp
   SHARED := -shared $(NO_UNDEFINED) -Wl,--version-script=$(LIBRETRO_DIR)/link.T -Wl,-Bsymbolic-functions
   # bionic grew forkpty() (uaelib's host shell) in API 23 and iconv() in
   # API 28, so the core targets API 28 — Android 9 and newer.
   ANDROID_API ?= 28
   ANDROID_NDK_LLVM ?= $(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64
   ifeq ($(platform), android-arm64)
      ANDROID_TRIPLE := aarch64-linux-android
      ARCH := aarch64
   else ifneq (,$(filter $(platform),android-x86_64 android-x64))
      ANDROID_TRIPLE := x86_64-linux-android
      ARCH := x86_64
   else
      $(error unknown Android platform '$(platform)')
   endif
   CC = $(ANDROID_NDK_LLVM)/bin/$(ANDROID_TRIPLE)$(ANDROID_API)-clang
   CXX = $(ANDROID_NDK_LLVM)/bin/$(ANDROID_TRIPLE)$(ANDROID_API)-clang++
   AR = $(ANDROID_NDK_LLVM)/bin/llvm-ar
   RANLIB = $(ANDROID_NDK_LLVM)/bin/llvm-ranlib
   STRIP = $(ANDROID_NDK_LLVM)/bin/llvm-strip
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   CHD_OSDLIB := $(AMIBERRY_DIR)/src/archivers/chd/osdlib_unix.cpp
   CHD_FILEIMP := $(AMIBERRY_DIR)/src/archivers/chd/posixfile.cpp
   CHD_DIR := $(AMIBERRY_DIR)/src/archivers/chd/posixdir.cpp
   CHD_PTTY := $(AMIBERRY_DIR)/src/archivers/chd/posixptty.cpp
   CHD_SOCKET := $(AMIBERRY_DIR)/src/archivers/chd/posixsocket.cpp
   SHARED := -dynamiclib
   LDFLAGS += -framework CoreFoundation -framework DiskArbitration -lz -liconv
else
   TARGET := $(TARGET_NAME)_libretro.dll
   CC ?= gcc
   CXX ?= g++
   CHD_OSDLIB := $(AMIBERRY_DIR)/src/archivers/chd/osdlib_win32.cpp
   CHD_FILEIMP := $(AMIBERRY_DIR)/src/archivers/chd/winfile.cpp
   CHD_DIR := $(AMIBERRY_DIR)/src/archivers/chd/windir.cpp
   CHD_PTTY := $(AMIBERRY_DIR)/src/archivers/chd/winptty.cpp
   CHD_SOCKET := $(AMIBERRY_DIR)/src/archivers/chd/winsocket.cpp
   SHARED := -shared -static -Wl,--no-undefined
endif

ifneq (,$(filter arm64 ARM64,$(ARCH)))
   override ARCH := aarch64
endif

ifeq ($(STATIC_LINKING), 1)
   TARGET := $(basename $(TARGET)).a
endif


LDFLAGS += $(fpic) $(SHARED)
FLAGS += $(fpic)
ifneq ($(DEBUG), 1)
FLAGS += -fvisibility=hidden
endif
FLAGS += -ffunction-sections -fdata-sections
ifneq ($(platform), osx)
LDFLAGS += -Wl,--gc-sections
else
LDFLAGS += -Wl,-dead_strip
endif

ifeq ($(DEBUG), 1)
   FLAGS += -O0 -g -fno-omit-frame-pointer
else
   FLAGS += -O3 -DNDEBUG
endif

ifeq ($(SANITIZE), 1)
   FLAGS += -fsanitize=address -fno-omit-frame-pointer -g
   LDFLAGS += -fsanitize=address
endif

# Extract version from root CMakeLists.txt (single source of truth)
AMIBERRY_VERSION := $(shell sed -n 's/^.*VERSION \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/p' $(AMIBERRY_DIR)/CMakeLists.txt | head -1)
ifeq ($(AMIBERRY_VERSION),)
AMIBERRY_VERSION := 0.0.0
endif
AMIBERRY_VERSION_MAJOR := $(word 1,$(subst ., ,$(AMIBERRY_VERSION)))
AMIBERRY_VERSION_MINOR := $(word 2,$(subst ., ,$(AMIBERRY_VERSION)))
AMIBERRY_VERSION_PATCH := $(word 3,$(subst ., ,$(AMIBERRY_VERSION)))
LIBRETRO_SUPPORTED_EXTENSIONS := $(shell sed -n 's/^[[:space:]]*info->valid_extensions = "\(.*\)";/\1/p' $(LIBRETRO_DIR)/libretro.cpp | head -1)

# Add Amiberry defines
AMIBERRY_DATADIR ?= .
AMIBERRY_LIBDIR ?= .
FLAGS += -DAMIBERRY -DLIBRETRO -DWITH_PPC -DWITH_QEMU_CPU -DUTF8PROC_STATIC -D_FILE_OFFSET_BITS=64 -DAMIBERRY_DATADIR=\"$(AMIBERRY_DATADIR)\" -DAMIBERRY_LIBDIR=\"$(AMIBERRY_LIBDIR)\"
FLAGS += -DAMIBERRY_VERSION=\"$(AMIBERRY_VERSION)\" -DAMIBERRY_VERSION_PRE_RELEASE=\"\"
FLAGS += -DAMIBERRY_VERSION_MAJOR=$(AMIBERRY_VERSION_MAJOR) -DAMIBERRY_VERSION_MINOR=$(AMIBERRY_VERSION_MINOR) -DAMIBERRY_VERSION_PATCH=$(AMIBERRY_VERSION_PATCH)
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

ifeq ($(ARCH),aarch64)
   FLAGS += -DCPU_AARCH64
else ifneq (,$(filter x86_64 amd64 AMD64,$(ARCH)))
   FLAGS += -DCPU_X86_64
endif

ifeq ($(platform), win)
   # Prefer llvm-mingw / MSYS2 CLANG64 on Windows: the main Amiberry build is
   # Clang/LLD-based, so the libretro core uses the same toolchain to keep ABI
   # and runtime libs aligned. Fall back to MinGW-w64 GCC if clang isn't found.
   #
   # NOTE: use `=` (not `?=`) because GNU make's implicit defaults
   # (CC=cc, CXX=g++) count as "already defined" for `?=`, so `?=` would be
   # a no-op and we'd fall back to `g++` which doesn't exist in CLANG64.
   # Command-line overrides like `make CC=foo CXX=bar` still win because the
   # command line beats makefile assignments in GNU make.
   ifneq ($(shell command -v x86_64-w64-mingw32-clang 2>/dev/null),)
      CC = x86_64-w64-mingw32-clang
      CXX = x86_64-w64-mingw32-clang++
   else ifneq ($(wildcard /clang64/bin/clang.exe),)
      CC = /clang64/bin/clang
      CXX = /clang64/bin/clang++
   else ifneq ($(shell command -v x86_64-w64-mingw32-gcc 2>/dev/null),)
      CC = x86_64-w64-mingw32-gcc
      CXX = x86_64-w64-mingw32-g++
   else ifneq ($(wildcard /mingw64/bin/gcc.exe),)
      CC = /mingw64/bin/gcc
      CXX = /mingw64/bin/g++
   endif
endif

# Ensure Windows target macros are set when using MinGW or llvm-mingw, even
# if platform isn't detected. clang's -dumpmachine on a mingw target reports
# e.g. x86_64-w64-windows-gnu, while GCC reports x86_64-w64-mingw32 — match
# either by looking for the "mingw" or "windows-gnu" substrings.
ifneq (,$(or $(findstring mingw,$(shell $(CXX) -dumpmachine 2>/dev/null)),$(findstring windows-gnu,$(shell $(CXX) -dumpmachine 2>/dev/null))))
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
INCFLAGS += -I$(AMIBERRY_DIR)/external/lhasa/lib
INCFLAGS += -I$(LIBRETRO_DIR)
INCFLAGS += -I$(LIBCO_DIR)
INCFLAGS += -I$(LIBRETRO_COMMON)/include
INCFLAGS += -I$(AMIBERRY_DIR)/external/imgui

ZLIB_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags zlib 2>/dev/null)
ZLIB_LIBS ?= $(shell $(PKG_CONFIG) --libs zlib 2>/dev/null)

INCFLAGS += $(ZLIB_CFLAGS)
LDFLAGS += $(ZLIB_LIBS)

# nlohmann/json (header-only): NLOHMANN_JSON_INCLUDE holds nlohmann/json.hpp.
# The default folder, deps/, gets the header downloaded once.
NLOHMANN_JSON_VERSION ?= v3.11.3
NLOHMANN_JSON_INCLUDE ?= $(LIBRETRO_DIR)/deps
NLOHMANN_JSON_HEADER := $(NLOHMANN_JSON_INCLUDE)/nlohmann/json.hpp
INCFLAGS += -I$(NLOHMANN_JSON_INCLUDE)

ifeq ($(platform), unix)
# forkpty() (uaelib's host shell) lives in libutil on glibc/BSD systems.
LDFLAGS += -ldl -lm -pthread -lutil
endif

ifneq (,$(findstring android,$(platform)))
# bionic keeps pthread and forkpty in libc; only the log sink is separate.
# NDK clang++ links libc++_shared by default, and RetroArch does not ship
# that library alongside downloaded cores, so link libc++ statically.
LDFLAGS += -ldl -lm -llog -static-libstdc++
endif

ifeq ($(platform), osx)
LDFLAGS += -lm -lpthread
endif

ifeq ($(platform), win)
# Target Windows 7 or newer: bsdsocket_host.cpp reads IP_ADAPTER_ADDRESSES
# fields (Ipv4Enabled, Ipv4Metric) that the MinGW headers only expose from
# Vista on, and default to the XP layout otherwise.
FLAGS += -D_WIN32_WINNT=0x0601
# llvm-mingw / MSYS2 CLANG64: clang's mingw driver does not auto-link
# libwinpthread the way GCC's does, so pthread/sem symbols (used by the
# SDL stubs in libretro/sdl_stub.o etc.) come up undefined at link time.
# Link winpthread explicitly. The link is also harmless under MinGW-w64
# GCC, where it's normally pulled in implicitly.
LDFLAGS += -liconv -lws2_32 -lz -liphlpapi -lgdi32 -lwinmm -lpthread
endif

# The target must have 8-byte pointers. Probed here, after the platform
# sections above have chosen the compiler (e.g. CLANG64 on Windows) and its
# flags. No "#" in the command: GNU make 3.81 (macOS) would read it as the
# start of a comment. tr strips the CR of Windows toolchains.
# Not when only clean/distclean are made: they must work without a target
# compiler. "make clean all" still probes.
ifneq (,$(if $(MAKECMDGOALS),$(filter-out clean distclean,$(MAKECMDGOALS)),all))
TARGET_POINTER_SIZE := $(strip $(shell printf '' | $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(FLAGS) -dM -E -x c++ - 2>/dev/null | grep __SIZEOF_POINTER__ | awk '{ print $$3 }' | tr -d '\r'))
ifeq ($(TARGET_POINTER_SIZE),)
   $(error Could not determine the target pointer size with $(CXX) (Amiberry requires a 64-bit target ABI); check the target compiler and flags)
else ifneq ($(TARGET_POINTER_SIZE),8)
   $(error Amiberry requires a 64-bit target ABI, $(CXX) targets $(TARGET_POINTER_SIZE)-byte pointers; check the target compiler and flags)
endif
endif

# Source files - Core Amiga Emulation
SOURCES_CORE := $(AMIBERRY_DIR)/src/a2065.cpp \
               $(AMIBERRY_DIR)/src/a2091.cpp \
               $(AMIBERRY_DIR)/src/akiko.cpp \
               $(AMIBERRY_DIR)/src/amax.cpp \
               $(AMIBERRY_DIR)/src/ar.cpp \
               $(AMIBERRY_DIR)/src/arcadia.cpp \
               $(AMIBERRY_DIR)/src/atonce.cpp \
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
               $(AMIBERRY_DIR)/src/fpp_softfloat.cpp \
               $(AMIBERRY_DIR)/src/softfloat/softfloat.cpp \
               $(AMIBERRY_DIR)/src/softfloat/softfloat_decimal.cpp \
               $(AMIBERRY_DIR)/src/softfloat/softfloat_fpsp.cpp \
               $(AMIBERRY_DIR)/src/framebufferboards.cpp \
               $(AMIBERRY_DIR)/src/zz9000.cpp \
               $(AMIBERRY_DIR)/src/zz9000_sdk.cpp \
               $(AMIBERRY_DIR)/src/zz9000_sdk_audio_codec.cpp \
               $(AMIBERRY_DIR)/src/zz9000_sdk_webp.cpp \
               $(AMIBERRY_DIR)/src/zz9000_sdk_media_av.cpp \
               $(AMIBERRY_DIR)/src/zz9000_sdk_console.cpp \
               $(AMIBERRY_DIR)/src/fsdb.cpp \
               $(AMIBERRY_DIR)/src/fsusage.cpp \
               $(AMIBERRY_DIR)/src/gayle.cpp \
               $(AMIBERRY_DIR)/src/gdb_monitor.cpp \
               $(AMIBERRY_DIR)/src/gdb_protocol.cpp \
               $(AMIBERRY_DIR)/src/gdb_server.cpp \
               $(AMIBERRY_DIR)/src/gdb_target.cpp \
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
                $(AMIBERRY_DIR)/src/osdep/amiberry_actions.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_curl.cpp \
                $(AMIBERRY_DIR)/src/osdep/perf_monitor.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_adpf.cpp \
                $(AMIBERRY_DIR)/src/osdep/host_detect.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_gfx.cpp \
                $(AMIBERRY_DIR)/src/osdep/gfx_window.cpp \
                $(AMIBERRY_DIR)/src/osdep/gfx_colors.cpp \
                $(AMIBERRY_DIR)/src/osdep/gfx_prefs_check.cpp \
                $(AMIBERRY_DIR)/src/osdep/imgui_osk.cpp \
                $(AMIBERRY_DIR)/src/osdep/display_modes.cpp \
                $(AMIBERRY_DIR)/src/osdep/renderer_factory.cpp \
                $(AMIBERRY_DIR)/src/osdep/sdl_renderer.cpp \
                $(AMIBERRY_DIR)/src/osdep/service_transport.cpp \
                $(AMIBERRY_DIR)/src/osdep/minigl_display.cpp \
                $(AMIBERRY_DIR)/src/osdep/dmabuf_surface.cpp \
                $(AMIBERRY_DIR)/src/osdep/gl_context_ladder.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_input.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_mem.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_serial.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_uaenet.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_hardfile.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_whdbooter.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_rp9.cpp \
                $(AMIBERRY_DIR)/src/osdep/rp9_manifest.cpp \
                $(AMIBERRY_DIR)/src/osdep/amiberry_filesys.cpp \
                $(AMIBERRY_DIR)/src/osdep/cda_play.cpp \
                $(AMIBERRY_DIR)/src/osdep/charset.cpp \
                $(AMIBERRY_DIR)/src/osdep/clipboard.cpp \
                $(AMIBERRY_DIR)/src/osdep/clipboard_common.cpp \
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
                $(AMIBERRY_DIR)/src/osdep/bsdsocket_host.cpp \
                $(AMIBERRY_DIR)/src/osdep/fsdb_host.cpp \
                $(AMIBERRY_DIR)/src/osdep/picasso96.cpp \
                $(AMIBERRY_DIR)/src/osdep/sigsegv_handler.cpp \
                $(AMIBERRY_DIR)/src/osdep/socket.cpp \
                $(AMIBERRY_DIR)/src/sounddep/sound.cpp \
                $(AMIBERRY_DIR)/src/threaddep/threading.cpp

ifeq ($(platform), osx)
SOURCES_OSDEP += $(AMIBERRY_DIR)/src/osdep/macos_authopen.cpp
endif

# libretro-common (VFS helpers + encoding/time)
SOURCES_LIBRETRO_COMMON := $(LIBRETRO_COMMON)/file/file_path.c \
                          $(LIBRETRO_COMMON)/file/file_path_io.c \
                          $(LIBRETRO_COMMON)/file/retro_dirent.c \
                          $(LIBRETRO_COMMON)/streams/file_stream.c \
                          $(LIBRETRO_COMMON)/string/stdstring.c \
                          $(LIBRETRO_COMMON)/compat/compat_strl.c \
                          $(LIBRETRO_COMMON)/compat/fopen_utf8.c \
                          $(LIBRETRO_COMMON)/encodings/encoding_utf.c \
                          $(LIBRETRO_COMMON)/time/rtime.c \
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

# The JIT is always left out of 64-bit x86 libretro builds (addressing
# 32-bit pointer checks).
ifneq (,$(findstring x86_64,$(ARCH))$(findstring amd64,$(ARCH)))
   override NO_JIT := 1
endif
ifeq ($(NO_JIT), 1)
   FLAGS += -DLIBRETRO_NO_JIT
   SOURCES_JIT :=
endif

ifeq ($(NO_STD_THREAD), 1)
   FLAGS += -DAMIBERRY_NO_STD_THREAD
endif
ifeq ($(NO_HOST_SIGNALS), 1)
   FLAGS += -DAMIBERRY_NO_HOST_SIGNALS
endif
ifeq ($(NO_CHD_SPECIAL_FILES), 1)
   ifeq ($(platform), win)
      $(error NO_CHD_SPECIAL_FILES is for the POSIX file layer, not platform=win)
   endif
   FLAGS += -DAMIBERRY_NO_CHD_SPECIAL_FILES
   CHD_PTTY :=
   CHD_SOCKET :=
endif

SOURCES_PPC := $(AMIBERRY_DIR)/src/ppc/ppc.cpp \
              $(AMIBERRY_DIR)/src/ppc/ppcd.cpp

SOURCES_QEMUVGA := $(AMIBERRY_DIR)/src/qemuvga/qemu.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/qemuuaeglue.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/vga.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/cirrus_vga.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/esp.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/lsi53c710.cpp \
                  $(AMIBERRY_DIR)/src/qemuvga/lsi53c895a.cpp \
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
                  $(AMIBERRY_DIR)/src/archivers/lha/uae_lha.cpp \
                  $(AMIBERRY_DIR)/src/archivers/lzx/lzx.cpp \
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
                  $(CHD_OSDLIB) \
                  $(AMIBERRY_DIR)/src/archivers/chd/osdsync.cpp \
                  $(AMIBERRY_DIR)/src/archivers/chd/palette.cpp \
                  $(CHD_DIR) \
                  $(CHD_FILEIMP) \
                  $(CHD_PTTY) \
                  $(CHD_SOCKET) \
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

# Object paths. Without OBJDIR an object sits next to its source. With it,
# ../src/x.cpp becomes $(OBJDIR)/src/x.o and ./x.cpp $(OBJDIR)/libretro/x.o
# (../libretro/x.cpp is the same file, so one pattern rule builds both);
# a source elsewhere, such as an EXTRA_SOURCES file, goes under
# $(OBJDIR)/extra/ by its whole path.
ifeq ($(OBJDIR),)
obj_of = $(addsuffix .o,$(basename $(1)))
else
obj_rel = $(if $(filter $(AMIBERRY_DIR)/%,$(1)),$(patsubst $(AMIBERRY_DIR)/%,%,$(1)),$(if $(filter $(LIBRETRO_DIR)/%,$(1)),libretro/$(patsubst $(LIBRETRO_DIR)/%,%,$(1)),extra/$(subst ..,__,$(subst :,,$(1)))))
obj_of = $(foreach s,$(1),$(OBJDIR)/$(basename $(call obj_rel,$(s))).o)
endif

SOURCES_CXX := $(SOURCES_CORE) $(SOURCES_OSDEP) $(SOURCES_CPU) $(SOURCES_JIT) $(SOURCES_PPC) $(SOURCES_QEMUVGA) $(SOURCES_ARCHIVE) $(SOURCES_SLIRP) $(SOURCES_MISC)

NET_SOURCES := $(SOURCES_SLIRP) $(AMIBERRY_DIR)/src/slirp_uae.cpp $(AMIBERRY_DIR)/src/osdep/bsdsocket_host.cpp
ifneq ($(NET_CFLAGS),)
$(call obj_of,$(NET_SOURCES)): FLAGS += $(NET_CFLAGS)
endif

# 86Box (x86 bridgeboards, DraCo, RTG boards): source lists, the opt-in
# USE_86BOX wiring and the per-object flags live in 86box.mk.
include 86box.mk

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

# Lhasa (ISC, vendored in external/lhasa): the decoders #include
# bit_stream_reader.c, lh_new_decoder.c, pma_common.c and tree_decode.c.
LHASA_DIR := $(AMIBERRY_DIR)/external/lhasa/lib
SOURCES_LHASA := $(addprefix $(LHASA_DIR)/,crc16.c ext_header.c lha_decoder.c \
                 lha_endian.c lha_file_header.c lha_input_stream.c \
                 lha_basic_reader.c lha_reader.c macbinary.c null_decoder.c \
                 lh1_decoder.c lh5_decoder.c lh6_decoder.c lh7_decoder.c \
                 lhx_decoder.c lk7_decoder.c lz5_decoder.c lzs_decoder.c \
                 pm1_decoder.c pm2_decoder.c)
ifeq ($(platform), win)
SOURCES_LHASA += $(LHASA_DIR)/lha_arch_win32.c
else
SOURCES_LHASA += $(LHASA_DIR)/lha_arch_unix.c
endif

SOURCES_C := $(SOURCES_7Z) $(SOURCES_LHASA) $(LIBCO_DIR)/libco.c $(SOURCES_LIBRETRO_COMMON)

SOURCES_CXX += $(filter %.cpp,$(EXTRA_SOURCES))
SOURCES_C += $(filter %.c,$(EXTRA_SOURCES))
ifneq ($(filter-out %.cpp %.c,$(EXTRA_SOURCES)),)
   $(error EXTRA_SOURCES takes .cpp and .c files: $(filter-out %.cpp %.c,$(EXTRA_SOURCES)))
endif

OBJECTS := $(call obj_of,$(SOURCES_CXX) $(SOURCES_C))

all: $(TARGET) $(LIBRETRO_INFO)

$(NLOHMANN_JSON_HEADER):
	@mkdir -p $(dir $@)
	curl -sfL -o $@ https://github.com/nlohmann/json/releases/download/$(NLOHMANN_JSON_VERSION)/json.hpp

ifeq ($(STATIC_LINKING), 1)
$(TARGET): $(OBJECTS)
	@rm -f $@
	$(AR) rcs $@ $^
else
$(TARGET): $(OBJECTS)
	$(CXX) $(INCFLAGS) $(FLAGS) -o $@ $^ $(LDFLAGS)
endif

# Canonical core info entry: whenever it changes, submit the generated file to
# libretro/libretro-super as dist/info/amiberry_libretro.info.
# libretro/libretro-core-info only mirrors that copy.
$(LIBRETRO_INFO): $(LIBRETRO_DIR)/amiberry_libretro.info.in $(LIBRETRO_DIR)/libretro.cpp $(AMIBERRY_DIR)/CMakeLists.txt
	sed -e 's/@SUPPORTED_EXTENSIONS@/$(LIBRETRO_SUPPORTED_EXTENSIONS)/' \
		-e 's/@VERSION@/$(AMIBERRY_VERSION)/' $< > $@

# STD_CXX is a variable so 86box.mk can build the imported 86Box sources as
# C++20 (see 86box.mk); everything else stays on C++17.
STD_CXX := -std=gnu++17
COMPILE_CXX = $(CXX) $(INCFLAGS) $(FLAGS) $(STD_CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
COMPILE_C = $(CC) $(INCFLAGS) $(FLAGS) -std=gnu99 $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp | $(NLOHMANN_JSON_HEADER)
	$(COMPILE_CXX)

%.o: %.c
	$(COMPILE_C)

ifneq ($(OBJDIR),)
$(OBJDIR)/%.o: $(AMIBERRY_DIR)/%.cpp | $(NLOHMANN_JSON_HEADER)
	@mkdir -p $(@D)
	$(COMPILE_CXX)

$(OBJDIR)/%.o: $(AMIBERRY_DIR)/%.c
	@mkdir -p $(@D)
	$(COMPILE_C)

define outside_source_rule
$(call obj_of,$(1)): $(1) | $(NLOHMANN_JSON_HEADER)
	@mkdir -p $$(@D)
	$$(COMPILE_$(if $(filter %.cpp,$(1)),CXX,C))
endef
$(foreach s,$(filter-out $(AMIBERRY_DIR)/% $(LIBRETRO_DIR)/%,$(SOURCES_CXX) $(SOURCES_C)),$(eval $(call outside_source_rule,$(s))))
endif

ifeq ($(platform), win)
# On Windows, GCC/Clang select libco's amd64.c raw stack-switching backend for
# x86_64. Those stacks are not registered in the TEB, so SEH-based C++
# exception unwinding (used by mingw GCC for throws, including the routine
# caught throws inside the vendored MAME CHD code) cannot dispatch on the
# emulation cothread stack and kills the process. Undefine __amd64__ for this
# TU so libco falls through to fiber.c, which uses the Win32 Fiber API
# (CreateFiber switches TEB stack bounds; exceptions unwind correctly).
$(call obj_of,$(LIBCO_DIR)/libco.c): FLAGS += -U__amd64__
endif

clean:
	rm -f $(OBJECTS) $(TARGET) $(LIBRETRO_INFO)

distclean: clean
	rm -rf $(LIBRETRO_DIR)/deps

.PHONY: clean all
