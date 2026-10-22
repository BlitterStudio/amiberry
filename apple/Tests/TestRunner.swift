/*
 * Copyright (C) 2026 BlitterStudio
 * Entry point the emulator calls after loading this bundle for
 * --native-ui-tests (src/osdep/amiberry_gui.cpp).
 */

import XCTest

/// Runs every test case in the bundle on the calling (main) thread;
/// returns 0 when all passed, 1 when any failed or none ran.
@_cdecl("amiberry_native_ui_run_tests")
public func amiberryNativeUIRunTests() -> Int32 {
	let suite = XCTestSuite.default
	suite.run()
	guard let run = suite.testRun, run.executionCount > 0 else {
		return 1
	}
	return run.totalFailureCount == 0 ? 0 : 1
}
