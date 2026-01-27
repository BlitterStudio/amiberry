dnl Process this file with autoconf to produce a configure script.

dnl Set various version strings - taken gratefully from the GTk sources
# See docs/release_checklist.md
m4_define([MAJOR_VERSION_MACRO], [2])
m4_define([MINOR_VERSION_MACRO], [22])
m4_define([MICRO_VERSION_MACRO], [0])

AC_INIT([SDL2_ttf],
        [MAJOR_VERSION_MACRO.MINOR_VERSION_MACRO.MICRO_VERSION_MACRO],
        [https://github.com/libsdl-org/SDL_ttf/issues],
        [SDL2_ttf])
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_SRCDIR([SDL_ttf.c])

AC_SUBST([MAJOR_VERSION], MAJOR_VERSION_MACRO)
AC_SUBST([MINOR_VERSION], MINOR_VERSION_MACRO)
AC_SUBST([MICRO_VERSION], MICRO_VERSION_MACRO)

BINARY_AGE=`expr $MINOR_VERSION \* 100 + $MICRO_VERSION`
AS_CASE(["$MINOR_VERSION"],
  [*@<:@02468@:>@],
    dnl Stable branch, 2.20.1 -> libSDL2_ttf-2.0.so.0.2000.1
    [INTERFACE_AGE="$MICRO_VERSION"],
  [*],
    dnl Development branch, 2.19.1 -> libSDL2_ttf-2.0.so.0.1901.0
    [INTERFACE_AGE=0])

dnl libtool versioning
LT_INIT([win32-dll])

# For historical reasons, the library name redundantly includes the major
# version twice: libSDL2_ttf-2.0.so.0.
# TODO: in SDL 3, stop using -release, which will simplify it to
# libSDL3_ttf.so.0
LT_RELEASE=2.0
# Increment this if there is an incompatible change - but if that happens,
# we should rename the library from SDL2 to SDL3, at which point this would
# reset to 0 anyway.
LT_MAJOR=0
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`
LT_CURRENT=`expr $LT_MAJOR + $LT_AGE`
LT_REVISION=$INTERFACE_AGE
LT_EXTRA=""
m4_pattern_allow([^LT_MAJOR$])

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_EXTRA)

dnl For use in static assertions
AC_DEFINE_UNQUOTED([SDL_BUILD_MAJOR_VERSION], $MAJOR_VERSION, [ ])
AC_DEFINE_UNQUOTED([SDL_BUILD_MINOR_VERSION], $MINOR_VERSION, [ ])
AC_DEFINE_UNQUOTED([SDL_BUILD_MICRO_VERSION], $MICRO_VERSION, [ ])

dnl Detect the canonical build and host environments
AC_CANONICAL_HOST

dnl Setup for automake
AM_INIT_AUTOMAKE([1.16 foreign subdir-objects tar-ustar])

dnl Check for tools
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_TOOL(RC,[windres],[:])
AC_SYS_LARGEFILE
AC_PROG_INSTALL
AC_PROG_MAKE_SET

case "$host" in
    *-*-beos*)
        ac_default_prefix=/boot/develop/tools/gnupro
        ;;
    *-*-cygwin* | *-*-mingw*)
        if test "$build" != "$host"; then # cross-compiling
            # Default cross-compile location
            ac_default_prefix=/usr/local/cross-tools/$host
        else
            # Look for the location of the tools and install there
            if test "$BUILD_PREFIX" != ""; then
                ac_default_prefix=$BUILD_PREFIX
            fi
        fi
        use_version_rc=true
        LT_EXTRA="-Wl,version.o"
        ;;
    *-*-os2*)
        # disable static builds on os/2
        enable_static=no
        # -DBUILD_SDL is needed for DECLSPEC
        CFLAGS="$CFLAGS -DBUILD_SDL"
        # OS/2 does not support a DLL name longer than 8 characters.
        LT_EXTRA="-os2dllname SDL2ttf"
        ;;
esac
AM_CONDITIONAL(USE_VERSION_RC, test x$use_version_rc = xtrue)

SUMMARY="\nSDL2_ttf Configure Summary:\n"

dnl Check for SDL
SDL_VERSION=2.0.10
AC_SUBST(SDL_VERSION)
AM_PATH_SDL2($SDL_VERSION,
            :,
	    AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

CheckNoUndef()
{
AC_MSG_CHECKING(for linker option --no-undefined)
    have_no_undefined=no
    case "${host_os}" in
dnl Skip this on platforms where it is just simply busted.
    openbsd*) ;;
    darwin*) have_no_undefined="-Wl,-undefined,error"
       LDFLAGS="$LDFLAGS -Wl,-undefined,error" ;;
    *) save_LDFLAGS="$LDFLAGS"
       LDFLAGS="$LDFLAGS -Wl,--no-undefined"
       AC_LINK_IFELSE([AC_LANG_PROGRAM],
          [have_no_undefined=yes],[LDFLAGS="$save_LDFLAGS"])
       ;;
    esac
    AC_MSG_RESULT($have_no_undefined)
}

dnl See if GCC's -Wall is supported.
CheckWarnAll()
{
    AC_MSG_CHECKING(for GCC -Wall option)
    have_gcc_Wall=no

    save_CFLAGS="$CFLAGS"
    CFLAGS="$save_CFLAGS -Wall"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([int x = 0;])], [have_gcc_Wall=yes])
    AC_MSG_RESULT($have_gcc_Wall)
    CFLAGS="$save_CFLAGS"

    if test x$have_gcc_Wall = xyes; then
        CFLAGS="$CFLAGS -Wall"
        CXXFLAGS="$CXXFLAGS -Wall"

dnl Haiku headers use multicharacter constants all over the place. Ignore these warnings when using -Wall.
        AC_MSG_CHECKING(for necessary GCC -Wno-multichar option)
        need_gcc_Wno_multichar=no
        case "$host" in
            *-*-haiku*)
                need_gcc_Wno_multichar=yes
                ;;
        esac
        AC_MSG_RESULT($need_gcc_Wno_multichar)
        if test x$need_gcc_Wno_multichar = xyes; then
            CFLAGS="$CFLAGS -Wno-multichar"
            CXXFLAGS="$CXXFLAGS -Wno-multichar"
        fi
    fi
}

dnl See if GCC's -fvisibility=hidden is supported (gcc4 and later, usually).
CheckVisibilityHidden()
{
    AC_MSG_CHECKING(for GCC -fvisibility=hidden option)
    have_gcc_fvisibility=no
    case "$host" in
    *-*-cygwin* | *-*-mingw* | *-*-os2*)
        AC_MSG_RESULT([ignored for $host_os])
        return
        ;;
    esac

    visibility_CFLAGS="-fvisibility=hidden"
    save_CFLAGS="$CFLAGS"
    CFLAGS="$save_CFLAGS $visibility_CFLAGS -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #if !defined(__GNUC__) || __GNUC__ < 4
    #error SDL only uses visibility attributes in GCC 4 or newer
    #endif
    ]],[])], [have_gcc_fvisibility=yes],[])
    AC_MSG_RESULT($have_gcc_fvisibility)
    CFLAGS="$save_CFLAGS"

    if test x$have_gcc_fvisibility = xyes; then
        CFLAGS="$CFLAGS $visibility_CFLAGS"
        CXXFLAGS="$CXXFLAGS $visibility_CFLAGS"
    fi
}

dnl check for GCC warning options
CheckWarnAll

dnl check for GCC visibility attributes
CheckVisibilityHidden

AC_SUBST([SDL2TTF_VENDORED])
AC_SUBST([TTF_USE_HARFBUZZ])
SDL2TTF_VENDORED=0
TTF_LIBS=
TTF_CPPFLAGS=
PC_REQUIRES=
PC_LIBS=

dnl Check for the FreeType 2 library
AC_ARG_ENABLE(freetype-builtin, [AS_HELP_STRING([--enable-freetype-builtin], [Use included version of FreeType [default=yes]])], [], enable_freetype_builtin=yes)

if test x$enable_freetype_builtin = xyes; then
	SUMMARY="${SUMMARY}Using included FreeType : YES\n"
else
	PKG_CHECK_MODULES([FT2], [freetype2 >= 7.0.1], [
		PC_REQUIRES="freetype2 $PC_REQUIRES"
	], [dnl
		AC_CHECK_FT2(,,[AC_MSG_ERROR([dnl
*** Unable to find FreeType2 library (https://www.freetype.org)])]
		)
		PC_LIBS="$FT2_LIBS $PC_LIBS"
	])
	TTF_CFLAGS="$TTF_CFLAGS $FT2_CFLAGS"
	TTF_LIBS="$TTF_LIBS $FT2_LIBS"

	SUMMARY="${SUMMARY}Using included FreeType : NO\n"
fi
AM_CONDITIONAL(USE_BUILTIN_FREETYPE, test x$enable_freetype_builtin = xyes)

dnl Check for the HarfBuzz library
AC_ARG_ENABLE(harfbuzz, [AS_HELP_STRING([--enable-harfbuzz], [Enable HarfBuzz [default=yes]])], [], enable_harfbuzz=yes)
AC_ARG_ENABLE(harfbuzz-builtin, [AS_HELP_STRING([--enable-harfbuzz-builtin], [Use included version of HarfBuzz [default=yes]])], [], enable_harfbuzz_builtin=yes)

TTF_USE_HARFBUZZ=0
if test x$enable_harfbuzz = xyes; then
	AC_DEFINE(TTF_USE_HARFBUZZ, 1, [])
	TTF_USE_HARFBUZZ=1
	SUMMARY="${SUMMARY}Using HarfBuzz          : YES\n"

	if test x$enable_harfbuzz_builtin = xyes; then
	    SDL2TTF_VENDORED=1
		SUMMARY="${SUMMARY}Using included HarfBuzz : YES\n"
		AX_CXX_COMPILE_STDCXX(11)
		AC_CHECK_ALIGNOF([struct{char;}])
		case "$host" in
		  *-*-mingw*) hb_os_win32=yes
			CXXFLAGS="$CXXFLAGS -static-libgcc -static-libstdc++"
			CFLAGS="$CFLAGS -static-libgcc"
			# for uniscribe support:
			TTF_LIBS="$TTF_LIBS -lusp10 -lgdi32 -lrpcrt4"
			PC_LIBS="$PC_LIBS -lusp10 -lgdi32 -lrpcrt4"
		  ;;
		  *)
			# compiler might optimize sinf+cosf into sincosf
			TTF_LIBS="$TTF_LIBS -lm"
		  ;;
		esac
		case "$host" in
		  arm-*-*)
		    if test "x$ac_cv_alignof_struct_char__" != x1; then
			CXXFLAGS="$CXXFLAGS -mstructure-size-boundary=8"
		    fi
		  ;;
		esac
		# silence a lot of harfbuzz warnings:
		CXXFLAGS="$CXXFLAGS -Wno-unused-result"
		# prevent linking to libstdc++ :
		CXXFLAGS="$CXXFLAGS -fno-rtti -fno-exceptions -fno-threadsafe-statics"
	else
		SUMMARY="${SUMMARY}Using included HarfBuzz : NO\n"
		PKG_CHECK_MODULES([HB], [harfbuzz >= 2.3.1], harfbuzz=yes, harfbuzz=no)

		if test x$harfbuzz = xyes; then
			save_CFLAGS="$CFLAGS"
			save_LIBS="$LIBS"
			CFLAGS="$CFLAGS $HB_CFLAGS"
			LIBS="$LIBS $HB_LIBS"
			AC_MSG_CHECKING(for freetype support in harfbuzz)
			harbuzz_has_freetype=no
			AC_LINK_IFELSE([AC_LANG_PROGRAM([],[[
					void* hb_ft_font_create(void*, void*);
					return !hb_ft_font_create((void*)0, (void*)0);]])
				],[harbuzz_has_freetype=yes])
			CFLAGS="$save_CFLAGS"
			LIBS="$save_LIBS"
			AC_MSG_RESULT($harbuzz_has_freetype)
			if test x$harbuzz_has_freetype = xno; then
				harfbuzz=no
				AC_MSG_ERROR([dnl
*** HarfBuzz library was built without FreeType support.)])
			fi
		fi
		if test x$harfbuzz = xyes; then
			AC_DEFINE(TTF_USE_HARFBUZZ, 1, [])
			PC_REQUIRES="harfbuzz $PC_REQUIRES"
			TTF_CFLAGS="$TTF_CFLAGS $HB_CFLAGS"
			TTF_LIBS="$TTF_LIBS $HB_LIBS"
			SUMMARY="${SUMMARY}Using harfbuzz      : YES\n"
		else
			AC_MSG_ERROR([dnl
*** Unable to find HarfBuzz library (https://harfbuzz.github.io)])
		fi
	fi
else
	AC_DEFINE(TTF_USE_HARFBUZZ, 0, [])
	SUMMARY="${SUMMARY}Using HarfBuzz          : NO\n"
	enable_harfbuzz_builtin=no
fi
AM_CONDITIONAL(OS_WIN32, test x$hb_os_win32 = xyes)
AM_CONDITIONAL(USE_BUILTIN_HARFBUZZ, test x$enable_harfbuzz_builtin = xyes)

dnl check for LD --no-undefined option
CheckNoUndef

dnl Check for OpenGL
case "$host" in
    *-*-cygwin* | *-*-mingw*)
        MATHLIB=""
        SYS_GL_LIBS="-lopengl32"
        ;;
    *-*-beos*)
        MATHLIB=""
        SYS_GL_LIBS="-lGL"
        ;;
    *-*-darwin*)
        MATHLIB=""
        SYS_GL_LIBS="-Wl,-framework,OpenGL"
        ;;
    *-*-aix*)
        if test x$ac_cv_c_compiler_gnu = xyes; then
            CFLAGS="-mthreads"
        fi
        SYS_GL_LIBS=""
        ;;
    *)
        MATHLIB="-lm"
        AC_PATH_X
        AC_PATH_XTRA
        if test x$have_x = xyes; then
            CFLAGS="$CFLAGS $X_CFLAGS"
            SYS_GL_LIBS="$X_LIBS -lGL"
        else
            SYS_GL_LIBS="-lGL"
        fi
        ;;
esac
AC_MSG_CHECKING(for OpenGL support)
have_opengl=no
save_LIBS="$LIBS"
LIBS="$LIBS $SYS_GL_LIBS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
 #include "SDL_opengl.h"
]], [[
 glOrtho(-2.0, 2.0, -2.0, 2.0, -20.0, 20.0);
]])], [have_opengl=yes],[])
AC_MSG_RESULT($have_opengl)
LIBS="$save_LIBS"
if test x$have_opengl = xyes; then
    CFLAGS="$CFLAGS -DHAVE_OPENGL"
    GL_LIBS="$SYS_GL_LIBS"
else
    GL_LIBS=""
fi
AC_SUBST([GL_LIBS])
AC_SUBST([MATHLIB])
AC_SUBST([TTF_CFLAGS])
AC_SUBST([TTF_LIBS])
AC_SUBST([PC_REQUIRES])
AC_SUBST([PC_LIBS])

dnl Calculate the location of the prefix, relative to the cmake folder
pkg_cmakedir='$libdir/cmake/SDL2_ttf'
AX_COMPUTE_RELATIVE_PATHS([pkg_cmakedir:prefix:cmake_prefix_relpath])
AC_SUBST([cmake_prefix_relpath])

# Finally create all the generated files
AC_CONFIG_FILES([
Makefile
sdl2_ttf-config.cmake
sdl2_ttf-config-version.cmake
SDL2_ttf.spec
SDL2_ttf.pc
])

AC_CONFIG_COMMANDS([summary], [printf "$SUMMARY"], [SUMMARY="$SUMMARY"])
AC_OUTPUT
