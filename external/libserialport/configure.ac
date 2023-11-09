##
## This file is part of the libserialport project.
##
## Copyright (C) 2010-2012 Bert Vermeulen <bert@biot.com>
## Copyright (C) 2013 Martin Ling <martin-libserialport@earth.li>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as
## published by the Free Software Foundation, either version 3 of the
## License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

# We require at least autoconf 2.63 (AC_INIT format changed there).
AC_PREREQ([2.63])

# libserialport package version number (NOT the same as shared lib version!).
m4_define([sp_package_version_major], [0])
m4_define([sp_package_version_minor], [1])
m4_define([sp_package_version_micro], [1])
m4_define([sp_package_version], [sp_package_version_major.sp_package_version_minor.sp_package_version_micro])

AC_INIT([libserialport], [sp_package_version], [martin-libserialport@earth.li],
	[libserialport], [http://sigrok.org/wiki/Libserialport])
AC_CONFIG_HEADERS([config.h libserialport.h])
AC_CONFIG_MACRO_DIR([autostuff])
AC_CONFIG_AUX_DIR([autostuff])

AH_BOTTOM([#if HAVE_STRUCT_TERMIOS_C_ISPEED && HAVE_STRUCT_TERMIOS_C_OSPEED
# define HAVE_TERMIOS_SPEED 1
#endif
#if HAVE_STRUCT_TERMIOS2_C_ISPEED && HAVE_STRUCT_TERMIOS2_C_OSPEED
# define HAVE_TERMIOS2_SPEED 1
#endif])

# We require at least automake 1.11 (needed for 'silent rules').
AM_INIT_AUTOMAKE([1.11 -Wall -Werror no-define check-news])
AM_SILENT_RULES([yes])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Initialize libtool.
LT_INIT

AC_DEFINE([SP_PACKAGE_VERSION_MAJOR], [sp_package_version_major], [.])
AC_DEFINE([SP_PACKAGE_VERSION_MINOR], [sp_package_version_minor], [.])
AC_DEFINE([SP_PACKAGE_VERSION_MICRO], [sp_package_version_micro], [.])
AC_DEFINE([SP_PACKAGE_VERSION_STRING], ["sp_package_version"], [.])
AC_SUBST([SP_PACKAGE_VERSION], [sp_package_version])

# Library version for libserialport (NOT the same as the package version).
# Carefully read the libtool docs before updating these numbers!
# The algorithm for determining which number to change (and how) is nontrivial!
# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
SP_LIB_VERSION_CURRENT=1
SP_LIB_VERSION_REVISION=0
SP_LIB_VERSION_AGE=1
AC_SUBST([SP_LIB_VERSION],
	["$SP_LIB_VERSION_CURRENT:$SP_LIB_VERSION_REVISION:$SP_LIB_VERSION_AGE"])

AC_DEFINE_UNQUOTED([SP_LIB_VERSION_CURRENT], [$SP_LIB_VERSION_CURRENT], [.])
AC_DEFINE_UNQUOTED([SP_LIB_VERSION_REVISION], [$SP_LIB_VERSION_REVISION], [.])
AC_DEFINE_UNQUOTED([SP_LIB_VERSION_AGE], [$SP_LIB_VERSION_AGE], [.])
AC_DEFINE_UNQUOTED([SP_LIB_VERSION_STRING], ["$SP_LIB_VERSION"], [.])

AM_CONDITIONAL([LINUX], [test -z "${host_os##linux*}"])
AM_CONDITIONAL([WIN32], [test -z "${host_os##mingw*}" || test -z "${host_os##cygwin*}"])
AM_CONDITIONAL([MACOSX], [test -z "${host_os##darwin*}"])
AM_CONDITIONAL([FREEBSD], [test -z "${host_os##freebsd*}"])

AM_COND_IF([WIN32], [SP_LIBS='-lsetupapi'], [SP_LIBS=])
AC_SUBST([SP_LIBS])

AM_COND_IF([MACOSX], [AC_CHECK_HEADER([IOKit/IOKitLib.h], [],
	[AC_MSG_ERROR([IOKit/IOKitLib.h not found])])])

AS_CASE([$host_os], [linux*|darwin*|mingw*|cygwin*|freebsd*],, [
	AC_DEFINE([NO_ENUMERATION], [1], [Enumeration is unsupported.])
	AC_DEFINE([NO_PORT_METADATA], [1], [Port metadata is unavailable.])
])

AC_SYS_LARGEFILE

# Define size_t if not defined as standard.
AC_TYPE_SIZE_T

# Check for specific termios structures.
AC_CHECK_TYPES([struct termios2, struct termiox],,,
	[[#include <linux/termios.h>]])
AC_CHECK_MEMBERS([struct termios.c_ispeed, struct termios.c_ospeed,
		struct termios2.c_ispeed, struct termios2.c_ospeed],,,
	[[#include <linux/termios.h>]])

# Check for the BOTHER definition, needed for setting arbitrary baud rates.
# We can't just #ifdef BOTHER in the code, because of the separation between
# code using libc headers and code using kernel termios.h headers.
AC_CHECK_DECLS([BOTHER],,, [[#include <linux/termios.h>]])

# Check for serial_struct.
AC_CHECK_TYPES([struct serial_struct],,, [[#include <linux/serial.h>]])

AC_CACHE_CHECK([for visibility control], [sp_cv_visibility_control], [
	sp_saved_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS -Werror"
	AC_COMPILE_IFELSE([AC_LANG_SOURCE(
		[[__attribute__((visibility("hidden"))) void foo(void) {}]])],
		[sp_cv_visibility_control=attribute],
		[AC_COMPILE_IFELSE([AC_LANG_SOURCE(
			[[__declspec(dllexport) void foo(void) {}]])],
			[sp_cv_visibility_control=declspec],
			[sp_cv_visibility_control=none])])
	CFLAGS=$sp_saved_CFLAGS
])
AS_CASE([$sp_cv_visibility_control],
	[attribute],	[SP_API='__attribute__((visibility("default")))'
			SP_PRIV='__attribute__((visibility("hidden")))'],
	[declspec],	[SP_API='__declspec(dllexport)' SP_PRIV=],
			[SP_API= SP_PRIV=])
AC_DEFINE_UNQUOTED([SP_API], [$SP_API], [Macro preceding public API functions])
AC_DEFINE_UNQUOTED([SP_PRIV], [$SP_PRIV], [Macro preceding private functions])

AC_CONFIG_FILES([Makefile libserialport.pc])

AC_OUTPUT

cat >&AS_MESSAGE_FD <<_EOF

libserialport configuration summary:

 - Package version (major.minor.micro):    $SP_PACKAGE_VERSION
 - Library version (current:revision:age): $SP_LIB_VERSION
 - Prefix: $prefix
 - Building on:  $build
 - Building for: $host

_EOF
