#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "uae.h"

#if defined(__linux__) && !defined(__ANDROID__)

#include <cstdio>
#include <cstring>

static bool read_device_tree_model(char* buf, const size_t len)
{
	FILE* f = fopen("/proc/device-tree/model", "rb");
	if (!f) {
		return false;
	}
	const size_t n = fread(buf, 1, len - 1, f);
	fclose(f);
	if (n == 0) {
		return false;
	}
	buf[n] = 0;
	return true;
}

// Returns true when running on a single-board computer known to be too slow
// for full cycle-exact emulation (Raspberry Pi 4 and older). The Pi 5 is
// deliberately NOT in this list.
bool host_detect_slow_sbc(void)
{
	char model[256];
	if (!read_device_tree_model(model, sizeof model)) {
		return false;
	}
	static const char* slow_boards[] = {
		"Raspberry Pi 4",
		"Raspberry Pi 3",
		"Raspberry Pi 2",
		"Raspberry Pi Zero",
		"Raspberry Pi Model",
		"Raspberry Pi Compute Module 4",
		"Raspberry Pi Compute Module 3",
		nullptr
	};
	for (int i = 0; slow_boards[i] != nullptr; i++) {
		if (strstr(model, slow_boards[i]) != nullptr) {
			write_log("Host board '%s' detected as a slow SBC\n", model);
			return true;
		}
	}
	return false;
}

#else

bool host_detect_slow_sbc(void)
{
	return false;
}

#endif
