// Machine presets (the Quickstart models): the model table moved here from
// src/osdep/gui/gui_handling.h, the Quickstart constraints moved from
// src/osdep/imgui/quickstart.cpp. Applying a preset runs the core's
// built_in_prefs() and then exactly the constraints the Quickstart panel
// enforced on top, so presets applied through the shared settings layer
// match the old Quickstart path.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "blkdev.h"
#include "disk.h"
#include "memory.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace amiberry_settings {

namespace {

struct preset_model {
	amiberry_settings_preset preset;
	int model;              // built_in_prefs() model index
	const char* const* configs;
};

const char* const a500_configs[] = {
	"1.3 ROM, OCS, 512 KB Chip + 512 KB Slow RAM (most common)",
	"1.3 ROM, ECS Agnus, 512 KB Chip RAM + 512 KB Slow RAM",
	"1.3 ROM, ECS Agnus, 1 MB Chip RAM",
	"1.3 ROM, OCS Agnus, 512 KB Chip RAM",
	"1.2 ROM, OCS Agnus, 512 KB Chip RAM",
	"1.2 ROM, OCS Agnus, 512 KB Chip RAM + 512 KB Slow RAM",
	nullptr,
};

const char* const a500plus_configs[] = {
	"Basic non-expanded configuration",
	"2 MB Chip RAM expanded configuration",
	"4 MB Fast RAM expanded configuration",
	nullptr,
};

const char* const a600_configs[] = {
	"Basic non-expanded configuration",
	"2 MB Chip RAM expanded configuration",
	"4 MB Fast RAM expanded configuration",
	"8 MB Fast RAM expanded configuration",
	nullptr,
};

const char* const a1000_configs[] = {
	"512 KB Chip RAM",
	"ICS Denise without EHB support",
	"256 KB Chip RAM",
	"A1000 Velvet prototype",
	nullptr,
};

#ifdef WITH_CPUBOARD
const char* const a1200_configs[] = {
	"Basic non-expanded configuration",
	"4 MB Fast RAM expanded configuration",
	"Blizzard 1230 IV",
	"Blizzard 1240",
	"Blizzard 1260",
#ifdef WITH_PPC
	"Blizzard PPC",
#endif
	nullptr,
};
#else
const char* const a1200_configs[] = {
	"Basic non-expanded configuration",
	"4 MB Fast RAM expanded configuration",
	"8 MB Fast RAM expanded configuration",
	nullptr,
};
#endif

const char* const a3000_configs[] = {
	"1.4 ROM, 2MB Chip + 8MB Fast",
	"2.04 ROM, 2MB Chip + 8MB Fast",
	"3.1 ROM, 2MB Chip + 8MB Fast",
	nullptr,
};

const char* const a4000_configs[] = {
	"68030, 3.1 ROM, 2MB Chip + 8MB Fast",
	"68040, 3.1 ROM, 2MB Chip + 8MB Fast",
#ifdef WITH_PPC
	"CyberStorm PPC",
#endif
	nullptr,
};

const char* const a4000t_configs[] = {
	"68030, 3.1 ROM, 2MB Chip + 8MB Fast",
	"68040, 3.1 ROM, 2MB Chip + 8MB Fast",
	nullptr,
};

const char* const cd32_configs[] = {
	"CD32",
	"CD32 with Full Motion Video cartridge",
	"Cubo CD32",
	"CD32, 8MB Fast",
	nullptr,
};

const char* const cdtv_configs[] = {
	"CDTV",
	"Floppy drive and 64KB SRAM card expanded",
	"CDTV-CR",
	nullptr,
};

const char* const alg_configs[] = {
	nullptr,
};

const char* const arcadia_configs[] = {
	nullptr,
};

const char* const macrosystem_configs[] = {
	"DraCo",
	"Casablanca",
	nullptr,
};

// The compatibility slider. Models routed to set_68020_compa (A1200=4,
// CD32=8) support level 4 (JIT); the 68000 models top out at level 3 (see
// set_68000_compa in cfgfile.cpp). The old amodels compalevels column
// disagreed with this and with the engine (A3000 claimed 2, A4000 1) and
// is gone; these are the levels the engine actually implements.
int compatibility_max(int model)
{
	return model == 4 || model == 8 ? 4 : 3;
}

const preset_model presets[] = {
	{{"a500", "Amiga 500", 0, 0}, 0, a500_configs},
	{{"a500plus", "Amiga 500+", 0, 0}, 1, a500plus_configs},
	{{"a600", "Amiga 600", 0, 0}, 2, a600_configs},
	{{"a1000", "Amiga 1000", 0, 0}, 3, a1000_configs},
	{{"a1200", "Amiga 1200", 0, 0}, 4, a1200_configs},
	{{"a3000", "Amiga 3000", 0, 0}, 5, a3000_configs},
	{{"a4000", "Amiga 4000", 0, 0}, 6, a4000_configs},
	{{"a4000t", "Amiga 4000T", 0, 0}, 7, a4000t_configs},
	{{"cd32", "CD32", 0, 0}, 8, cd32_configs},
	{{"cdtv", "CDTV", 0, 0}, 9, cdtv_configs},
	{{"alg", "American Laser Games / Picmatic", 0, 0}, 10, alg_configs},
	{{"arcadia", "Arcadia Multi Select system", 0, 0}, 11, arcadia_configs},
	{{"macrosystem", "Macrosystem", 0, 0}, 12, macrosystem_configs},
};

constexpr size_t preset_count = sizeof presets / sizeof presets[0];

const char* const compatibility_labels[] = {
	"Best compatibility (cycle-exact)",
	"Good compatibility (approximate)",
	"Less compatible (faster CPU)",
	"Low compatibility (fastest CPU)",
	"Low compatibility (JIT)",
};

constexpr size_t compatibility_count =
	sizeof compatibility_labels / sizeof compatibility_labels[0];

const preset_model* find_preset(std::string_view id)
{
	for (const preset_model& p : presets) {
		if (id == p.preset.id)
			return &p;
	}
	return nullptr;
}

const preset_model* preset_by_model(int model)
{
	for (const preset_model& p : presets) {
		if (p.model == model)
			return &p;
	}
	return nullptr;
}

int config_count_of(const preset_model& p)
{
	int count = 0;
	while (p.configs[count])
		count++;
	return count;
}

// Completed preset descriptors, built on first use so the table above stays
// declarative: the config counts come from the label lists, the
// compatibility maximum from the model's engine tier.
const std::vector<amiberry_settings_preset>& completed_presets()
{
	static const std::vector<amiberry_settings_preset> completed = [] {
		std::vector<amiberry_settings_preset> list;
		list.reserve(preset_count);
		for (const preset_model& p : presets) {
			amiberry_settings_preset item = p.preset;
			item.config_count = config_count_of(p);
			item.compatibility_max = compatibility_max(p.model);
			list.push_back(item);
		}
		return list;
	}();
	return completed;
}

// Every field the presets write that the CPU, chipset, memory and ROM
// domains do not already commit, so a preset applied to a session survives
// commit. uae_prefs is never freed field by field, so whole-struct commits
// are safe.
bool commit_machine(const uae_prefs& base, const uae_prefs& from, uae_prefs& to)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(floppyslots);
	FIELD(nr_floppies);
	FIELD(floppy_speed);
	FIELD(produce_sound);
	FIELD(sound_filter);
	FIELD(sound_filter_type);
	FIELD(sound_volume_cd);
	FIELD(scsi);
	FIELD(uaeserial);
	FIELD(socket_emu);
	FIELD(clipboard_sharing);
	FIELD(cpu060_revision);
	FIELD(fpu_revision);
	FIELD(turbo_emulation);
	FIELD(turbo_emulation_limit);
	FIELD(catweasel);
	FIELD(tod_hack);
	FIELD(cpuboard_type);
	FIELD(cpuboard_subtype);
	FIELD(cpuboard_settings);
	FIELD(cpuboardmem2);
	FIELD(rtgboards);
	FIELD(expansionboard);
	FIELD(cdslots);
	FIELD(jports);
	FIELD(gfx_monitor);
	FIELD(picasso96_nocustom);
	FIELD(cart_internal);
	FIELD(ne2000pciname);
	FIELD(ne2000pcmcianame);
	FIELD(a2065name);
	FIELD(zz9000net_name);
	FIELD(zz9000_int2);
	FIELD(prtname);
	FIELD(sername);
	FIELD(mountitems);
	FIELD(debug_mem);
	FIELD(ntscmode); // the ALG preset switches TV standard per game
#undef FIELD
	return wrote;
}

