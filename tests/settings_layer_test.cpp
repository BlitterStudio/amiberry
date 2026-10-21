// Harness for the shared settings layer (src/osdep/settings). Built by
// tests/lib/settings_layer_build.sh against the layer sources and the core's
// fixup_cpu(), sliced out of src/main.cpp.
//
// Without arguments it checks descriptors, rules, sessions and validation.
// With --emit-roundtrip it prints configurations built through the layer for
// tests/test_settings_roundtrip.sh, which feeds them to a real binary.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "cpuboard.h"
#include "autoconf.h"
#include "x86.h"

#include "settings/amiberry_settings.h"
#include "settings/settings_definitions.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "settings/settings_actions_queue.h"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

// The globals and core functions the layer and fixup_cpu() use. The
// stand-in preview runs the real CPU fixup, the part of fixup_prefs() the
// CPU options depend on.
uae_prefs changed_prefs;
int emulating;
// uae_prefs is too large for the stack.
static uae_prefs test_prefs;

// Besides the real fixup_cpu(), the stand-in applies fixup_prefs()'s rule
// that a 24-bit address space removes Z3 RAM, so commit is tested on an
// adjustment outside the CPU options.
void fixup_prefs_preview(uae_prefs* p)
{
	fixup_cpu(p);
	if (p->address_space_24)
		p->z3fastmem[0].size = 0;
}
void error_log(const TCHAR*, ...) {}
// Controlled by the tests: whether the selected accelerator board is 32-bit
// only, and the address of a UAE boot ROM the configuration needs (0: none).
static bool board_is_32bit;
static uaecptr boot_rom_address;
int uae_boot_rom_type;
bool cpuboard_32bit(uae_prefs*) { return board_is_32bit; }
bool cpuboard_is_ppc_accelerator(uae_prefs*) { return false; }
void cpuboard_setboard(uae_prefs* p, int type, int subtype)
{
	p->cpuboard_type = type;
	p->cpuboard_subtype = subtype;
}
// Like the real one, records the boot ROM type in a global.
uaecptr need_uae_boot_rom(uae_prefs*)
{
	uae_boot_rom_type = boot_rom_address ? 1 : 0;
	return boot_rom_address;
}
bool is_ppc_cpu(uae_prefs*) { return false; }
int is_x86_cpu(uae_prefs*) { return 0; }

namespace settings = amiberry_settings;

namespace {

int failures;

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

const settings::context idle{false};

// The CPU fields of default_prefs() (src/cfgfile.cpp).
void reset_prefs(uae_prefs& p)
{
	std::memset(static_cast<void*>(&p), 0, sizeof p);
	p.cpu_model = 68000;
	p.cpu_compatible = true;
	p.address_space_24 = true;
	p.cpu_cycle_exact = true;
	p.cpu_memory_cycle_exact = true;
	p.blitter_cycle_exact = true;
	p.compnf = true;
	p.comp_constjump = true;
	p.comp_catchfault = true;
#ifdef USE_JIT_FPU
	p.compfpu = true;
#endif
}

std::string value_of(const uae_prefs& p, const char* id)
{
	char text[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(settings::get(settings::require(id), p, text, sizeof text) == AMIBERRY_SETTINGS_OK);
	return text;
}

amiberry_settings_status set(uae_prefs& p, const char* id, const char* text,
	const settings::context& ctx = idle)
{
	return settings::set(settings::require(id), p, ctx, text);
}

bool available(const uae_prefs& p, const char* id, const settings::context& ctx = idle)
{
	return settings::is_available(settings::require(id), p, ctx);
}

bool choice_available(const uae_prefs& p, const char* id, const char* choice)
{
	return settings::is_choice_available(settings::require(id), p, idle, choice);
}

bool jit_compiled()
{
#ifdef JIT
	return true;
#else
	return false;
#endif
}

// Every descriptor is complete and its default is a value it accepts.
void check_descriptors()
{
	CHECK(amiberry_settings_count() == settings::definition_count());
	CHECK(amiberry_settings_at(amiberry_settings_count()) == nullptr);
	CHECK(amiberry_settings_find("no_such_option") == nullptr);
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		CHECK(d && d->id && d->group && d->label && d->help && d->default_value);
		if (!d)
			continue;
		CHECK(amiberry_settings_find(d->id) == d);
		switch (d->type) {
		case AMIBERRY_SETTING_BOOL:
			CHECK(!std::strcmp(d->default_value, "true") || !std::strcmp(d->default_value, "false"));
			break;
		case AMIBERRY_SETTING_INT:
		case AMIBERRY_SETTING_REAL: {
			CHECK(d->min < d->max && d->step > 0);
			const double v = std::atof(d->default_value);
			CHECK(v >= d->min && v <= d->max);
			break;
		}
		case AMIBERRY_SETTING_CHOICE: {
			CHECK(d->choices && d->choice_count > 0);
			bool found = false;
			for (size_t c = 0; c < d->choice_count; c++) {
				CHECK(d->choices[c].value && d->choices[c].label);
				found = found || !std::strcmp(d->choices[c].value, d->default_value);
			}
			CHECK(found);
			break;
		}
		}
	}

	// The defaults are the values default_prefs() produces.
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const settings::definition& d = settings::definition_at(i);
		const std::string v = value_of(p, d.descriptor.id);
		if (v != d.descriptor.default_value)
			std::fprintf(stderr, "default of %s: prefs %s, descriptor %s\n", d.descriptor.id, v.c_str(),
				d.descriptor.default_value);
		CHECK(v == d.descriptor.default_value);
	}
}

