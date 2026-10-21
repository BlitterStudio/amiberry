// CPU, FPU, MMU, JIT, PPC and x86 bridgeboard CPU options.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "cpuboard.h"
#include "x86.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif

// newcpu.h only defines the JIT cache bounds in JIT builds; the option
// exists in every build (always unavailable without the JIT) so
// configurations keep round-tripping.
#ifdef MAX_JIT_CACHE
static_assert(MAX_JIT_CACHE == 16384, "update max_jit_cache");
#endif
constexpr int max_jit_cache = 16384;

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>

namespace amiberry_settings {

namespace {

void write_bool(char* out, size_t size, bool v)
{
	std::snprintf(out, size, "%s", v ? "true" : "false");
}

void write_int(char* out, size_t size, int v)
{
	std::snprintf(out, size, "%d", v);
}

// cfgfile writes the throttle options with one decimal.
void write_real(char* out, size_t size, float v)
{
	std::snprintf(out, size, "%.1f", v);
}

// Index of text in choices, or -1.
int choice_index(const amiberry_setting_choice* choices, size_t count, std::string_view text)
{
	for (size_t i = 0; i < count; i++) {
		if (text == choices[i].value)
			return static_cast<int>(i);
	}
	return -1;
}

void write_choice(char* out, size_t size, const amiberry_setting_choice* choices, size_t count, int index)
{
	std::snprintf(out, size, "%s", index >= 0 && static_cast<size_t>(index) < count ? choices[index].value : "");
}

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

// The JIT needs a 32-bit 68020 or later without MMU emulation.
bool jit_possible(const uae_prefs& p)
{
#ifdef JIT
	return p.cpu_model >= 68020 && !p.address_space_24 && !p.mmu_model;
#else
	(void)p;
	return false;
#endif
}

bool jit_enabled(const uae_prefs& p)
{
	return jit_possible(p) && p.cachesize;
}

bool jit_enabled_option(const uae_prefs& p, const context&)
{
	return jit_enabled(p);
}

bool has_fpu(const uae_prefs& p, const context&)
{
	return p.fpu_model > 0;
}

// The JIT's preference check is the only code that copies fpu_strict into a
// running configuration.
#ifdef JIT
constexpr amiberry_apply_class fpu_strict_apply = AMIBERRY_APPLY_LIVE;
#else
constexpr amiberry_apply_class fpu_strict_apply = AMIBERRY_APPLY_RESTART;
#endif

// ---------------------------------------------------------------- CPU

const amiberry_setting_choice cpu_model_choices[] = {
	{"68000", "68000 (A500/A1000/A2000)"},
	{"68010", "68010"},
	{"68020", "68020 (A1200)"},
	{"68030", "68030 (A3000/A4000)"},
	{"68040", "68040"},
	{"68060", "68060"},
};

void get_cpu_model(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cpu_model);
}

// Switching the CPU resets the MMU and data cache, keeps an FPU the new CPU
// can use, and picks the matching addressing mode and cycle-exact speed. A
// CPU below the 68040 has no internal FPU, so one carried over from a
// 68040/060 becomes a 68881.
void set_cpu_model(uae_prefs& p, const value& v)
{
	const int cpu = static_cast<int>(v.number);
	if (cpu == p.cpu_model)
		return;
	p.cpu_model = cpu;
	p.mmu_model = 0;
	p.mmu_ec = false;
	p.cpu_data_cache = false;

	switch (cpu) {
	case 68000:
	case 68010:
		p.address_space_24 = true;
		if (p.cpu_compatible || p.cpu_memory_cycle_exact)
			p.fpu_model = 0;
		else if (p.fpu_model && p.fpu_model != 68882)
			p.fpu_model = 68881;
		break;
	case 68020:
	case 68030:
		if (cpu == 68030)
			p.address_space_24 = false;
		if (p.fpu_model && p.fpu_model != 68882)
			p.fpu_model = 68881;
		break;
	case 68040:
	case 68060:
		p.address_space_24 = false;
		if (p.fpu_model)
			p.fpu_model = cpu;
		break;
	default:
		break;
	}

	if (p.cpu_compatible) {
		if (cpu <= 68010)
			p.cpu_clock_multiplier = 2 * 256;
		else if (cpu == 68020)
			p.cpu_clock_multiplier = 4 * 256;
		else
			p.cpu_clock_multiplier = 8 * 256;
	}
}

void get_address_space_24(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.address_space_24);
}

