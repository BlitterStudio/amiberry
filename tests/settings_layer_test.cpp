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
#include "custom.h"
#include "disk.h"
#include "blkdev.h"
#include "x86.h"
#include "gfxboard.h"
#include "rommgr.h"

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

// The memory window the memory domain clamps against; the real value is
// host-dependent, the tests use the common 512 MB reservation.
size_t max_z3fastmem = 512 * 1024 * 1024;
int cpuboard_maxmemory(uae_prefs*) { return 128 * 1024 * 1024; }
// Like the real one, no board has no memory type; the tests pick the type
// a configured board uses.
static int board_memory_type = BOARD_MEMORY_HIGHMEM;
int cpuboard_memorytype(uae_prefs* p) { return p->cpuboard_type ? board_memory_type : 0; }
// Zorro II for the UAE Z2 board type, Zorro III otherwise.
int gfxboard_get_configtype(rtgboardconfig* rbc) { return rbc->rtgmem_type == GFXBOARD_UAE_Z2 ? 2 : 3; }

// A scanned ROM list with one cartridge dump, for the cartridge ident.
static romdata test_cart_rom = [] {
	romdata rd{};
	rd.name = "Action Replay III";
	rd.configname = "AR3";
	rd.type = ROMTYPE_AR;
	return rd;
}();
static romlist test_rom_list[] = {{const_cast<TCHAR*>("/roms/ar3.rom"), &test_cart_rom}};
int romlist_count() { return 1; }
romlist* romlist_getit() { return test_rom_list; }

