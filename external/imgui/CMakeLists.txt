cmake_minimum_required(VERSION 3.16)
project(imgui)

add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_sdl2.cpp
    backends/imgui_impl_sdlrenderer2.cpp
)

if(IMGUI_USE_OPENGL)
    target_sources(imgui PRIVATE backends/imgui_impl_opengl3.cpp)
endif()

target_include_directories(imgui PUBLIC .)
target_include_directories(imgui PUBLIC backends)

find_package(SDL2 REQUIRED)
target_link_libraries(imgui PRIVATE SDL2::SDL2)

if(IMGUI_USE_OPENGL)
    if(ANDROID)
        # On Android, we use OpenGL ES. GLESv3 is usually what's needed for the OpenGL3 backend.
        target_link_libraries(imgui PRIVATE GLESv3)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(imgui PRIVATE OpenGL::GL)
    endif()
endif()
