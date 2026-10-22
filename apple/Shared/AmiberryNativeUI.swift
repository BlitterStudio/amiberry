/*
 * Copyright (C) 2026 BlitterStudio
 * Amiberry native Apple UI layer: shared Swift sources used by the
 * macOS (NSHostingController) and iOS (UIHostingController) front ends.
 *
 * The layer imports the C settings, actions and media APIs through the
 * AmiberrySettings clang module (module.modulemap next to this file) and
 * exposes a C-callable entry point the emulator calls at startup. Bridge/
 * is the typed Swift API the front ends use over those C APIs; apple/Tests
 * tests it inside the emulator (tests/test_native_ui_bridge.sh).
 */

import Foundation
import os
import AmiberrySettings

private let nativeUILog = OSLog(subsystem: "com.blitterstudio.amiberry", category: "NativeUI")

/*
 * Called from src/osdep/main.cpp before the emulator starts. The @_cdecl
 * attribute gives the function a plain C symbol so the C++ side can declare
 * it extern "C" without any generated header. It proves the interop in both
 * directions: C++ calls Swift, and Swift calls amiberry_settings_count()
 * from the C settings layer compiled into the executable. The result goes
 * to the unified log, never stdout: command-line modes such as
 * --dump-config own stdout.
 */
@_cdecl("amiberry_native_ui_startup_log")
public func amiberryNativeUIStartupLog() -> Int32 {
	let count = amiberry_settings_count()
	os_log("settings layer ready: %{public}d options via C API", log: nativeUILog, type: .info,
		Int32(count))
	return Int32(count)
}
