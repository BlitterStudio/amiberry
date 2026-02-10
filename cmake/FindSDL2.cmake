# cmake/FindSDL2.cmake
# Workaround to help imgui find SDL2 which we've already set up in Dependencies.cmake via FetchContent (on Android)
# or via find_package(CONFIG) (on Desktop).

if(TARGET SDL2::SDL2)
    set(SDL2_FOUND TRUE)
    return()
endif()

# Check for targets created by FetchContent of SDL2 (e.g. static build)
if(TARGET SDL2-static)
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 ALIAS SDL2-static)
    endif()
    set(SDL2_FOUND TRUE)
    return()
endif()

# Check for generic alias created by Dependencies.cmake
if(TARGET SDL2)
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 ALIAS SDL2)
    endif()
    set(SDL2_FOUND TRUE)
    return()
endif()

# Fallback: try to search for it using Config mode.
# This ensures that if this module is picked up on a system where SDL2 is installed only as a Config package, it still works.
find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND)
    return()
endif()
