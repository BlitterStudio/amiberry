cmake_minimum_required(VERSION 3.16)
project(imgui)

add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)

# SDL3 backend
target_sources(imgui PRIVATE
    backends/imgui_impl_sdl3.cpp
    backends/imgui_impl_sdlrenderer3.cpp
)

if(IMGUI_USE_OPENGL)
    target_sources(imgui PRIVATE backends/imgui_impl_opengl3.cpp)
endif()

target_include_directories(imgui PUBLIC .)
target_include_directories(imgui PUBLIC backends)

# SDL3 library linkage
# On Android, SDL3 is built via FetchContent in the parent project and the
# target already exists. Only call find_package when no SDL3 target is available.
if(TARGET SDL3::SDL3)
    set(_IMGUI_SDL_TARGET SDL3::SDL3)
elseif(TARGET SDL3-static)
    set(_IMGUI_SDL_TARGET SDL3-static)
elseif(TARGET SDL3)
    set(_IMGUI_SDL_TARGET SDL3)
else()
    find_package(SDL3 REQUIRED)
    set(_IMGUI_SDL_TARGET SDL3::SDL3)
endif()
target_link_libraries(imgui PRIVATE ${_IMGUI_SDL_TARGET})

if(IMGUI_USE_OPENGL)
    if(ANDROID)
        # On Android, we use OpenGL ES. GLESv3 is usually what's needed for the OpenGL3 backend.
        target_link_libraries(imgui PRIVATE GLESv3)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(imgui PRIVATE OpenGL::GL)
    endif()
endif()
