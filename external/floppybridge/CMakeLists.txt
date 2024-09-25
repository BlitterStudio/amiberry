cmake_minimum_required(VERSION 3.16)
project (floppybridge VERSION 1.6.4)
enable_language(C CXX)
add_library(floppybridge SHARED
    src/ArduinoFloppyBridge.cpp
    src/ArduinoInterface.cpp
    src/CommonBridgeTemplate.cpp
    src/FloppyBridge.cpp
    src/ftdi.cpp
    src/GreaseWeazleBridge.cpp
    src/GreaseWeazleInterface.cpp
    src/pll.cpp
    src/RotationExtractor.cpp
    src/SerialIO.cpp
    src/SuperCardProBridge.cpp
    src/SuperCardProInterface.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE src)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "macOS platform detected")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND cp libfloppybridge.dylib libfloppybridge.so
        COMMAND install_name_tool -id libfloppybridge.so libfloppybridge.so
    )
endif ()

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)