# Native Apple UI layer, enabled with -DAMIBERRY_NATIVE_APPLE_UI=ON.
#
# Compiles the shared Swift sources from apple/Shared into a static library
# (Swift module AmiberryNativeUI) linked into the amiberry target. This
# proves mixed Swift/C++ linking with both generators the project uses:
# Ninja (macOS) and Xcode (iOS). Swift imports the C settings API through
# the clang module map in apple/Shared (module AmiberrySettings) and exposes
# a C-callable startup hook (amiberry_native_ui_startup_log) that
# src/osdep/main.cpp calls. apple/Shared/Bridge is the typed Swift API over
# the settings, actions and media C APIs that the SwiftUI front ends use.
#
# The settings API symbols live in the amiberry executable itself, so the
# Swift archive links against them from the final executable link; no extra
# library is needed.

# Swift 6 for every Swift target. The targets also set Swift_LANGUAGE_VERSION,
# but the Xcode generator of the CMake on the CI runners adds the default
# language version (4) to OTHER_SWIFT_FLAGS after it, and the later
# -swift-version wins: the iOS sources then compiled in Swift 4 mode, where
# UIKit's API notes give the pre-4.2 spellings.
set(CMAKE_Swift_LANGUAGE_VERSION 6)
enable_language(Swift)

add_library(amiberry_native_ui STATIC
        apple/Shared/AmiberryNativeUI.swift
        apple/Shared/Bridge/EmulatorActions.swift
        apple/Shared/Bridge/LayerStrings.swift
        apple/Shared/Bridge/MediaLibrary.swift
        apple/Shared/Bridge/SettingsCatalog.swift
        apple/Shared/Bridge/SettingsSession.swift
        apple/Shared/Bridge/SettingsStore.swift
)
set_target_properties(amiberry_native_ui PROPERTIES
        Swift_MODULE_NAME AmiberryNativeUI
        Swift_LANGUAGE_VERSION 6
)
# The native front ends: what the emulator shows instead of the built-in
# GUI (src/osdep/native_ui_host.cpp). apple/Shared/UI holds the settings UI
# and the C entry points; each platform adds how they are presented, its
# alerts and file requesters: the settings window and menu bar on macOS,
# the settings screen, emulation overlay and on-screen keyboard on iOS.
target_sources(amiberry_native_ui PRIVATE
        apple/Shared/UI/AmigaKeyboard.swift
        apple/Shared/UI/AppFiles.swift
        apple/Shared/UI/HostEntryPoints.swift
        apple/Shared/UI/SettingsModel.swift
        apple/Shared/UI/SettingsViews.swift
        apple/Shared/UI/Library.swift
)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_sources(amiberry_native_ui PRIVATE
            apple/iOS/AmigaKeyboardView.swift
            apple/iOS/EmulationOverlay.swift
            apple/iOS/NativeHost.swift
            apple/iOS/SettingsPresenter.swift
    )
else()
    target_sources(amiberry_native_ui PRIVATE
            apple/macOS/MainMenu.swift
            apple/macOS/NativeHost.swift
            apple/macOS/SettingsWindow.swift
    )
endif()
target_sources(${PROJECT_NAME} PRIVATE src/osdep/native_ui_host.cpp)

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
target_compile_definitions(${PROJECT_NAME} PRIVATE AMIBERRY_NATIVE_APPLE_UI)
# Link the executable with the C++ driver. CMake would otherwise drive the
# final link with swiftc because of the Swift dependency, and swiftc rejects
# the C/C++ compile flags on amiberry's link line (for example -pipe). The
# Swift static library carries autolink notes for the Swift runtime, which
# the Apple linker resolves from the toolchain.
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Tests of the Swift bridge against the real layer (apple/Tests), for macOS
# test builds only: an XCTest bundle that the emulator loads with
# --native-ui-tests=<bundle> where the built-in GUI would open
# (tests/test_native_ui_bridge.sh). The option also compiles that flag into
# the emulator, so it is never set for a release build. The bundle is
# linked against the executable (-bundle_loader) for the C APIs and carries
# its own copy of the bridge, which the app only links as far as it uses it.
option(AMIBERRY_NATIVE_UI_TESTS "Build the native UI bridge tests (macOS test builds only, never for release)" OFF)
if(AMIBERRY_NATIVE_UI_TESTS)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(FATAL_ERROR "AMIBERRY_NATIVE_UI_TESTS runs the tests in the macOS app; iOS is not supported")
    endif()
    execute_process(
            COMMAND xcrun --sdk macosx --show-sdk-platform-path
            OUTPUT_VARIABLE AMIBERRY_MACOS_PLATFORM_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
    set(AMIBERRY_XCTEST_FRAMEWORKS "${AMIBERRY_MACOS_PLATFORM_PATH}/Developer/Library/Frameworks")
    set(AMIBERRY_XCTEST_LIBRARIES "${AMIBERRY_MACOS_PLATFORM_PATH}/Developer/usr/lib")

    add_library(amiberry_native_ui_tests MODULE
            apple/Tests/MacOSHostTests.swift
            apple/Tests/NativeUIBridgeTests.swift
            apple/Tests/SharedUITests.swift
            apple/Tests/TestRunner.swift
    )
    # The macOS front end's types are internal; the tests use @testable import.
    target_compile_options(amiberry_native_ui PRIVATE -enable-testing)
    set_target_properties(amiberry_native_ui_tests PROPERTIES
            OUTPUT_NAME AmiberryNativeUITests
            PREFIX ""
            SUFFIX ".bundle"
            Swift_MODULE_NAME AmiberryNativeUITests
            Swift_LANGUAGE_VERSION 6
    )
    # XCTest and its Swift overlay ship with Xcode's macOS platform, not the SDK.
    target_compile_options(amiberry_native_ui_tests PRIVATE
            "SHELL:-F ${AMIBERRY_XCTEST_FRAMEWORKS}"
            "SHELL:-I ${AMIBERRY_XCTEST_LIBRARIES}"
    )
    target_link_options(amiberry_native_ui_tests PRIVATE
            "SHELL:-F ${AMIBERRY_XCTEST_FRAMEWORKS}"
            "SHELL:-L ${AMIBERRY_XCTEST_LIBRARIES}"
            "SHELL:-Xlinker -bundle_loader -Xlinker $<TARGET_FILE:${PROJECT_NAME}>"
            "SHELL:-Xlinker -rpath -Xlinker ${AMIBERRY_XCTEST_FRAMEWORKS}"
            "SHELL:-Xlinker -rpath -Xlinker ${AMIBERRY_XCTEST_LIBRARIES}"
    )
    target_link_libraries(amiberry_native_ui_tests PRIVATE amiberry_native_ui "-framework XCTest")
    add_dependencies(amiberry_native_ui_tests ${PROJECT_NAME})
    target_sources(${PROJECT_NAME} PRIVATE src/osdep/native_ui_tests.cpp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE AMIBERRY_NATIVE_UI_TESTS)
endif()