// Preset engine stand-in: the harness cannot link cfgfile.cpp, so it
// records the request and applies a few marker fields the Quickstart
// constraints are known to react to, plus one target default outside every
// option domain (kbd_led_num) the way a fresh configuration resets it.
// Both the layer and the frozen reference implementation below run against
// this same stand-in, so the test still proves the constraint move changed
// nothing.
static int stub_preset_model = -1, stub_preset_config = -1, stub_preset_compa = -1;
static int stub_preview_calls;
int built_in_prefs(uae_prefs* p, int model, int config, int compa, int romcheck)
{
	(void)romcheck;
	stub_preset_model = model;
	stub_preset_config = config;
	stub_preset_compa = compa;
	p->floppyslots[0].dfxtype = DRV_35_HD; // constraints must force DD on A500-class
	p->floppyslots[1].dfxtype = DRV_35_DD;
	p->cdslots[0].inuse = true;
	p->cdslots[0].type = SCSI_UNIT_DEFAULT;
	p->jports[1].mode = 3;
	p->kbd_led_num = -1;
	return 1;
}
int built_in_prefs_preview(uae_prefs* p, int model, int config, int compa)
{
	stub_preview_calls++;
	return built_in_prefs(p, model, config, compa, 0);
}

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
	// The machine fields of default_prefs() (src/cfgfile.cpp), resolved the
	// way fixup_prefs() leaves them: with the Generic chipset extra the
	// built-in chipset table rewrites the A500-like options (the default
	// 512 KB slow RAM carries an RTC), and the resolved values are what the
	// emulator settles on.
	p.chipset_mask = CSMASK_ECS_AGNUS;
	p.collision_level = 3;
	p.keyboard_nkro = true;
	p.cs_compatible = CP_GENERIC;
	p.cs_rtc = 1;
	p.cs_ciaoverlay = true;
	p.cs_ksmirror_e0 = true;
	p.cs_df0idhw = false;
	p.cs_resetwarning = false;
	p.cs_ciatodbug = true;
	p.cs_memorypatternfill = true;
	p.cs_fatgaryrev = -1;
	p.cs_ramseyrev = -1;
	p.cs_agnusrev = -1;
	p.cs_deniserev = -1;
	p.chipmem.size = 0x80000;
	p.bogomem.size = 0x80000;
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
		case AMIBERRY_SETTING_PATH:
		case AMIBERRY_SETTING_STRING:
			// Free-form values only need a non-null default string.
			break;
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
// values that survive the configuration checks are expected back: the
// writer omits some unavailable options (the PPC options while PPC
// emulation is off), and a held value the layer would refuse (the RTC of a
// machine preset another option has since replaced) is not the value a
// reload produces. Sub-1 MB RAM boards are represented by their *_size_k
// key only; a zero one would reset a megabyte board the writer keeps in
// <name>_size, a combination no saved configuration contains.
void emit(const char* name, const uae_prefs& p)
{
	std::printf("S %s\n", name);
	const char* cycle_exact = p.cpu_cycle_exact ? "true" : p.cpu_memory_cycle_exact ? "memory" : "false";
	std::printf("A cycle_exact=%s\n", cycle_exact);
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const settings::definition& d = settings::definition_at(i);
		const std::string v = value_of(p, d.descriptor.id);
		const bool size_k_view = std::strstr(d.descriptor.id, "_size_k") != nullptr;
		if (size_k_view && v == "0")
			continue;
		std::printf("A %s=%s\n", d.descriptor.id, v.c_str());
		if (!settings::is_available(d, p, idle))
			continue;
		if ((d.descriptor.type == AMIBERRY_SETTING_CHOICE || d.descriptor.type == AMIBERRY_SETTING_BOOL)
			&& !settings::is_choice_available(d, p, idle, v))
			continue;
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
	auto apply = [&p](const char* id, const char* v) {
		if (set(p, id, v) != AMIBERRY_SETTINGS_OK) {
			std::fprintf(stderr, "round-trip scenario: setting %s=%s failed\n", id, v);
			failures++;
		}
	};
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

	// Machine scenarios: chipset, memory and ROM option classes.
	reset_prefs(p);
	apply("chipset", "aga");
	apply("collision_level", "sprites");
	apply("keyboard_type", "a1200_6805");
	apply("keyboard_nkro", "false");
	apply("display_optimizations", "none");
	apply("hvcsync", "csync");
	apply("ntsc", "true");
	emit("aga_ntsc", p);

	reset_prefs(p);
	apply("cpu_model", "68020");
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("cycle_exact", "memory");
	apply("immediate_blits", "true");
	emit("cycle_exact_memory", p);

	reset_prefs(p);
	apply("chipset_compatible", "A500");
	emit("a500_compatible", p);

	reset_prefs(p);
	// The A600 extra enables the machine's IDE controller (ide=a600/a1200
	// is written by the core); the layer's ide option itself stays greyed
	// out under a chipset extra, exactly like the old panel.
	apply("chipset_compatible", "A600");
	apply("chipmem_size", "2");
	emit("a600_compatible", p);

	reset_prefs(p);
	apply("cpu_model", "68030");
	apply("cpu_24bit_addressing", "false");
	apply("cycle_exact", "false");
	apply("cpu_compatible", "false");
	apply("chipmem_size", "4");
	apply("bogomem_size", "0");
	apply("fastmem_size", "8");
	apply("z3mem_size", "128");
	apply("z3mem2_size", "16");
	apply("a3000mem_size", "8");
	apply("mbresmem_size", "32");
	apply("megachipmem_size", "32");
	apply("z3mapping", "real");
	emit("bigbox_ram", p);

	reset_prefs(p);
	apply("fastmem_size_k", "512");
	emit("fastmem_kb", p);

	reset_prefs(p);
	apply("cpu_model", "68030");
	apply("cpu_24bit_addressing", "false");
	apply("cycle_exact", "false");
	apply("cpu_compatible", "false");
	apply("maprom", "0x0f000000");
	apply("kickshifter", "true");
	apply("uaeboard", "full");
	emit("maprom_uaeboard", p);

	reset_prefs(p);
	apply("genlock", "true");
	apply("genlockmode", "testcard");
	apply("genlock_mix", "75");
	apply("genlock_alpha", "true");
	apply("genlock_aspect", "true");
	emit("genlock", p);

	// Values next to the fixup rules the layer mirrors: each must come back
	// from the real fixup_prefs() unchanged.
	reset_prefs(p);
	apply("chipset_compatible", "A1200");
	apply("bogomem_size", "6");
	emit("a1200_slow_ram", p);

	reset_prefs(p);
	apply("chipset_compatible", "-");
	apply("cpu_model", "68030");
	apply("ide", "a4000");
	apply("ramsey", "13");
	emit("custom_a4000_ide", p);

	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("waiting_blits", "automatic");
	apply("cycle_exact", "true");
	emit("cycle_exact_68000_blitter", p);

	reset_prefs(p);
	apply("uaeboard", "full_off");
	apply("fastmem2_size_k", "128");
	emit("uaeboard_nodiag", p);

	if (jit_compiled()) {
		reset_prefs(p);
		apply("cpu_model", "68040");
		apply("cycle_exact", "false");
		apply("cpu_compatible", "false");
		apply("cachesize", "8192");
		apply("chipmem_size", "4");
		apply("bogomem_size", "4");
		emit("jit_ram", p);
	}
}

// ---------------------------------------------------------------- machine

