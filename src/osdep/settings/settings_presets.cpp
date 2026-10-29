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
#include "rommgr.h"

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

// The first installed game: see arcade_game_config().
const char* const alg_configs[] = {
	"First installed game ROM",
	nullptr,
};

const char* const arcadia_configs[] = {
	"First installed game ROM",
	nullptr,
};

const char* const macrosystem_configs[] = {
	"DraCo",
	"Casablanca",
	nullptr,
};

// The compatibility slider: the levels each builder in cfgfile.cpp actually
// implements. A1200 and CD32 go through set_68020_compa (up to 4, the JIT),
// the 68000 machines (and ALG/Arcadia) through set_68000_compa (up to 3),
// bip_a3000 only tells cycle-exact (0) from the rest, and the A4000,
// A4000T and Macrosystem builders ignore the level.
int compatibility_max(int model)
{
	switch (model) {
	case 4: // A1200
	case 8: // CD32
		return 4;
	case 5: // A3000
		return 1;
	case 6: // A4000
	case 7: // A4000T
	case 12: // Macrosystem
		return 0;
	default:
		return 3;
	}
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

} // namespace

bool is_arcade_model(int model)
{
	return model == 10 || model == 11;
}

int arcade_game_config(int model)
{
	if (!is_arcade_model(model))
		return -1;
	romlist** list = getarcadiaroms(model == 10 ? 1 : 0);
	int found = -1;
	for (int i = 0; list && list[i]; i++) {
		const int type = list[i]->rd->type;
		const bool game = model == 10 ? type == ROMTYPE_ALG : type == ROMTYPE_ARCADIAGAME;
		if (game && getromlistbyromdata(list[i]->rd)) {
			found = i;
			break;
		}
	}
	xfree(list);
	return found;
}

// The Quickstart constraints that follow built_in_prefs(), moved verbatim
// from Quickstart_ApplyDefaults (src/osdep/imgui/quickstart.cpp).
void apply_preset(uae_prefs& p, int model, int config, int compatibility, bool preview)
{
	const int max_compa = compatibility_max(model);
	if (compatibility < 0)
		compatibility = 0;
	if (compatibility > max_compa)
		compatibility = max_compa;
	// The arcade boards' one configuration is their first installed game;
	// with none, the builder loads no game (no index matches -1).
	if (is_arcade_model(model))
		config = arcade_game_config(model);

	if (preview)
		built_in_prefs_preview(&p, model, config, compatibility);
	else
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

// built_in_prefs_preview() only writes the copy it is given: configure_rom
// with romcheck = 0 never raises the missing-ROM dialog, get_nvram_path and
// cpuboard_setboard write the copy, and the target defaults leave the
// global drawing, WHDLoad and FloppyBridge state alone. Its return value
// tells whether every ROM the configuration needs is in the scanned list,
// except the ALG/Arcadia game itself: their builders succeed with only the
// BIOS, so a game must be installed too (arcade_game_config).
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
	const bool arcade = is_arcade_model(preset->model);
	const int game = arcade ? arcade_game_config(preset->model) : config;
	bool installed = (!arcade || game >= 0) && built_in_prefs_preview(scratch, preset->model, game, 1) != 0;
	std::free(scratch);
	return installed;
}

amiberry_settings_status check_preset(const char* id, int config, int compatibility, int& model)
{
	const preset_model* preset = id ? find_preset(id) : nullptr;
	if (!preset)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (config < 0 || config >= config_count_of(*preset))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (compatibility < 0 || compatibility > compatibility_max(preset->model))
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	model = preset->model;
	return AMIBERRY_SETTINGS_OK;
}

} // namespace amiberry_settings

extern "C" {

// The descriptors are built on first use (a std::vector) and the ROM check
// runs core code that uses std::string; nothing may throw across the C API.
size_t amiberry_settings_preset_count(void)
{
	return amiberry_settings::machine_preset_count();
}

const amiberry_settings_preset* amiberry_settings_preset_at(size_t index)
{
	return amiberry_settings::no_throw<const amiberry_settings_preset*>(nullptr, [index] {
		return amiberry_settings::machine_preset_at(index);
	});
}

const amiberry_settings_preset* amiberry_settings_preset_find(const char* id)
{
	return amiberry_settings::no_throw<const amiberry_settings_preset*>(nullptr, [id] {
		return amiberry_settings::machine_preset_find(id);
	});
}

const char* amiberry_settings_preset_config_label(const amiberry_settings_preset* preset,
	int config)
{
	if (!preset || !preset->id)
		return nullptr;
	return amiberry_settings::machine_preset_config_label(preset, config);
}

const char* amiberry_settings_preset_compatibility_label(int level)
{
	return amiberry_settings::machine_preset_compatibility_label(level);
}

bool amiberry_settings_preset_rom_installed(const char* id, int config)
{
	return amiberry_settings::no_throw(false, [id, config] {
		return amiberry_settings::machine_preset_rom_installed(id, config);
	});
}

} // extern "C"
