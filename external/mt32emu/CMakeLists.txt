cmake_minimum_required(VERSION 2.8.12...3.27)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(libmt32emu CXX)

include(cmake/project_data.cmake)

include(GNUInstallDirs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG TRUE)
else()
  set(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG FALSE)
endif()

# By default, we build a shared library, unless a higher-level script defines BUILD_SHARED_LIBS.
if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS TRUE)
endif()

option(libmt32emu_SHARED "Build shared library" ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ${libmt32emu_SHARED})

if(BUILD_SNAPSHOTS AND NOT libmt32emu_SHARED)
  include(../cmake/build_snapshots.cmake)
endif()

if(libmt32emu_REQUIRE_C_INTERFACE)
  unset(libmt32emu_C_INTERFACE CACHE)
  set(libmt32emu_C_INTERFACE TRUE)
else()
  option(libmt32emu_C_INTERFACE "Provide C-compatible API" TRUE)
endif()

if(libmt32emu_REQUIRE_CPP_INTERFACE OR NOT libmt32emu_SHARED)
  unset(libmt32emu_CPP_INTERFACE CACHE)
  set(libmt32emu_CPP_INTERFACE TRUE)
else()
  option(libmt32emu_CPP_INTERFACE "Provide C++ classes (compiler-specific ABI)" TRUE)
endif()

option(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER "Use built-in sample rate conversion" TRUE)

if(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG)
  option(libmt32emu_REQUIRE_ANSI "Require ANSI C++ compatibility when compiling with GNU C++ or Clang" TRUE)
  mark_as_advanced(libmt32emu_REQUIRE_ANSI)
else()
  unset(libmt32emu_REQUIRE_ANSI CACHE)
endif()

# Variable LIB_INSTALL_DIR is now deprecated, we use CMAKE_INSTALL_LIBDIR et. al.
# However, if it is set in an existing script, we make use of its value.
if(DEFINED LIB_INSTALL_DIR)
  set(CMAKE_INSTALL_LIBDIR ${LIB_INSTALL_DIR})
endif()

set(libmt32emu_PACKAGE_TYPE Complete CACHE STRING "Selects output binary package type: Complete, Runtime or Devel")
mark_as_advanced(libmt32emu_PACKAGE_TYPE)
set_property(CACHE libmt32emu_PACKAGE_TYPE PROPERTY STRINGS Complete Runtime Devel)

# Note, GNUInstallDirs provides no facility to locate pkg-config files, as they are normally placed
# along with libraries. However, some systems use other non-GNU layouts.
set(libmt32emu_PKGCONFIG_INSTALL_PREFIX
  "" CACHE PATH
  "Relative installation path prefix for pkgconfig files. If unset, defaults to $CMAKE_INSTALL_LIBDIR"
)
mark_as_advanced(libmt32emu_PKGCONFIG_INSTALL_PREFIX)
if(NOT libmt32emu_PKGCONFIG_INSTALL_PREFIX)
  set(libmt32emu_PKGCONFIG_INSTALL_PREFIX ${CMAKE_INSTALL_LIBDIR})
endif()

set(libmt32emu_PKGCONFIG_INSTALL_DIR
  "" CACHE PATH
  "Relative installation path for pkgconfig files. If unset, defaults to $libmt32emu_PKGCONFIG_INSTALL_PREFIX/pkgconfig"
)
mark_as_advanced(libmt32emu_PKGCONFIG_INSTALL_DIR)
if(NOT libmt32emu_PKGCONFIG_INSTALL_DIR)
  set(libmt32emu_PKGCONFIG_INSTALL_DIR ${libmt32emu_PKGCONFIG_INSTALL_PREFIX}/pkgconfig)
endif()

