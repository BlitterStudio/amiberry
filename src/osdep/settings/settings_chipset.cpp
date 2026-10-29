// Chipset, keyboard and genlock options (the Chipset and Advanced Chipset
// panels). Choice values are the exact .uae spellings cfgfile.cpp writes;
// the inline value tables there (csmode[], cscompa[], ...) are frozen
// configuration compatibility, so the layer keeps its own copies and the
// round-trip test guards them against drift.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "custom.h"

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

// The Advanced Chipset panel keeps every control except the RTC radios
// disabled while a chipset extra is active.
bool custom_chipset(const uae_prefs& p, const context&)
{
	return p.cs_compatible == 0;
}

// ------------------------------------------------------------------ chipset

const amiberry_setting_choice chipset_choices[] = {
	{"a1000_noehb", "A1000 (No EHB)"},
	{"a1000", "A1000"},
	{"ocs", "OCS + OCS Denise"},
	{"ecs_agnus", "ECS + OCS Denise"},
	{"ecs_denise", "OCS + ECS Denise"},
	{"ecs", "Full ECS"},
	{"aga", "AGA"},
};

// The .uae spelling of a chipset mask, exactly how cfgfile saves it.
void get_chipset(const uae_prefs& p, char* out, size_t size)
{
	const char* v = "ocs";
	if (p.chipset_mask & CSMASK_AGA)
		v = "aga";
	else if ((p.chipset_mask & CSMASK_ECS_AGNUS) && (p.chipset_mask & CSMASK_ECS_DENISE))
		v = "ecs";
	else if (p.chipset_mask & CSMASK_ECS_AGNUS)
		v = "ecs_agnus";
	else if (p.chipset_mask & CSMASK_ECS_DENISE)
		v = "ecs_denise";
	else if (p.chipset_mask & CSMASK_A1000_NOEHB)
		v = "a1000_noehb";
	else if (p.chipset_mask & CSMASK_A1000)
		v = "a1000";
	std::snprintf(out, size, "%s", v);
}

// set_chipset_mask (cfgfile.cpp): AGA always includes both ECS chips.
void set_chipset(uae_prefs& p, const value& v)
{
	unsigned int mask = CSMASK_OCS;
	if (v.text == "a1000_noehb")
		mask = CSMASK_A1000_NOEHB;
	else if (v.text == "a1000")
		mask = CSMASK_A1000;
	else if (v.text == "ecs_agnus")
		mask = CSMASK_ECS_AGNUS;
	else if (v.text == "ecs_denise")
		mask = CSMASK_ECS_DENISE;
	else if (v.text == "ecs")
		mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	else if (v.text == "aga")
		mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p.chipset_mask = mask;
}

void get_ntsc(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.ntscmode);
}

// Quickstart always moved the output refresh rate with the TV standard; the
// chipset panel did not, leaving NTSC with a 50 Hz output. NTSC is 60 Hz, so
// the layer follows Quickstart, but only on a real change so a re-clicked
// checkbox keeps a refresh rate the user set separately.
void set_ntsc(uae_prefs& p, const value& v)
{
	const bool ntsc = v.number != 0;
	if (ntsc != p.ntscmode)
		p.chipset_refreshrate = ntsc ? 60.0f : 50.0f;
	p.ntscmode = ntsc;
}

// ---------------------------------------------------------- cycle exactness
// One key for the two booleans the Chipset panel exposes: cycle_exact is the
// value cfgfile writes for the combination (false / memory / true), and full
// cycle-exact always implies memory cycle-exact (parsing cpu_cycle_exact
// sets the memory flag too), so the three states cover every combination the
// panels and the configuration file produce.

const amiberry_setting_choice cycle_exact_choices[] = {
	{"false", "Off"},
	{"memory", "Cycle Exact (DMA/Memory)"},
	{"true", "Cycle Exact (Full)"},
};

void get_cycle_exact(const uae_prefs& p, char* out, size_t size)
{
	if (p.cpu_cycle_exact)
		std::snprintf(out, size, "true");
	else if (p.cpu_memory_cycle_exact)
		std::snprintf(out, size, "memory");
	else
		std::snprintf(out, size, "false");
}

// The dependent changes of the old Cycle Exact (DMA/Memory) checkbox: a
// 68000/68010 cannot run memory-exact without full cycle-exact, 68030 and
// slower cycle-exact modes need real speed and More compatible, and neither
// works with immediate blits, the JIT or frameskip. Enabling full
// cycle-exact applies them too (it is a superset), which the old panel only
// did for the memory checkbox.
void set_cycle_exact(uae_prefs& p, const value& v)
{
	const bool full = v.text == "true";
	const bool memory = v.text != "false";
	p.cpu_cycle_exact = full;
	p.cpu_memory_cycle_exact = p.blitter_cycle_exact = memory;
	if (!memory) {
		clamp_ram_for_fixups(p); // the JIT may run again
		return;
	}
	if (p.cpu_model < 68020)
		p.cpu_cycle_exact = true;
	if (p.cpu_model <= 68030) {
		p.m68k_speed = 0;
		p.cpu_compatible = true;
	}
	p.immediate_blits = false;
	// fixup_prefs drops Wait for Blitter in 68000/010 cycle-exact modes.
	if (p.cpu_model <= 68010)
		p.waiting_blits = 0;
	p.cachesize = 0;
	p.gfx_framerate = 1;
}

// The old panel greyed out the DMA/Memory checkbox below a 68020 (where it
// is indistinguishable from full cycle-exact anyway).
bool cycle_exact_choice_available(const uae_prefs& p, std::string_view choice)
{
	return choice != "memory" || p.cpu_model >= 68020;
}

// ---------------------------------------------------------------- blitter

void get_immediate_blits(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.immediate_blits);
}

void set_immediate_blits(uae_prefs& p, const value& v)
{
	p.immediate_blits = v.number != 0;
	if (p.immediate_blits)
		p.waiting_blits = 0;
}

