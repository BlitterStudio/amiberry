# Toolchain file for Windows ARM64 (aarch64) builds using llvm-mingw.
#
# llvm-mingw ships clang wrappers named <triple>-clang / <triple>-clang++
# which invoke `clang --target=<triple>` with the right sysroot already
# configured. Download from: https://github.com/mstorsjo/llvm-mingw
#
# Usage via the windows-arm64-* CMake presets; this file is loaded as
# VCPKG_CHAINLOAD_TOOLCHAIN_FILE so vcpkg still runs on top.
#
# The llvm-mingw install root can be provided via the LLVM_MINGW_ROOT
# environment variable. When set, its bin/ is prepended to the search
# path so the prefixed wrappers are found regardless of PATH order.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_CROSSCOMPILING TRUE)

if(DEFINED ENV{LLVM_MINGW_ROOT})
    set(_LLVM_MINGW_BIN "$ENV{LLVM_MINGW_ROOT}/bin")
    list(PREPEND CMAKE_PROGRAM_PATH "${_LLVM_MINGW_BIN}")
endif()

set(_TRIPLE aarch64-w64-mingw32)

set(CMAKE_C_COMPILER    ${_TRIPLE}-clang)
set(CMAKE_CXX_COMPILER  ${_TRIPLE}-clang++)
set(CMAKE_RC_COMPILER   ${_TRIPLE}-windres)
set(CMAKE_AR            ${_TRIPLE}-ar)
set(CMAKE_RANLIB        ${_TRIPLE}-ranlib)
set(CMAKE_STRIP         ${_TRIPLE}-strip)

# Disable the default `-fuse-linker-plugin` / GCC-specific flags that CMake
# may try on GNU-like drivers; clang picks LLD via its driver configuration.
set(CMAKE_C_COMPILER_TARGET   ${_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${_TRIPLE})

# Confine find_* calls to the cross sysroot + vcpkg overlays. Without this
# CMake can pick up host Linux/macOS paths during tool discovery.
if(DEFINED ENV{LLVM_MINGW_ROOT})
    set(CMAKE_SYSROOT "$ENV{LLVM_MINGW_ROOT}/${_TRIPLE}")
    list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{LLVM_MINGW_ROOT}/${_TRIPLE}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

unset(_TRIPLE)
unset(_LLVM_MINGW_BIN)