// AE1: availability follows the CPU. A 68000 keeps 24-bit addressing and has
// no JIT, MMU or data cache; the 32-bit CPUs gain them.
void check_availability()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(available(p, "cpu_24bit_addressing"));
	CHECK(!available(p, "cachesize"));
	CHECK(!available(p, "mmu_model"));
	CHECK(!available(p, "cpu_compatible")); // memory cycle-exact forces it on
	CHECK(!available(p, "cpu_data_cache"));
	CHECK(!available(p, "cpu_threaded"));
	CHECK(!available(p, "fpu_strict"));
	CHECK(choice_available(p, "fpu_model", "0"));
	CHECK(!choice_available(p, "fpu_model", "68881")); // 68000 in More compatible mode
	CHECK(!choice_available(p, "fpu_model", "68040"));

	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.cpu_compatible = false;
	CHECK(available(p, "cpu_compatible"));
	CHECK(choice_available(p, "fpu_model", "68881")); // possible without More compatible

	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "cpu_24bit_addressing"));
	CHECK(value_of(p, "cpu_24bit_addressing") == "false");
	CHECK(available(p, "cachesize") == jit_compiled());
	CHECK(available(p, "mmu_model"));
	CHECK(choice_available(p, "mmu_model", "68030"));
	CHECK(choice_available(p, "mmu_model", "68ec030"));
	CHECK(!choice_available(p, "mmu_model", "68040"));
	CHECK(available(p, "cpu_threaded"));
	CHECK(!available(p, "cpu_threaded", settings::context{true}));

	CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "cpu_24bit_addressing"));
	CHECK(!choice_available(p, "fpu_model", "68881"));
	CHECK(choice_available(p, "fpu_model", "68040"));
	CHECK(!choice_available(p, "fpu_model", "68060"));
	CHECK(!available(p, "cpu_no_unimplemented"));

	CHECK(set(p, "mmu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "cachesize")); // no JIT with MMU emulation
	CHECK(!available(p, "cpu_threaded")); // MMU emulation without the JIT

	// The speed adjustment can only slow the CPU down at fastest speed.
	double min, max;
	CHECK(set(p, "cpu_speed", "max") == AMIBERRY_SETTINGS_OK);
	settings::get_range(settings::require("cpu_throttle"), p, min, max);
	CHECK(min == -900 && max == 0);
	CHECK(set(p, "cpu_throttle", "500") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpu_speed", "real") == AMIBERRY_SETTINGS_OK);
	settings::get_range(settings::require("cpu_throttle"), p, min, max);
	CHECK(max == 5000);
	CHECK(set(p, "cpu_throttle", "500") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpu_throttle") == "500.0");
}

void check_dependent_changes()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// A 68040 keeps an FPU as its internal one; a 68020 gets a 68881 back.
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fpu_model", "68882") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpu_multiplier") == "8");
	CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "68040");
	CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "68881");
	CHECK(value_of(p, "cpu_multiplier") == "4");
	CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "0"); // More compatible
	CHECK(value_of(p, "cpu_24bit_addressing") == "true");
	CHECK(value_of(p, "cpu_multiplier") == "2");

	// Without More compatible a 68000 keeps an external FPU, and an internal
	// one carried over from a 68040 becomes a 68881.
	p.cpu_compatible = false;
	CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "68881");

	// Switching the CPU resets the MMU, but setting the same CPU again keeps
	// it.
	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "mmu_model", "68ec030") == AMIBERRY_SETTINGS_OK);
	CHECK(p.mmu_model == 68030 && p.mmu_ec);
	CHECK(value_of(p, "mmu_model") == "68ec030");
	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "mmu_model") == "68ec030");
	CHECK(set(p, "cpu_model", "68060") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "mmu_model") == "0");

	// Choosing the current speed mode or memory access mode again
	// re-applies it, as a re-clicked radio button did in the panel.
	p.m68k_speed = -1;
	p.m68k_speed_throttle = -500;
	CHECK(set(p, "cpu_speed", "max") == AMIBERRY_SETTINGS_OK);
	CHECK(p.m68k_speed_throttle == 0);
	p.m68k_speed = 3 * CYCLE_UNIT; // cpu_speed=3
	CHECK(value_of(p, "cpu_speed") == "3");
	CHECK(set(p, "cpu_speed", "real") == AMIBERRY_SETTINGS_OK);
	CHECK(p.m68k_speed == 0);

	// Turning the JIT on leaves cycle-exact mode. A 68040/060 JIT greys out
	// More compatible but keeps its value for when the JIT is off again.
	if (jit_compiled()) {
		reset_prefs(p);
		CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
		CHECK(p.cpu_compatible);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(!p.cpu_cycle_exact && !p.cpu_memory_cycle_exact && !p.blitter_cycle_exact);
		CHECK(!available(p, "cpu_compatible"));
		CHECK(set(p, "cachesize", "0") == AMIBERRY_SETTINGS_OK);
		CHECK(available(p, "cpu_compatible") && p.cpu_compatible);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "comp_trustbyte", "indirect") == AMIBERRY_SETTINGS_OK);
		CHECK(p.comptrustbyte == 1 && p.comptrustword == 1 && p.comptrustlong == 1 && p.comptrustnaddr == 1);
		p.comptrustlong = 0; // a configuration can set them separately
		CHECK(set(p, "comp_trustbyte", "indirect") == AMIBERRY_SETTINGS_OK);
		CHECK(p.comptrustlong == 1);
	}

	// More compatible stays on in memory cycle-exact mode and drops the
	// FPU of a 68000.
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.cpu_compatible = false;
	CHECK(set(p, "fpu_model", "68881") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpu_compatible", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "0");

	// x86 bridgeboard speed is 0 at fastest CPU speed.
	reset_prefs(p);
	p.m68k_speed = -1;
	CHECK(set(p, "cpu_x86_throttle", "500") == AMIBERRY_SETTINGS_UNAVAILABLE); // no bridgeboard
}

