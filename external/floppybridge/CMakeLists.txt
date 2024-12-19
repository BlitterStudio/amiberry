cmake_minimum_required(VERSION 3.27)
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
target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)

#install(TARGETS ${PROJECT_NAME}
#  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/amiberry
#  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)