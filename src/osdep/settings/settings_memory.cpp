// Memory options (the RAM panel): chip, slow, Z2 fast, Z3 fast, 32-bit
// chip and motherboard/processor slot RAM, plus the Z3 mapping mode.
// Values are the exact .uae spellings: chipmem_size and bogomem_size are
// stored in 512 KB / 256 KB units (with -1 and 0 as 128 KB and 256 KB for
// chip), everything else in MB, and boards below 1 MB use the *_size_k keys
// in KB.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "cpuboard.h"
#include "custom.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>

namespace amiberry_settings {

namespace {

void write_int(char* out, size_t size, int v)
{
	std::snprintf(out, size, "%d", v);
}

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

// WinUAE fix_values_memorydlg(): more than 2 MB chip RAM conflicts with the
// Z2 fast RAM window, so the boards shrink to a 256 KB total.
void clamp_fast_for_chip(uae_prefs& p)
{
	if (p.chipmem.size <= 0x200000)
		return;
	int total = 0;
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		if (p.fastmem[i].size > 262144)
			p.fastmem[i].size = 262144;
		total += p.fastmem[i].size;
		if (total > 262144)
			p.fastmem[i].size = 0;
	}
}

bool z3_possible(const uae_prefs& p, const context&)
{
	return !p.address_space_24;
}

// ---------------------------------------------------------------- chip RAM

const amiberry_setting_choice chipmem_size_choices[] = {
	{"-1", "128 KB"},
	{"0", "256 KB"},
	{"1", "512 KB"},
	{"2", "1M"},
	{"3", "1.5M"},
	{"4", "2M"},
	{"8", "4M"},
	{"16", "8M"},
};

void get_chipmem_size(const uae_prefs& p, char* out, size_t size)
{
	int v = static_cast<int>(p.chipmem.size / 0x80000);
	if (p.chipmem.size == 0x20000)
		v = -1;
	else if (p.chipmem.size == 0x40000)
		v = 0;
	write_int(out, size, v);
}

// More than 512 KB chip RAM needs an ECS Agnus (WinUAE values_to_chipsetdlg
// logic the RAM panel applied), and more than 2 MB pushes Z2 fast RAM out.
void set_chipmem_size(uae_prefs& p, const value& v)
{
	const int units = static_cast<int>(v.number);
	p.chipmem.size = units < 0 ? 0x20000 : units == 0 ? 0x40000
		: static_cast<uae_u32>(units) * 0x80000;
	if (p.chipmem.size > 0x80000 && !(p.chipset_mask & CSMASK_ECS_AGNUS))
		p.chipset_mask |= CSMASK_ECS_AGNUS;
	clamp_fast_for_chip(p);
}

// ---------------------------------------------------------------- slow RAM

const amiberry_setting_choice bogomem_size_choices[] = {
	{"0", "none"},
	{"2", "512 KB"},
	{"4", "1M"},
	{"6", "1.5M"},
	{"7", "1.8M"},
};

void get_bogomem_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, static_cast<int>(p.bogomem.size / 0x40000));
}

void set_bogomem_size(uae_prefs& p, const value& v)
{
	p.bogomem.size = static_cast<uae_u32>(v.number) * 0x40000;
}

// ------------------------------------------------------------- Z2 fast RAM

// Enabling Z2 fast RAM shrinks chip RAM to 2 MB (same conflict as above,
// from the other side). The board-sync keeps an accelerator board that
// mirrors fastmem[0] consistent, the way the panel did every frame.
void set_fastmem_size(uae_prefs& p, ramboard* board, uae_u32 size)
{
	board->size = size;
	if (size > 0 && p.chipmem.size > 0x200000)
		p.chipmem.size = 0x200000;
	if (board == &p.fastmem[0])
		sync_cpuboard_mem(p);
}

const amiberry_setting_choice fastmem_size_choices[] = {
	{"0", "none"},
	{"1", "1M"},
	{"2", "2M"},
	{"4", "4M"},
	{"8", "8M"},
};

void get_fastmem_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, static_cast<int>(p.fastmem[0].size / 0x100000));
}

