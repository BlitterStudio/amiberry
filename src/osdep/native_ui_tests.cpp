/*
 * Runner for --native-ui-tests (see native_ui_tests.h).
 */
#include "native_ui_tests.h"

#include <cstdio>
#include <dlfcn.h>
#include <string>

namespace {
std::string bundle;
int exit_status = 2;
}

void native_ui_tests_request(const char* bundle_path)
{
	bundle = bundle_path ? bundle_path : "";
}

bool native_ui_tests_requested()
{
	return !bundle.empty();
}

void native_ui_tests_run()
{
	// The bundle resolves the C APIs against this executable
	// (-bundle_loader) and stays loaded until the process exits.
	void* handle = dlopen(bundle.c_str(), RTLD_NOW | RTLD_LOCAL);
	if (!handle) {
		std::fprintf(stderr, "--native-ui-tests: %s\n", dlerror());
		exit_status = 2;
		return;
	}
	using run_tests = int (*)();
	auto* run = reinterpret_cast<run_tests>(dlsym(handle, "amiberry_native_ui_run_tests"));
	if (!run) {
		std::fprintf(stderr, "--native-ui-tests: %s has no amiberry_native_ui_run_tests\n", bundle.c_str());
		exit_status = 2;
		return;
	}
	exit_status = run() == 0 ? 0 : 1;
}

int native_ui_tests_exit_status()
{
	return exit_status;
}
