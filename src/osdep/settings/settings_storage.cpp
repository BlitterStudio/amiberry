// CD and filesystem options: the CD drive's image, its speed, host CD
// automounting and the filesystem metadata options.
//
// cdimage0 is the configuration's value for the first CD unit, in the
// spelling the writer uses (the path, then ",delay:" and the unit type when
// they are not the defaults). Inserting a disc into a running emulation is
// an action (amiberry_actions_insert_cd); this option is what a
// configuration holds, which is also what the CD panels edit.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "blkdev.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

// ------------------------------------------------------------------- CD

// No line in the configuration means no CD drive (""). A drive without a
// disc is written as an empty value, which would read as no drive here,
// so it is reported with the reader's other spelling for it, "empty".
void get_cdimage0(const uae_prefs& p, char* out, size_t size)
{
	const cdslot& cd = p.cdslots[0];
	if (!cd.inuse && !cd.name[0]) {
		std::snprintf(out, size, "%s", "");
		return;
	}
	write_cdimage_entry(&cd, cd.name, out, size);
	if (!out[0])
		std::snprintf(out, size, "%s", "empty");
}

// "" removes the drive the way the CD panels' checkbox does; anything else
// is parsed the way the configuration reader parses it.
void set_cdimage0(uae_prefs& p, const value& v)
{
	cdslot& cd = p.cdslots[0];
	if (v.text.empty()) {
		cd.inuse = false;
		cd.delayed = false;
		cd.type = SCSI_UNIT_DISABLED;
		cd.name[0] = 0;
		return;
	}
	const std::string text(v.text);
	parse_cdimage_entry(text.c_str(), &cd);
}

// The path is stored in a MAX_DPATH field; a longer value is refused
// rather than truncated into another file's name.
bool cdimage_fits(const uae_prefs&, const value& v)
{
	return v.text.size() < MAX_DPATH;
}

const amiberry_setting_choice cd_speed_choices[] = {
	{"100", "Normal"},
	{"0", "Turbo"},
};

void get_cd_speed(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.cd_speed);
}

void set_cd_speed(uae_prefs& p, const value& v)
{
	p.cd_speed = static_cast<int>(v.number);
}

void get_map_cd_drives(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.automount_cddrives);
}

// The SCSI device layer the automounted drives need is turned on by
// target_fixup_options() from the final value, so turning the option on and
// off again leaves it as it was.
void set_map_cd_drives(uae_prefs& p, const value& v)
{
	p.automount_cddrives = v.number != 0;
}

// ------------------------------------------------------------ filesystem

void get_filesys_custom_uaefsdb(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.filesys_custom_uaefsdb);
}

void set_filesys_custom_uaefsdb(uae_prefs& p, const value& v)
{
	p.filesys_custom_uaefsdb = v.number != 0;
}

void get_filesys_no_fsdb(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.filesys_no_uaefsdb);
}

void set_filesys_no_fsdb(uae_prefs& p, const value& v)
{
	p.filesys_no_uaefsdb = v.number != 0;
}

// Mounted volumes keep the metadata mode they were mounted with, so it is
// only changed while the emulation is stopped.
bool not_emulating(const uae_prefs&, const context& ctx)
{
	return !ctx.emulating;
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, 0, 0, 0, def, nullptr, 0, constraint}

const definition definitions[] = {
	{STRING_OPTION("cdimage0", "cd", "CD drive",
			"The CD drive's image or host drive, then optional fields (,image ,ioctl ,delay:). "
			"Empty for no CD drive, \"empty\" for a drive without a disc.",
			AMIBERRY_APPLY_LIVE, "",
			"A change of disc or host drive applies at once; the startup delay (delay:) when the emulation starts."),
		get_cdimage0, set_cdimage0, nullptr, nullptr, nullptr, cdimage_fits, nullptr},
	{CHOICE_OPTION("cd_speed", "cd", "CD speed",
			"Normal CD drive speed, or Turbo to read without the drive's delays.",
			AMIBERRY_APPLY_RESTART, "100", cd_speed_choices,
			"The running emulation keeps the speed it started with."),
		get_cd_speed, set_cd_speed, nullptr, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.map_cd_drives", "cd", "CDFS automount CD/DVD drives",
			"Mount the host's CD and DVD drives in AmigaOS automatically.",
			AMIBERRY_APPLY_RESTART, "false", "The filesystem reads it when the emulation starts."),
		get_map_cd_drives, set_map_cd_drives, nullptr, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("filesys_custom_uaefsdb", "filesystem", "Create .uaem metadata files",
			"Store Amiga file attributes and comments in .uaem files next to the host files.",
			AMIBERRY_APPLY_LIVE, "true", nullptr),
		get_filesys_custom_uaefsdb, set_filesys_custom_uaefsdb, nullptr, nullptr, nullptr, nullptr,
		nullptr},
	{BOOL_OPTION("filesys_no_fsdb", "filesystem", "Disable UAEFSDB",
			"Do not keep the Amiga file attributes the host filesystem cannot store.",
			AMIBERRY_APPLY_RESET, "false", "Only while the emulation is stopped."),
		get_filesys_no_fsdb, set_filesys_no_fsdb, not_emulating, nullptr, nullptr, nullptr, nullptr},
};

bool commit_storage(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(cdslots[0].name);
	FIELD(cdslots[0].inuse);
	FIELD(cdslots[0].delayed);
	FIELD(cdslots[0].type);
	FIELD(cd_speed);
	FIELD(automount_cddrives);
	FIELD(filesys_custom_uaefsdb);
	FIELD(filesys_no_uaefsdb);
#undef FIELD
	return wrote;
}

const domain storage = {
	definitions, sizeof definitions / sizeof definitions[0], commit_storage, nullptr,
};

} // namespace

const domain& storage_domain()
{
	return storage;
}

} // namespace amiberry_settings