const amiberry_setting_choice waiting_blits_choices[] = {
	{"disabled", "Disabled"},
	{"automatic", "Automatic"},
	{"noidleonly", "No idle only"},
	{"always", "Always"},
};

void get_waiting_blits(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(waiting_blits_choices), p.waiting_blits);
}

void set_waiting_blits(uae_prefs& p, const value& v)
{
	p.waiting_blits = choice_index(CHOICES(waiting_blits_choices), v.text);
	if (p.waiting_blits != 0)
		p.immediate_blits = false;
}

// The panel kept forcing this to Disabled every frame while greyed out; the
// layer expresses that as availability instead.
bool waiting_blits_available(const uae_prefs& p, const context&)
{
	return !p.immediate_blits && !(p.cpu_memory_cycle_exact && p.cpu_model <= 68010);
}

// --------------------------------------------------------------- keyboard

// keyboard_type values; the keyboard controller names are frozen .uae
// compatibility spellings (kbtype[] in cfgfile.cpp).
const amiberry_setting_choice keyboard_type_choices[] = {
	{"disconnected", "Keyboard disconnected"},
	{"UAE", "UAE high level emulation"},
	{"a500_6570-036", "A500 / A500+ (6500-1 MCU)"},
	{"a600_6570-036", "A600 (6570-036 MCU)"},
	{"a1000_6500-1", "A1000 (6500-1 MCU)"},
	{"a1000_6570-036", "A1000 (6570-036 MCU)"},
	{"a1200_6805", "A1200 (68HC05C MCU)"},
	{"a2000_8039", "A2000 (Cherry, 8039 MCU)"},
	{"ax000_6570-036", "A2000/A3000/A4000 (6570-036 MCU)"},
};

void get_keyboard_type(const uae_prefs& p, char* out, size_t size)
{
	int index = p.keyboard_mode + 1;
	if (index < 0 || static_cast<size_t>(index) >= sizeof keyboard_type_choices / sizeof keyboard_type_choices[0])
		index = 0;
	write_choice(out, size, CHOICES(keyboard_type_choices), index);
}

// The UAE and Cherry keyboards have no N-key rollover; the panel forced the
// option on every frame while greyed out, the layer reports it unavailable.
bool keyboard_nkro_available(const uae_prefs& p, const context&)
{
	return p.keyboard_mode != KB_UAE && p.keyboard_mode != KB_A2000_8039;
}

void set_keyboard_type(uae_prefs& p, const value& v)
{
	p.keyboard_mode = choice_index(CHOICES(keyboard_type_choices), v.text) - 1;
	// The controllers without N-key rollover force it on (the panel wrote
	// this every frame while the option was greyed out).
	if (!keyboard_nkro_available(p, {}))
		p.keyboard_nkro = true;
}

void get_keyboard_nkro(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.keyboard_nkro);
}

void set_keyboard_nkro(uae_prefs& p, const value& v)
{
	p.keyboard_nkro = v.number != 0;
}

// ------------------------------------------------------- misc chipset options

const amiberry_setting_choice display_optimizations_choices[] = {
	{"full", "Full"},
	{"partial", "Partial"},
	{"none", "None"},
};

void get_display_optimizations(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(display_optimizations_choices), p.cs_optimizations);
}

void set_display_optimizations(uae_prefs& p, const value& v)
{
	p.cs_optimizations = choice_index(CHOICES(display_optimizations_choices), v.text);
}

const amiberry_setting_choice collision_level_choices[] = {
	{"none", "None"},
	{"sprites", "Sprites only"},
	{"playfields", "Sprites and Sprites vs. Playfield"},
	{"full", "Full"},
};

void get_collision_level(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(collision_level_choices), p.collision_level);
}

void set_collision_level(uae_prefs& p, const value& v)
{
	p.collision_level = choice_index(CHOICES(collision_level_choices), v.text);
}

// chipset_compatible: the exact table cscompa[] writes.
const amiberry_setting_choice chipset_compatible_choices[] = {
	{"-", "Custom"},
	{"Generic", "Generic"},
	{"CDTV", "CDTV"},
	{"CDTV-CR", "CDTV-CR"},
	{"CD32", "CD32"},
	{"A500", "A500"},
	{"A500+", "A500+"},
	{"A600", "A600"},
	{"A1000", "A1000"},
	{"A1200", "A1200"},
	{"A2000", "A2000"},
	{"A3000", "A3000"},
	{"A3000T", "A3000T"},
	{"A4000", "A4000"},
	{"A4000T", "A4000T"},
	{"Velvet", "Velvet"},
	{"Casablanca", "Casablanca"},
	{"DraCo", "DraCo"},
};

void get_chipset_compatible(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(chipset_compatible_choices), p.cs_compatible);
}

// Selecting a machine rewrites the compatible chipset features, the same
// call the panel made (built_in_chipset_prefs writes p only).
void set_chipset_compatible(uae_prefs& p, const value& v)
{
	p.cs_compatible = choice_index(CHOICES(chipset_compatible_choices), v.text);
	built_in_chipset_prefs(&p);
	clamp_ram_for_fixups(p); // the extra's Gayle/Gary/Ramsey cap slow RAM
}

// hvcsync: all six values the configuration can hold; the old panel offered
// only three and read out of bounds for the rest.
const amiberry_setting_choice hvcsync_choices[] = {
	{"hvcsync", "Combined"},
	{"csync", "CSync"},
	{"hvsync", "H/VSync"},
	{"hvcsync_s", "Combined (sync)"},
	{"csync_s", "CSync (sync)"},
	{"hvsync_s", "H/VSync (sync)"},
};

void get_hvcsync(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(hvcsync_choices), p.cs_hvcsync);
}

void set_hvcsync(uae_prefs& p, const value& v)
{
	p.cs_hvcsync = choice_index(CHOICES(hvcsync_choices), v.text);
}

// ------------------------------------------------------------------ genlock