// The availability rules the panels used to enforce with disabled widgets
// (and, in three cases, with writes on every frame).
void check_machine_availability()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	// Memory cycle-exact on a 68000 keeps the blitter waiting option off.
	CHECK(!available(p, "waiting_blits"));
	set(p, "cycle_exact", "false");
	CHECK(available(p, "waiting_blits"));
	CHECK(!available(p, "keyboard_nkro")); // default keyboard is UAE
	set(p, "keyboard_type", "a500_6570-036");
	CHECK(available(p, "keyboard_nkro"));
	set(p, "keyboard_type", "a2000_8039");
	CHECK(!available(p, "keyboard_nkro"));
	set(p, "keyboard_type", "disconnected");
	CHECK(available(p, "keyboard_nkro"));

	set(p, "immediate_blits", "true");
	CHECK(!available(p, "waiting_blits"));
	set(p, "immediate_blits", "false");
	CHECK(available(p, "waiting_blits"));

	// Advanced chipset options are greyed out under a chipset extra.
	CHECK(!available(p, "ciaatod"));
	CHECK(!available(p, "ide"));
	CHECK(!available(p, "pcmcia"));
	CHECK(!available(p, "ramsey"));
	// The RTC radios stay usable under an extra, but only the chip of the
	// selected machine survives the configuration checks (fixup_prefs
	// re-runs the built-in chipset table); the default 512 KB slow RAM
	// carries an RTC, so Generic resolves to MSM6242B.
	CHECK(available(p, "rtc"));
	CHECK(choice_available(p, "rtc", "MSM6242B"));
	CHECK(!choice_available(p, "rtc", "RP5C01A"));
	CHECK(value_of(p, "rtc") == "MSM6242B");
	CHECK(set(p, "rtc", "RP5C01A") == AMIBERRY_SETTINGS_UNAVAILABLE);
	set(p, "chipset_compatible", "-");
	CHECK(available(p, "ciaatod"));
	CHECK(available(p, "ide"));
	CHECK(available(p, "ramsey"));
	CHECK(choice_available(p, "rtc", "RP5C01A"));

	// The 1M chip jumper only exists below 1 MB chip RAM.
	CHECK(available(p, "1mchipjumper"));
	set(p, "chipmem_size", "2");
	CHECK(!available(p, "1mchipjumper"));

	// Z3 memory needs 32-bit addressing.
	CHECK(!available(p, "z3mem_size"));
	CHECK(!available(p, "megachipmem_size"));
	CHECK(!available(p, "z3mapping"));
	CHECK(!choice_available(p, "z3mem_size", "512"));
	set(p, "cpu_model", "68030"); // clears 24-bit addressing
	CHECK(value_of(p, "cpu_24bit_addressing") == "false");
	CHECK(available(p, "z3mem_size"));
	CHECK(choice_available(p, "z3mem_size", "512"));
	CHECK(!choice_available(p, "z3mem_size", "1024")); // above the 512 MB window

	// MapROM conflicts with an accelerator board; the UAE board needs a
	// stopped emulator.
	CHECK(available(p, "maprom"));
	CHECK(available(p, "uaeboard"));
	p.cpuboard_type = 1;
	CHECK(!available(p, "maprom"));
	p.cpuboard_type = 0;
	CHECK(!available(p, "uaeboard", settings::context{true}));

	// The genlock group needs a genlock (or genlock effects).
	CHECK(!available(p, "genlockmode"));
	CHECK(!available(p, "genlock_mix"));
	set(p, "genlock", "true");
	CHECK(available(p, "genlockmode"));
	CHECK(available(p, "genlock_mix"));
	CHECK(choice_available(p, "genlockmode", "testcard"));
	CHECK(!choice_available(p, "genlockmode", "video")); // VIDEOGRAB-only
	set(p, "genlockmode", "image");
	CHECK(available(p, "genlock_image"));
	CHECK(!available(p, "genlock_video"));
}

