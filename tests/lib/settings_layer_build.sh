# Sourced by the settings layer tests.
# settings_layer_compile OUT
# Builds tests/settings_layer_test.cpp against the layer sources and the
# core's fixup_cpu(), which is sliced out of src/main.cpp so validation runs
# the real rules. SETTINGS_LAYER_DEFINES defaults to the default CMake
# options; a caller testing a binary built otherwise overrides it.
settings_layer_compile() {
	local out="$1" fixup="$1.fixup_cpu.cpp" chipfix="$1.built_in_chipset_prefs.cpp" refresh="$1.chipset_refresh.cpp"
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' \
			'#include "memory.h"' '#include "newcpu.h"' '#include "cpuboard.h"' \
			'#include "autoconf.h"' '#include "xwin.h"' '#include <algorithm>'
		# fixup_prefs_dimensions (with its static helper) is the part of
		# fixup_prefs() the display options answer to.
		awk '/^static void fixup_prefs_dim2/ { copy = 1 } copy { print } copy && /^void fixup_prefs_dimensions/ { seen = 1 } seen && /^}$/ { exit }' src/main.cpp
		awk '/^void fixup_cpu \(struct uae_prefs \*p\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/main.cpp
	} > "$fixup"
	if ! grep -q '^void fixup_cpu' "$fixup"; then
		echo "settings layer test: fixup_cpu() not found in src/main.cpp" >&2
		rm -f "$fixup"
		return 1
	fi
	# built_in_chipset_prefs() is the chipset half of fixup_prefs the
	# chipset domain depends on; it only writes the prefs it is given.
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' '#include "custom.h"'
		awk '/^bool has_expansion_with_rtc\(struct uae_prefs\* p, int chiplimit\)$/ { copy = 1 } copy { print } copy && /^}$/ && ++closed == 2 { exit }' src/cfgfile.cpp
	} > "$chipfix"
	if ! grep -q '^int built_in_chipset_prefs' "$chipfix"; then
		echo "settings layer test: built_in_chipset_prefs() not found in src/cfgfile.cpp" >&2
		rm -f "$fixup" "$chipfix" "$refresh"
		return 1
	fi
	# The displaydata options are exactly the entry the configuration writer
	# emits and the reader parses, so the writer and parser (with the
	# cfgfile helpers they use) are sliced out of src/cfgfile.cpp too.
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' '#include "custom.h"' '#include <cstdarg>'
		# cfgfile_unescape and its two-argument overload.
		awk '/^static TCHAR \*cfgfile_unescape\(const TCHAR \*s, const TCHAR \*\*endpos, TCHAR separator, bool min\)$/ { copy = 1 } copy { print } copy && /^}$/ && ++closed == 2 { exit }' src/cfgfile.cpp
		awk '/^static TCHAR \*cfgfile_escape \(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^static void cfgfile_append_optionf\(.*\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void write_chipset_refresh_entry\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void parse_chipset_refresh_entry\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
	} > "$refresh"
	local fn
	for fn in 'static TCHAR \*cfgfile_unescape(const TCHAR \*s, const TCHAR \*\*endpos)$' \
		'static TCHAR \*cfgfile_escape' 'static void cfgfile_append_optionf' \
		'void write_chipset_refresh_entry' 'void parse_chipset_refresh_entry'; do
		if ! grep -q "^$fn" "$refresh"; then
			echo "settings layer test: '$fn' was not found in src/cfgfile.cpp" >&2
			rm -f "$fixup" "$chipfix" "$refresh"
			return 1
		fi
	done
	# -O1: GCC emits unused static functions from the core headers at -O0,
	# and those reference emulator globals the harness does not link.
	# shellcheck disable=SC2046,SC2086
	${CXX:-c++} -std=c++17 -O1 -Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep \
		${SETTINGS_LAYER_DEFINES--DWITH_PPC -DUSE_PCEM} $(pkg-config --cflags sdl3) \
		-o "$out" tests/settings_layer_test.cpp "$fixup" "$chipfix" "$refresh" src/osdep/settings/settings_*.cpp \
		$(pkg-config --libs sdl3)
	local status=$?
	rm -f "$fixup" "$chipfix" "$refresh"
	return $status
}