# TODO: Use macro GNUInstallDirs_get_absolute_install_dir when available.
set(libmt32emu_PKGCONFIG_INSTALL_FULL_DIR
  "" CACHE PATH
  "Absolute installation path for pkgconfig files. If unset, defaults to $CMAKE_INSTALL_PREFIX/$libmt32emu_PKGCONFIG_INSTALL_DIR"
)
mark_as_advanced(libmt32emu_PKGCONFIG_INSTALL_FULL_DIR)
if(NOT libmt32emu_PKGCONFIG_INSTALL_FULL_DIR)
  set(libmt32emu_PKGCONFIG_INSTALL_FULL_DIR ${CMAKE_INSTALL_PREFIX}/${libmt32emu_PKGCONFIG_INSTALL_DIR})
endif()

set(libmt32emu_CMAKE_PACKAGE_INSTALL_PREFIX ${CMAKE_INSTALL_LIBDIR}/cmake)
set(libmt32emu_CMAKE_PACKAGE_INSTALL_DIR ${libmt32emu_CMAKE_PACKAGE_INSTALL_PREFIX}/MT32Emu)

if(libmt32emu_SHARED)
  option(libmt32emu_WITH_VERSION_TAGGING "Tags shared object with library version and enables runtime version check")
  if(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG)
    option(libmt32emu_WITH_SYMBOL_VERSIONING "Adds versions of version tags and C-compatible API symbols on supported platforms")
  else()
    unset(libmt32emu_WITH_SYMBOL_VERSIONING CACHE)
  endif()

  option(libmt32emu_INSTALL_DEFAULT_LOCALE "Install user-preferred system locale as C locale when initialising shared library.
 This is generally not necessary, although may be required in some cases (for instance, when building shared library with old
 versions of MS VC with the C runtime statically linked) to open ROM files with localised pathnames successfully.")
  mark_as_advanced(libmt32emu_INSTALL_DEFAULT_LOCALE)
else(libmt32emu_SHARED)
  unset(libmt32emu_WITH_VERSION_TAGGING CACHE)
  unset(libmt32emu_WITH_SYMBOL_VERSIONING CACHE)
  unset(libmt32emu_INSTALL_DEFAULT_LOCALE CACHE)
endif(libmt32emu_SHARED)

set(libmt32emu_SOURCES
  src/Analog.cpp
  src/BReverbModel.cpp
  src/Display.cpp
  src/File.cpp
  src/FileStream.cpp
  src/LA32FloatWaveGenerator.cpp
  src/LA32Ramp.cpp
  src/LA32WaveGenerator.cpp
  src/MidiStreamParser.cpp
  src/Part.cpp
  src/Partial.cpp
  src/PartialManager.cpp
  src/Poly.cpp
  src/ROMInfo.cpp
  src/Synth.cpp
  src/Tables.cpp
  src/TVA.cpp
  src/TVF.cpp
  src/TVP.cpp
  src/sha1/sha1.cpp
  src/SampleRateConverter.cpp
)

# Public headers that always need to be installed:
set(libmt32emu_COMMON_HEADERS
  mt32emu.h
  globals.h
  Enumerations.h
  Types.h
)

# Public headers used by C++ clients:
set(libmt32emu_CPP_HEADERS
  File.h
  FileStream.h
  MidiStreamParser.h
  ROMInfo.h
  SampleRateConverter.h
  Synth.h
)

# Public headers that support C-compatible and plugin-style API:
set(libmt32emu_C_HEADERS
  c_interface/c_interface.h
  c_interface/c_types.h
  c_interface/cpp_interface.h
)

if(libmt32emu_CPP_INTERFACE AND NOT libmt32emu_C_INTERFACE)
  # C++ API type
  set(libmt32emu_EXPORTS_TYPE 0)

  set(libmt32emu_HEADERS
    ${libmt32emu_COMMON_HEADERS}
    ${libmt32emu_CPP_HEADERS}
  )
else(libmt32emu_CPP_INTERFACE AND NOT libmt32emu_C_INTERFACE)
  list(APPEND libmt32emu_SOURCES
    src/c_interface/c_interface.cpp
  )
  # All API types
  if(libmt32emu_CPP_INTERFACE)
    set(libmt32emu_EXPORTS_TYPE 3)

    set(libmt32emu_HEADERS
      ${libmt32emu_COMMON_HEADERS}
      ${libmt32emu_CPP_HEADERS}
      ${libmt32emu_C_HEADERS}
    )
  else(libmt32emu_CPP_INTERFACE)
    if(libmt32emu_C_INTERFACE)
      # C API type
      set(libmt32emu_EXPORTS_TYPE 1)
    else(libmt32emu_C_INTERFACE)
      # Plugin API type
      set(libmt32emu_EXPORTS_TYPE 2)
    endif(libmt32emu_C_INTERFACE)

    set(libmt32emu_HEADERS
      ${libmt32emu_COMMON_HEADERS}
      ${libmt32emu_C_HEADERS}
    )
  endif(libmt32emu_CPP_INTERFACE)
endif(libmt32emu_CPP_INTERFACE AND NOT libmt32emu_C_INTERFACE)

if(libmt32emu_WITH_VERSION_TAGGING)
  list(APPEND libmt32emu_SOURCES src/VersionTagging.cpp)
  list(APPEND libmt32emu_HEADERS VersionTagging.h)
  if(libmt32emu_WITH_SYMBOL_VERSIONING)
    set(libmt32emu_RUNTIME_VERSION_CHECK 1)
  else()
    set(libmt32emu_RUNTIME_VERSION_CHECK 2)
  endif()
else(libmt32emu_WITH_VERSION_TAGGING)
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu/VersionTagging.h)
  set(libmt32emu_RUNTIME_VERSION_CHECK 0)