bool genlock_group(const uae_prefs& p, const context&)
{
	return p.genlock || p.genlock_effects;
}

void get_genlock(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.genlock);
}

void set_genlock(uae_prefs& p, const value& v)
{
	p.genlock = v.number != 0;
}

// fixup_prefs() turns genlock off while A2024 or Graffiti emulation is on
// (they cannot be active together).
bool genlock_accepts(const uae_prefs& p, const value& v)
{
	return v.number == 0 || !p.monitoremu;
}

const amiberry_setting_choice genlockmode_choices[] = {
	{"none", "None"},
	{"noise", "Noise (built-in)"},
	{"testcard", "Test card (built-in)"},
	{"image", "Image file (png)"},
	{"video", "Video file"},
	{"stream", "Capture device"},
	{"ld", "American Laser Games/Picmatic LaserDisc Player"},
	{"sony_ld", "Sony LaserDisc Player"},
	{"pioneer_ld", "Pioneer LaserDisc Player"},
};

void get_genlockmode(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(genlockmode_choices), p.genlock_image);
}

void set_genlockmode(uae_prefs& p, const value& v)
{
	p.genlock_image = choice_index(CHOICES(genlockmode_choices), v.text);
}

// The sources after the image file need the VIDEOGRAB build.
bool genlockmode_needs_videograb(std::string_view choice)
{
	return choice_index(CHOICES(genlockmode_choices), choice) > 3;
}

// The old panel clamped an unavailable genlock source to none with a write
// on every frame; the layer makes the sources that need the VIDEOGRAB build
// unavailable instead.
bool genlockmode_choice_available(const uae_prefs&, std::string_view choice)
{
#ifdef VIDEOGRAB
	(void)choice;
	return true;
#else
	return choice_index(CHOICES(genlockmode_choices), choice) >= 0 && !genlockmode_needs_videograb(choice);
#endif
}

void get_genlock_image_file(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.genlock_image_file);
}

void set_genlock_image_file(uae_prefs& p, const value& v)
{
	std::snprintf(p.genlock_image_file, sizeof p.genlock_image_file, "%.*s",
		static_cast<int>(v.text.size()), v.text.data());
}

bool genlock_image_file_available(const uae_prefs& p, const context& ctx)
{
	return genlock_group(p, ctx) && p.genlock_image == 3;
}

void get_genlock_video_file(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.genlock_video_file);
}

void set_genlock_video_file(uae_prefs& p, const value& v)
{
	std::snprintf(p.genlock_video_file, sizeof p.genlock_video_file, "%.*s",
		static_cast<int>(v.text.size()), v.text.data());
}

bool genlock_video_file_available(const uae_prefs& p, const context& ctx)
{
#ifdef VIDEOGRAB
	return genlock_group(p, ctx) && (p.genlock_image == 4 || p.genlock_image >= 6);
#else
	(void)p;
	(void)ctx;
	return false;
#endif
}


void get_genlock_mix(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.genlock_mix);
}

void set_genlock_mix(uae_prefs& p, const value& v)
{
	p.genlock_mix = static_cast<int>(v.number);
}

void get_genlock_alpha(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.genlock_alpha);
}

void set_genlock_alpha(uae_prefs& p, const value& v)
{
	p.genlock_alpha = v.number != 0;
}

void get_genlock_aspect(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.genlock_aspect > 0);
}

void set_genlock_aspect(uae_prefs& p, const value& v)
{
	p.genlock_aspect = v.number != 0 ? 1 : 0;
}

// ----------------------------------------------- advanced chipset features

const amiberry_setting_choice rtc_choices[] = {
	{"none", "None"},
	{"MSM6242B", "MSM6242B"},
	{"RP5C01A", "RF5C01A"},
	{"MSM6242B_A2000", "A2000 MSM6242B"},
};

// The RTC value a chipset extra forces: the cs_rtc column of
// built_in_chipset_prefs() (cfgfile.cpp). While an extra is active
// fixup_prefs() re-runs that table on every configuration, so of all the
// RTC chips only the one the selected machine ships with survives; this
// mirrors the table so choice availability can offer exactly that value.
// The round-trip test guards the copy against drift.
int rtc_of(const uae_prefs& p)
{
	switch (p.cs_compatible) {
	case CP_GENERIC:
		if (p.cpu_model >= 68020)
			return 2; // big box-like: RP5C01A
		if (p.cpu_compatible)
			return has_expansion_with_rtc(const_cast<uae_prefs*>(&p), 0x80000) ? 1 : 0;
		return 1; // sort of A500-like
	case CP_CDTV:
	case CP_CDTVCR:
	case CP_A500P:
	case CP_A2000:
		return 1;
	case CP_CD32:
		return has_expansion_with_rtc(const_cast<uae_prefs*>(&p), 0x200000) ? 1 : 0;
	case CP_A500:
	case CP_A600:
	case CP_A1000:
		return has_expansion_with_rtc(const_cast<uae_prefs*>(&p), 0x100000) ? 1 : 0;
	case CP_A3000:
	case CP_A3000T:
	case CP_A4000:
	case CP_A4000T:
		return 2;
	default:
		return p.cs_rtc; // Custom (and unlisted extras): untouched
	}
}

bool rtc_choice_available(const uae_prefs& p, std::string_view choice)
{
	if (!p.cs_compatible)
		return true; // Custom: every chip survives
	return choice_index(CHOICES(rtc_choices), choice) == rtc_of(p);
}

void get_rtc(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(rtc_choices), p.cs_rtc);
}

void set_rtc(uae_prefs& p, const value& v)
{
	p.cs_rtc = choice_index(CHOICES(rtc_choices), v.text);
}

void get_rtc_adjust(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cs_rtc_adjust);
}

void set_rtc_adjust(uae_prefs& p, const value& v)
{
	p.cs_rtc_adjust = static_cast<int>(v.number);
}

