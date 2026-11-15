#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "autoconf.h"
#include "cpuboard.h"
#include "filesys.h"
#include "rommgr.h"
#include "expansion_display_name.h"

#include "settings_definitions.h"

#include <cstring>
#include <utility>
#include <string>
#include <vector>

namespace amiberry_settings {
namespace {

std::string controller_label(const expansionromtype& erc, const uae_prefs* p)
{
	const TCHAR* const display_name = expansion_display_name(erc.friendlyname, erc.friendlymanufacturer);
	std::string label;
	if (erc.friendlymanufacturer && _tcsicmp(erc.friendlymanufacturer, display_name)) {
		label += erc.friendlymanufacturer;
		label += ' ';
	}
	label += display_name;
	if (p && p->cpuboard_type && erc.romtype == ROMTYPE_CPUBOARD) {
		const cpuboardsubtype& subtype = cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype];
		label += " (";
		label += subtype.name;
		label += ')';
	}
	return label;
}

std::vector<std::string> accelerator_ids(const expansionromtype& erc, int flags)
{
	std::vector<std::string> ids;
	if (erc.romtype != ROMTYPE_CPUBOARD)
		return ids;
	for (int type = 1; cpuboards[type].name; type++) {
		const cpuboardsubtype* subtypes = cpuboards[type].subtypes;
		for (int subtype = 0; subtypes && subtypes[subtype].name; subtype++) {
			if ((subtypes[subtype].deviceflags & flags) && subtypes[subtype].configname)
				ids.emplace_back(subtypes[subtype].configname);
		}
	}
	return ids;
}

bool controller_enabled(const uae_prefs* p, const expansionromtype& erc, int flags)
{
	if (!p)
		return true;
	if (erc.romtype == ROMTYPE_CPUBOARD) {
		if (!p->cpuboard_type)
			return false;
		const cpuboardsubtype& subtype = cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype];
		if (!(subtype.deviceflags & flags))
			return false;
	}
	return get_boardromconfig(const_cast<uae_prefs*>(p), erc.romtype, nullptr)
		|| get_boardromconfig(const_cast<uae_prefs*>(p), erc.romtype_extra, nullptr);
}

void add_units(mount_controller& controller, const expansionromtype* erc)
{
	if (controller.type == HD_CONTROLLER_TYPE_UAE) {
		for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++)
			controller.units.push_back({i, std::to_string(i), controller.kinds});
		return;
	}
	if (controller.type >= HD_CONTROLLER_TYPE_IDE_FIRST && controller.type <= HD_CONTROLLER_TYPE_IDE_LAST) {
		// Master and slave per port, as inithdcontroller() lists them.
		const int ports = 2 + (erc ? erc->extrahdports : 0);
		for (int i = 0; i < ports; i += 2) {
			controller.units.push_back({i, std::to_string(i), controller.kinds});
			controller.units.push_back({i + 1, std::to_string(i + 1), controller.kinds});
		}
		return;
	}
	if (controller.type >= HD_CONTROLLER_TYPE_SCSI_FIRST && controller.type <= HD_CONTROLLER_TYPE_SCSI_LAST) {
		controller.units.push_back({0, "0", controller.kinds});
		controller.units.push_back({1, "1", controller.kinds});
		if (!erc || !(erc->deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM))) {
			for (int i = 2; i < 8; i++)
				controller.units.push_back({i, std::to_string(i), controller.kinds});
			if (erc && !_tcscmp(erc->name, _T("a2091")))
				controller.units.push_back({8, "XT", MOUNT_KIND_HDF});
			if (erc && !_tcscmp(erc->name, _T("a2090a"))) {
				controller.units.push_back({8, "ST-506 #1", MOUNT_KIND_HDF});
				controller.units.push_back({9, "ST-506 #2", MOUNT_KIND_HDF});
			}
		}
		return;
	}
	// A custom disk controller has one drive, unit 0 (the panel offers no
	// choice).
	controller.units.push_back({0, "0", controller.kinds});
}

