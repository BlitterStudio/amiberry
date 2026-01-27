# Create our own Findharfbuzz.cmake module because harfbuzz-config.cmake,
# distributed along e.g. msys2-mingw64 does not support anything other then APPLE and UNIX.

include(FindPackageHandleStandardArgs)

find_library(harfbuzz_LIBRARY
    NAMES harfbuzz
)

find_path(harfbuzz_INCLUDE_PATH
    NAMES hb.h
    PATH_SUFFIXES harfbuzz
)

set(harfbuzz_COMPILE_FLAGS "" CACHE STRING "Extra compile flags of harfbuzz")

set(harfbuzz_LINK_LIBRARIES "" CACHE STRING "Extra link libraries of harfbuzz")

set(harfbuzz_LINK_FLAGS "" CACHE STRING "Extra link flags of harfbuzz")

find_package_handle_standard_args(harfbuzz
    REQUIRED_VARS harfbuzz_LIBRARY harfbuzz_INCLUDE_PATH
)

if (harfbuzz_FOUND)
    if (NOT TARGET harfbuzz::harfbuzz)
        add_library(harfbuzz::harfbuzz UNKNOWN IMPORTED)
        set_target_properties(harfbuzz::harfbuzz PROPERTIES
            IMPORTED_LOCATION "${harfbuzz_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${harfbuzz_INCLUDE_PATH}"
            COMPILE_FLAGS "${harfbuzz_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${harfbuzz_LINK_LIBRARIES}"
            INTERFACE_LINK_FLAGS "${harfbuzz_LINK_FLAGS}"
        )
    endif()
endif()
