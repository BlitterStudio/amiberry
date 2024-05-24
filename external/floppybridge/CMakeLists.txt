cmake_minimum_required(VERSION 3.13)
project (floppybridge VERSION 1.6.0)
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
target_include_directories(floppybridge PRIVATE src)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "macOS platform detected")
    add_custom_command(TARGET floppybridge POST_BUILD
        COMMAND cp libfloppybridge.dylib libfloppybridge.so
        COMMAND install_name_tool -id libfloppybridge.so libfloppybridge.so
    )
endif ()