// The dependent changes the RAM and Chipset panels made on interaction.
void check_machine_dependents()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// AGA implies both ECS chips; ECS Agnus follows chip RAM above 512 KB.
	set(p, "chipset", "aga");
	CHECK((p.chipset_mask & CSMASK_AGA) && (p.chipset_mask & CSMASK_ECS_AGNUS)
		&& (p.chipset_mask & CSMASK_ECS_DENISE));
	reset_prefs(p);
	set(p, "chipset", "ocs");
	CHECK(p.chipset_mask == CSMASK_OCS);
	set(p, "chipmem_size", "2");
	CHECK(p.chipset_mask & CSMASK_ECS_AGNUS);

	// Chip RAM above 2 MB pushes Z2 fast RAM out; the other direction too.
	// (chipmem_size counts 512 KB units: "8" is 4 MB, "16" is 8 MB.)
	p.fastmem[0].size = 8 * 1024 * 1024;
	set(p, "chipmem_size", "8");
	CHECK(p.fastmem[0].size <= 262144);
	set(p, "chipmem_size", "16");
	CHECK(p.chipmem.size == 0x800000);
	p.fastmem[0].size = 4 * 1024 * 1024;
	set(p, "fastmem_size", "8");
	CHECK(p.chipmem.size == 0x200000);

	// Z3 memory above the reserved window is unavailable (the panel used
	// to clamp the slider silently); inside it, it stores verbatim.
	set(p, "cpu_model", "68030");
	max_z3fastmem = 256 * 1024 * 1024;
	CHECK(set(p, "z3mem_size", "512") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "z3mem_size", "256") == AMIBERRY_SETTINGS_OK);
	CHECK(p.z3fastmem[0].size == 256 * 1024 * 1024);
	CHECK(set(p, "megachipmem_size", "512") == AMIBERRY_SETTINGS_UNAVAILABLE);
	max_z3fastmem = 512 * 1024 * 1024;
	set(p, "z3mem_size", "512");
	CHECK(p.z3fastmem[0].size == 512 * 1024 * 1024);

	// NTSC moves the output refresh rate, but only on a real change.
	set(p, "ntsc", "true");
	CHECK(p.ntscmode && p.chipset_refreshrate == 60.0f);
	p.chipset_refreshrate = 57.0f;
	set(p, "ntsc", "true"); // re-clicked
	CHECK(p.chipset_refreshrate == 57.0f);
	set(p, "ntsc", "false");
	CHECK(!p.ntscmode && p.chipset_refreshrate == 50.0f);

	// Blitter waiting and immediate blits exclude each other.
	set(p, "immediate_blits", "true");
	CHECK(p.waiting_blits == 0);
	set(p, "immediate_blits", "false");
	set(p, "waiting_blits", "always");
	CHECK(!p.immediate_blits && p.waiting_blits == 3);

	// Cycle exactness: memory on a 68020 applies the WinUAE dependents; off
	// clears everything. (Memory exact needs a 68020: below that it is
	// indistinguishable from full cycle exact and unavailable.)
	reset_prefs(p);
	set(p, "cpu_model", "68020");
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.gfx_framerate = 2;
	set(p, "cycle_exact", "memory");
	CHECK(!p.cpu_cycle_exact && p.cpu_memory_cycle_exact && p.blitter_cycle_exact);
	CHECK(p.m68k_speed == 0 && p.cpu_compatible && !p.immediate_blits && p.cachesize == 0
		&& p.gfx_framerate == 1);
	set(p, "cycle_exact", "true");
	CHECK(p.cpu_cycle_exact && p.cpu_memory_cycle_exact && p.blitter_cycle_exact);
	set(p, "cycle_exact", "false");
	CHECK(!p.cpu_cycle_exact && !p.cpu_memory_cycle_exact && !p.blitter_cycle_exact);
	CHECK(!choice_available(p, "cycle_exact", "memory") || p.cpu_model >= 68020);

	// A chipset extra rewrites the advanced chipset options, and the RTC
	// radios normalise to the machine's chip.
	reset_prefs(p);
	set(p, "chipset_compatible", "A500");
	CHECK(!p.cs_df0idhw && !p.cs_resetwarning);
	CHECK(p.cs_rtc == 1 && value_of(p, "rtc") == "MSM6242B");
	set(p, "chipset_compatible", "A3000");
	CHECK(value_of(p, "rtc") == "RP5C01A");

	// Keyboards without N-key rollover force the option on (the panel
	// wrote it every frame while the control was greyed out).
	reset_prefs(p);
	set(p, "keyboard_nkro", "false");
	set(p, "keyboard_type", "UAE");
	CHECK(p.keyboard_nkro);
	set(p, "keyboard_type", "a500_6570-036");
	CHECK(available(p, "keyboard_nkro"));
	set(p, "keyboard_nkro", "false");
	CHECK(!p.keyboard_nkro);

	// Processor slot RAM syncs the accelerator board copy.
	reset_prefs(p);
	p.cpuboard_type = 1;
	set(p, "mbresmem_size", "32");
	CHECK(p.cpuboardmem1.size == 32 * 1024 * 1024);

	// The automatic PPC configuration installs the accelerator board the
	// fixups would (with its RAM mirrored into the processor slot), and
	// leaving it removes the board again.
