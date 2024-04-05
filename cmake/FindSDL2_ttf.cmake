# Downloaded from http://cmake.3232098.n2.nabble.com/Find-modules-for-SDL2-td7585211.html and adapted to SDL2_ttf
#
# - Find SDL2_ttf library and headers
# 
# Find module for SDL_ttf 2.0 (http://www.libsdl.org/projects/SDL_ttf/).
# It defines the following variables:
#  SDL2_TTF_INCLUDE_DIRS - The location of the headers, e.g., SDL_ttf.h.
#  SDL2_TTF_LIBRARIES - The libraries to link against to use SDL2_ttf.
#  SDL2_TTF_FOUND - If false, do not try to use SDL2_ttf.
#  SDL2_TTF_VERSION_STRING
#    Human-readable string containing the version of SDL2_ttf.
#
# Also defined, but not for general use are:
#   SDL2_TTF_INCLUDE_DIR - The directory that contains SDL_ttf.h.
#   SDL2_TTF_LIBRARY - The location of the SDL2_ttf library.
#

#=============================================================================
# Copyright 2013 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_SDL2_TTF QUIET SDL2_ttf)

find_path(SDL2_TTF_INCLUDE_DIR
  NAMES SDL_ttf.h
  HINTS
    ${PC_SDL2_TTF_INCLUDEDIR}
    ${PC_SDL2_TTF_INCLUDE_DIRS}
  PATH_SUFFIXES SDL2
)

find_library(SDL2_TTF_LIBRARY
  NAMES SDL2_ttf
  HINTS
    ${PC_SDL2_TTF_LIBDIR}
    ${PC_SDL2_TTF_LIBRARY_DIRS}
  PATH_SUFFIXES x64 x86
)

if(SDL2_TTF_INCLUDE_DIR AND EXISTS "${SDL2_TTF_INCLUDE_DIR}/SDL_ttf.h")
  file(STRINGS "${SDL2_TTF_INCLUDE_DIR}/SDL_ttf.h" SDL2_TTF_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_TTF_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_TTF_INCLUDE_DIR}/SDL_ttf.h" SDL2_TTF_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_TTF_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_TTF_INCLUDE_DIR}/SDL_ttf.h" SDL2_TTF_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_TTF_PATCHLEVEL[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL_TTF_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_TTF_VERSION_MAJOR "${SDL2_TTF_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_TTF_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_TTF_VERSION_MINOR "${SDL2_TTF_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_TTF_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL2_TTF_VERSION_PATCH "${SDL2_TTF_VERSION_PATCH_LINE}")
  set(SDL2_TTF_VERSION_STRING ${SDL2_TTF_VERSION_MAJOR}.${SDL2_TTF_VERSION_MINOR}.${SDL2_TTF_VERSION_PATCH})
  unset(SDL2_TTF_VERSION_MAJOR_LINE)
  unset(SDL2_TTF_VERSION_MINOR_LINE)
  unset(SDL2_TTF_VERSION_PATCH_LINE)
  unset(SDL2_TTF_VERSION_MAJOR)
  unset(SDL2_TTF_VERSION_MINOR)
  unset(SDL2_TTF_VERSION_PATCH)
endif()

set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIR})
set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SDL2_ttf
                                  REQUIRED_VARS SDL2_TTF_INCLUDE_DIRS SDL2_TTF_LIBRARIES
                                  VERSION_VAR SDL2_TTF_VERSION_STRING)

mark_as_advanced(SDL2_TTF_INCLUDE_DIR SDL2_TTF_LIBRARY)