endif(libmt32emu_WITH_VERSION_TAGGING)

if(${PROJECT_NAME}_COMPILER_IS_GNU_OR_CLANG)
  add_compile_options(-Wall -Wextra -Wnon-virtual-dtor -Wshadow -Wold-style-cast)
  if(libmt32emu_REQUIRE_ANSI)
    add_compile_options(-ansi -pedantic)
  endif()
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1)
endif()

if(libmt32emu_SHARED)
  set(libmt32emu_SHARED_DEFINITION "#define MT32EMU_SHARED")
  if(NOT OS2)
    # A workaround for the OS/2 port of GCC 9 that emits lots of warnings otherwise.
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  endif()
  if(libmt32emu_INSTALL_DEFAULT_LOCALE)
    add_definitions(-DMT32EMU_INSTALL_DEFAULT_LOCALE)
  endif()
else(libmt32emu_SHARED)
  set(libmt32emu_SHARED_DEFINITION "#undef MT32EMU_SHARED")
endif(libmt32emu_SHARED)

if(NOT libmt32emu_CURRENT_EXPORTS_TYPE STREQUAL libmt32emu_EXPORTS_TYPE)
  set(libmt32emu_CURRENT_EXPORTS_TYPE ${libmt32emu_EXPORTS_TYPE} CACHE INTERNAL "Stores current exports type")
  file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu)
endif()

foreach(HEADER ${libmt32emu_HEADERS})
  configure_file("src/${HEADER}" "include/mt32emu/${HEADER}" COPYONLY)
endforeach(HEADER)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu.h" "#include <mt32emu/mt32emu.h>")

configure_file("src/config.h.in" "include/mt32emu/config.h")

if(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER)
  add_definitions(-DMT32EMU_WITH_INTERNAL_RESAMPLER)
  set(${PROJECT_NAME}_SOURCES ${${PROJECT_NAME}_SOURCES}
    src/srchelper/srctools/src/FIRResampler.cpp
    src/srchelper/srctools/src/SincResampler.cpp
    src/srchelper/srctools/src/IIR2xResampler.cpp
    src/srchelper/srctools/src/LinearResampler.cpp
    src/srchelper/srctools/src/ResamplerModel.cpp
    src/srchelper/InternalResampler.cpp
  )