void set_fastmem_size(uae_prefs& p, const value& v)
{
	set_fastmem_size(p, &p.fastmem[0], static_cast<uae_u32>(v.number) * 0x100000);
}

// Boards below 1 MB are written in KB (fastmem_size_k); the MB choice reads
void get_fastmem_size_k(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.fastmem[0].size < 0x100000 ? static_cast<int>(p.fastmem[0].size / 1024) : 0);
}

void set_fastmem_size_k(uae_prefs& p, const value& v)
{
	const int kb = static_cast<int>(v.number);
	if (kb == 0) {
		if (p.fastmem[0].size < 0x100000)
			p.fastmem[0].size = 0;
	} else {
		p.fastmem[0].size = static_cast<uae_u32>(kb) * 1024;
	}
	if (p.fastmem[0].size > 0)
		sync_cpuboard_mem(p);
}
// key is the board number in the option name (fastmem2_size is board 1,
// because cfgfile numbers boards from the second), index the fastmem[]
// slot it stores.
#define FASTMEM_BOARD(key, index) \
	void get_fastmem##key##_size(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, static_cast<int>(p.fastmem[index].size / 0x100000)); \
	} \
	void set_fastmem##key##_size(uae_prefs& p, const value& v) \
	{ \
		set_fastmem_size(p, &p.fastmem[index], static_cast<uae_u32>(v.number) * 0x100000); \
	} \
	void get_fastmem##key##_size_k(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, p.fastmem[index].size < 0x100000 ? static_cast<int>(p.fastmem[index].size / 1024) : 0); \
	} \
	void set_fastmem##key##_size_k(uae_prefs& p, const value& v) \
	{ \
		const int kb = static_cast<int>(v.number); \
		if (kb == 0) { \
			if (p.fastmem[index].size < 0x100000) \
				p.fastmem[index].size = 0; \
		} else { \
			p.fastmem[index].size = static_cast<uae_u32>(kb) * 1024; \
		} \
	}

FASTMEM_BOARD(2, 1)
FASTMEM_BOARD(3, 2)
FASTMEM_BOARD(4, 3)

// ------------------------------------------------------------- Z3 fast RAM

const amiberry_setting_choice z3mem_size_choices[] = {
	{"0", "none"},
	{"1", "1M"},
	{"2", "2M"},
	{"4", "4M"},
	{"8", "8M"},
	{"16", "16M"},
	{"32", "32M"},
	{"64", "64M"},
	{"128", "128M"},
	{"256", "256M"},
	{"512", "512M"},
	{"1024", "1G"},
};

void set_z3mem_size(uae_prefs& p, ramboard* board, uae_u32 size)
{
	board->size = size > max_z3fastmem ? static_cast<uae_u32>(max_z3fastmem) : size;
}

#define Z3MEM_BOARD(n) \
	void get_z3mem##n##_size(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, static_cast<int>(p.z3fastmem[n - 1].size / 0x100000)); \
	} \
	void set_z3mem##n##_size(uae_prefs& p, const value& v) \
	{ \
		set_z3mem_size(p, &p.z3fastmem[n - 1], static_cast<uae_u32>(v.number) * 0x100000); \
	}

// z3mem_size is board 0; z3mem2_size..z3mem4_size are boards 1..3.
void get_z3mem_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, static_cast<int>(p.z3fastmem[0].size / 0x100000));
}

void set_z3mem_size_board0(uae_prefs& p, const value& v)
{
	set_z3mem_size(p, &p.z3fastmem[0], static_cast<uae_u32>(v.number) * 0x100000);
}

Z3MEM_BOARD(2)
Z3MEM_BOARD(3)
Z3MEM_BOARD(4)

// Sizes above the reserved Z3 window are unavailable (the panel clamped
// them silently, up to the 1 GB entry of can_have_1gb() hosts).
bool z3mem_choice_available(const uae_prefs&, std::string_view choice)
{
	const double mb = std::atof(std::string(choice).c_str());
	return mb * 1024 * 1024 <= static_cast<double>(max_z3fastmem);
}

// ------------------------------------------------- motherboard / CPU slot