// Every option offered as available must survive fixup_cpu(): a value the
// core would clear at once is unavailable, so neither the panel nor an API
// client can choose it.
void check_fixup_consistency()
{
	uae_prefs& p = test_prefs;
	auto survives = [&p](const char* id) {
		uae_prefs* copy = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
		std::memcpy(static_cast<void*>(copy), static_cast<const void*>(&p), sizeof p);
		fixup_cpu(copy);
		const bool same = value_of(*copy, id) == value_of(p, id);
		std::free(copy);
		return same;
	};

	// 24-bit addressing with a 32-bit-only accelerator board.
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.cpu_model = 68030;
	p.address_space_24 = false;
	p.cpuboard_type = 1;
	board_is_32bit = true;
	CHECK(!available(p, "cpu_24bit_addressing"));
	board_is_32bit = false;
	CHECK(available(p, "cpu_24bit_addressing"));
	CHECK(set(p, "cpu_24bit_addressing", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("cpu_24bit_addressing"));
	p.cpuboard_type = 0;
	p.address_space_24 = false;

	// Data cache emulation needs no UAE boot ROM or the indirect one, and the
	// check must not change the boot ROM type the running emulation uses.
	uae_boot_rom_type = 7;
	boot_rom_address = 0xf00000;
	CHECK(!available(p, "cpu_data_cache"));
	CHECK(uae_boot_rom_type == 7);
	p.uaeboard = 3;
	CHECK(available(p, "cpu_data_cache"));
	CHECK(set(p, "cpu_data_cache", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("cpu_data_cache"));
	p.uaeboard = 0;
	boot_rom_address = 0;
	CHECK(available(p, "cpu_data_cache"));
	CHECK(survives("cpu_data_cache"));

	// A 68000 in memory cycle-exact mode cannot keep an external FPU even
	// without More compatible.
	reset_prefs(p);
	p.cpu_compatible = false;
	CHECK(!choice_available(p, "fpu_model", "68881"));

	// A JIT left behind by a switch to a 68000 is cleared, so it cannot keep
	// softfloat and the other JIT-gated options unavailable.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		p.cpu_compatible = false;
		CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_24bit_addressing", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "fpu_model", "68881") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(p.compfpu);
		// The JIT FPU refuses softfloat but keeps host floating point
		// selectable.
		CHECK(available(p, "fpu_softfloat"));
		CHECK(!choice_available(p, "fpu_softfloat", "true"));
		CHECK(choice_available(p, "fpu_softfloat", "false"));
		CHECK(set(p, "fpu_softfloat", "true") == AMIBERRY_SETTINGS_INVALID_VALUE);
		CHECK(set(p, "fpu_softfloat", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
		CHECK(p.cachesize == 0 && p.fpu_model == 68881);
		CHECK(choice_available(p, "fpu_softfloat", "true"));
		CHECK(set(p, "fpu_softfloat", "true") == AMIBERRY_SETTINGS_OK);
		CHECK(survives("fpu_softfloat"));

		// Turning the JIT FPU on switches softfloat off, as fixup_cpu() would.
		CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_24bit_addressing", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(!p.compfpu); // softfloat kept the JIT FPU off
		CHECK(set(p, "compfpu", "true") == AMIBERRY_SETTINGS_OK);
		CHECK(p.fpu_mode == 0);
	}

	// A speed set with cpu_speed=N reads back as N, not as "real", and
	// writing it back is refused rather than silently changing the speed.
	reset_prefs(p);
	p.m68k_speed = 5 * CYCLE_UNIT;
	CHECK(value_of(p, "cpu_speed") == "5");
	CHECK(set(p, "cpu_speed", "5") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(p.m68k_speed == 5 * CYCLE_UNIT);

	// An EC flag without an MMU model reads as no MMU.
	p.mmu_model = 0;
	p.mmu_ec = true;
	CHECK(value_of(p, "mmu_model") == "0");

	// JIT caches below 128 KB are turned off by fixup_cpu(), so they are
	// refused; the JIT inhibit flag makes the JIT unavailable.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		p.cpu_model = 68020;
		p.address_space_24 = false;
		CHECK(set(p, "cachesize", "64") == AMIBERRY_SETTINGS_INVALID_VALUE);
		CHECK(set(p, "cachesize", "128") == AMIBERRY_SETTINGS_OK);
		CHECK(survives("cachesize"));
		p.cachesize_inhibit = true;
		CHECK(!available(p, "cachesize"));
	}

	// Turning softfloat off leaves the host 80-bit mode of MSVC long double
	// builds alone.
	reset_prefs(p);
	p.fpu_model = 68881;
	p.fpu_mode = -1;
	CHECK(set(p, "fpu_softfloat", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fpu_mode == -1);
	CHECK(set(p, "fpu_softfloat", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fpu_softfloat", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fpu_mode == 0);
}

void check_rejections()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(settings::find("no_such_option") == nullptr);
	CHECK(set(p, "cpu_model", "68050") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpu_24bit_addressing", "yes") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.cpu_idle", "20") == AMIBERRY_SETTINGS_UNAVAILABLE); // real speed
	p.m68k_speed = -1;
	CHECK(set(p, "amiberry.cpu_idle", "20") == AMIBERRY_SETTINGS_INVALID_VALUE); // not a step
	CHECK(set(p, "amiberry.cpu_idle", "180") == AMIBERRY_SETTINGS_INVALID_VALUE); // above range
	CHECK(set(p, "amiberry.cpu_idle", "30x") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.cpu_idle", "30") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "mmu_model", "68030") == AMIBERRY_SETTINGS_UNAVAILABLE); // 68000
	p.cpu_model = 68030;
	CHECK(set(p, "mmu_model", "68040") == AMIBERRY_SETTINGS_UNAVAILABLE); // other CPU's MMU
	CHECK(value_of(p, "mmu_model") == "0");
#ifdef WITH_PPC
	// Manual PPC setups come from an accelerator board configuration; the
	// option itself only switches automatic PPC on and off.
	p.cpu_model = 68060;
	CHECK(set(p, "ppc_model", "manual") == AMIBERRY_SETTINGS_INVALID_VALUE);
#endif
	char small[3];
	CHECK(settings::get(settings::require("cpu_model"), p, small, sizeof small) == AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
}

// AE2 for the CPU domain: validation reports what fixup_cpu() would change
// without changing the session, and commit stores the adjusted values.
void check_session()
{
	reset_prefs(changed_prefs);
	changed_prefs.cpu_model = 68040;
	changed_prefs.address_space_24 = true;

	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(session);
	amiberry_settings_adjustment adjustments[4];
	CHECK(amiberry_settings_validate(session, adjustments, 4) == 1);
	CHECK(!std::strcmp(adjustments[0].id, "cpu_24bit_addressing"));
	CHECK(!std::strcmp(adjustments[0].requested, "true"));
	CHECK(!std::strcmp(adjustments[0].adjusted, "false"));
	CHECK(adjustments[0].reason && std::strstr(adjustments[0].reason, "68040"));
	CHECK(amiberry_settings_validate(session, nullptr, 0) == 1); // counts without storage
	char text[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(amiberry_settings_get(session, "cpu_24bit_addressing", text, sizeof text) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(text, "true"));

	// 24-bit addressing only reaches a running emulation on restart, so
	// storing its adjustment needs a restart.
	bool changed = false;
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	CHECK(changed && changed_prefs.cpu_model == 68040 && !changed_prefs.address_space_24);
	CHECK(amiberry_settings_validate(session, adjustments, 4) == 0);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(!changed);

	// Nothing reaches the pending configuration before commit; a new CPU
	// applies live.
	CHECK(amiberry_settings_set(session, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(changed_prefs.cpu_model == 68040);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.cpu_model == 68030);

	// Fields the session did not change keep what other code set since.
	changed_prefs.cpu_idle = 45;
	CHECK(amiberry_settings_set(session, "fpu_model", "68882") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.fpu_model == 68882 && changed_prefs.cpu_idle == 45);

	// Changes made elsewhere are resolved together with the session's: an
	// external switch to a 68040 turns the session's 68881 into the internal
	// FPU, and validation says so before commit.
	CHECK(amiberry_settings_set(session, "fpu_model", "68881") == AMIBERRY_SETTINGS_OK);
	changed_prefs.cpu_model = 68040;
	std::memset(adjustments, 0xa5, sizeof adjustments);
	CHECK(amiberry_settings_validate(session, adjustments, 4) == 1);
	CHECK(!std::strcmp(adjustments[0].id, "fpu_model"));
	CHECK(!std::strcmp(adjustments[0].requested, "68881"));
	CHECK(!std::strcmp(adjustments[0].adjusted, "68040"));
	// No stack bytes leak through the fixed-size buffers.
	for (size_t i = std::strlen(adjustments[0].adjusted); i < sizeof adjustments[0].adjusted; i++)
		CHECK(adjustments[0].adjusted[i] == 0);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.cpu_model == 68040 && changed_prefs.fpu_model == 68040);
	CHECK(amiberry_settings_get(session, "fpu_model", text, sizeof text) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(text, "68040")); // the session continues from what it stored

	// Dependent changes outside the option list are committed too.
	if (jit_compiled()) {
		CHECK(amiberry_settings_set(session, "cachesize", "4096") == AMIBERRY_SETTINGS_OK);
		CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
		CHECK(changed_prefs.cachesize == 4096 && !changed_prefs.cpu_cycle_exact
			&& !changed_prefs.blitter_cycle_exact);
	}
	amiberry_settings_session_close(session);

	// Re-selecting the current JIT memory access mode aligns the other
	// modes; that is a change even though the option's own value is not.
	if (jit_compiled()) {
		changed_prefs.comptrustbyte = 1;
		changed_prefs.comptrustlong = 0;
		session = amiberry_settings_session_open();
		CHECK(amiberry_settings_set(session, "comp_trustbyte", "indirect") == AMIBERRY_SETTINGS_OK);
		CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
		CHECK(changed && changed_prefs.comptrustlong == 1);
		amiberry_settings_session_close(session);
	}

	// The strongest apply class among the changed options wins; an option
	// that can only change while stopped is unavailable during emulation.
	reset_prefs(changed_prefs);
	changed_prefs.cpu_cycle_exact = changed_prefs.cpu_memory_cycle_exact = changed_prefs.blitter_cycle_exact = false;
	changed_prefs.cpu_model = 68030;
	changed_prefs.address_space_24 = false;
	session = amiberry_settings_session_open();
	CHECK(amiberry_settings_set(session, "cpu_compatible", "false") == AMIBERRY_SETTINGS_OK);
	emulating = 1;
	CHECK(!amiberry_settings_is_available(session, "cpu_threaded"));
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_UNAVAILABLE);
	emulating = 0;
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	CHECK(changed && changed_prefs.cpu_thread);

	// Re-submitting an unchanged restart-only option with a live change
	// does not ask for a restart.
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "amiberry.cpu_idle", "0") == AMIBERRY_SETTINGS_UNAVAILABLE); // real speed
	CHECK(amiberry_settings_set(session, "cpu_speed", "max") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.m68k_speed < 0);

	// Commit stores what the fixups adjust outside the session's options:
	// 24-bit addressing removes Z3 RAM.
	CHECK(amiberry_settings_set(session, "cpu_threaded", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	changed_prefs.z3fastmem[0].size = 16 * 1024 * 1024;
	CHECK(amiberry_settings_set(session, "cpu_24bit_addressing", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	CHECK(changed && changed_prefs.address_space_24 && changed_prefs.z3fastmem[0].size == 0);

	double min, max;
	CHECK(amiberry_settings_get_range(session, "cpu_throttle", &min, &max));
	CHECK(!amiberry_settings_get_range(session, "cpu_model", &min, &max));
	CHECK(amiberry_settings_choice_is_available(session, "mmu_model", "68030"));
	CHECK(!amiberry_settings_choice_is_available(session, "mmu_model", "nonsense"));
	CHECK(amiberry_settings_get(session, "no_such_option", text, sizeof text) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	amiberry_settings_session_close(session);
}

// Prints a configuration for the round-trip test: the cycle-exact mode the
// CPU options depend on, then every option in table order (which keeps
// cpu_model ahead of fpu_model, whose value it resets when parsed). Only
// available options are expected back: the writer omits some of the others,
// for example the PPC options while PPC emulation is off.
void emit(const char* name, const uae_prefs& p)
{
	std::printf("S %s\n", name);
	const char* cycle_exact = p.cpu_cycle_exact ? "true" : p.cpu_memory_cycle_exact ? "memory" : "false";
	std::printf("A cycle_exact=%s\n", cycle_exact);
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const settings::definition& d = settings::definition_at(i);
		const std::string v = value_of(p, d.descriptor.id);
		std::printf("A %s=%s\n", d.descriptor.id, v.c_str());
		if (settings::is_available(d, p, idle))
			std::printf("E %s=%s %s\n", d.descriptor.id, v.c_str(), d.descriptor.default_value);
	}
}

void emit_roundtrip()
{
	std::printf("S defaults\n");
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const amiberry_setting_descriptor& d = settings::definition_at(i).descriptor;
		std::printf("E %s=%s %s\n", d.id, d.default_value, d.default_value);
	}

	// A failed set would quietly test a different configuration.
	uae_prefs& p = test_prefs;
	auto apply = [&p](const char* id, const char* v) { CHECK(set(p, id, v) == AMIBERRY_SETTINGS_OK); };
	reset_prefs(p);
	apply("cpu_model", "68020");
	emit("a1200_cycle_exact", p);

	reset_prefs(p);
	apply("cpu_model", "68030");
	apply("mmu_model", "68ec030");
	apply("fpu_model", "68882");
	apply("cpu_data_cache", "true");
	apply("cpu_multiplier", "8");
	emit("a3000_ec030", p);

	reset_prefs(p);
	apply("cpu_model", "68060");
	apply("fpu_model", "68060");
	apply("mmu_model", "68060");
	apply("fpu_softfloat", "true");
	apply("fpu_strict", "true");
	apply("cpu_no_unimplemented", "true");
	apply("fpu_no_unimplemented", "true");
	emit("a4000_060_softfloat", p);

	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("cpu_model", "68020");
	apply("cpu_24bit_addressing", "false");
	apply("cpu_multiplier", "4");
	apply("cpu_throttle", "500");
	emit("a1200_faster", p);

	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("cpu_model", "68020");
	apply("cpu_24bit_addressing", "false");
	apply("cpu_compatible", "false");
	apply("cpu_threaded", "true");
	apply("cpu_speed", "max");
	apply("amiberry.cpu_idle", "60");
	emit("threaded_020", p);

	if (jit_compiled()) {
		reset_prefs(p);
		apply("cpu_model", "68040");
		apply("fpu_model", "68040");
		apply("cachesize", "8192");
		apply("comp_trustbyte", "indirect");
		apply("comp_flushmode", "hard");
		apply("comp_nf", "false");
		apply("comp_catchfault", "false");
		apply("comp_constjump", "false");
		apply("cpu_speed", "max");
		emit("a4000_040_jit", p);
	}

#ifdef WITH_PPC
	reset_prefs(p);
	apply("cpu_model", "68060");
	apply("ppc_model", "automatic");
	apply("ppc_implementation", "pearpc");
	apply("ppc_cpu_idle", "max");
	emit("ppc", p);
#endif
}

} // namespace

// === Runtime action queue (settings/settings_actions_queue.h) ===
// The queue the emulator drains on its main thread: ordering, cross-thread
// posting, completion bookkeeping and bounded retention.

struct queue_log {
	std::mutex mutex;
	std::vector<int> executed; // the `number` of each executed request
	std::vector<std::string> errors;
};

static bool log_executor(const amiberry_actions::request& r, void* context, std::string* error)
{
	auto* log = static_cast<queue_log*>(context);
	std::lock_guard<std::mutex> lock(log->mutex);
	log->executed.push_back(r.number);
	if (r.flag_a) {
		*error = "requested failure";
		return false;
	}
	return true;
}

static void check_action_queue()
{
	amiberry_actions::action_queue queue;
	queue_log log;

	// Posting assigns increasing non-zero ids; a request stays pending
	// until run_pending() executes it.
	const auto first = queue.post(amiberry_actions::kind::pause, 0, 1, false, false, nullptr);
	const auto second = queue.post(amiberry_actions::kind::resume, 0, 2, false, false, nullptr);
	CHECK(first != 0 && second == first + 1);
	CHECK(queue.poll(first, nullptr) == 0);

	// Actions execute in posting order on the caller of run_pending().
	CHECK(queue.run_pending(log_executor, &log) == 2);
	CHECK(log.executed.size() == 2 && log.executed[0] == 1 && log.executed[1] == 2);

	// Polling consumes the result: done once, unknown afterwards. A failed
	// action reports its error text.
	CHECK(queue.poll(first, nullptr) == 1);
	CHECK(queue.poll(first, nullptr) == 3);
	std::string error;
	const auto failing = queue.post(amiberry_actions::kind::pause, 0, 3, true, false, nullptr);
	queue.run_pending(log_executor, &log);
	CHECK(queue.poll(failing, &error) == 2);
	CHECK(error == "requested failure");
	CHECK(queue.poll(failing, nullptr) == 3);

	// Unknown ids (never assigned, or zero) never report pending.
	CHECK(queue.poll(999999, nullptr) == 3);
	CHECK(queue.poll(0, nullptr) == 3);

	// The queue refuses more than max_pending requests; run_pending drains
	// them all and executes each exactly once.
	size_t accepted = 0;
	for (int i = 0; i < amiberry_actions::action_queue::max_pending + 10; i++) {
		if (queue.post(amiberry_actions::kind::pause, 0, 5000 + i, false, false, nullptr) != 0)
			accepted++;
	}
	CHECK(accepted == amiberry_actions::action_queue::max_pending);
	CHECK(queue.run_pending(log_executor, &log) == accepted);
	CHECK(log.executed.size() == 2 + 1 + accepted);

	// Posts from other threads interleave safely and keep their order per
	// the single executor: everything posted runs exactly once.
	std::vector<std::thread> posters;
	std::atomic<int> posted{0};
	for (int t = 0; t < 4; t++) {
		posters.emplace_back([&queue, &posted, t]() {
			for (int i = 0; i < 250; i++) {
				if (queue.post(amiberry_actions::kind::pause, 0, 20000 + t * 1000 + i,
					false, false, nullptr) != 0)
					posted.fetch_add(1);
			}
		});
	}
	for (auto& thread : posters)
		thread.join();
	CHECK(posted.load() == 1000);
	CHECK(queue.run_pending(log_executor, &log) == 1000);
	std::map<int, int> seen;
	{
		std::lock_guard<std::mutex> lock(log.mutex);
		for (int number : log.executed)
			seen[number]++;
	}
	CHECK(seen.size() == 3 + accepted + 1000); // 1,2,3 + queued + threaded
	for (const auto& entry : seen)
		CHECK(entry.second == 1);

	// Results are retained for the most recent max_results completions;
	// older unconsumed results retire to unknown.
	amiberry_actions::action_queue retiring;
	queue_log retiring_log;
	uint64_t oldest = 0;
	for (int round = 0; round < 11; round++) {
		for (int i = 0; i < 100; i++) {
			const auto id = retiring.post(amiberry_actions::kind::pause, 0, i,
				false, false, nullptr);
			if (round == 0 && i == 0)
				oldest = id;
		}
		retiring.run_pending(log_executor, &retiring_log);
	}
	CHECK(retiring_log.executed.size() == 1100);
	CHECK(retiring.poll(oldest, nullptr) == 3);
}

int main(int argc, char** argv)
{
	check_action_queue();

	if (argc > 1 && !std::strcmp(argv[1], "--emit-roundtrip")) {
		emit_roundtrip();
		return failures ? 1 : 0;
	}
	check_descriptors();
	check_availability();
	check_dependent_changes();
	check_rejections();
	check_session();
	check_fixup_consistency();
	if (failures) {
		std::fprintf(stderr, "settings layer: %d check(s) failed\n", failures);
		return 1;
	}
	std::printf("settings layer checks passed\n");
	return 0;
}