#ifdef WITH_PPC
	reset_prefs(p);
	set(p, "cpu_model", "68060");
	set(p, "ppc_model", "automatic");
	CHECK(p.ppc_mode == 1 && p.cpuboardmem1.size == 8 * 1024 * 1024);
	CHECK(p.mbresmem_high.size == 8 * 1024 * 1024);
	CHECK(value_of(p, "mbresmem_size") == "8");
	set(p, "ppc_model", "");
	CHECK(p.ppc_mode == 0 && p.cpuboard_type == 0 && p.mbresmem_high.size == 0);
#endif
	// The UAE board combo stores board and boot ROM as the panel did.
	set(p, "uaeboard", "full");
	CHECK(p.uaeboard == 2 && p.boot_rom == 0);
	CHECK(value_of(p, "boot_rom_uae") == "automatic");
}

void check_machine_rejections()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(set(p, "chipset", "ocs_agnus") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "chipmem_size", "5") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "bogomem_size", "3") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "collision_level", "4") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "rtc", "RP5C01") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "hvcsync", "nope") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// STRING values are validated by the setter: an unparseable maprom
	// token is accepted but leaves the address unchanged.
	set(p, "maprom", "0x00e00000");
	set(p, "maprom", "not-hex");
	CHECK(value_of(p, "maprom") == "0xe00000");
	set(p, "genlock", "true");
	CHECK(set(p, "genlock_mix", "30") == AMIBERRY_SETTINGS_INVALID_VALUE); // not a step
	CHECK(set(p, "z3mem_size", "8") == AMIBERRY_SETTINGS_UNAVAILABLE); // 24-bit
	CHECK(set(p, "keyboard_nkro", "false") == AMIBERRY_SETTINGS_UNAVAILABLE); // UAE keyboard
	CHECK(set(p, "ide", "a4000") == AMIBERRY_SETTINGS_UNAVAILABLE); // chipset extra active
	CHECK(set(p, "ciaatod", "50hz") == AMIBERRY_SETTINGS_UNAVAILABLE);
}

// The Quickstart constraints, frozen as they were in
// src/osdep/imgui/quickstart.cpp before the move: the layer must reproduce
// them exactly for every model, configuration and compatibility level.
void reference_constraints(uae_prefs& p, int model, int conf, int compa)
{
	if (compa < 0)
		compa = 0;
	if (compa > ((model == 4 || model == 8) ? 4 : 3))
		compa = (model == 4 || model == 8) ? 4 : 3;
	built_in_prefs(&p, model, conf, compa, 0);
	if (model <= 4) {
		if (p.floppyslots[0].dfxtype == DRV_35_HD)
			p.floppyslots[0].dfxtype = DRV_35_DD;
	}
	switch (model) {
	case 0: case 1: case 2: case 3: case 4: case 5:
		if (p.floppyslots[0].dfxtype == DRV_NONE)
			p.floppyslots[0].dfxtype = DRV_35_DD;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = false;
		p.cdslots[0].type = SCSI_UNIT_DISABLED;
		p.jports[1].mode = 0;
		break;
	case 6: case 7: case 12:
		if (p.floppyslots[0].dfxtype == DRV_NONE)
			p.floppyslots[0].dfxtype = DRV_35_HD;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = false;
		p.cdslots[0].type = SCSI_UNIT_DISABLED;
		p.jports[1].mode = 0;
		break;
	case 8: case 9: case 10: case 11:
		p.floppyslots[0].dfxtype = DRV_NONE;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = true;
		p.cdslots[0].type = SCSI_UNIT_DEFAULT;
		p.gfx_monitor[0].gfx_size.width = 720;
		p.gfx_monitor[0].gfx_size.height = 568;
		p.jports[1].mode = 7;
		break;
	default:
		break;
	}
}