const amiberry_setting_choice a3000mem_size_choices[] = {
	{"0", "none"},
	{"1", "1M"},
	{"2", "2M"},
	{"4", "4M"},
	{"8", "8M"},
	{"16", "16M"},
	{"32", "32M"},
	{"64", "64M"},
};

void get_a3000mem_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, static_cast<int>(p.mbresmem_low.size / 0x100000));
}

void set_a3000mem_size(uae_prefs& p, const value& v)
{
	p.mbresmem_low.size = static_cast<uae_u32>(v.number) * 0x100000;
}

const amiberry_setting_choice mbresmem_size_choices[] = {
	{"0", "none"},
	{"1", "1M"},
	{"2", "2M"},
	{"4", "4M"},
	{"8", "8M"},
	{"16", "16M"},
	{"32", "32M"},
	{"64", "64M"},
	{"128", "128M"},
};

void get_mbresmem_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, static_cast<int>(p.mbresmem_high.size / 0x100000));
}

// Processor slot RAM is the mirror an accelerator board in that slot uses;
// keep the board copy in sync like the panel did every frame.
void set_mbresmem_size(uae_prefs& p, const value& v)
{
	p.mbresmem_high.size = static_cast<uae_u32>(v.number) * 0x100000;
	sync_cpuboard_mem(p);
}

// ------------------------------------------------------------ 32-bit chip

const amiberry_setting_choice megachipmem_size_choices[] = {
	{"0", "none"},
	{"16", "16M"},
	{"32", "32M"},
	{"64", "64M"},
	{"128", "128M"},
	{"256", "256M"},
	{"384", "384M"},
	{"512", "512M"},
	{"768", "768M"},
	{"1024", "1G"},
};

void get_megachipmem_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, static_cast<int>(p.z3chipmem.size / 0x100000));
}

void set_megachipmem_size(uae_prefs& p, const value& v)
{
	uae_u32 size = static_cast<uae_u32>(v.number) * 0x100000;
	p.z3chipmem.size = size > max_z3fastmem ? static_cast<uae_u32>(max_z3fastmem) : size;
}

bool megachipmem_choice_available(const uae_prefs&, std::string_view choice)
{
	const double mb = std::atof(std::string(choice).c_str());
	return mb * 1024 * 1024 <= static_cast<double>(max_z3fastmem);
}

// ------------------------------------------------------------- Z3 mapping

const amiberry_setting_choice z3mapping_choices[] = {
	{"auto", "Automatic (*)"},
	{"uae", "UAE"},
	{"real", "Real"},
};

void get_z3mapping(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(z3mapping_choices), p.z3_mapping_mode);
}

void set_z3mapping(uae_prefs& p, const value& v)
{
	p.z3_mapping_mode = choice_index(CHOICES(z3mapping_choices), v.text);
}

// ---------------------------------------------------------------- table

constexpr double no_range = 0;

#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, CHOICES(choices), constraint}
#define NUMBER_OPTION(id, group, label, help, type, apply, min, max, step, def, constraint) \
	{id, group, label, help, type, apply, min, max, step, def, nullptr, 0, constraint}

const char* const z3_constraint =
	"Needs 32-bit addressing, and is limited by the reserved Zorro III window.";