const domain machine = {
	nullptr, 0, commit_machine,
};

} // namespace

// The Quickstart constraints that follow built_in_prefs(), moved verbatim
// from Quickstart_ApplyDefaults (src/osdep/imgui/quickstart.cpp).
void apply_preset(uae_prefs& p, int model, int config, int compatibility)
{
	const int max_compa = compatibility_max(model);
	if (compatibility < 0)
		compatibility = 0;
	if (compatibility > max_compa)
		compatibility = max_compa;

	built_in_prefs(&p, model, config, compatibility, 0);

	// Enforce constraints similar to WinUAE
	if (model <= 4) { // A500, A500+, A600, A1000, A1200
		// Force DF0 to DD if it was set to HD
		if (p.floppyslots[0].dfxtype == DRV_35_HD)
			p.floppyslots[0].dfxtype = DRV_35_DD;
	}

	switch (model) {
	case 0: // A500
	case 1: // A500+
	case 2: // A600
	case 3: // A1000
	case 4: // A1200
	case 5: // A3000
		// df0 always active
		// Ensure type is DD for A500-A1200 (Cases 0-4) handled above,
		// A3000 (Case 5) can have HD, so we don't force it to DD here.
		if (p.floppyslots[0].dfxtype == DRV_NONE)
			p.floppyslots[0].dfxtype = DRV_35_DD;

		p.floppyslots[1].dfxtype = DRV_NONE;

		// No CD available
		p.cdslots[0].inuse = false;
		p.cdslots[0].type = SCSI_UNIT_DISABLED;

		// Set joystick port to Default
		p.jports[1].mode = 0;
		break;
	case 6: // A4000
	case 7: // A4000T
	case 12: // Macrosystem
		// df0 always active
		if (p.floppyslots[0].dfxtype == DRV_NONE)
			p.floppyslots[0].dfxtype = DRV_35_HD;
		p.floppyslots[1].dfxtype = DRV_NONE;

		// No CD available
		p.cdslots[0].inuse = false;
		p.cdslots[0].type = SCSI_UNIT_DISABLED;

		// Set joystick port to Default
		p.jports[1].mode = 0;
		break;

	case 8: // CD32
	case 9: // CDTV
	case 10: // American Laser Games / Picmatic
	case 11: // Arcadia Multi Select system
		// No floppy drive available, CD available
		p.floppyslots[0].dfxtype = DRV_NONE;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = true;
		p.cdslots[0].type = SCSI_UNIT_DEFAULT;
		p.gfx_monitor[0].gfx_size.width = 720;
		p.gfx_monitor[0].gfx_size.height = 568;
		// Set joystick port to CD32 mode
		p.jports[1].mode = 7;
		break;
	default:
		break;
	}
}