// Values the fixups would change are not offered, setters make the changes
// the fixups would, and the mirrors of fixup tables agree with the real ones.
void check_machine_fixup_rules()
{
	uae_prefs& p = test_prefs;
	static uae_prefs q; // uae_prefs is too large for the stack

	// Slow RAM 1.8 MB is capped for the Gayle/Gary conflict after the
	// chipset extra's table ran; the layer's mirror must agree with the real
	// table for every extra, CPU class and IDE controller.
	for (int cs = 0; cs <= CP_DRACO; cs++) {
		for (const int cpu : {68000, 68020}) {
			for (const int ide : {0, IDE_A600A1200}) {
				reset_prefs(p);
				p.cs_compatible = cs;
				p.cpu_model = cpu;
				p.cs_ide = ide;
				p.cs_fatgaryrev = p.cs_ramseyrev = -1;
				std::memcpy(static_cast<void*>(&q), static_cast<const void*>(&p), sizeof q);
				built_in_chipset_prefs(&q);
				q.cs_ide = ide; // fixup_prefs keeps the configured controller
				const bool capped = q.cs_ide || q.cs_fatgaryrev >= 0 || q.cs_ramseyrev >= 0;
				if (choice_available(p, "bogomem_size", "7") == capped) {
					std::fprintf(stderr, "bogomem_size 7: chipset %d cpu %d ide %d disagrees with the table\n",
						cs, cpu, ide);
					failures++;
				}
			}
		}
	}
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_ide = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(choice_available(p, "bogomem_size", "7"));
	p.cs_ramseyrev = 0x0f;
	CHECK(!choice_available(p, "bogomem_size", "7"));

	// The JIT cannot map 1.5 MB chip or slow RAM.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		p.cpu_model = 68020;
		p.cachesize = 8192;
		CHECK(!choice_available(p, "chipmem_size", "3"));
		CHECK(!choice_available(p, "bogomem_size", "6"));
		CHECK(choice_available(p, "chipmem_size", "4"));
		p.cachesize = 0;
		CHECK(choice_available(p, "chipmem_size", "3"));
		CHECK(choice_available(p, "bogomem_size", "6"));
	}

	// Chip RAM above 2 MB and a Zorro II RTG board exclude each other.
	reset_prefs(p);
	p.rtgboards[0].rtgmem_size = 0x400000;
	p.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z2;
	CHECK(!choice_available(p, "chipmem_size", "8"));
	CHECK(choice_available(p, "chipmem_size", "4"));
	p.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
	CHECK(choice_available(p, "chipmem_size", "8"));

	// An accelerator whose RAM is the Z2 board follows the chip RAM shrink,
	// so the fixup's copy back does not undo it.
	reset_prefs(p);
	p.cpuboard_type = 1;
	board_memory_type = BOARD_MEMORY_Z2;
	p.fastmem[0].size = p.cpuboardmem1.size = 0x400000;
	CHECK(set(p, "chipmem_size", "8") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fastmem[0].size <= 0x40000 && p.cpuboardmem1.size == p.fastmem[0].size);
	board_memory_type = BOARD_MEMORY_HIGHMEM;

	// Z2 boards below 1 MB must be a power of two of at least 64 KB.
	reset_prefs(p);
	CHECK(set(p, "fastmem_size_k", "192") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "fastmem2_size_k", "896") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "fastmem3_size_k", "128") == AMIBERRY_SETTINGS_OK && p.fastmem[2].size == 0x20000);
	CHECK(set(p, "fastmem_size_k", "256") == AMIBERRY_SETTINGS_OK && p.fastmem[0].size == 0x40000);

	// Wait for Blitter goes with 68000/010 cycle-exact modes, from either
	// side.
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	CHECK(set(p, "waiting_blits", "automatic") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cycle_exact", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "waiting_blits") == "disabled");
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cycle_exact", "memory") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "waiting_blits", "automatic") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "waiting_blits") == "disabled");

	// The A4000 IDE controller keeps Gary and Ramsey on a custom chipset.
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(set(p, "ide", "a4000") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cs_fatgaryrev == 0 && p.cs_ramseyrev == 0x0f);
	CHECK(set(p, "ramsey", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "fatgary", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "ramsey", "13") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "ide", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "ramsey", "-1") == AMIBERRY_SETTINGS_OK);

	// Genlock cannot be on with A2024 or Graffiti emulation.
	reset_prefs(p);
	p.monitoremu = 1;
	CHECK(!choice_available(p, "genlock", "true"));
	CHECK(set(p, "genlock", "true") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "genlock", "false") == AMIBERRY_SETTINGS_OK);

	// The cartridge ident follows the cartridge file.
	reset_prefs(p);
	CHECK(set(p, "cart_file", "/roms/ar3.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.cartident, ":AR3") == 0);
	CHECK(set(p, "cart_file", "/roms/unknown.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cartident[0] == 0);
	// Writing back the current path keeps an ident typed for a custom dump.
	std::snprintf(p.cartident, sizeof p.cartident, ":DeMoNv1");
	CHECK(set(p, "cart_file", "/roms/unknown.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.cartident, ":DeMoNv1") == 0);

	// Options that make a RAM size impossible change it the way the fixup
	// would: the JIT turns 1.5 MB chip into 2 MB and 1.5 MB slow into 1 MB,
	// and a chipset extra with Gayle/Gary caps 1.8 MB slow RAM at 1.5 MB.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		CHECK(set(p, "cpu_24bit_addressing", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "chipmem_size", "3") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "bogomem_size", "6") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(value_of(p, "chipmem_size") == "4");
		CHECK(value_of(p, "bogomem_size") == "4");
	}
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_ide = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(set(p, "bogomem_size", "7") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "chipset_compatible", "A1200") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "bogomem_size") == "6");
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_ide = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(set(p, "bogomem_size", "7") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "ramsey", "15") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "bogomem_size") == "6");


	// The UAE board reports and stores the no-diagnostics spellings.
	reset_prefs(p);
	p.uaeboard = 2;
	p.uaeboard_nodiag = true;
	CHECK(value_of(p, "uaeboard") == "full_off");
	CHECK(set(p, "uaeboard", "min") == AMIBERRY_SETTINGS_OK && p.uaeboard == 1 && !p.uaeboard_nodiag);
	CHECK(set(p, "uaeboard", "full+indirect_off") == AMIBERRY_SETTINGS_OK
		&& p.uaeboard == 3 && p.uaeboard_nodiag);

	// MapROM takes one hexadecimal address.
	reset_prefs(p);
	CHECK(set(p, "maprom", "zz") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "maprom", "0x") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "maprom", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "maprom", "e00000") == AMIBERRY_SETTINGS_OK && p.maprom == 0xe00000);
	CHECK(set(p, "maprom", "0x00f00000") == AMIBERRY_SETTINGS_OK && p.maprom == 0xf00000);
}