void set_address_space_24(uae_prefs& p, const value& v)
{
	p.address_space_24 = v.number != 0;
}

bool address_space_24_available(const uae_prefs& p, const context&)
{
	return p.cpu_model <= 68030 && p.cachesize == 0;
}

void get_cpu_compatible(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.cpu_compatible);
}

void set_cpu_compatible(uae_prefs& p, const value& v)
{
	p.cpu_compatible = p.cpu_memory_cycle_exact || v.number != 0;
	if (p.cpu_compatible && p.cpu_model <= 68010)
		p.fpu_model = 0;
}

bool cpu_compatible_available(const uae_prefs& p, const context&)
{
	return !p.cpu_memory_cycle_exact && !(p.cachesize && p.cpu_model >= 68040);
}

void get_cpu_no_unimplemented(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.int_no_unimplemented);
}

void set_cpu_no_unimplemented(uae_prefs& p, const value& v)
{
	p.int_no_unimplemented = v.number != 0;
}

bool cpu_no_unimplemented_available(const uae_prefs& p, const context&)
{
	return p.cpu_model == 68060 && p.cachesize == 0;
}

void get_cpu_data_cache(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.cpu_data_cache);
}

void set_cpu_data_cache(uae_prefs& p, const value& v)
{
	p.cpu_data_cache = v.number != 0;
}

bool cpu_data_cache_available(const uae_prefs& p, const context&)
{
	return p.cpu_model >= 68030 && p.cachesize == 0 && p.cpu_compatible;
}

// ---------------------------------------------------------------- MMU

const amiberry_setting_choice mmu_model_choices[] = {
	{"0", "None"},
	{"68030", "68030 MMU"},
	{"68040", "68040 MMU"},
	{"68060", "68060 MMU"},
	{"68ec030", "68EC030 (no MMU)"},
	{"68ec040", "68EC040 (no MMU)"},
	{"68ec060", "68EC060 (no MMU)"},
};

// The CPU model a non-"0" MMU choice belongs to.
int mmu_choice_cpu(std::string_view choice)
{
	if (choice.size() == 7 && choice.substr(0, 4) == "68ec")
		return 68000 + (choice[5] - '0') * 10;
	return choice.size() == 5 ? 68000 + (choice[3] - '0') * 10 : 0;
}

void get_mmu_model(const uae_prefs& p, char* out, size_t size)
{
	if (!p.mmu_model)
		std::snprintf(out, size, "0");
	else if (p.mmu_ec)
		std::snprintf(out, size, "68ec0%d", p.mmu_model % 100);
	else
		write_int(out, size, p.mmu_model);
}

void set_mmu_model(uae_prefs& p, const value& v)
{
	p.mmu_model = v.text == "0" ? 0 : mmu_choice_cpu(v.text);
	p.mmu_ec = v.text.size() == 7;
}

bool mmu_model_available(const uae_prefs& p, const context&)
{
	return p.cpu_model >= 68030 && p.cachesize == 0;
}

bool mmu_model_choice_available(const uae_prefs& p, std::string_view choice)
{
	return choice == "0" || mmu_choice_cpu(choice) == p.cpu_model;
}

// ---------------------------------------------------------------- FPU

const amiberry_setting_choice fpu_model_choices[] = {
	{"0", "None"},
	{"68881", "68881"},
	{"68882", "68882"},
	{"68040", "CPU internal (68040)"},
	{"68060", "CPU internal (68060)"},
};