const amiberry_setting_choice ciaatod_choices[] = {
	{"vblank", "Vertical sync"},
	{"50hz", "Power supply 50 Hz"},
	{"60hz", "Power supply 60 Hz"},
};

void get_ciaatod(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(ciaatod_choices), p.cs_ciaatod);
}

void set_ciaatod(uae_prefs& p, const value& v)
{
	p.cs_ciaatod = choice_index(CHOICES(ciaatod_choices), v.text);
}

// cs_pcmcia is an int in uae_prefs but a boolean in the configuration.
void get_pcmcia(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.cs_pcmcia != 0);
}

void set_pcmcia(uae_prefs& p, const value& v)
{
	p.cs_pcmcia = v.number != 0;
}

// ide is a plain enum in the configuration (ide=none|a600/a1200|a4000); the
// two checkboxes of the old panel were mutually exclusive in practice, so
// the layer models them as the choice they always stored.
const amiberry_setting_choice ide_choices[] = {
	{"none", "None"},
	{"a600/a1200", "A600/A1200 IDE"},
	{"a4000", "A4000/A4000T IDE"},
};

void get_ide(const uae_prefs& p, char* out, size_t size)
{
	int index = 0;
	if (p.cs_ide == IDE_A600A1200)
		index = 1;
	else if (p.cs_ide == IDE_A4000)
		index = 2;
	write_choice(out, size, CHOICES(ide_choices), index);
}

// With a custom chipset the A4000 IDE controller brings Gary and Ramsey
// with it: fixup_prefs() raises a disabled Fat Gary to revision 0 and a
// disabled Ramsey to 0x0f, so the setter does the same.
void set_ide(uae_prefs& p, const value& v)
{
	p.cs_ide = v.text == "a600/a1200" ? IDE_A600A1200 : v.text == "a4000" ? IDE_A4000 : 0;
	if (p.cs_compatible == 0 && p.cs_ide == IDE_A4000) {
		if (p.cs_fatgaryrev < 0)
			p.cs_fatgaryrev = 0;
		if (p.cs_ramseyrev < 0)
			p.cs_ramseyrev = 0x0f;
	}
	clamp_ram_for_fixups(p);
}

void get_scsi_a3000(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, (p.cs_mbdmac & 1) != 0);
}

void set_scsi_a3000(uae_prefs& p, const value& v)
{
	if (v.number)
		p.cs_mbdmac |= 1;
	else
		p.cs_mbdmac &= ~1;
}

void get_scsi_a4000t(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, (p.cs_mbdmac & 2) != 0);
}

void set_scsi_a4000t(uae_prefs& p, const value& v)
{
	if (v.number)
		p.cs_mbdmac |= 2;
	else
		p.cs_mbdmac &= ~2;
}

// The 1M chip jumper only exists on machines wired for it with less than
// 1 MB chip RAM; with 1 MB or more the core fixes it on.
bool chip1m_available(const uae_prefs& p, const context& ctx)
{
	return custom_chipset(p, ctx) && p.chipmem.size < 0x100000;
}

void get_chip1m(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.cs_1mchipjumper);
}

void set_chip1m(uae_prefs& p, const value& v)
{
	p.cs_1mchipjumper = v.number != 0;
}

const amiberry_setting_choice unmapped_choices[] = {
	{"floating", "Floating"},
	{"zero", "All zeros"},
	{"one", "All ones (FF)"},
};

void get_unmapped_space(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(unmapped_choices), p.cs_unmapped_space);
}

void set_unmapped_space(uae_prefs& p, const value& v)
{
	p.cs_unmapped_space = choice_index(CHOICES(unmapped_choices), v.text);
}

const amiberry_setting_choice eclocksync_choices[] = {
	{"default", "Automatic"},
	{"68000", "68000"},
	{"Gayle", "Gayle"},
	{"68000_opt", "68000 alternate"},
};

void get_eclocksync(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(eclocksync_choices), p.cs_eclocksync);
}

void set_eclocksync(uae_prefs& p, const value& v)
{
	p.cs_eclocksync = choice_index(CHOICES(eclocksync_choices), v.text);
}

// ------------------------------------------------------------ chip revisions

void get_ramsey(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cs_ramseyrev);
}

void set_ramsey(uae_prefs& p, const value& v)
{
	p.cs_ramseyrev = static_cast<int>(v.number);
	clamp_ram_for_fixups(p);
}

void get_fatgary(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cs_fatgaryrev);
}

void set_fatgary(uae_prefs& p, const value& v)
{
	p.cs_fatgaryrev = static_cast<int>(v.number);
	clamp_ram_for_fixups(p);
}

// fixup_prefs() keeps Gary and Ramsey present while the A4000 IDE
// controller is configured on a custom chipset (see set_ide).
bool gary_ramsey_accepts(const uae_prefs& p, const value& v)
{
	return v.number >= 0 || !(p.cs_compatible == 0 && p.cs_ide == IDE_A4000);
}

void get_agnusrev(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cs_agnusrev);
}

void set_agnusrev(uae_prefs& p, const value& v)
{
	p.cs_agnusrev = static_cast<int>(v.number);
}

void get_deniserev(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cs_deniserev);
}

void set_deniserev(uae_prefs& p, const value& v)
{
	p.cs_deniserev = static_cast<int>(v.number);
}

const amiberry_setting_choice agnusmodel_choices[] = {
	{"default", "Automatic"},
	{"velvet", "Velvet"},
	{"a1000", "A1000"},
	{"ocs", "OCS"},
	{"ecs", "ECS"},
	{"aga", "AGA"},
};

void get_agnusmodel(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(agnusmodel_choices), p.cs_agnusmodel);
}

void set_agnusmodel(uae_prefs& p, const value& v)
{
	p.cs_agnusmodel = choice_index(CHOICES(agnusmodel_choices), v.text);
}

const amiberry_setting_choice agnussize_choices[] = {
	{"default", "Automatic"},
	{"512k", "512 KB"},
	{"1m", "1 MB"},
	{"2m", "2 MB"},
};

