cmake_minimum_required(VERSION 3.16)
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
install (TARGETS floppybridge DESTINATION lib)