else(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER)
  # Prefer using SOXR if it is available
  find_package(SOXR 0.1.2 MODULE)
  if(SOXR_FOUND)
    set(libmt32emu_PC_REQUIRES_PRIVATE "soxr >= 0.1.2")
    add_definitions(-DMT32EMU_WITH_LIBSOXR_RESAMPLER)
    set(libmt32emu_EXT_TARGET Soxr::soxr)
    list(APPEND ${PROJECT_NAME}_SOURCES src/srchelper/SoxrAdapter.cpp)
    if(NOT libmt32emu_SHARED)
      set(libmt32emu_EXT_DEPENDENCY SOXR 0.1.2)
      set(libmt32emu_EXT_MODULE "FindSOXR.cmake")
    endif()
  else(SOXR_FOUND)
    # Try SampleRate if SOXR is unavailable
    find_package(SampleRate 0.1.8 MODULE)
    if(SampleRate_FOUND)
      set(libmt32emu_PC_REQUIRES_PRIVATE "samplerate >= 0.1.8")
      add_definitions(-DMT32EMU_WITH_LIBSAMPLERATE_RESAMPLER)
      set(libmt32emu_EXT_TARGET SampleRate::samplerate)
      list(APPEND ${PROJECT_NAME}_SOURCES src/srchelper/SamplerateAdapter.cpp)
      if(NOT libmt32emu_SHARED)
        set(libmt32emu_EXT_DEPENDENCY SampleRate 0.1.8)
        set(libmt32emu_EXT_MODULE "FindSampleRate.cmake")
      endif()
    else(SampleRate_FOUND)
      message(STATUS "Could NOT find any sample rate conversion library, resampling unavailable")
    endif(SampleRate_FOUND)
  endif(SOXR_FOUND)
endif(${PROJECT_NAME}_WITH_INTERNAL_RESAMPLER)

configure_file("src/mt32emu.pc.in" "mt32emu.pc" @ONLY)

add_library(mt32emu ${libmt32emu_SOURCES})

if(libmt32emu_EXT_TARGET)
  target_link_libraries(mt32emu PRIVATE ${libmt32emu_EXT_TARGET})
endif()

set_target_properties(mt32emu
  PROPERTIES VERSION ${libmt32emu_VERSION}
  SOVERSION ${libmt32emu_VERSION_MAJOR}
)

if(libmt32emu_SHARED AND CMAKE_SYSTEM_NAME STREQUAL Windows)
  set_target_properties(mt32emu
    PROPERTIES RUNTIME_OUTPUT_NAME mt32emu-${libmt32emu_VERSION_MAJOR}
  )
endif()

if(libmt32emu_WITH_SYMBOL_VERSIONING)
  set(${PROJECT_NAME}_VERSION_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map)
  include(cmake/build_version_map.cmake)
  build_version_map("src/c_interface/c_interface.h" ${${PROJECT_NAME}_VERSION_SCRIPT})
  set_target_properties(mt32emu
    PROPERTIES LINK_FLAGS "-Wl,--version-script=${${PROJECT_NAME}_VERSION_SCRIPT}"
  )
endif()

target_include_directories(mt32emu
  PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu>"
  INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)
set(libmt32emu_COMPONENT_DEVEL COMPONENT Devel)

install(TARGETS mt32emu EXPORT MT32EmuTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} ${libmt32emu_COMPONENT_DEVEL}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# TODO: Use NAMELINK_COMPONENT with CMake 3.12 instead.
if(libmt32emu_SHARED)
  install(TARGETS mt32emu EXPORT MT32EmuTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ${libmt32emu_COMPONENT_DEVEL} NAMELINK_ONLY
  )
endif()

