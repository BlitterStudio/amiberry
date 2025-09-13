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

target_include_directories(imgui PUBLIC .)
target_include_directories(imgui PUBLIC backends)

find_package(SDL2 REQUIRED)
target_link_libraries(imgui PRIVATE SDL2::SDL2)
