# Native Apple UI layer, enabled with -DAMIBERRY_NATIVE_APPLE_UI=ON.
#
# Compiles the shared Swift sources from apple/Shared into a static library
# linked into the amiberry target. This proves mixed Swift/C++ linking with
# both generators the project uses: Ninja (macOS) and Xcode (iOS). Swift
# imports the C settings API through the clang module map in apple/Shared
# (module AmiberrySettings) and exposes a C-callable startup hook
# (amiberry_native_ui_startup_log) that src/osdep/main.cpp calls.
#
# The settings API symbols live in the amiberry executable itself, so the
# Swift archive links against them from the final executable link; no extra
# library is needed.

enable_language(Swift)

add_library(amiberry_native_ui STATIC
        apple/Shared/AmiberryNativeUI.swift
)

# apple/Shared contains module.modulemap; putting it on the include path lets
# the Swift compiler discover the AmiberrySettings module (CMake passes
# include directories to swiftc as -I flags).
target_include_directories(amiberry_native_ui PUBLIC
        "${CMAKE_SOURCE_DIR}/apple/Shared"
)

# Linked as a build requirement so the Swift library exists before the
# executable's final link, and so amiberry_native_ui compiles with the same
# toolchain platform/deployment settings as the rest of the app.
target_link_libraries(${PROJECT_NAME} PRIVATE amiberry_native_ui)

# Guards the startup hook call in src/osdep/main.cpp.

# Link the executable with the C++ driver. CMake would otherwise drive the
# final link with swiftc because of the Swift dependency, and swiftc rejects
# the C/C++ compile flags on amiberry's link line (for example -pipe). The
# Swift static library carries autolink notes for the Swift runtime, which
# the Apple linker resolves from the toolchain.
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(${PROJECT_NAME} PRIVATE AMIBERRY_NATIVE_APPLE_UI)