void get_fpu_model(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.fpu_model);
}

void set_fpu_model(uae_prefs& p, const value& v)
{
	p.fpu_model = static_cast<int>(v.number);
}

bool fpu_model_choice_available(const uae_prefs& p, std::string_view choice)
{
	if (choice == "0")
		return true;
	if (choice == "68881" || choice == "68882")
		return p.cpu_model < 68040 && (p.cpu_model >= 68020 || !p.cpu_compatible);
	return mmu_choice_cpu(choice) == p.cpu_model;
}

void get_fpu_strict(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.fpu_strict);
}

void set_fpu_strict(uae_prefs& p, const value& v)
{
	p.fpu_strict = v.number != 0;
}

void get_fpu_softfloat(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.fpu_mode > 0);
}

void set_fpu_softfloat(uae_prefs& p, const value& v)
{
	p.fpu_mode = v.number != 0 ? 1 : 0;
}

// The JIT FPU emulates the FPU with host floating point, so softfloat is
// unavailable while it is on (fixup_cpu() would switch softfloat off).
bool fpu_softfloat_available(const uae_prefs& p, const context&)
{
	return p.fpu_model > 0 && !(p.cachesize && p.compfpu);
}

void get_fpu_no_unimplemented(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.fpu_no_unimplemented);
}

void set_fpu_no_unimplemented(uae_prefs& p, const value& v)
{
	p.fpu_no_unimplemented = v.number != 0;
}

bool fpu_no_unimplemented_available(const uae_prefs& p, const context&)
{
	return p.fpu_model > 0 && p.cachesize == 0;
}

// ---------------------------------------------------------------- CPU speed

const amiberry_setting_choice cpu_speed_choices[] = {
	{"max", "Fastest possible"},
	{"real", "Approximate A500/A1200 or cycle-exact"},
};

void get_cpu_speed(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.m68k_speed < 0 ? "max" : "real");
}

void set_cpu_speed(uae_prefs& p, const value& v)
{
	if (v.text == "max") {
		p.m68k_speed = -1;
		p.m68k_speed_throttle = 0;
	} else {
		p.m68k_speed = 0;
	}
}

void get_cpu_throttle(const uae_prefs& p, char* out, size_t size)
{
	write_real(out, size, p.m68k_speed_throttle);
}

void set_cpu_throttle(uae_prefs& p, const value& v)
{
	p.m68k_speed_throttle = static_cast<float>(v.number);
}

bool cpu_throttle_available(const uae_prefs& p, const context&)
{
	return !p.cpu_cycle_exact;
}

// Only slowing down is possible at fastest speed and in memory cycle-exact
// mode without full cycle-exact.
void cpu_throttle_range(const uae_prefs& p, double&, double& max)
{
	if (p.m68k_speed < 0 || (p.cpu_memory_cycle_exact && !p.cpu_cycle_exact))
		max = 0;
}

void get_cpu_idle(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cpu_idle);
}

void set_cpu_idle(uae_prefs& p, const value& v)
{
	p.cpu_idle = static_cast<int>(v.number);
}

bool cpu_idle_available(const uae_prefs& p, const context&)
{
	return p.m68k_speed != 0;
}

const amiberry_setting_choice cpu_multiplier_choices[] = {
	{"0", "Default for the CPU"},
	{"1", "1x"},
	{"2", "2x (A500)"},
	{"4", "4x (A1200)"},
	{"8", "8x"},
	{"16", "16x"},
};

void get_cpu_multiplier(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cpu_clock_multiplier / 256);
}

void set_cpu_multiplier(uae_prefs& p, const value& v)
{
	p.cpu_frequency = 0;
	p.cpu_clock_multiplier = static_cast<int>(v.number) * 256;
}

bool cpu_multiplier_available(const uae_prefs& p, const context&)
{
	return (p.cpu_cycle_exact || p.cpu_compatible) && p.m68k_speed >= 0;
}