void check_presets()
{
	// Preset metadata: the panel's model list, configurations and
	// compatibility tiers.
	CHECK(amiberry_settings_preset_count() == 13);
	CHECK(amiberry_settings_preset_at(13) == nullptr);
	CHECK(amiberry_settings_preset_find("a1200") == amiberry_settings_preset_at(4));
	CHECK(amiberry_settings_preset_find("nope") == nullptr);
	const amiberry_settings_preset* a1200 = amiberry_settings_preset_find("a1200");
	CHECK(a1200 && a1200->compatibility_max == 4);
	CHECK(amiberry_settings_preset_find("a500")->compatibility_max == 3);
	CHECK(amiberry_settings_preset_config_label(a1200, -1) == nullptr);
	CHECK(amiberry_settings_preset_config_label(a1200, 1)
		&& std::strstr(amiberry_settings_preset_config_label(a1200, 1), "4 MB Fast"));
	CHECK(amiberry_settings_preset_compatibility_label(4)
		&& std::strstr(amiberry_settings_preset_compatibility_label(4), "JIT"));
	CHECK(amiberry_settings_preset_compatibility_label(5) == nullptr);
	CHECK(settings::preset_model_index("cd32") == 8);
	CHECK(std::strcmp(settings::preset_id(8), "cd32") == 0);

	// Every preset applied through the layer equals the frozen Quickstart
	// reference, byte for byte, for every configuration and tier.
	static uae_prefs reference; // uae_prefs is too large for the stack
	for (size_t i = 0; i < amiberry_settings_preset_count(); i++) {
		const amiberry_settings_preset* preset = amiberry_settings_preset_at(i);
		for (int conf = 0; conf < preset->config_count; conf++) {
			for (int compa = 0; compa <= preset->compatibility_max; compa++) {
				reset_prefs(test_prefs);
				reset_prefs(reference);
				settings::apply_preset(test_prefs, settings::preset_model_index(preset->id), conf, compa, false);
				reference_constraints(reference, settings::preset_model_index(preset->id), conf, compa);
				if (std::memcmp(&test_prefs, &reference, sizeof(uae_prefs)) != 0) {
					std::fprintf(stderr, "preset %s config %d compa %d differs from the Quickstart reference\n",
						preset->id, conf, compa);
					failures++;
				}
			}
		}
		// Every model has a configuration (ALG and Arcadia apply the first
		// game ROM, as Quickstart did) and the engine saw the model.
		CHECK(preset->config_count > 0);
		CHECK(stub_preset_model == settings::preset_model_index(preset->id));
	}
	CHECK(amiberry_settings_preset_find("alg")->config_count == 1);
	CHECK(amiberry_settings_preset_find("arcadia")->config_count == 1);
	// Out-of-range compatibility is clamped, not rejected, at this level.
	reset_prefs(test_prefs);
	settings::apply_preset(test_prefs, 4, 0, 99, false);
	CHECK(stub_preset_compa == 4);
	// Live application (Quickstart, --apply-preset) runs the engine as a
	// configuration load does; preview never does.
	const int preview_calls = stub_preview_calls;
	settings::apply_preset(test_prefs, 4, 0, 1, false);
	CHECK(stub_preview_calls == preview_calls);
	settings::apply_preset(test_prefs, 4, 0, 1, true);
	CHECK(stub_preview_calls == preview_calls + 1);

	// ROM availability cannot be tested against a real ROM scan here; it
	// must reject unknown arguments, use the engine's preview (which
	// leaves global state alone) and never touch the config.
	reset_prefs(changed_prefs);
	CHECK(!amiberry_settings_preset_rom_installed("nope", 0));
	CHECK(!amiberry_settings_preset_rom_installed("a500", 99));
	CHECK(amiberry_settings_preset_rom_installed("arcadia", 0));
	CHECK(stub_preview_calls == preview_calls + 2);
	CHECK(changed_prefs.cpu_model == 68000 && changed_prefs.kbd_led_num == 0);
}

