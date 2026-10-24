# Sourced by the settings layer tests.
# settings_layer_compile OUT
# Builds tests/settings_layer_test.cpp against the layer sources and the
# core's fixup_cpu(), which is sliced out of src/main.cpp so validation runs
# the real rules. SETTINGS_LAYER_DEFINES defaults to the default CMake
# options; a caller testing a binary built otherwise overrides it.
settings_layer_compile() {
	local out="$1" fixup="$1.fixup_cpu.cpp" chipfix="$1.built_in_chipset_prefs.cpp" refresh="$1.chipset_refresh.cpp"
	local events="$1.custom_events.cpp"
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' \
			'#include "memory.h"' '#include "newcpu.h"' '#include "cpuboard.h"' \
			'#include "autoconf.h"' '#include "xwin.h"' '#include <algorithm>'
		# fixup_prefs_dimensions (with its static helper) is the part of
		# fixup_prefs() the display options answer to.
		awk '/^static void fixup_prefs_dim2/ { copy = 1 } copy { print } copy && /^void fixup_prefs_dimensions/ { seen = 1 } seen && /^}$/ { exit }' src/main.cpp
		awk '/^void fixup_cpu \(struct uae_prefs \*p\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/main.cpp
		# The accelerator board rules the expansions domain applies, as the
		# core has them (over the harness's board table).
		awk '/^bool cpuboard_is_ppc_accelerator_subtype\(int type, int subtype\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cpuboard.cpp
		awk '/^int cpuboard_memorytype\(struct uae_prefs \*p\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cpuboard.cpp
		awk '/^int cpuboard_maxmemory\(struct uae_prefs \*p\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cpuboard.cpp
		awk '/^void cpuboard_set_cpu\(struct uae_prefs \*p\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cpuboard.cpp
	} > "$fixup"
	for fn in 'void fixup_cpu' 'bool cpuboard_is_ppc_accelerator_subtype' 'int cpuboard_memorytype' 'int cpuboard_maxmemory' 'void cpuboard_set_cpu'; do
		if ! grep -q "^$fn" "$fixup"; then
			echo "settings layer test: '$fn' was not found in src/main.cpp or src/cpuboard.cpp" >&2
			rm -f "$fixup"
			return 1
		fi
	done
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
	# The displaydata and cdimage options are exactly the entries the
	# configuration writer emits and the reader parses, so the writers and
	# parsers (with the cfgfile helpers they use) are sliced out of
	# src/cfgfile.cpp too.
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' '#include "custom.h"' '#include "blkdev.h"' '#include "autoconf.h"' '#include <cstdarg>' '#include <string>'
		grep '^static const TCHAR \*cdmodes\[\] = ' src/cfgfile.cpp
		awk '/^static int match_string \(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		# cfgfile_unescape and its two-argument overload.
		awk '/^static TCHAR \*cfgfile_unescape\(const TCHAR \*s, const TCHAR \*\*endpos, TCHAR separator, bool min\)$/ { copy = 1 } copy { print } copy && /^}$/ && ++closed == 2 { exit }' src/cfgfile.cpp
		awk '/^static TCHAR \*cfgfile_unescape_min\(const TCHAR \*s\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^static TCHAR \*cfgfile_option_find_it\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^bool cfgfile_option_find\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^TCHAR \*cfgfile_option_get\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^static TCHAR \*cfgfile_escape \(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^static TCHAR \*cfgfile_escape_min\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		# The board settings codec cpuboard_settings is spelled with.
		awk '/^int cfgfile_read_rom_settings\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void cfgfile_write_rom_settings\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^static void cfgfile_append_optionf\(.*\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void write_chipset_refresh_entry\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void parse_chipset_refresh_entry\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void write_cdimage_entry\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
		awk '/^void parse_cdimage_entry\(/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/cfgfile.cpp
	} > "$refresh"
	local fn
	for fn in 'static const TCHAR \*cdmodes' 'static int match_string' \
		'static TCHAR \*cfgfile_unescape(const TCHAR \*s, const TCHAR \*\*endpos)$' \
		'static TCHAR \*cfgfile_escape' 'static void cfgfile_append_optionf' \
		'static TCHAR \*cfgfile_unescape_min' 'static TCHAR \*cfgfile_option_find_it' 'bool cfgfile_option_find' 'TCHAR \*cfgfile_option_get' \
		'static TCHAR \*cfgfile_escape_min' 'int cfgfile_read_rom_settings' 'void cfgfile_write_rom_settings' \
		'void write_chipset_refresh_entry' 'void parse_chipset_refresh_entry' \
		'void write_cdimage_entry' 'void parse_cdimage_entry'; do
		if ! grep -q "^$fn" "$refresh"; then
			echo "settings layer test: '$fn' was not found in src/cfgfile.cpp" >&2
			rm -f "$fixup" "$chipfix" "$refresh"
			return 1
		fi
	done
	# How configurations spell a custom control's event: the reader and the
	# writer's helper, from src/osdep/amiberry_input.cpp.
	{
		printf '%s\n' '#include "sysconfig.h"' '#include "sysdeps.h"' '#include "options.h"' '#include "inputdevice.h"' '#include "amiberry_input.h"'
		awk '/^int find_inputevent\(const TCHAR\* key\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/osdep/amiberry_input.cpp
		awk '/^int custom_event_of\(const TCHAR\* value\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/osdep/amiberry_input.cpp
		awk '/^const TCHAR\* custom_event_name\(const int event\)$/ { copy = 1 } copy { print } copy && /^}$/ { exit }' src/osdep/amiberry_input.cpp
	} > "$events"
	for fn in 'int find_inputevent(' 'int custom_event_of(' 'const TCHAR\* custom_event_name('; do
		if ! grep -q "^$fn" "$events"; then
			echo "settings layer test: '$fn' was not found in src/osdep/amiberry_input.cpp" >&2
			rm -f "$fixup" "$chipfix" "$refresh" "$events"
			return 1
		fi
	done
	# -O1: GCC emits unused static functions from the core headers at -O0,
	# and those reference emulator globals the harness does not link.
	# shellcheck disable=SC2046,SC2086
	${CXX:-c++} -std=c++17 -O1 -Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep \
		${SETTINGS_LAYER_DEFINES--DWITH_PPC -DUSE_PCEM} $(pkg-config --cflags sdl3) \
		-o "$out" tests/settings_layer_test.cpp "$fixup" "$chipfix" "$refresh" "$events" src/osdep/settings/settings_*.cpp \
		$(pkg-config --libs sdl3)
	local status=$?
	rm -f "$fixup" "$chipfix" "$refresh" "$events"
	return $status
}