void add_expansion(std::vector<mount_controller>& out, const uae_prefs* p, const expansionromtype& erc,
	int index, int type, int flags, unsigned int kinds)
{
	if (!controller_enabled(p, erc, flags))
		return;
	const std::vector<std::string> accelerators = accelerator_ids(erc, flags);
	for (int unit = 0; unit < (erc.singleonly ? 1 : MAX_DUPLICATE_EXPANSION_BOARDS); unit++) {
		if (p && unit && !is_board_enabled(const_cast<uae_prefs*>(p), erc.romtype, unit))
			continue;
		mount_controller controller{};
		controller.type = type + index;
		controller.type_unit = unit;
		// The configuration file's spelling, so the id is what a file says.
		TCHAR id[MAX_DPATH];
		cfgfile_controller_template(controller.type, unit, id, sizeof id / sizeof id[0]);
		controller.id = id;
		controller.label = controller_label(erc, p);
		if (unit)
			controller.label += " [" + std::to_string(unit + 1) + "]";
		if (accelerators.empty()) {
			controller.board = erc.name;
			controller.board_unit = unit + 1;
		} else {
			controller.accelerators = accelerators;
		}
		controller.kinds = kinds;
		add_units(controller, &erc);
		out.push_back(std::move(controller));
	}
}

void add_expansions(std::vector<mount_controller>& out, const uae_prefs* p, int flags,
	int first_type, unsigned int kinds, bool first_only = false)
{
	for (int i = 0; expansionroms[i].name; i++) {
		const expansionromtype& erc = expansionroms[i];
		if (!(erc.deviceflags & flags))
			continue;
		add_expansion(out, p, erc, i, first_type, flags, kinds);
		if (first_only)
			return;
	}
}

} // namespace

std::vector<mount_controller> mount_controllers(const uae_prefs* p)
{
	std::vector<mount_controller> controllers;
	controllers.push_back({HD_CONTROLLER_TYPE_UAE, 0, "uae%d", "UAE (uaehf.device)", {}, 0, {},
		MOUNT_KIND_HDF | MOUNT_KIND_TAPE, {}});
	add_units(controllers.back(), nullptr);
	controllers.push_back({HD_CONTROLLER_TYPE_IDE_AUTO, 0, "ide%d", "IDE (Auto)", {}, 0, {},
		MOUNT_KIND_HDF | MOUNT_KIND_CD | MOUNT_KIND_TAPE, {}});
	add_units(controllers.back(), nullptr);
	add_expansions(controllers, p, EXPANSIONTYPE_IDE, HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST,
		MOUNT_KIND_HDF | MOUNT_KIND_CD | MOUNT_KIND_TAPE);
	controllers.push_back({HD_CONTROLLER_TYPE_SCSI_AUTO, 0, "scsi%d", "SCSI (Auto)", {}, 0, {},
		MOUNT_KIND_HDF | MOUNT_KIND_CD | MOUNT_KIND_TAPE, {}});
	add_units(controllers.back(), nullptr);
	add_expansions(controllers, p, EXPANSIONTYPE_SCSI, HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST,
		MOUNT_KIND_HDF | MOUNT_KIND_CD | MOUNT_KIND_TAPE);
	add_expansions(controllers, p, EXPANSIONTYPE_CUSTOMDISK, HD_CONTROLLER_TYPE_CUSTOM_FIRST,
		MOUNT_KIND_HDF, true);
	return controllers;
}

bool mount_rdb_geometry(const char* path, uaedev_config_info& ci)
{
	uae_u8 id[512]{};
	hardfiledata hfd{};
	hfd.ci.readonly = true;
	hfd.ci.blocksize = 512;
	if (!path || hdf_open(&hfd, path) <= 0)
		return false;
	bool found = false;
	for (int i = 0; i < 16; i++) {
		hdf_read_rdb(&hfd, id, i * 512, 512);
		const bool babe = id[0] == 0xBA && id[1] == 0xBE;
		if (!std::memcmp(id, "RDSK\0\0\0", 7) || !std::memcmp(id, "CDSK\0\0\0", 7)
			|| !std::memcmp(id, "DRKS\0\0", 6)
			|| (id[0] == 0x53 && id[1] == 0x10 && id[2] == 0x9b && id[3] == 0x13 && id[4] == 0 && id[5] == 0)
			|| babe) {
			const int blocksize = babe ? 512 : (id[16] << 24) | (id[17] << 16) | (id[18] << 8) | id[19];
			ci.cyls = ci.highcyl = 0;
			ci.sectors = ci.surfaces = ci.reserved = 0;
			ci.bootpri = 0;
			if (blocksize >= 512)
				ci.blocksize = blocksize;
			found = true;
			break;
		}
	}
	hdf_close(&hfd);
	return found;
}

} // namespace amiberry_settings