// AE for the machine domains: sessions commit preset-written fields and
// report the strongest apply class.
void check_machine_session()
{
	reset_prefs(changed_prefs);
	changed_prefs.cpuboard_type = 1;

	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(session);
	// A preset through the C API: everything it writes must survive commit,
	// including the fresh-configuration defaults outside every option.
	const int preview_calls = stub_preview_calls;
	CHECK(amiberry_settings_preset_apply(session, "a1200", 1, 1) == AMIBERRY_SETTINGS_OK);
	CHECK(stub_preview_calls > preview_calls); // never the live engine
	CHECK(amiberry_settings_preset_apply(session, "nope", 0, 0) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(amiberry_settings_preset_apply(session, "a1200", 99, 0) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_preset_apply(session, "a1200", 0, 5) == AMIBERRY_SETTINGS_UNAVAILABLE);
	bool changed = false;
	// The stub engine changes no option with a descriptor; a preset still
	// commits as a machine change.
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed);

	// The Quickstart constraints (floppy/CD/joystick) and the target
	// defaults are committed too.
	CHECK(changed_prefs.floppyslots[1].dfxtype == DRV_NONE);
	CHECK(!changed_prefs.cdslots[0].inuse);
	CHECK(changed_prefs.jports[1].mode == 0);
	CHECK(changed_prefs.kbd_led_num == -1);

	// The preset is replayed on the pending configuration: changes made
	// before it survive where it does not write, changes made after it
	// win, and changes made elsewhere meanwhile are kept.
	reset_prefs(changed_prefs);
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", "/roms/before.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_preset_apply(session, "a500", 0, 1) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "chipset", "aga") == AMIBERRY_SETTINGS_OK);
	changed_prefs.kbd_led_scr = 3; // another front end, meanwhile
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET && changed);
	CHECK(std::strcmp(changed_prefs.romfile, "/roms/before.rom") == 0);
	CHECK(value_of(changed_prefs, "chipset") == "aga");
	CHECK(changed_prefs.kbd_led_num == -1 && changed_prefs.kbd_led_scr == 3);
	CHECK(changed_prefs.floppyslots[0].dfxtype == DRV_35_DD); // A500 constraint

	// A machine option through a session: strongest class wins.
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", "/roms/kick31.rom")
		== AMIBERRY_SETTINGS_OK);
	char path[settings::max_value_length] = {};
	CHECK(amiberry_settings_get(session, "kickstart_rom_file", path, sizeof path)
		== AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(path, "/roms/kick31.rom") == 0);
	char tiny[4];
	CHECK(amiberry_settings_get(session, "kickstart_rom_file", tiny, sizeof tiny)
		== AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed && std::strcmp(changed_prefs.romfile, "/roms/kick31.rom") == 0);

	// Two paths that share a 63-character prefix still differ.
	char long_a[settings::max_value_length], long_b[settings::max_value_length];
	std::memset(long_a, 'a', sizeof long_a - 1);
	std::memset(long_b, 'a', sizeof long_b - 1);
	long_a[sizeof long_a - 1] = long_b[sizeof long_b - 1] = 0;
	long_b[100] = 'b';
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", long_a) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", long_b) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed_prefs.romfile[100] == 'b');
	amiberry_settings_session_close(session);
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
	check_fixup_consistency();
	check_machine_availability();
	check_machine_dependents();
	check_machine_rejections();
	check_machine_fixup_rules();
	check_presets();
	check_machine_session();
	if (failures) {
		std::fprintf(stderr, "settings layer: %d check(s) failed\n", failures);
		return 1;
	}
	std::printf("settings layer checks passed\n");
	return 0;
}
