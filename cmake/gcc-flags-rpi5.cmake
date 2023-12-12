set(CMAKE_C_FLAGS_INIT "-march=armv8.2-a+crypto+fp16+rcpc+dotprod -mcpu=cortex-a76+crypto -mtune=cortex-a76")
set(CMAKE_CXX_FLAGS_INIT  "${CMAKE_C_FLAGS_INIT}")