const definition definitions[] = {
	{CHOICE_OPTION("chipmem_size", "memory", "Chip RAM",
			"Main memory accessible by all custom chips. A500 had 512 KB, A1200 2 MB; more than 512 KB requires ECS Agnus.",
			AMIBERRY_APPLY_RESET, "1", chipmem_size_choices,
			"More than 2 MB excludes Zorro II fast RAM; sizes must be powers of two (1.5 MB allowed)."),
		get_chipmem_size, set_chipmem_size, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("bogomem_size", "memory", "Slow RAM",
			"512 KB expansion in the A500/A2000 trapdoor slot (Ranger RAM): accessible by custom chips but slower.",
			AMIBERRY_APPLY_RESET, "2", bogomem_size_choices,
			"Only none, 512 KB, 1M, 1.5M and 1.8M exist on real hardware."),
		get_bogomem_size, set_bogomem_size, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("fastmem_size", "memory", "Z2 Fast RAM",
			"Zorro II expansion memory. 16-bit, not accessible by custom chips, max 8 MB.",
			AMIBERRY_APPLY_RESET, "0", fastmem_size_choices,
			"More than 256 KB excludes chip RAM above 2 MB."),
		get_fastmem_size, set_fastmem_size, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("fastmem_size_k", "memory", "Z2 Fast RAM (KB)",
			"Zorro II fast RAM below 1 MB, in KB (written as fastmem_size_k). 0 clears a sub-1 MB board.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_RESET, 0, 896, 64, "0", nullptr),
		get_fastmem_size_k, set_fastmem_size_k, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("fastmem2_size", "memory", "Z2 Fast RAM board 2",
			"Second Zorro II fast RAM board.",
			AMIBERRY_APPLY_RESET, "0", fastmem_size_choices, nullptr),
		get_fastmem2_size, set_fastmem2_size, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("fastmem2_size_k", "memory", "Z2 Fast RAM board 2 (KB)",
			"Second Zorro II fast RAM board below 1 MB, in KB.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_RESET, 0, 896, 64, "0", nullptr),
		get_fastmem2_size_k, set_fastmem2_size_k, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("fastmem3_size", "memory", "Z2 Fast RAM board 3",
			"Third Zorro II fast RAM board.",
			AMIBERRY_APPLY_RESET, "0", fastmem_size_choices, nullptr),
		get_fastmem3_size, set_fastmem3_size, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("fastmem3_size_k", "memory", "Z2 Fast RAM board 3 (KB)",
			"Third Zorro II fast RAM board below 1 MB, in KB.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_RESET, 0, 896, 64, "0", nullptr),
		get_fastmem3_size_k, set_fastmem3_size_k, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("fastmem4_size", "memory", "Z2 Fast RAM board 4",
			"Fourth Zorro II fast RAM board.",
			AMIBERRY_APPLY_RESET, "0", fastmem_size_choices, nullptr),
		get_fastmem4_size, set_fastmem4_size, nullptr, nullptr, nullptr},
	{NUMBER_OPTION("fastmem4_size_k", "memory", "Z2 Fast RAM board 4 (KB)",
			"Fourth Zorro II fast RAM board below 1 MB, in KB.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_RESET, 0, 896, 64, "0", nullptr),
		get_fastmem4_size_k, set_fastmem4_size_k, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("z3mem_size", "memory", "Z3 Fast RAM",
			"Zorro III expansion memory. 32-bit, used with 68030+ CPUs; not accessible by custom chips.",
			AMIBERRY_APPLY_RESET, "0", z3mem_size_choices, z3_constraint),
		get_z3mem_size, set_z3mem_size_board0, z3_possible, z3mem_choice_available, nullptr},
	{CHOICE_OPTION("z3mem2_size", "memory", "Z3 Fast RAM board 2",
			"Second Zorro III fast RAM board.",
			AMIBERRY_APPLY_RESET, "0", z3mem_size_choices, z3_constraint),
		get_z3mem2_size, set_z3mem2_size, z3_possible, z3mem_choice_available, nullptr},
	{CHOICE_OPTION("z3mem3_size", "memory", "Z3 Fast RAM board 3",
			"Third Zorro III fast RAM board.",
			AMIBERRY_APPLY_RESET, "0", z3mem_size_choices, z3_constraint),
		get_z3mem3_size, set_z3mem3_size, z3_possible, z3mem_choice_available, nullptr},
	{CHOICE_OPTION("z3mem4_size", "memory", "Z3 Fast RAM board 4",
			"Fourth Zorro III fast RAM board.",
			AMIBERRY_APPLY_RESET, "0", z3mem_size_choices, z3_constraint),
		get_z3mem4_size, set_z3mem4_size, z3_possible, z3mem_choice_available, nullptr},
	{CHOICE_OPTION("a3000mem_size", "memory", "Motherboard Fast RAM",
			"Fast RAM on the A3000/A4000 motherboard. Not accessible by custom chips.",
			AMIBERRY_APPLY_RESET, "0", a3000mem_size_choices,
			"Up to 64 MB, aligned to MB."),
		get_a3000mem_size, set_a3000mem_size, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("mbresmem_size", "memory", "Processor slot Fast RAM",
			"Fast RAM on a CPU accelerator card in the processor slot. Not accessible by custom chips.",
			AMIBERRY_APPLY_RESET, "0", mbresmem_size_choices,
			"Up to 128 MB, aligned to MB."),
		get_mbresmem_size, set_mbresmem_size, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("megachipmem_size", "memory", "32-bit Chip RAM",
			"Extended chip RAM accessible by custom chips. Requires 32-bit addressing.",
			AMIBERRY_APPLY_RESET, "0", megachipmem_size_choices, z3_constraint),
		get_megachipmem_size, set_megachipmem_size, z3_possible, megachipmem_choice_available, nullptr},
	{CHOICE_OPTION("z3mapping", "memory", "Z3 mapping mode",
			"Z3 memory address mapping. Automatic picks the best layout, UAE and Real match those address spaces.",
			AMIBERRY_APPLY_RESET, "auto", z3mapping_choices,
			"Needs 32-bit addressing."),
		get_z3mapping, set_z3mapping, z3_possible, nullptr, nullptr},
};

bool commit_memory(const uae_prefs& base, const uae_prefs& from, uae_prefs& to)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(chipmem);
	FIELD(bogomem);
	FIELD(fastmem);
	FIELD(z3fastmem);
	FIELD(mbresmem_low);
	FIELD(mbresmem_high);
	FIELD(z3chipmem);
	FIELD(cpuboardmem1);
	FIELD(cpuboardmem2);
	FIELD(mem25bit);
	FIELD(z3_mapping_mode);
	FIELD(z3autoconfig_start);
	FIELD(chipset_mask); // the chip RAM setters may add ECS Agnus
#undef FIELD
	return wrote;
}

#undef CHOICE_OPTION
#undef NUMBER_OPTION
#undef CHOICES

const domain memory = {
	definitions, sizeof definitions / sizeof definitions[0], commit_memory,
};

} // namespace

const domain& memory_domain()
{
	return memory;
}

// WinUAE copycpuboardmem(tomem = true): copy the memory subsystem the user
// edited into the accelerator board RAM and clamp it to the board maximum.
// The RAM panel used to run this on every frame; the memory setters run it
// now, so rendering no longer writes prefs and the core fixup (which copies
// the other direction) still sees a consistent board.
void sync_cpuboard_mem(uae_prefs& p)
{
	const int maxmem = cpuboard_maxmemory(&p);
	const int memtype = cpuboard_memorytype(&p);
	if (memtype == BOARD_MEMORY_Z2)
		p.cpuboardmem1.size = p.fastmem[0].size;
	else if (memtype == BOARD_MEMORY_25BITMEM)
		p.cpuboardmem1.size = p.mem25bit.size;
	else if (memtype == BOARD_MEMORY_HIGHMEM)
		p.cpuboardmem1.size = p.mbresmem_high.size;
	if (p.cpuboardmem1.size > static_cast<uae_u32>(maxmem))
		p.cpuboardmem1.size = maxmem;
}

// The copy fixup_prefs() makes after any accelerator board change: the
// board RAM lands in the memory slot the board uses (main.cpp). Options
// that install a board themselves run it, so the configuration they leave
// behind is the one the fixup would resolve to anyway.
void mirror_cpuboard_mem(uae_prefs& p)
{
	const int memtype = cpuboard_memorytype(&p);
	if (memtype == BOARD_MEMORY_HIGHMEM)
		p.mbresmem_high.size = p.cpuboardmem1.size;
	else if (memtype == BOARD_MEMORY_Z2)
		p.fastmem[0].size = p.cpuboardmem1.size;
	else if (memtype == BOARD_MEMORY_25BITMEM)
		p.mem25bit.size = p.cpuboardmem1.size;
	else if (memtype == BOARD_MEMORY_CUSTOM_32)
		p.mem25bit.size = 0;
}

} // namespace amiberry_settings
