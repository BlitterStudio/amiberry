# crosscompile for amd64 (x86_64) target on a ARM64 Debian Linux host
# with clang and llvm linker (important)
# on host do
# dpkg --add-architecture amd64
# apt install clang
# apt install lld
# apt install cmake
# apt install crossbuild-essential-amd64
# apt install libusb-1.0-0-dev
# apt install rpm
# apt-get update
# apt-get install libusb-1.0-0-dev:amd64

# amd64 libs are now in /lib/x86_64-linux-gnu
# includes for x86_64 are now in /usr/x86_64-linux-gnu/include
# sometimes clang does not find the correct includes, so we need to provide the gcc includes directly
# for gcc version 12: /usr/x86_64-linux-gnu/include/c++/12/x86_64-linux-gnu
# start crossbuild with: cmake -DCMAKE_TOOLCHAIN_FILE=/path-to-this-file path-to-CMakeLists.txt -B path-to-build-dir

# it may be necessary to replace the link /usr/bin/strip to /usr/bin/llvm-strip- since gnu does not support foreign archs


set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CLANG_TARGET_TRIPLE x86_64-linux-gnu)
set(triple x86_64-linux-gnu)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})

add_link_options(-fuse-ld=lld)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-linux-gnu)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)
include_directories(SYSTEM /usr/x86_64-linux-gnu/include/;/usr/x86_64-linux-gnu/include/c++/10/x86_64-linux-gnu)
