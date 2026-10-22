/*
 * --native-ui-tests=<bundle>: loads the Swift bridge's XCTest bundle
 * (apple/Tests) into the emulator where the built-in GUI would open, and
 * runs it on the main thread against the real settings layer. Only built
 * with -DAMIBERRY_NATIVE_UI_TESTS=ON (cmake/AppleNativeUI.cmake), which
 * is never set for a release build.
 */
#pragma once

#ifdef AMIBERRY_NATIVE_UI_TESTS

// Records the bundle named on the command line.
void native_ui_tests_request(const char* bundle_path);
bool native_ui_tests_requested();
// Loads the bundle and runs its tests; the result is the exit status.
void native_ui_tests_run();
// 0 when every test passed, 1 when a test failed or none ran, 2 when the
// bundle could not be loaded or the tests never ran.
int native_ui_tests_exit_status();

#endif
