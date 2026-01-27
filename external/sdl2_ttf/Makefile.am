# Makefile.am for the SDL truetype font loading library and viewer
ACLOCAL_AMFLAGS = -I acinclude

lib_LTLIBRARIES = libSDL2_ttf.la

libSDL2_ttfincludedir = $(includedir)/SDL2
libSDL2_ttfinclude_HEADERS =	\
	SDL_ttf.h

libSDL2_ttf_la_SOURCES =		\
	SDL_ttf.c $(LOCAL_FT2_SOURCES) $(LOCAL_HB_SOURCES)

libSDL2_ttf_la_CPPFLAGS = $(TTF_CFLAGS) $(LOCAL_FT2_FLAGS) $(LOCAL_HB_FLAGS)

FREETYPE_PATH = external/freetype
FREETYPE_SOURCES = \
	$(FREETYPE_PATH)/src/autofit/autofit.c \
	$(FREETYPE_PATH)/src/base/ftbase.c \
	$(FREETYPE_PATH)/src/base/ftbbox.c \
	$(FREETYPE_PATH)/src/base/ftbdf.c \
	$(FREETYPE_PATH)/src/base/ftbitmap.c \
	$(FREETYPE_PATH)/src/base/ftcid.c \
	$(FREETYPE_PATH)/src/base/ftdebug.c \
	$(FREETYPE_PATH)/src/base/ftfstype.c \
	$(FREETYPE_PATH)/src/base/ftgasp.c \
	$(FREETYPE_PATH)/src/base/ftglyph.c \
	$(FREETYPE_PATH)/src/base/ftgxval.c \
	$(FREETYPE_PATH)/src/base/ftinit.c \
	$(FREETYPE_PATH)/src/base/ftmm.c \
	$(FREETYPE_PATH)/src/base/ftotval.c \
	$(FREETYPE_PATH)/src/base/ftpatent.c \
	$(FREETYPE_PATH)/src/base/ftpfr.c \
	$(FREETYPE_PATH)/src/base/ftstroke.c \
	$(FREETYPE_PATH)/src/base/ftsynth.c \
	$(FREETYPE_PATH)/src/base/ftsystem.c \
	$(FREETYPE_PATH)/src/base/fttype1.c \
	$(FREETYPE_PATH)/src/base/ftwinfnt.c \
	$(FREETYPE_PATH)/src/bdf/bdf.c \
	$(FREETYPE_PATH)/src/bzip2/ftbzip2.c \
	$(FREETYPE_PATH)/src/cache/ftcache.c \
	$(FREETYPE_PATH)/src/cff/cff.c \
	$(FREETYPE_PATH)/src/cid/type1cid.c \
	$(FREETYPE_PATH)/src/gzip/ftgzip.c \
	$(FREETYPE_PATH)/src/lzw/ftlzw.c \
	$(FREETYPE_PATH)/src/pcf/pcf.c \
	$(FREETYPE_PATH)/src/pfr/pfr.c \
	$(FREETYPE_PATH)/src/psaux/psaux.c \
	$(FREETYPE_PATH)/src/pshinter/pshinter.c \
	$(FREETYPE_PATH)/src/psnames/psmodule.c \
	$(FREETYPE_PATH)/src/raster/raster.c \
	$(FREETYPE_PATH)/src/sdf/sdf.c \
	$(FREETYPE_PATH)/src/sfnt/sfnt.c \
	$(FREETYPE_PATH)/src/smooth/smooth.c \
	$(FREETYPE_PATH)/src/svg/svg.c \
	$(FREETYPE_PATH)/src/truetype/truetype.c \
	$(FREETYPE_PATH)/src/type1/type1.c \
	$(FREETYPE_PATH)/src/type42/type42.c \
	$(FREETYPE_PATH)/src/winfonts/winfnt.c

if USE_BUILTIN_FREETYPE
LOCAL_FT2_FLAGS = -I$(srcdir)/$(FREETYPE_PATH)/include -DFT2_BUILD_LIBRARY -DFT_PUBLIC_FUNCTION_ATTRIBUTE=
LOCAL_FT2_SOURCES = $(FREETYPE_SOURCES)
endif

