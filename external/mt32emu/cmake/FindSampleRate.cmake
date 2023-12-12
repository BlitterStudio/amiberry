# Find-module for libsamplerate a.k.a. Secret Rabbit Code. Can be found
# at http://libsndfile.github.io/libsamplerate/download.html and https://github.com/libsndfile/libsamplerate
# Defines variable SampleRate_FOUND. Upon success, this variable is set to TRUE and an IMPORTED target
# SampleRate::samplerate is created.
#
# NOTE: This module is provided for compatibility with old versions found in some GNU distributions.
# Recent versions are expected to supply a CMake config file, which takes precedence if found.

if(TARGET SampleRate::samplerate)
  if(NOT SampleRate_FIND_QUIETLY)
    message("Finding package SampleRate skipped because target SampleRate::samplerate already exists; assuming it is suitable.")
  endif()
  set(SampleRate_FOUND TRUE)
  return()
endif()

find_package(SampleRate ${SampleRate_FIND_VERSION} QUIET CONFIG)
if(SampleRate_FOUND)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SampleRate CONFIG_MODE)
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_SampleRate QUIET samplerate)
set(SampleRate_VERSION ${PC_SampleRate_VERSION})

find_path(SampleRate_INCLUDE_DIR samplerate.h
  HINTS ${PC_SampleRate_INCLUDEDIR} ${PC_SampleRate_INCLUDE_DIRS}
)

find_library(SampleRate_LIBRARY
  NAMES samplerate samplerate-0 libsamplerate-0
  HINTS ${PC_SampleRate_LIBDIR} ${PC_SampleRate_LIBRARY_DIRS}
)

mark_as_advanced(SampleRate_INCLUDE_DIR SampleRate_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SampleRate
  REQUIRED_VARS SampleRate_LIBRARY SampleRate_INCLUDE_DIR
  VERSION_VAR SampleRate_VERSION
)

if(CMAKE_VERSION VERSION_LESS 3.3)
  set(SampleRate_FOUND ${SAMPLERATE_FOUND})
endif()

if(SampleRate_FOUND)
  add_library(SampleRate::samplerate UNKNOWN IMPORTED)
  set_target_properties(SampleRate::samplerate PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SampleRate_INCLUDE_DIR}"
    IMPORTED_LOCATION "${SampleRate_LIBRARY}"
  )
endif()