void get_cpu_threaded(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.cpu_thread);
}

void set_cpu_threaded(uae_prefs& p, const value& v)
{
	p.cpu_thread = v.number != 0;
}

// Only the JIT and the plain 68020+ interpreter can run on the CPU thread,
// and the thread is chosen when emulation starts.
bool cpu_threaded_available(const uae_prefs& p, const context& ctx)
{
	const bool no_thread = p.cpu_compatible || p.ppc_mode || p.cpu_memory_cycle_exact
		|| p.cpu_cycle_exact || (p.mmu_model && !p.cachesize) || p.cpu_model < 68020;
	return !no_thread && !ctx.emulating;
}

// ---------------------------------------------------------------- PPC

#ifdef WITH_PPC
const amiberry_setting_choice ppc_model_choices[] = {
	{"", "Disabled"},
	{"automatic", "Automatic"},
};

void get_ppc_model(const uae_prefs& p, char* out, size_t size)
{
	if (p.ppc_mode == 1)
		std::snprintf(out, size, "automatic");
	else if (p.ppc_mode == 2)
		std::snprintf(out, size, "%s", p.ppc_model[0] ? p.ppc_model : "manual");
	else
		std::snprintf(out, size, "%s", "");
}

void set_ppc_model(uae_prefs& p, const value& v)
{
	p.ppc_model[0] = 0;
	p.ppc_mode = v.text == "automatic" ? 1 : 0;
}

bool ppc_model_available(const uae_prefs& p, const context&)
{
	return p.cpu_model >= 68040
		&& (p.ppc_mode == 1 || (p.ppc_mode == 0 && !is_ppc_cpu(const_cast<uae_prefs*>(&p))));
}

const amiberry_setting_choice ppc_implementation_choices[] = {
	{"auto", "Automatic"},
	{"dummy", "Dummy"},
	{"pearpc", "PearPC"},
	{"qemu", "QEMU"},
};

void get_ppc_implementation(const uae_prefs& p, char* out, size_t size)
{
	int index = p.ppc_implementation;
	if (index < PPC_IMPLEMENTATION_AUTO || index > PPC_IMPLEMENTATION_QEMU)
		index = PPC_IMPLEMENTATION_AUTO;
	write_choice(out, size, CHOICES(ppc_implementation_choices), index);
}

void set_ppc_implementation(uae_prefs& p, const value& v)
{
	p.ppc_implementation = choice_index(CHOICES(ppc_implementation_choices), v.text);
}

bool not_emulating(const uae_prefs&, const context& ctx)
{
	return !ctx.emulating;
}

const amiberry_setting_choice ppc_cpu_idle_choices[] = {
	{"disabled", "Disabled"},
	{"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"},
	{"6", "6"}, {"7", "7"}, {"8", "8"}, {"9", "9"},
	{"max", "Maximum"},
};

void get_ppc_cpu_idle(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(ppc_cpu_idle_choices), p.ppc_cpu_idle);
}

void set_ppc_cpu_idle(uae_prefs& p, const value& v)
{
	p.ppc_cpu_idle = choice_index(CHOICES(ppc_cpu_idle_choices), v.text);
}

bool ppc_cpu_idle_available(const uae_prefs& p, const context&)
{
	return p.ppc_mode != 0;
}
#endif

// ---------------------------------------------------------------- x86

void get_x86_throttle(const uae_prefs& p, char* out, size_t size)
{
	write_real(out, size, p.x86_speed_throttle);
}

void set_x86_throttle(uae_prefs& p, const value& v)
{
	p.x86_speed_throttle = p.m68k_speed < 0 ? 0.0f : static_cast<float>(v.number);
}

bool x86_throttle_available(const uae_prefs& p, const context&)
{
#ifdef USE_PCEM
	return is_x86_cpu(const_cast<uae_prefs*>(&p)) != 0;
#else
	(void)p;
	return false;
#endif
}

