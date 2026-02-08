set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_COMPILER_ARG1 ${CMAKE_C_COMPILER_ARG1})
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM AND NOT CMAKE_C_COMPILER MATCHES "ccache")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to "Release" if user did not specify any build type yet
# Other possible values: Debug and None.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Accumulate compile and link flags in variables.
# These are applied to the amiberry target in SourceFiles.cmake after the target is created,
# so they do not leak into third-party FetchContent builds.
set(AMIBERRY_COMPILE_OPTIONS "-pipe")
set(AMIBERRY_LINK_OPTIONS "")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND AMIBERRY_COMPILE_OPTIONS "-Og" "-funwind-tables" "-DDEBUG")

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND AMIBERRY_COMPILE_OPTIONS "-ggdb")
    else()
        list(APPEND AMIBERRY_COMPILE_OPTIONS "-g")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND AMIBERRY_COMPILE_OPTIONS "-fdata-sections" "-ffunction-sections")
elseif(CMAKE_BUILD_TYPE)
    list(APPEND AMIBERRY_COMPILE_OPTIONS "-O1")
endif()

# Platform-specific linker flags
if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND AMIBERRY_LINK_OPTIONS "-Wl,--no-undefined" "-Wl,-z,combreloc" "-Wl,--as-needed")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND AMIBERRY_LINK_OPTIONS
            "-Wl,--gc-sections"
            "-Wl,--strip-all"
            "-Wl,-O1"
            "-Wl,-z,relro"
            "-Wl,-z,now"
        )

        # GNU ld-only flags (do not work on FreeBSD)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            list(APPEND AMIBERRY_LINK_OPTIONS
                "-Wl,--sort-common=descending"
                "-Wl,--hash-style=gnu"
            )
        endif()
    endif()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND AMIBERRY_LINK_OPTIONS "-Wl,-dead_strip")
    endif()
endif()

if(WITH_OPTIMIZE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        include(${CMAKE_SOURCE_DIR}/cmake/optimize.cmake)
    else()
        message(FATAL_ERROR "WITH_OPTIMIZE can only be used on Release builds")
    endif()
endif()

if(WITH_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
    if(lto_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(FATAL_ERROR "LTO is not supported: ${lto_error}")
    endif()
endif()

# Platform-specific include/link paths and frameworks.
# Stored in variables and applied to the target in SourceFiles.cmake.
set(AMIBERRY_PLATFORM_INCLUDE_DIRS "")
set(AMIBERRY_PLATFORM_LINK_DIRS "")
set(AMIBERRY_PLATFORM_LIBS "")

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        list(APPEND AMIBERRY_PLATFORM_INCLUDE_DIRS "/opt/homebrew/include")
        list(APPEND AMIBERRY_PLATFORM_LINK_DIRS "/opt/homebrew/lib")
    else()
        list(APPEND AMIBERRY_PLATFORM_INCLUDE_DIRS "/usr/local/include")
        list(APPEND AMIBERRY_PLATFORM_LINK_DIRS "/usr/local/lib")
    endif()

    list(APPEND AMIBERRY_PLATFORM_LIBS "-framework IOKit" "-framework Foundation" "-framework CoreFoundation" "-framework DiskArbitration" "iconv")

    list(APPEND AMIBERRY_COMPILE_OPTIONS "$<$<CONFIG:Debug>:-fno-omit-frame-pointer;-mno-omit-leaf-frame-pointer>")
endif()