void get_agnussize(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(agnussize_choices), p.cs_agnussize);
}

void set_agnussize(uae_prefs& p, const value& v)
{
	p.cs_agnussize = choice_index(CHOICES(agnussize_choices), v.text);
}

const amiberry_setting_choice denisemodel_choices[] = {
	{"default", "Automatic"},
	{"velvet", "Velvet"},
	{"a1000_noehb", "A1000 (no EHB)"},
	{"a1000", "A1000"},
	{"ocs", "OCS"},
	{"ecs", "ECS"},
	{"aga", "AGA"},
};

void get_denisemodel(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(denisemodel_choices), p.cs_denisemodel);
}

void set_denisemodel(uae_prefs& p, const value& v)
{
	p.cs_denisemodel = choice_index(CHOICES(denisemodel_choices), v.text);
}

const amiberry_setting_choice cia_type_choices[] = {
	{"default", "Default"},
	{"391078-01", "391078-01"},
};

void get_ciaa_type(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(cia_type_choices), p.cs_ciatype[0]);
}

// The panel's single CIA 391078-01 checkbox sets both sockets; ciab_type
// keeps its own configuration key but follows the panel's behaviour.
void set_ciaa_type(uae_prefs& p, const value& v)
{
	p.cs_ciatype[0] = p.cs_ciatype[1] = choice_index(CHOICES(cia_type_choices), v.text);
}

#define CUSTOM_BOOL(id, group, label, help, apply, def, field, constraint) \
	{{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}, \
		[](const uae_prefs& p, char* out, size_t size) { write_bool(out, size, p.field); }, \
		[](uae_prefs& p, const value& v) { p.field = v.number != 0; }, \
		custom_chipset, nullptr, nullptr}
#define CUSTOM_CHOICE(id, group, label, help, apply, def, choices, get, set, constraint) \
	{{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, CHOICES(choices), constraint}, \
		get, set, custom_chipset, nullptr, nullptr}

// ---------------------------------------------------------------- table

constexpr double no_range = 0;

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, CHOICES(choices), constraint}
#define NUMBER_OPTION(id, group, label, help, type, apply, min, max, step, def, constraint) \
	{id, group, label, help, type, apply, min, max, step, def, nullptr, 0, constraint}
// A number with its unit and whether it is a level set by feel, which a
// front end may show as a slider (amiberry_settings.h).
#define MEASURED_OPTION(id, group, label, help, type, apply, min, max, step, def, constraint, unit, level) \
	{id, group, label, help, type, apply, min, max, step, def, nullptr, 0, constraint, false, unit, 0, level}
#define PATH_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_PATH, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}

const char* const custom_constraint =
	"Not available while a chipset extra is set; the extra sets it.";

