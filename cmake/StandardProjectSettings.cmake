set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_COMPILER_ARG1 ${CMAKE_C_COMPILER_ARG1})
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM AND NOT CMAKE_C_COMPILER MATCHES "ccache")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Clear out environment CFLAGS/CXXFLAGS first
if(CMAKE_BUILD_TYPE MATCHES "^(Release|Debug)$")
    set(CMAKE_C_FLAGS "")
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
endif()

# Base compiler flags
add_compile_options("-pipe")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options("-Og" "-funwind-tables" "-DDEBUG")

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options("-ggdb")
    else()
        add_compile_options("-g")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options("-fdata-sections" "-ffunction-sections")
elseif(CMAKE_BUILD_TYPE)
    add_compile_options("-O1")
endif()

# Platform-specific linker flags
if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_link_options("-Wl,--no-undefined" "-Wl,-z,combreloc" "-Wl,--as-needed")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_link_options(
            "-Wl,--gc-sections"
            "-Wl,--strip-all"
            "-Wl,-O1"
            "-Wl,-z,relro"
            "-Wl,-z,now"
            "-Wl,--sort-common=descending"
            "-Wl,--hash-style=gnu"
        )
    endif()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_link_options("-Wl,-dead_strip")
    endif()
endif()

# Set build type to "Release" if user did not specify any build type yet
# Other possible values: Debug and None. 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
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

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        include_directories("/opt/homebrew/include")
        link_directories("/opt/homebrew/lib")
    else()
        include_directories("/usr/local/include")
        link_directories("/usr/local/lib")
    endif()

    link_libraries("-framework IOKit" "-framework Foundation" "iconv")

    add_compile_options("$<$<CONFIG:Debug>:-fno-omit-frame-pointer;-mno-omit-leaf-frame-pointer>")
endif()
