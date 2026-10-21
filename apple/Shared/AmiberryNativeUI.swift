/*
 * Copyright (C) 2026 BlitterStudio
 * Amiberry native Apple UI layer: shared Swift sources used by the
 * macOS (NSHostingController) and iOS (UIHostingController) front ends.
 *
 * The layer proves the mixed Swift/C++ link: it imports the C settings API
 * through the AmiberrySettings clang module (module.modulemap next to this
 * file) and exposes a C-callable entry point the emulator calls at startup.
 */

import Foundation
import AmiberrySettings

/*
 * Called from src/osdep/main.cpp before the emulator starts. The @_cdecl
 * attribute gives the function a plain C symbol so the C++ side can declare
 * it extern "C" without any generated header. It both directions-proves the
 * interop: C++ calls Swift, and Swift calls amiberry_settings_count() from
 * the C settings layer compiled into the executable.
 */
@_cdecl("amiberry_native_ui_startup_log")
public func amiberryNativeUIStartupLog() -> Int32 {
	let count = amiberry_settings_count()
	print("[AmiberryNativeUI] settings layer ready: \(count) options via C API")
	return Int32(count)
}
