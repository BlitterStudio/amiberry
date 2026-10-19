# If anything enables ASM, CMake takes CMAKE_C_COMPILER as the assembler but
# not CMAKE_C_COMPILER_ARG1, so with CC="ccache distcc gcc" it would get just
# ccache.
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_COMPILER_ARG1 ${CMAKE_C_COMPILER_ARG1})

# Use ccache unless CC/CXX or CMAKE_<LANG>_COMPILER_LAUNCHER already set a launcher
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    foreach(lang C CXX)
        if(NOT CMAKE_${lang}_COMPILER_LAUNCHER
                AND NOT "${CMAKE_${lang}_COMPILER} ${CMAKE_${lang}_COMPILER_ARG1}" MATCHES "ccache")
            set(CMAKE_${lang}_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        endif()
    endforeach()
endif()

# Set build type to "Release" if user did not specify any build type yet.
# Other possible values: Debug, RelWithDebInfo, MinSizeRel and None.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Accumulate compile and link flags in variables.
# These are applied to the amiberry target in SourceFiles.cmake after the target is created,
# so they do not leak into third-party FetchContent builds.
# The flags below are GCC/Clang flags, which MSVC and clang-cl don't take.
set(AMIBERRY_COMPILE_OPTIONS "")
if(NOT WIN32)
    list(APPEND AMIBERRY_COMPILE_OPTIONS "-pipe")
endif()
set(AMIBERRY_LINK_OPTIONS "")

if(WITH_PGO_GENERATE OR WITH_PGO_USE)
    if(WITH_PGO_GENERATE AND WITH_PGO_USE)
        message(FATAL_ERROR "WITH_PGO_GENERATE and WITH_PGO_USE are mutually exclusive")
    endif()
    if(NOT PGO_PROFILE_DIR)
        message(FATAL_ERROR "PGO_PROFILE_DIR must be set when PGO is enabled")
    endif()
    if(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(FATAL_ERROR "PGO requires a single-config, non-Debug build")
    endif()
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "PGO requires GNU C and C++ compilers")
    endif()

    if(WITH_PGO_GENERATE)
        list(APPEND AMIBERRY_COMPILE_OPTIONS
            "-fprofile-generate=${PGO_PROFILE_DIR}"
            "-fprofile-prefix-path=${CMAKE_BINARY_DIR}"
            "-fprofile-update=atomic"
        )
        list(APPEND AMIBERRY_LINK_OPTIONS "-fprofile-generate=${PGO_PROFILE_DIR}")
    else()
        list(APPEND AMIBERRY_COMPILE_OPTIONS
            "-fprofile-use=${PGO_PROFILE_DIR}"
            "-fprofile-prefix-path=${CMAKE_BINARY_DIR}"
            "-Werror=missing-profile"
            "-Werror=coverage-mismatch"
        )
        list(APPEND AMIBERRY_LINK_OPTIONS "-fprofile-use=${PGO_PROFILE_DIR}")
    endif()
endif()

# Tune default Linux aarch64 builds for the slowest supported board
# (Raspberry Pi 4 / Cortex-A72). -mtune only affects instruction scheduling;
# the ISA baseline stays generic armv8-a. Leave WITH_OPTIMIZE builds alone so
# their native CPU flags remain authoritative.
if(NOT MSVC
        AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND ARCH_LOWER MATCHES "aarch64|arm64")
    set(AMIBERRY_ARM_TUNE "cortex-a72" CACHE STRING "CPU passed to -mtune for Linux aarch64 builds (set empty to disable)")
    if(WITH_OPTIMIZE)
        message(STATUS "Linux aarch64: WITH_OPTIMIZE enabled, not applying default -mtune=${AMIBERRY_ARM_TUNE}")
    elseif(AMIBERRY_ARM_TUNE)
        list(APPEND AMIBERRY_COMPILE_OPTIONS "-mtune=${AMIBERRY_ARM_TUNE}")
        message(STATUS "Linux aarch64: adding -mtune=${AMIBERRY_ARM_TUNE}")
    endif()
endif()

# Other build types keep CMake's own -O level. Generator expressions so
# multi-config generators (Xcode, Visual Studio) get the right flags too.
if(NOT MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(debug_info "-ggdb")
    else()
        set(debug_info "-g")
    endif()
    list(APPEND AMIBERRY_COMPILE_OPTIONS
        "$<$<CONFIG:Debug>:-Og;-funwind-tables;-DDEBUG;${debug_info}>"
        "$<$<CONFIG:Release>:-fdata-sections;-ffunction-sections>"
    )
    if(APPLE)
        list(APPEND AMIBERRY_COMPILE_OPTIONS "$<$<CONFIG:Debug>:-fno-omit-frame-pointer;-mno-omit-leaf-frame-pointer>")
    endif()
endif()

# Platform-specific linker flags
if(NOT MSVC)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT IOS)
        # ELF/PE linker flags (not for Apple platforms)
        list(APPEND AMIBERRY_LINK_OPTIONS "-Wl,--no-undefined" "-Wl,--as-needed")

        # ELF-specific flags (not available on Windows PE)
        if(NOT WIN32)
            list(APPEND AMIBERRY_LINK_OPTIONS "-Wl,-z,combreloc")
        endif()

        set(release_link_options "-Wl,--gc-sections" "-Wl,--strip-all" "-Wl,-O1")

        # ELF-specific hardening flags (not available on Windows PE)
        if(NOT WIN32)
            list(APPEND release_link_options "-Wl,-z,relro" "-Wl,-z,now")
        endif()

        # lld (FreeBSD's linker) rejects --sort-common=descending
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            list(APPEND release_link_options "-Wl,--sort-common=descending" "-Wl,--hash-style=gnu")
        endif()
        list(APPEND AMIBERRY_LINK_OPTIONS "$<$<CONFIG:Release>:${release_link_options}>")
    else()
        list(APPEND AMIBERRY_LINK_OPTIONS "$<$<CONFIG:Release>:-Wl,-dead_strip>")
    endif()
endif()

if(WITH_OPTIMIZE)
    if(MSVC)
        message(FATAL_ERROR "WITH_OPTIMIZE requires GCC or Clang")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(FATAL_ERROR "WITH_OPTIMIZE cannot be used with Debug builds")
    endif()
    include(${CMAKE_SOURCE_DIR}/cmake/optimize.cmake)
endif()

if(WITH_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
    if(NOT lto_supported)
        message(FATAL_ERROR "LTO is not supported: ${lto_error}")
    endif()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

    # Clang's LTO needs lld on FreeBSD. For all targets, as bundled libraries
    # are built with LTO too. Not CMAKE_LINKER_TYPE: CMake only knows LLD on
    # FreeBSD for GCC, so it fails there. lld runs ThinLTO on one thread per
    # physical core, which LLVM can't count on FreeBSD, so it would use one.
    # FreeBSD's CMake therefore uses full LTO, which optimizes on one thread
    # too (https://bugs.freebsd.org/281765); with all threads ThinLTO is faster.
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        foreach(lang C CXX)
            set(CMAKE_${lang}_COMPILE_OPTIONS_IPO "-flto=thin")
        endforeach()
        add_link_options(-fuse-ld=lld -Wl,--thinlto-jobs=all)
    endif()
endif()

# Platform-specific include/link paths and frameworks.
# Stored in variables and applied to the target in SourceFiles.cmake.
set(AMIBERRY_PLATFORM_INCLUDE_DIRS "")
set(AMIBERRY_PLATFORM_LINK_DIRS "")
set(AMIBERRY_PLATFORM_LIBS "")

if(IOS)
    # iOS: no Homebrew paths, different framework set (no IOKit/DiskArbitration)
    list(APPEND AMIBERRY_PLATFORM_LIBS "-framework Foundation" "-framework CoreFoundation" "-framework UIKit" "-framework OpenGLES" "iconv")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        list(APPEND AMIBERRY_PLATFORM_INCLUDE_DIRS
            "/opt/homebrew/include"
            "/opt/local/include"
        )
        list(APPEND AMIBERRY_PLATFORM_LINK_DIRS
            "/opt/homebrew/lib"
            "/opt/local/lib"
        )
    else()
        list(APPEND AMIBERRY_PLATFORM_INCLUDE_DIRS
            "/usr/local/include"
            "/opt/local/include"
        )
        list(APPEND AMIBERRY_PLATFORM_LINK_DIRS
            "/usr/local/lib"
            "/opt/local/lib"
        )
    endif()

    list(APPEND AMIBERRY_PLATFORM_LIBS "-framework IOKit" "-framework AppKit" "-framework Foundation" "-framework CoreFoundation" "-framework DiskArbitration" "iconv")
endif()

if(WIN32)
    list(APPEND AMIBERRY_PLATFORM_LIBS ws2_32 winmm imm32 version setupapi shlwapi advapi32 iphlpapi)
endif()
