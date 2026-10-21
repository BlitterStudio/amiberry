# Sourced by the settings layer tests.
# settings_layer_compile OUT
# Builds tests/settings_layer_test.cpp against the layer sources and the
# core's fixup_cpu(), which is sliced out of src/main.cpp so validation runs
# the real rules. The defines match the default CMake options.
settings_layer_compile() {
	local out="$1" fixup="$1.fixup_cpu.cpp"
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' \
			'#include "memory.h"' '#include "newcpu.h"' '#include "cpuboard.h"' \
			'#include "autoconf.h"' '#include <algorithm>'
		awk '/^void fixup_cpu \(struct uae_prefs \*p\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/main.cpp
	} > "$fixup"
	if ! grep -q '^void fixup_cpu' "$fixup"; then
		echo "settings layer test: fixup_cpu() not found in src/main.cpp" >&2
		rm -f "$fixup"
		return 1
	fi
	# shellcheck disable=SC2046
	${CXX:-c++} -std=c++17 -Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep \
		-DWITH_PPC -DUSE_PCEM $(pkg-config --cflags sdl3) \
		-o "$out" tests/settings_layer_test.cpp "$fixup" src/osdep/settings/settings_*.cpp
	local status=$?
	rm -f "$fixup"
	return $status
}
