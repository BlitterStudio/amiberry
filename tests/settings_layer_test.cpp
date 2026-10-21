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

// The globals and core functions the layer and fixup_cpu() use. The
// stand-in preview runs the real CPU fixup, the part of fixup_prefs() the
// CPU options depend on.
uae_prefs changed_prefs;
int emulating;
// uae_prefs is too large for the stack.
static uae_prefs test_prefs;

void fixup_prefs_preview(uae_prefs* p) { fixup_cpu(p); }
void error_log(const TCHAR*, ...) {}
bool cpuboard_32bit(uae_prefs*) { return false; }
bool cpuboard_is_ppc_accelerator(uae_prefs*) { return false; }
void cpuboard_setboard(uae_prefs* p, int type, int subtype)
{
	p->cpuboard_type = type;
	p->cpuboard_subtype = subtype;
}
uaecptr need_uae_boot_rom(uae_prefs*) { return 0; }
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
	p.m68k_speed = 3; // finegrain_cpu_speed reads as "real"
	CHECK(value_of(p, "cpu_speed") == "real");
	CHECK(set(p, "cpu_speed", "real") == AMIBERRY_SETTINGS_OK);
	CHECK(p.m68k_speed == 0);

	// Turning the JIT on leaves cycle-exact mode, and a 68040/060 JIT leaves
	// More compatible.
	if (jit_compiled()) {
		reset_prefs(p);
		CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
		CHECK(p.cpu_compatible);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(!p.cpu_cycle_exact && !p.cpu_memory_cycle_exact && !p.blitter_cycle_exact);
		CHECK(!p.cpu_compatible);
		CHECK(!available(p, "cpu_compatible"));
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

	// Dependent changes outside the option list are committed too.
	if (jit_compiled()) {
		CHECK(amiberry_settings_set(session, "cachesize", "4096") == AMIBERRY_SETTINGS_OK);
		CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
		CHECK(changed_prefs.cachesize == 4096 && !changed_prefs.cpu_cycle_exact
			&& !changed_prefs.blitter_cycle_exact);
	}

	// The strongest apply class among the changed options wins; an option
	// that can only change while stopped is unavailable during emulation.
	CHECK(amiberry_settings_set(session, "cachesize", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "cpu_compatible", "false") == AMIBERRY_SETTINGS_OK);
	emulating = 1;
	CHECK(!amiberry_settings_is_available(session, "cpu_threaded"));
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_UNAVAILABLE);
	emulating = 0;
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	CHECK(changed && changed_prefs.cpu_thread);

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

int main(int argc, char** argv)
{
	if (argc > 1 && !std::strcmp(argv[1], "--emit-roundtrip")) {
		emit_roundtrip();
		return failures ? 1 : 0;
	}
	check_descriptors();
	check_availability();
	check_dependent_changes();
	check_rejections();
	check_session();
	if (failures) {
		std::fprintf(stderr, "settings layer: %d check(s) failed\n", failures);
		return 1;
	}
	std::printf("settings layer checks passed\n");
	return 0;
}