install(DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ${libmt32emu_COMPONENT_DEVEL}
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/mt32emu.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ${libmt32emu_COMPONENT_DEVEL}
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/mt32emu.pc
  DESTINATION ${libmt32emu_PKGCONFIG_INSTALL_DIR}
  ${libmt32emu_COMPONENT_DEVEL}
)
install(EXPORT MT32EmuTargets
  DESTINATION ${libmt32emu_CMAKE_PACKAGE_INSTALL_DIR}
  NAMESPACE MT32Emu::
  ${libmt32emu_COMPONENT_DEVEL}
)
configure_file(cmake/MT32EmuConfig.cmake.in cmake/MT32EmuConfig.cmake @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(cmake/MT32EmuConfigVersion.cmake
  VERSION ${libmt32emu_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/MT32EmuConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/MT32EmuConfigVersion.cmake
  DESTINATION ${libmt32emu_CMAKE_PACKAGE_INSTALL_DIR}
  ${libmt32emu_COMPONENT_DEVEL}
)
if(libmt32emu_EXT_MODULE)
  install(FILES
    cmake/CheckDependencies.cmake
    cmake/${libmt32emu_EXT_MODULE}
    DESTINATION ${libmt32emu_CMAKE_PACKAGE_INSTALL_DIR}
    ${libmt32emu_COMPONENT_DEVEL}
  )
endif()

install(FILES
  AUTHORS.txt COPYING.txt COPYING.LESSER.txt NEWS.txt README.md TODO.txt
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/munt/${PROJECT_NAME}
)

# Facilitates usage of the library in another project either through
# an imported target or directly from the build tree.
add_library(MT32Emu::mt32emu ALIAS mt32emu)

# Locations are exported for the benefit of a higher-level CMakeLists.txt.
# These variables are irrelevant if compiling stand-alone.
# NOTE: This method is deprecated, the alias target should be used instead
# to link with mt32emu from the same build tree.
set(MT32EMU_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" CACHE INTERNAL "")
set(MT32EMU_LIBRARY MT32Emu::mt32emu CACHE INTERNAL "")
unset(MT32EMU_EXT_LIBS CACHE)

# build a CPack driven installer package
set(CPACK_PACKAGE_VERSION_MAJOR "${libmt32emu_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${libmt32emu_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${libmt32emu_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "${libmt32emu_CONTACT}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/info.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${libmt32emu_DESCRIPTION_SUMMARY}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
# The default package file name has "-${CPACK_SYSTEM_NAME}" at the end, which doesn't match our previous releases
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${libmt32emu_VERSION}")

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)

if(libmt32emu_SHARED AND NOT libmt32emu_PACKAGE_TYPE STREQUAL "Complete")
  # TODO: We want to create a couple of per-component packages with RPM and DEB in one go, albeit this
  # may only be possible with CMake 3.6+ since we have to override incorrect default package names.
  set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR}" "${PROJECT_NAME}" "${libmt32emu_PACKAGE_TYPE}" "/")
endif()

set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "LGPLv2.1+")
set(CPACK_RPM_PACKAGE_URL "${libmt32emu_URL}")
# Normally, pkgconfig file and cmake package are installed into the system directories that should
# pre-exist on the target system, and hence cause conflict with other packages if not excluded.
list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
  ${libmt32emu_PKGCONFIG_INSTALL_FULL_DIR}
  ${CMAKE_INSTALL_PREFIX}/${libmt32emu_CMAKE_PACKAGE_INSTALL_PREFIX}
)
if(munt_SOURCE_DIR)
  # Also publish the list for the benefit of the parent CMakeLists.txt file.
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION ${CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION} PARENT_SCOPE)
endif()
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${libmt32emu_URL}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
if(libmt32emu_PACKAGE_TYPE STREQUAL "Devel")
  set(CPACK_RPM_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-devel")
  set(CPACK_RPM_FILE_NAME "${CPACK_RPM_PACKAGE_NAME}-${libmt32emu_VERSION}.rpm")
  set(CPACK_RPM_PACKAGE_REQUIRES "${CMAKE_PROJECT_NAME} == ${libmt32emu_VERSION}")
  set(CPACK_DEBIAN_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-dev")
  set(CPACK_DEBIAN_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-${libmt32emu_VERSION}.deb")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CMAKE_PROJECT_NAME} (= ${libmt32emu_VERSION})")
endif()
if(libmt32emu_SHARED)
  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS "ON")
  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
endif()

include(CPack)