HARFBUZZ_PATH = external/harfbuzz
HARFBUZZ_SOURCES = \
	$(HARFBUZZ_PATH)/src/hb-aat-layout.cc \
	$(HARFBUZZ_PATH)/src/hb-aat-map.cc \
	$(HARFBUZZ_PATH)/src/hb-blob.cc \
	$(HARFBUZZ_PATH)/src/hb-buffer-serialize.cc \
	$(HARFBUZZ_PATH)/src/hb-buffer-verify.cc \
	$(HARFBUZZ_PATH)/src/hb-buffer.cc \
	$(HARFBUZZ_PATH)/src/hb-common.cc \
	$(HARFBUZZ_PATH)/src/hb-draw.cc \
	$(HARFBUZZ_PATH)/src/hb-face.cc \
	$(HARFBUZZ_PATH)/src/hb-fallback-shape.cc \
	$(HARFBUZZ_PATH)/src/hb-font.cc \
	$(HARFBUZZ_PATH)/src/hb-ft.cc \
	$(HARFBUZZ_PATH)/src/hb-number.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-cff1-table.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-cff2-table.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-color.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-face.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-font.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-layout.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-map.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-math.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-metrics.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-arabic.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-default.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-hangul.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-hebrew.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-indic.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-indic-table.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-khmer.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-myanmar.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-syllabic.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-thai.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-use.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shaper-vowel-constraints.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shape.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shape-fallback.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-shape-normalize.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-tag.cc \
	$(HARFBUZZ_PATH)/src/hb-ot-var.cc \
	$(HARFBUZZ_PATH)/src/hb-outline.cc \
	$(HARFBUZZ_PATH)/src/hb-paint.cc \
	$(HARFBUZZ_PATH)/src/hb-paint-extents.cc \
	$(HARFBUZZ_PATH)/src/hb-set.cc \
	$(HARFBUZZ_PATH)/src/hb-shape-plan.cc \
	$(HARFBUZZ_PATH)/src/hb-shape.cc \
	$(HARFBUZZ_PATH)/src/hb-shaper.cc \
	$(HARFBUZZ_PATH)/src/hb-static.cc \
	$(HARFBUZZ_PATH)/src/hb-ucd.cc \
	$(HARFBUZZ_PATH)/src/hb-coretext.cc \
	$(HARFBUZZ_PATH)/src/hb-gdi.cc \
	$(HARFBUZZ_PATH)/src/hb-uniscribe.cc \
	$(HARFBUZZ_PATH)/src/hb-unicode.cc

if USE_BUILTIN_HARFBUZZ
LOCAL_HB_FLAGS = -I$(srcdir)/$(HARFBUZZ_PATH) -I$(srcdir)/$(HARFBUZZ_PATH)/src -DHAVE_CONFIG_H -DFT_CONFIG_OPTION_USE_HARFBUZZ
LOCAL_HB_SOURCES = $(HARFBUZZ_SOURCES)
if OS_WIN32
LINKER = $(CXXLINK)
else
LINKER = $(LINK)
endif
else
LINKER = $(LINK)
endif

libSDL2_ttf_la_LDFLAGS = 	\
	-no-undefined		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(LT_EXTRA)
if USE_VERSION_RC
libSDL2_ttf_la_DEPENDENCIES = version.o
endif
libSDL2_ttf_la_LINK = $(LINKER) $(libSDL2_ttf_la_LDFLAGS)
libSDL2_ttf_la_LIBADD = $(TTF_LIBS) @MATHLIB@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = SDL2_ttf.pc

.rc.o:
	$(RC) $< $@

EXTRA_DIST = 			\
	.gitmodules		\
	Android.mk 		\
	CHANGES.txt 		\
	CMakeLists.txt 		\
	LICENSE.txt 		\
	Makefile.os2 		\
	README.txt 		\
	SDL2_ttf.pc.in		\
	SDL2_ttf.spec.in	\
	SDL2_ttfConfig.cmake.in	\
	VisualC 		\
	VisualC-WinRT 		\
	Xcode 			\
	autogen.sh		\
	cmake			\
	external		\
	mingw			\
	sdl2_ttf-config-version.cmake.in \
	sdl2_ttf-config.cmake.in \
	version.rc

noinst_PROGRAMS = showfont glfont

showfont_LDADD = libSDL2_ttf.la
glfont_LDADD = libSDL2_ttf.la @GL_LIBS@ @MATHLIB@

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: distcheck

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz

distclean-local:
	-rm -rf `find $(srcdir) -name .deps`
dist-hook:
	-rm -rf `find $(distdir) -name .deps`
	-rm -rf `find $(distdir)/external -name '.git*'`
	-rm -rf `find $(distdir)/external -name '.ci*'`
	-rm -rf `find $(distdir)/external -name .dirstamp`
	-rm -rf $(distdir)/external/harfbuzz/test

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)/cmake/SDL2_ttf
	$(INSTALL) -m 644 sdl2_ttf-config.cmake $(DESTDIR)$(libdir)/cmake/SDL2_ttf
	$(INSTALL) -m 644 sdl2_ttf-config-version.cmake $(DESTDIR)$(libdir)/cmake/SDL2_ttf

uninstall-hook:
	rm $(DESTDIR)$(libdir)/cmake/SDL2_ttf/sdl2_ttf-config.cmake
	rm $(DESTDIR)$(libdir)/cmake/SDL2_ttf/sdl2_ttf-config-version.cmake
	rm -r $(DESTDIR)$(libdir)/cmake/SDL2_ttf