int preset_model_index(std::string_view id)
{
	const preset_model* p = find_preset(id);
	return p ? p->model : -1;
}

const char* preset_id(int model)
{
	const preset_model* p = preset_by_model(model);
	return p ? p->preset.id : nullptr;
}

size_t machine_preset_count()
{
	return preset_count;
}

const amiberry_settings_preset* machine_preset_at(size_t index)
{
	if (index >= preset_count)
		return nullptr;
	return &completed_presets()[index];
}

const amiberry_settings_preset* machine_preset_find(const char* id)
{
	if (!id)
		return nullptr;
	const preset_model* p = find_preset(id);
	return p ? &completed_presets()[p - presets] : nullptr;
}

const char* machine_preset_config_label(const amiberry_settings_preset* preset, int config)
{
	for (const preset_model& p : presets) {
		if (std::strcmp(p.preset.id, preset->id) != 0)
			continue;
		if (config < 0 || config >= config_count_of(p))
			return nullptr;
		return p.configs[config];
	}
	return nullptr;
}

const char* machine_preset_compatibility_label(int level)
{
	if (level < 0 || static_cast<size_t>(level) >= compatibility_count)
		return nullptr;
	return compatibility_labels[level];
}

// built_in_prefs() applied to a scratch copy is side-effect free: it only
// writes the copy (configure_rom with romcheck = 0 never raises the
// missing-ROM dialog; get_nvram_path and cpuboard_setboard write the copy),
// so its return value tells whether every ROM the configuration needs is in
// the scanned ROM list.
bool machine_preset_rom_installed(const char* id, int config)
{
	const preset_model* preset = id ? find_preset(id) : nullptr;
	if (!preset || config < 0 || config >= config_count_of(*preset))
		return false;

	// uae_prefs is far too large for the stack.
	auto* scratch = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
	if (!scratch)
		return false;
	std::memcpy(static_cast<void*>(scratch), static_cast<const void*>(&changed_prefs), sizeof(uae_prefs));
	const bool installed = built_in_prefs(scratch, preset->model, config, 1, 0) != 0;
	std::free(scratch);
	return installed;
}

// Applies a preset to a configuration after checking the arguments; returns
// UNKNOWN_ID / INVALID_VALUE / UNAVAILABLE so the public
// amiberry_settings_preset_apply can pass them through.
amiberry_settings_status apply_preset_checked(uae_prefs& p, const char* id, int config,
	int compatibility)
{
	const preset_model* preset = id ? find_preset(id) : nullptr;
	if (!preset)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (config < 0 || config >= config_count_of(*preset))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (compatibility < 0 || compatibility > compatibility_max(preset->model))
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	apply_preset(p, preset->model, config, compatibility);
	return AMIBERRY_SETTINGS_OK;
}

const domain& machine_domain()
{
	return machine;
}

} // namespace amiberry_settings

extern "C" {

size_t amiberry_settings_preset_count(void)
{
	return amiberry_settings::machine_preset_count();
}

const amiberry_settings_preset* amiberry_settings_preset_at(size_t index)
{
	return amiberry_settings::machine_preset_at(index);
}

const amiberry_settings_preset* amiberry_settings_preset_find(const char* id)
{
	return amiberry_settings::machine_preset_find(id);
}

const char* amiberry_settings_preset_config_label(const amiberry_settings_preset* preset,
	int config)
{
	if (!preset)
		return nullptr;
	return amiberry_settings::machine_preset_config_label(preset, config);
}

const char* amiberry_settings_preset_compatibility_label(int level)
{
	return amiberry_settings::machine_preset_compatibility_label(level);
}

bool amiberry_settings_preset_rom_installed(const char* id, int config)
{
	return amiberry_settings::machine_preset_rom_installed(id, config);
}

} // extern "C"
