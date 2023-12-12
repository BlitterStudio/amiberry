# Find-module for the SoX Resampler library - get from https://sourceforge.net/projects/soxr/
# Defines variable SOXR_FOUND. Upon success, this variable is set to TRUE and an IMPORTED target
# Soxr::soxr is created.

if(TARGET Soxr::soxr)
  if(NOT SOXR_FIND_QUIETLY)
    message("Finding package SOXR skipped because target Soxr::soxr already exists; assuming it is suitable.")
  endif()
  set(SOXR_FOUND TRUE)
  return()
endif()

find_package(PkgConfig MODULE)
pkg_check_modules(PC_SOXR QUIET soxr)
set(SOXR_VERSION ${PC_SOXR_VERSION})

find_path(SOXR_INCLUDE_DIR soxr.h
  HINTS ${PC_SOXR_INCLUDEDIR} ${PC_SOXR_INCLUDE_DIRS}
)

find_library(SOXR_LIBRARY
  NAMES soxr libsoxr
  HINTS ${PC_SOXR_LIBDIR} ${PC_SOXR_LIBRARY_DIRS}
)

mark_as_advanced(SOXR_INCLUDE_DIR SOXR_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SOXR
  REQUIRED_VARS SOXR_LIBRARY SOXR_INCLUDE_DIR
  VERSION_VAR SOXR_VERSION
)

if(SOXR_FOUND)
  add_library(Soxr::soxr UNKNOWN IMPORTED)
  set_target_properties(Soxr::soxr PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SOXR_INCLUDE_DIR}"
    IMPORTED_LOCATION "${SOXR_LIBRARY}"
  )
endif()