// ---------------------------------------------------------------- JIT

void get_cachesize(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cachesize);
}

// Turning the JIT on leaves cycle-exact modes and turns the JIT FPU on
// unless softfloat is selected. A 68040/060 JIT cannot run in More
// compatible mode, but the option keeps its value while greyed out so that
// turning the JIT off again restores it; the core fixup clears it when the
// configuration is applied, and validation reports that.
void set_cachesize(uae_prefs& p, const value& v)
{
	const bool enabling = p.cachesize == 0 && v.number > 0;
	p.cachesize = static_cast<int>(v.number);
	if (enabling) {
		p.cpu_cycle_exact = false;
		p.blitter_cycle_exact = false;
		p.cpu_memory_cycle_exact = false;
		p.compfpu = p.fpu_mode <= 0;
	}
}

bool cachesize_available(const uae_prefs& p, const context&)
{
	return jit_possible(p);
}

void get_compfpu(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.compfpu);
}

// Turning the JIT FPU on replaces softfloat, as the core fixup would.
void set_compfpu(uae_prefs& p, const value& v)
{
	p.compfpu = v.number != 0;
	if (p.compfpu && p.cachesize)
		p.fpu_mode = 0;
}

// cfgfile only reads and writes compfpu in JIT FPU builds; the JIT is
// unavailable in the others, so the option is too.
bool compfpu_available(const uae_prefs& p, const context&)
{
#ifdef USE_JIT_FPU
	return jit_enabled(p) && p.fpu_model > 0;
#else
	(void)p;
	return false;
#endif
}

#ifdef USE_JIT_FPU
const char* const compfpu_default = "true";
#else
const char* const compfpu_default = "false";
#endif

void get_comp_constjump(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.comp_constjump);
}

void set_comp_constjump(uae_prefs& p, const value& v)
{
	p.comp_constjump = v.number != 0;
}

const amiberry_setting_choice comp_flushmode_choices[] = {
	{"soft", "Soft"},
	{"hard", "Hard"},
};

void get_comp_flushmode(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.comp_hardflush ? "hard" : "soft");
}

void set_comp_flushmode(uae_prefs& p, const value& v)
{
	p.comp_hardflush = v.text == "hard";
}

void get_comp_nf(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.compnf);
}

void set_comp_nf(uae_prefs& p, const value& v)
{
	p.compnf = v.number != 0;
}

void get_comp_catchfault(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.comp_catchfault);
}

void set_comp_catchfault(uae_prefs& p, const value& v)
{
	p.comp_catchfault = v.number != 0;
}

const amiberry_setting_choice comp_trust_choices[] = {
	{"direct", "Direct"},
	{"indirect", "Indirect"},
	{"indirectKS", "Indirect for Kickstart"},
	{"afterPic", "Indirect after Picasso96"},
};

void get_comp_trustbyte(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(comp_trust_choices), p.comptrustbyte);
}

// The JIT uses one memory access mode for bytes, words, longs and
// non-address accesses.
void set_comp_trustbyte(uae_prefs& p, const value& v)
{
	const int mode = choice_index(CHOICES(comp_trust_choices), v.text);
	p.comptrustbyte = p.comptrustword = p.comptrustlong = p.comptrustnaddr = mode;
}

// ---------------------------------------------------------------- table

constexpr double no_range = 0;

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, CHOICES(choices), constraint}
#define NUMBER_OPTION(id, group, label, help, type, apply, min, max, step, def, constraint) \
	{id, group, label, help, type, apply, min, max, step, def, nullptr, 0, constraint}

const char* const jit_constraint = "Needs the JIT, which needs a 68020 or later with 32-bit addressing and no MMU.";

