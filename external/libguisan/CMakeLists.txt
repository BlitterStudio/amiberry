cmake_minimum_required(VERSION 3.16)
project(guisan VERSION 1.0 DESCRIPTION "Guisan")
add_library(guisan STATIC
        src/actionevent.cpp
        src/basiccontainer.cpp
        src/cliprectangle.cpp
        src/color.cpp
        src/defaultfont.cpp
        src/event.cpp
        src/exception.cpp
        src/focushandler.cpp
        src/font.cpp
        src/genericinput.cpp
        src/graphics.cpp
        src/gui.cpp
        src/guisan.cpp
        src/image.cpp
        src/imagefont.cpp
        src/inputevent.cpp
        src/key.cpp
        src/keyevent.cpp
        src/keyinput.cpp
        src/mouseevent.cpp
        src/mouseinput.cpp
        src/rectangle.cpp
        src/selectionevent.cpp
        src/widget.cpp
        src/opengl/opengl.cpp
        src/opengl/openglgraphics.cpp
        src/opengl/openglimage.cpp
        src/sdl/sdl.cpp
        src/sdl/sdl2graphics.cpp
        src/sdl/sdlgraphics.cpp
        src/sdl/sdlimage.cpp
        src/sdl/sdlimageloader.cpp
        src/sdl/sdlinput.cpp
        src/sdl/sdltruetypefont.cpp
        src/widgets/button.cpp
        src/widgets/checkbox.cpp
        src/widgets/container.cpp
        src/widgets/dropdown.cpp
        src/widgets/icon.cpp
        src/widgets/imagebutton.cpp
        src/widgets/imagetextbutton.cpp
        src/widgets/inputbox.cpp
        src/widgets/label.cpp
        src/widgets/listbox.cpp
        src/widgets/messagebox.cpp
        src/widgets/progressbar.cpp
        src/widgets/radiobutton.cpp
        src/widgets/scrollarea.cpp
        src/widgets/slider.cpp
        src/widgets/tab.cpp
        src/widgets/tabbedarea.cpp
        src/widgets/textbox.cpp
        src/widgets/textfield.cpp
        src/widgets/togglebutton.cpp
        src/widgets/window.cpp)
target_include_directories(guisan PRIVATE ${SDL2_INCLUDE_DIRS} ./include)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2IMAGE_LIBRARIES})

install(TARGETS guisan DESTINATION ./lib)