const definition definitions[] = {
	{CHOICE_OPTION("chipset", "chipset", "Chipset",
			"Original Chip Set (A500/A1000/A2000), Enhanced Chip Set (A600/A3000) or AGA (A1200/A4000).",
			AMIBERRY_APPLY_LIVE, "ecs_agnus", chipset_choices, nullptr),
		get_chipset, set_chipset, nullptr, nullptr, nullptr},
	{BOOL_OPTION("ntsc", "chipset", "NTSC",
			"NTSC video mode (60 Hz) instead of PAL (50 Hz). Also moves the output refresh rate.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_ntsc, set_ntsc, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("cycle_exact", "chipset", "Cycle exactness",
			"Bit-perfect CPU and DMA timing. Full needs a fast host; DMA/Memory only covers chipset accesses.",
			AMIBERRY_APPLY_LIVE, "true", cycle_exact_choices, nullptr),
		get_cycle_exact, set_cycle_exact, nullptr, cycle_exact_choice_available, nullptr},
	{BOOL_OPTION("immediate_blits", "chipset", "Immediate Blitter",
			"Blitter operations complete instantly. Faster but less accurate.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_immediate_blits, set_immediate_blits, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("waiting_blits", "chipset", "Wait for Blitter",
			"CPU waits for the Blitter to finish. More compatible than Immediate Blitter.",
			AMIBERRY_APPLY_LIVE, "disabled", waiting_blits_choices,
			"Not available with immediate blits, nor in the cycle-exact modes on a 68000 or 68010."),
		get_waiting_blits, set_waiting_blits, waiting_blits_available, nullptr, nullptr},
	{CHOICE_OPTION("keyboard_type", "chipset", "Keyboard layout",
			"Keyboard controller emulation mode.",
			AMIBERRY_APPLY_LIVE, "UAE", keyboard_type_choices, nullptr),
		get_keyboard_type, set_keyboard_type, nullptr, nullptr, nullptr},
	{BOOL_OPTION("keyboard_nkro", "chipset", "Keyboard N-key rollover",
			"Allow multiple simultaneous key presses.",
			AMIBERRY_APPLY_LIVE, "true",
			"The UAE and Cherry (A2000 8039) keyboard controllers have no N-key rollover."),
		get_keyboard_nkro, set_keyboard_nkro, keyboard_nkro_available, nullptr, nullptr},
	{CHOICE_OPTION("display_optimizations", "chipset", "Optimizations",
			"Chipset drawing optimizations. Keep at Full for best compatibility.",
			AMIBERRY_APPLY_LIVE, "full", display_optimizations_choices, nullptr),
		get_display_optimizations, set_display_optimizations, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("collision_level", "chipset", "Collision level",
			"Sprite and playfield collision detection. Full is the most compatible.",
			AMIBERRY_APPLY_LIVE, "full", collision_level_choices, nullptr),
		get_collision_level, set_collision_level, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("chipset_compatible", "chipset", "Chipset extra",
			"Machine-specific chipset behaviour. Selecting one rewrites the advanced chipset options.",
			AMIBERRY_APPLY_RESET, "Generic", chipset_compatible_choices, nullptr),
		get_chipset_compatible, set_chipset_compatible, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("hvcsync", "chipset", "Monitor sync source",
			"Video port sync signal layout for special monitors.",
			AMIBERRY_APPLY_LIVE, "hvcsync", hvcsync_choices, nullptr),
		get_hvcsync, set_hvcsync, nullptr, nullptr, nullptr},
	{BOOL_OPTION("genlock", "genlock", "Genlock connected",
			"Connect a genlock device for video overlay mixing.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_genlock, set_genlock, nullptr, nullptr, nullptr, genlock_accepts},
	{CHOICE_OPTION("genlockmode", "genlock", "Genlock source",
			"What the genlock overlays on the Amiga video signal. The video and LaserDisc sources need a VIDEOGRAB build.",
			AMIBERRY_APPLY_LIVE, "none", genlockmode_choices,
			"Needs genlock on."),
		get_genlockmode, set_genlockmode, genlock_group, genlockmode_choice_available, nullptr},
	{PATH_OPTION("genlock_image", "genlock", "Genlock image file",
			"PNG/JPEG/BMP image the genlock overlays (genlockmode=image).",
			AMIBERRY_APPLY_LIVE, "", nullptr),
		get_genlock_image_file, set_genlock_image_file, genlock_image_file_available, nullptr, nullptr},
	{PATH_OPTION("genlock_video", "genlock", "Genlock video file",
			"Video the genlock overlays (genlockmode=video or a LaserDisc source).",
			AMIBERRY_APPLY_LIVE, "", nullptr),
		get_genlock_video_file, set_genlock_video_file, genlock_video_file_available, nullptr, nullptr},
	{MEASURED_OPTION("genlock_mix", "genlock", "Genlock mix",
			"Weight (of 256) of the Amiga picture where the genlock source shows through: 0 shows the source alone; each step of 25 blends in about 10% more of the Amiga picture.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 0, 255, 25, "0", "Needs genlock on.", nullptr, true),
		get_genlock_mix, set_genlock_mix, genlock_group, nullptr, nullptr},
	{BOOL_OPTION("genlock_alpha", "genlock", "Include alpha channels",
			"Include alpha channels in screenshots and video captures.",
			AMIBERRY_APPLY_LIVE, "false", "Needs genlock on."),
		get_genlock_alpha, set_genlock_alpha, genlock_group, nullptr, nullptr},
	{BOOL_OPTION("genlock_aspect", "genlock", "Keep aspect ratio",
			"Keep the genlock source aspect ratio.",
			AMIBERRY_APPLY_LIVE, "false", "Needs genlock on."),
		get_genlock_aspect, set_genlock_aspect, genlock_group, nullptr, nullptr},

	{CHOICE_OPTION("rtc", "adv_chipset", "Battery backed up real time clock",
			"Real-time clock chip.",
			AMIBERRY_APPLY_LIVE, "MSM6242B", rtc_choices,
			"With a chipset extra active, only the RTC chip of the selected machine survives the configuration checks."),
		get_rtc, set_rtc, nullptr, rtc_choice_available, nullptr},
	{MEASURED_OPTION("chipset_rtc_adjust", "adv_chipset", "RTC adjustment",
			"Real-time clock adjustment in seconds.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_RESET, -100000, 100000, 1, "0", custom_constraint, "s", false),
		get_rtc_adjust, set_rtc_adjust, custom_chipset, nullptr, nullptr},
	CUSTOM_CHOICE("ciaatod", "adv_chipset", "CIA-A TOD clock source",
		"Time-of-day clock source: vertical blank or power line frequency.",
		AMIBERRY_APPLY_LIVE, "vblank", ciaatod_choices, get_ciaatod, set_ciaatod, custom_constraint),
	CUSTOM_BOOL("cia_overlay", "adv_chipset", "CIA ROM overlay",
		"Enable CIA-based Kickstart ROM overlay mechanism.", AMIBERRY_APPLY_RESET, "true", cs_ciaoverlay, custom_constraint),
	CUSTOM_BOOL("a1000ram", "adv_chipset", "A1000 Boot RAM/ROM",
		"Enable A1000-style boot RAM at $C00000.", AMIBERRY_APPLY_RESET, "false", cs_a1000ram, custom_constraint),
	CUSTOM_BOOL("df0idhw", "adv_chipset", "DF0: ID hardware",
		"Enable DF0 drive ID hardware detection.", AMIBERRY_APPLY_LIVE, "false", cs_df0idhw, custom_constraint),
	CUSTOM_BOOL("cd32cd", "adv_chipset", "CD32 CD",
		"Enable CD32 CD-ROM drive.", AMIBERRY_APPLY_LIVE, "false", cs_cd32cd, custom_constraint),
	CUSTOM_BOOL("cd32c2p", "adv_chipset", "CD32 C2P",
		"Enable CD32 chunky-to-planar hardware acceleration.", AMIBERRY_APPLY_LIVE, "false", cs_cd32c2p, custom_constraint),
	CUSTOM_BOOL("cd32nvram", "adv_chipset", "CD32 NVRAM",
		"Enable CD32 non-volatile RAM for saved data.", AMIBERRY_APPLY_LIVE, "false", cs_cd32nvram, custom_constraint),
	CUSTOM_BOOL("cdtvcd", "adv_chipset", "CDTV CD",
		"Enable CDTV CD-ROM drive.", AMIBERRY_APPLY_LIVE, "false", cs_cdtvcd, custom_constraint),
	CUSTOM_BOOL("cdtvram", "adv_chipset", "CDTV SRAM",
		"Enable CDTV static RAM for saved data.", AMIBERRY_APPLY_RESET, "false", cs_cdtvram, custom_constraint),
	CUSTOM_BOOL("cdtv-cr", "adv_chipset", "CDTV-CR",
		"Enable CDTV card reader.", AMIBERRY_APPLY_RESET, "false", cs_cdtvcr, custom_constraint),
	CUSTOM_BOOL("ksmirror_e0", "adv_chipset", "ROM mirror (E0)",
		"Enable Kickstart ROM mirror at $E00000.", AMIBERRY_APPLY_RESET, "true", cs_ksmirror_e0, custom_constraint),
	CUSTOM_BOOL("ksmirror_a8", "adv_chipset", "ROM mirror (A8)",
		"Enable Kickstart ROM mirror at $A80000.", AMIBERRY_APPLY_RESET, "false", cs_ksmirror_a8, custom_constraint),
	CUSTOM_BOOL("color_burst", "adv_chipset", "Composite color burst",
		"Enable color burst signal on composite video output.", AMIBERRY_APPLY_LIVE, "false", cs_color_burst, custom_constraint),
	CUSTOM_BOOL("resetwarning", "adv_chipset", "Keyboard reset warning",
		"Show warning when keyboard reset is pressed.", AMIBERRY_APPLY_RESET, "false", cs_resetwarning, custom_constraint),
	CUSTOM_BOOL("cia_todbug", "adv_chipset", "CIA TOD bug",
		"Emulate CIA Time of Day hardware bug.", AMIBERRY_APPLY_RESET, "true", cs_ciatodbug, custom_constraint),
	CUSTOM_BOOL("cs_bytecustomwritebug", "adv_chipset", "Custom register byte write bug",
		"Emulate byte write bug in custom chip registers.", AMIBERRY_APPLY_LIVE, "false", cs_bytecustomwritebug, custom_constraint),
	CUSTOM_BOOL("memory_pattern", "adv_chipset", "Power up memory pattern",
		"Fill memory with pattern on power-up like real hardware.", AMIBERRY_APPLY_LIVE, "true", cs_memorypatternfill, custom_constraint),
	CUSTOM_BOOL("rom_is_slow", "adv_chipset", "Kickstart ROM has Chip RAM speed",
		"Kickstart ROM runs at Chip RAM speed instead of faster.", AMIBERRY_APPLY_LIVE, "false", cs_romisslow, custom_constraint),
	CUSTOM_BOOL("toshiba_gary", "adv_chipset", "Toshiba Gary",
		"Use Toshiba Gary instead of Commodore Gary.", AMIBERRY_APPLY_LIVE, "false", cs_toshibagary, custom_constraint),
	CUSTOM_BOOL("z3_autoconfig", "adv_chipset", "Z3 Autoconfig",
		"Enable Zorro III AutoConfig for expansion boards.", AMIBERRY_APPLY_LIVE, "false", cs_z3autoconfig, custom_constraint),
	{BOOL_OPTION("pcmcia", "adv_chipset", "PCMCIA",
			"Enable the PCMCIA slot (A600/A1200).",
			AMIBERRY_APPLY_LIVE, "false", custom_constraint),
		get_pcmcia, set_pcmcia, custom_chipset, nullptr, nullptr},
	{BOOL_OPTION("1mchipjumper", "adv_chipset", "1 MB Chip / 0.5 MB + 0.5 MB",
			"Jumper for 1MB chip RAM or 512KB+512KB configuration.",
			AMIBERRY_APPLY_RESET, "false",
			"Only available with less than 1 MB chip RAM; the core fixes it on above that."),
		get_chip1m, set_chip1m, chip1m_available, nullptr, nullptr},
	{CHOICE_OPTION("ciaa_type", "adv_chipset", "CIA revision",
			"Use the newer CIA chip revision 391078-01 in both sockets.",
			AMIBERRY_APPLY_LIVE, "default", cia_type_choices, custom_constraint),
		get_ciaa_type, set_ciaa_type, custom_chipset, nullptr, nullptr},
	{CHOICE_OPTION("ide", "adv_chipset", "IDE controller",
			"Built-in IDE controller type. The A600/A1200 and A4000 controllers exclude each other, as on real hardware.",
			AMIBERRY_APPLY_RESET, "none", ide_choices,
			custom_constraint),
		get_ide, set_ide, custom_chipset, nullptr, nullptr},
	{BOOL_OPTION("scsi_a3000", "adv_chipset", "A3000 WD33C93 SCSI",
			"Enable the A3000 built-in WD33C93 SCSI controller.",
			AMIBERRY_APPLY_LIVE, "false",
			custom_constraint),
		get_scsi_a3000, set_scsi_a3000, custom_chipset, nullptr, nullptr},
	{BOOL_OPTION("scsi_a4000t", "adv_chipset", "A4000T NCR53C710 SCSI",
			"Enable the A4000T built-in NCR53C710 SCSI controller.",
			AMIBERRY_APPLY_LIVE, "false",
			custom_constraint),
		get_scsi_a4000t, set_scsi_a4000t, custom_chipset, nullptr, nullptr},
	{CHOICE_OPTION("unmapped_address_space", "adv_chipset", "Unmapped address space",
			"What unmapped memory reads return.",
			AMIBERRY_APPLY_LIVE, "floating", unmapped_choices, custom_constraint),
		get_unmapped_space, set_unmapped_space, custom_chipset, nullptr, nullptr},
	{CHOICE_OPTION("eclocksync", "adv_chipset", "CIA E-clock sync",
			"CIA E-Clock synchronization method.",
			AMIBERRY_APPLY_LIVE, "default", eclocksync_choices, custom_constraint),
		get_eclocksync, set_eclocksync, custom_chipset, nullptr, nullptr},
	{NUMBER_OPTION("ramsey", "adv_chipset", "Ramsey revision",
			"Ramsey memory controller revision (hexadecimal); -1 disables the revision override.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -1, 255, 1, "-1", custom_constraint),
		get_ramsey, set_ramsey, custom_chipset, nullptr, nullptr, gary_ramsey_accepts},
	{NUMBER_OPTION("fatgary", "adv_chipset", "Fat Gary revision",
			"Fat Gary chip revision (hexadecimal); -1 disables the revision override.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -1, 255, 1, "-1", custom_constraint),
		get_fatgary, set_fatgary, custom_chipset, nullptr, nullptr, gary_ramsey_accepts},
	{NUMBER_OPTION("agnusrev", "adv_chipset", "Agnus/Alice revision",
			"Agnus/Alice chip revision (hexadecimal); -1 disables the revision override.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -1, 255, 1, "-1", custom_constraint),
		get_agnusrev, set_agnusrev, custom_chipset, nullptr, nullptr},
	{NUMBER_OPTION("deniserev", "adv_chipset", "Denise/Lisa revision",
			"Denise/Lisa chip revision (hexadecimal); -1 disables the revision override.",
			AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, -1, 255, 1, "-1", custom_constraint),
		get_deniserev, set_deniserev, custom_chipset, nullptr, nullptr},
	{CHOICE_OPTION("agnusmodel", "adv_chipset", "Agnus/Alice model",
			"Agnus/Alice chip model variant.",
			AMIBERRY_APPLY_LIVE, "default", agnusmodel_choices, custom_constraint),
		get_agnusmodel, set_agnusmodel, custom_chipset, nullptr, nullptr},
	{CHOICE_OPTION("agnussize", "adv_chipset", "Agnus/Alice size",
			"Agnus/Alice addressable memory size.",
			AMIBERRY_APPLY_LIVE, "default", agnussize_choices, custom_constraint),
		get_agnussize, set_agnussize, custom_chipset, nullptr, nullptr},
	{CHOICE_OPTION("denisemodel", "adv_chipset", "Denise/Lisa model",
			"Denise/Lisa chip model variant.",
			AMIBERRY_APPLY_LIVE, "default", denisemodel_choices, custom_constraint),
		get_denisemodel, set_denisemodel, custom_chipset, nullptr, nullptr},
};

// A chipset choice replaces the whole selection. Otherwise the mask only
// changed as a dependent change (chip RAM above 512 KB adds ECS Agnus in
// set_chipmem_size), and only the bits that changed are merged, so a
// chipset chosen elsewhere meanwhile (AGA, say) survives. The set options
// tell the two apart: a chosen ECS Agnus changes the same single bit.
static bool commit_chipset_mask(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info& info)
{
	static const definition& chipset = require("chipset");
	if (info.was_set(chipset))
		return commit_field(base.chipset_mask, from.chipset_mask, to.chipset_mask);
	const unsigned int changed = base.chipset_mask ^ from.chipset_mask;
	if (!changed)
		return false;
	const unsigned int merged = (to.chipset_mask & ~changed) | (from.chipset_mask & changed);
	const bool wrote = merged != to.chipset_mask;
	to.chipset_mask = merged;
	return wrote;
}

bool commit_chipset(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info& info)
{
	bool wrote = commit_chipset_mask(base, from, to, info);
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(ntscmode);
	FIELD(chipset_refreshrate);
	FIELD(cpu_cycle_exact);
	FIELD(cpu_memory_cycle_exact);
	FIELD(blitter_cycle_exact);
	FIELD(immediate_blits);
	FIELD(waiting_blits);
	FIELD(keyboard_mode);
	FIELD(keyboard_nkro);
	FIELD(collision_level);
	FIELD(cs_optimizations);
	FIELD(cs_compatible);
	FIELD(cs_hvcsync);
	FIELD(cs_rtc);
	FIELD(cs_rtc_adjust);
	FIELD(cs_rtc_adjust_mode);
	FIELD(cs_ciaatod);
	FIELD(cs_ciaoverlay);
	FIELD(cs_a1000ram);
	FIELD(cs_df0idhw);
	FIELD(cs_cd32cd);
	FIELD(cs_cd32c2p);
	FIELD(cs_cd32nvram);
	FIELD(cs_cd32fmv);
	FIELD(cs_cdtvcd);
	FIELD(cs_cdtvram);
	FIELD(cs_cdtvcr);
	FIELD(cs_ksmirror_e0);
	FIELD(cs_ksmirror_a8);
	FIELD(cs_color_burst);
	FIELD(cs_resetwarning);
	FIELD(cs_ciatodbug);
	FIELD(cs_bytecustomwritebug);
	FIELD(cs_memorypatternfill);
	FIELD(cs_romisslow);
	FIELD(cs_toshibagary);
	FIELD(cs_z3autoconfig);
	FIELD(cs_pcmcia);
	FIELD(cs_1mchipjumper);
	FIELD(cs_ciatype);
	FIELD(cs_ide);
	FIELD(cs_mbdmac);
	FIELD(cs_unmapped_space);
	FIELD(cs_eclocksync);
	FIELD(cs_fatgaryrev);
	FIELD(cs_ramseyrev);
	FIELD(cs_agnusrev);
	FIELD(cs_deniserev);
	FIELD(cs_agnusmodel);
	FIELD(cs_agnussize);
	FIELD(cs_denisemodel);
	FIELD(cs_bkpthang);
	FIELD(cs_cia6526);
	FIELD(cs_floppydatapullup);
	FIELD(genlock);
	FIELD(genlock_image);
	FIELD(genlock_image_file);
	FIELD(genlock_video_file);
	FIELD(genlock_mix);
	FIELD(genlock_alpha);
	FIELD(genlock_aspect);
	FIELD(genlock_effects);
	FIELD(gfx_framerate); // written by the cycle-exact dependent change
#undef FIELD
	return wrote;
}

#undef BOOL_OPTION
#undef CHOICE_OPTION
#undef NUMBER_OPTION
#undef MEASURED_OPTION
#undef PATH_OPTION
#undef CUSTOM_BOOL
#undef CUSTOM_CHOICE
#undef CHOICES

const domain chipset = {
	definitions, sizeof definitions / sizeof definitions[0], commit_chipset,
};

} // namespace

const domain& chipset_domain()
{
	return chipset;
}

const char* choice_requires(std::string_view id, std::string_view value)
{
	return id == "genlockmode" && genlockmode_needs_videograb(value) ? "videograb" : nullptr;
}

} // namespace amiberry_settings