const definition definitions[] = {
	{CHOICE_OPTION("cpu_model", "cpu", "CPU model",
			"CPU model. 68020+ are 32-bit; 68040/68060 include FPU and MMU. Higher models are faster.",
			AMIBERRY_APPLY_LIVE, "68000", cpu_model_choices, nullptr),
		get_cpu_model, set_cpu_model, nullptr, nullptr, nullptr},
	// The core copies it into the running configuration only together
	// with a CPU model or mode change (prefs_changed_cpu()).
	{BOOL_OPTION("cpu_24bit_addressing", "cpu", "24-bit addressing",
			"Essential for A500/A1200 compatibility. Disable for Z3 RAM usage.",
			AMIBERRY_APPLY_RESTART, "true",
			"24-bit addressing is not available on a 68040 or 68060, with the JIT, or with a 32-bit accelerator board."),
		get_address_space_24, set_address_space_24, address_space_24_available, nullptr, nullptr},
	{BOOL_OPTION("cpu_compatible", "cpu", "More compatible",
			"Approximates prefetch pipeline of 68000/010. Required for some timing sensitive games.",
			AMIBERRY_APPLY_LIVE, "true",
			"More compatible is always on in cycle-exact modes and always off for a 68040 or 68060 with the JIT."),
		get_cpu_compatible, set_cpu_compatible, cpu_compatible_available, nullptr, nullptr},
	{BOOL_OPTION("cpu_no_unimplemented", "cpu", "No unimplemented CPU emulation",
			"Do not emulate the 68060 instructions the CPU traps as unimplemented.",
			AMIBERRY_APPLY_LIVE, "false", "Unimplemented instruction emulation is always on with the JIT."),
		get_cpu_no_unimplemented, set_cpu_no_unimplemented, cpu_no_unimplemented_available, nullptr, nullptr},
	{BOOL_OPTION("cpu_data_cache", "cpu", "Data cache emulation",
			"Emulate the CPU data cache. Required for some 68030+ software.",
			AMIBERRY_APPLY_LIVE, "false",
			"Data cache emulation needs a 68030 or later in More compatible mode, without the JIT, and the indirect UAE Boot ROM."),
		get_cpu_data_cache, set_cpu_data_cache, cpu_data_cache_available, nullptr, nullptr},
	{CHOICE_OPTION("mmu_model", "mmu", "MMU",
			"Full MMU emulation is required for some operating systems; EC models have no MMU.",
			AMIBERRY_APPLY_LIVE, "0", mmu_model_choices,
			"MMU emulation needs a 68030 or later and is not available with the JIT."),
		get_mmu_model, set_mmu_model, mmu_model_available, mmu_model_choice_available, nullptr},
	{CHOICE_OPTION("fpu_model", "fpu", "FPU",
			"Floating point unit. A 68040 or 68060 uses its internal FPU.",
			AMIBERRY_APPLY_LIVE, "0", fpu_model_choices,
			"A 68000 or 68010 in More compatible or cycle-exact mode has no FPU, and a 68040 or 68060 uses its internal FPU."),
		get_fpu_model, set_fpu_model, nullptr, fpu_model_choice_available, nullptr},
	{BOOL_OPTION("fpu_strict", "fpu", "More compatible FPU",
			"More accurate but slower FPU emulation.",
			fpu_strict_apply, "false", nullptr),
		get_fpu_strict, set_fpu_strict, has_fpu, nullptr, nullptr},
	{BOOL_OPTION("fpu_softfloat", "fpu", "Softfloat FPU (80-bit)",
			"FPU precision mode. Softfloat is most accurate but slowest.",
			AMIBERRY_APPLY_LIVE, "false", "Softfloat is not available when the JIT emulates the FPU."),
		get_fpu_softfloat, set_fpu_softfloat, fpu_softfloat_available, nullptr, nullptr},
	{BOOL_OPTION("fpu_no_unimplemented", "fpu", "No unimplemented FPU emulation",
			"Do not emulate the FPU instructions the CPU traps as unimplemented.",
			AMIBERRY_APPLY_LIVE, "false", "Unimplemented instruction emulation is always on with the JIT."),
		get_fpu_no_unimplemented, set_fpu_no_unimplemented, fpu_no_unimplemented_available, nullptr, nullptr},
	{CHOICE_OPTION("cpu_speed", "cpu_speed", "CPU speed",
			"Fastest possible runs the CPU as fast as the host allows; the other mode matches original Amiga speed.",
			AMIBERRY_APPLY_LIVE, "real", cpu_speed_choices, nullptr),
		get_cpu_speed, set_cpu_speed, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("cpu_throttle", "cpu_speed", "CPU speed adjustment",
			"Speed adjustment in tenths of a percent.",
			AMIBERRY_SETTING_REAL, AMIBERRY_APPLY_LIVE, -900, 5000, 100, "0.0", nullptr),
		get_cpu_throttle, set_cpu_throttle, cpu_throttle_available, nullptr, cpu_throttle_range},
	{NUMBER_OPTION("amiberry.cpu_idle", "cpu_speed", "CPU idle",
			"How much the host CPU sleeps while the emulated CPU waits; 0 disables it.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 0, 165, 15, "0", nullptr),
		get_cpu_idle, set_cpu_idle, cpu_idle_available, nullptr, nullptr},
	{CHOICE_OPTION("cpu_multiplier", "cpu_speed", "CPU frequency",
			"CPU clock as a multiple of the chipset clock in More compatible and cycle-exact modes.",
			AMIBERRY_APPLY_LIVE, "0", cpu_multiplier_choices, nullptr),
		get_cpu_multiplier, set_cpu_multiplier, cpu_multiplier_available, nullptr, nullptr},
	{BOOL_OPTION("cpu_threaded", "cpu_speed", "Multi-threaded CPU",
			"Run CPU emulation on a separate thread. Experimental. May help CPU-heavy software, but hardware-register accesses require synchronization and can make it slower.",
			AMIBERRY_APPLY_RESTART, "false",
			"The CPU thread needs a 68020 or later and is not available with PPC emulation, More compatible, cycle-exact or MMU modes."),
		get_cpu_threaded, set_cpu_threaded, cpu_threaded_available, nullptr, nullptr},
#ifdef WITH_PPC
	{CHOICE_OPTION("ppc_model", "ppc", "PPC CPU emulation",
			"Enable PowerPC CPU emulation for CyberStorm PPC or Blizzard PPC boards.",
			AMIBERRY_APPLY_RESTART, "", ppc_model_choices, nullptr),
		get_ppc_model, set_ppc_model, ppc_model_available, nullptr, nullptr},
	{CHOICE_OPTION("ppc_implementation", "ppc", "PPC implementation",
			"Select the PPC backend. Automatic tries QEMU first when plugin support is compiled, then falls back to other available implementations.",
			AMIBERRY_APPLY_RESTART, "auto", ppc_implementation_choices, nullptr),
		get_ppc_implementation, set_ppc_implementation, not_emulating, nullptr, nullptr},
	{CHOICE_OPTION("ppc_cpu_idle", "ppc", "Stopped M68K CPU idle mode",
			"How much the M68K CPU sleeps while PPC is running. Higher values reduce host CPU usage.",
			AMIBERRY_APPLY_LIVE, "disabled", ppc_cpu_idle_choices, nullptr),
		get_ppc_cpu_idle, set_ppc_cpu_idle, ppc_cpu_idle_available, nullptr, nullptr},
#endif
	{NUMBER_OPTION("cpu_x86_throttle", "x86", "x86 CPU speed",
			"x86 CPU speed as a percentage of maximum. Only has an effect when not running at fastest possible speed.",
			AMIBERRY_SETTING_REAL, AMIBERRY_APPLY_LIVE, 0, 100000, 100, "0.0", nullptr),
		get_x86_throttle, set_x86_throttle, x86_throttle_available, nullptr, nullptr},
	{NUMBER_OPTION("cachesize", "jit", "JIT cache size",
			"Just-In-Time compilation cache in KB; 0 turns the JIT off. Greatly speeds up CPU emulation but reduces compatibility.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 0, max_jit_cache, 1, "0",
			"The JIT needs a 68020 or later, is not available in cycle-exact modes, and needs at least 128 KB."),
		get_cachesize, set_cachesize, cachesize_available, nullptr, nullptr},
	{BOOL_OPTION("compfpu", "jit", "JIT FPU support",
			"Enable JIT compilation for FPU instructions.",
			AMIBERRY_APPLY_LIVE, compfpu_default, nullptr),
		get_compfpu, set_compfpu, compfpu_available, nullptr, nullptr},
	{BOOL_OPTION("comp_constjump", "jit", "Constant jump",
			"JIT compilation optimization.",
			AMIBERRY_APPLY_LIVE, "true", jit_constraint),
		get_comp_constjump, set_comp_constjump, jit_enabled_option, nullptr, nullptr},
	{CHOICE_OPTION("comp_flushmode", "jit", "Cache flush mode",
			"Hard flushes the JIT cache on every resize/reset.",
			AMIBERRY_APPLY_LIVE, "soft", comp_flushmode_choices, jit_constraint),
		get_comp_flushmode, set_comp_flushmode, jit_enabled_option, nullptr, nullptr},
	{BOOL_OPTION("comp_nf", "jit", "No flags",
			"Don't update status flags (faster but less compatible).",
			AMIBERRY_APPLY_LIVE, "true", jit_constraint),
		get_comp_nf, set_comp_nf, jit_enabled_option, nullptr, nullptr},
	{BOOL_OPTION("comp_catchfault", "jit", "Catch exceptions",
			"Catch memory access faults in compiled code.",
			AMIBERRY_APPLY_RESTART, "true", jit_constraint),
		get_comp_catchfault, set_comp_catchfault, jit_enabled_option, nullptr, nullptr},
	{CHOICE_OPTION("comp_trustbyte", "jit", "Memory access",
			"Direct memory access is faster; indirect is slower but more compatible.",
			AMIBERRY_APPLY_LIVE, "direct", comp_trust_choices, jit_constraint),
		get_comp_trustbyte, set_comp_trustbyte, jit_enabled_option, nullptr, nullptr},
};

bool commit_cpu(const uae_prefs& base, const uae_prefs& from, uae_prefs& to)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(cpu_model);
	FIELD(fpu_model);
	FIELD(mmu_model);
	FIELD(mmu_ec);
	FIELD(address_space_24);
	FIELD(cpu_compatible);
	FIELD(int_no_unimplemented);
	FIELD(fpu_no_unimplemented);
	FIELD(cpu_data_cache);
	FIELD(fpu_strict);
	FIELD(fpu_mode);
	FIELD(m68k_speed);
	FIELD(m68k_speed_throttle);
	FIELD(cpu_idle);
	FIELD(cpu_clock_multiplier);
	FIELD(cpu_frequency);
	FIELD(cpu_thread);
	FIELD(cpu_cycle_exact);
	FIELD(cpu_memory_cycle_exact);
	FIELD(blitter_cycle_exact);
	FIELD(ppc_mode);
	FIELD(ppc_model);
	FIELD(ppc_implementation);
	FIELD(ppc_cpu_idle);
	FIELD(x86_speed_throttle);
	FIELD(cachesize);
	FIELD(compfpu);
	FIELD(comp_constjump);
	FIELD(comp_hardflush);
	FIELD(compnf);
	FIELD(comp_catchfault);
	FIELD(comptrustbyte);
	FIELD(comptrustword);
	FIELD(comptrustlong);
	FIELD(comptrustnaddr);
#undef FIELD
	return wrote;
}

const domain cpu = {
	definitions, sizeof definitions / sizeof definitions[0], commit_cpu,
};

} // namespace

const domain& cpu_domain()
{
	return cpu;
}

} // namespace amiberry_settings
