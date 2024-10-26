#include <cstdio>
#include <strings.h>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>

#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "sysdeps.h"
#include "uae.h"
#include "options.h"
#include "keybuf.h"
#include "zfile.h"
#include "gui.h"
#include "osdep/gui/SelectorEntry.hpp"
#include "gui/gui_handling.h"
#include "rommgr.h"
#include "custom.h"
#include "inputdevice.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "savestate.h"
#include "filesys.h"
#include "blkdev.h"
#include "memory.h"
#include "amiberry_gfx.h"
#ifdef ARCADIA
#include "arcadia.h"
#endif
#include "autoconf.h"
#include "disk.h"
#include "xwin.h"
#include "drawing.h"
#include "fsdb.h"
#include "gayle.h"
#include "parser.h"
#include "scsi.h"
#include "target.h"

#ifdef AMIBERRY
#ifndef __MACH__
#include <linux/kd.h>
#endif
#include <sys/ioctl.h>
#endif

int emulating = 0;
bool config_loaded = false;
int gui_active;

std::vector<std::string> serial_ports;
std::vector<std::string> midi_in_ports;
std::vector<std::string> midi_out_ports;

std::vector<controller_map> controller;
std::vector<string> controller_unit;
std::vector<string> controller_type;
std::vector<string> controller_feature_level;

struct gui_msg
{
	int num;
	const char* msg;
};

struct gui_msg gui_msglist[] = {
  { NUMSG_NEEDEXT2,       "The software uses a non-standard floppy disk format. You may need to use a custom floppy disk image file instead of a standard one. This message will not appear again." },
  { NUMSG_NOROM,          "Could not load system ROM, trying AROS ROM replacement." },
  { NUMSG_NOROMKEY,       "Could not find system ROM key file." },
  { NUMSG_KSROMCRCERROR,  "System ROM checksum incorrect. The system ROM image file may be corrupt." },
  { NUMSG_KSROMREADERROR, "Error while reading system ROM." },
  { NUMSG_NOEXTROM,       "No extended ROM found." },
  { NUMSG_KS68EC020,      "The selected system ROM requires a 68EC020 or later CPU." },
  { NUMSG_KS68020,        "The selected system ROM requires a 68020 or later CPU." },
  { NUMSG_KS68030,        "The selected system ROM requires a 68030 CPU." },
  { NUMSG_STATEHD,        "WARNING: Current configuration is not fully compatible with state saves." },
  { NUMSG_KICKREP,        "You need to have a floppy disk (image file) in DF0: to use the system ROM replacement." },
  { NUMSG_KICKREPNO,      "The floppy disk (image file) in DF0: is not compatible with the system ROM replacement functionality." },
  { NUMSG_ROMNEED,        "One of the following system ROMs is required:\n\n%s\n\nCheck the System ROM path in the Paths panel and click Rescan ROMs." },
  { NUMSG_EXPROMNEED,     "One of the following expansion boot ROMs is required:\n\n%s\n\nCheck the System ROM path in the Paths panel and click Rescan ROMs." },
  { NUMSG_NOMEMORY,       "Out of memory or too much Z3 autoconfig space configured." },
  { NUMSG_NOCAPS,         "capsimg.so not found. CAPS/IPF support not available." },
  { NUMSG_OLDCAPS,        "Old version of capsimg.so found." },

  { -1, "" }
};

std::vector<ConfigFileInfo*> ConfigFilesList;
std::vector<AvailableROM*> lstAvailableROMs;
std::vector<std::string> lstMRUDiskList;
std::vector<std::string> lstMRUCDList;
std::vector<std::string> lstMRUWhdloadList;

std::string get_full_path_from_disk_list(std::string element)
{
	if (element.empty())
		return element;
	const std::string first = element.substr(element.find_first_of('{') + 2);
	std::string full_path = first.substr(0, first.find_last_of('}') - 1);
	return full_path;
}

void add_file_to_mru_list(std::vector<std::string>& vec, const std::string& file)
{
	// Check if the string already exists in the vector
	if (std::find(vec.begin(), vec.end(), file) == vec.end()) {
		// The string does not exist in the vector, so add it at the first position
		vec.insert(vec.begin(), file);
	}

	while (vec.size() > MAX_MRU_LIST)
		vec.pop_back();
}

void ClearAvailableROMList()
{
	for (const auto* rom : lstAvailableROMs)
	{
		delete rom;
	}
	lstAvailableROMs.clear();
}

static int addrom(struct romdata* rd, const char* path)
{
	char tmpName[MAX_DPATH];
	auto* const tmp = new AvailableROM();
	getromname(rd, tmpName);
	tmp->Name.assign(tmpName);
	if (path != nullptr)
		tmp->Path.assign(path);
	tmp->ROMType = rd->type;
	lstAvailableROMs.emplace_back(tmp);
	romlist_add(path, rd);
	return 1;
}

struct romscandata
{
	uae_u8* keybuf;
	int keysize;
};

static struct romdata* scan_single_rom_2(struct zfile* f)
{
	uae_u8 buffer[20] = {0};
	auto cl = 0;
	struct romdata* rd = nullptr;

	zfile_fseek(f, 0, SEEK_END);
	int size = zfile_ftell32(f);
	zfile_fseek(f, 0, SEEK_SET);
	if (size > 524288 * 2) /* don't skip KICK disks or 1M ROMs */
		return nullptr;
	zfile_fread(buffer, 1, 11, f);
	if (!memcmp(buffer, "KICK", 4))
	{
		zfile_fseek(f, 512, SEEK_SET);
		if (size > 262144)
			size = 262144;
	}
	else if (!memcmp(buffer, "AMIROMTYPE1", 11))
	{
		cl = 1;
		size -= 11;
	}
	else
	{
		zfile_fseek(f, 0, SEEK_SET);
	}
	auto* rombuf = xcalloc(uae_u8, size);
	if (!rombuf)
		return nullptr;
	zfile_fread(rombuf, 1, size, f);
	if (cl > 0)
	{
		decode_cloanto_rom_do(rombuf, size, size);
		cl = 0;
	}
	if (!cl)
	{
		rd = getromdatabydata(rombuf, size);
		if (!rd && (size & 65535) == 0)
		{
			for (auto i = 0; i < size; i += 2)
			{
				uae_u8 b = rombuf[i];
				rombuf[i] = rombuf[i + 1];
				rombuf[i + 1] = b;
			}
			rd = getromdatabydata(rombuf, size);
		}
	}
	free(rombuf);
	return rd;
}

struct romdata *scan_single_rom (const TCHAR *path)
{
	struct zfile *z;
	TCHAR tmp[MAX_DPATH];
	struct romdata *rd;

	_tcscpy (tmp, path);
	rd = scan_arcadia_rom (tmp, 0);
	if (rd)
		return rd;
	rd = getromdatabypath (path);
	if (rd && rd->crc32 == 0xffffffff)
		return rd;
	z = zfile_fopen (path, _T("rb"), ZFD_NORMAL);
	if (!z)
		return 0;
	return scan_single_rom_2 (z);
}

static int isromext(const std::string& path)
{
	if (path.empty())
		return 0;
	const auto ext_pos = path.find_last_of('.');
	if (ext_pos == std::string::npos)
		return 0;
	const std::string ext = path.substr(ext_pos + 1);

	static const std::vector<std::string> extensions = { "rom", "bin", "adf", "key", "a500", "a1200", "a4000", "cdtv", "cd32" };
	if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end())
		return 1;

	if (ext.size() >= 2 && std::toupper(ext[0]) == 'U' && std::isdigit(ext[1]))
		return 1;

	for (auto i = 0; uae_archive_extensions[i]; i++)
	{
		if (strcasecmp(ext.c_str(), uae_archive_extensions[i]) == 0)
			return 1;
	}
	return 0;
}

static int scan_rom_2(struct zfile* f, void* dummy)
{
	auto* const path = zfile_getname(f);

	if (!isromext(path))
		return 0;
	auto* const rd = scan_single_rom_2(f);
	if (rd)
		addrom(rd, path);
	return 0;
}

static void scan_rom(const std::string& path)
{
	struct romdata* rd;
	int cnt = 0;

	if (!isromext(path)) {
		//write_log("ROMSCAN: skipping file '%s', unknown extension\n", path);
		return;
	}
#ifdef ARCADIA
	for (;;) {
		TCHAR tmp[MAX_DPATH];
		_tcscpy(tmp, path.c_str());
		rd = scan_arcadia_rom(tmp, cnt++);
		if (rd) {
			if (!addrom(rd, tmp))
				return;
			continue;
		}
		break;
	}
#endif
	zfile_zopen(path, scan_rom_2, nullptr);
}

void SymlinkROMs()
{
	symlink_roms(&changed_prefs);
}

void RescanROMs()
{
	std::vector<std::string> dirs;
	std::vector<std::string> files;
	char path[MAX_DPATH];

	romlist_clear();
	ClearAvailableROMList();
	get_rom_path(path, MAX_DPATH);

	load_keyring(&changed_prefs, path);
	read_directory(path, &dirs, &files);

	// Root level scan
	for (const auto& file : files)
	{
		scan_rom(std::string(path) + file);
	}

	// Recursive scan
	for (const auto& dir : dirs)
	{
		if (dir != "..")
		{
			std::string full_path = std::string(path) + dir;
			read_directory(full_path, nullptr, &files);
			for (const auto& file : files)
			{
				scan_rom(full_path + "/" + file);
			}
		}
	}

	for (int id = 1;; ++id)
	{
		auto* rd = getromdatabyid(id);
		if (!rd)
			break;
		if (rd->crc32 == 0xffffffff)
		{
			if (strncmp(rd->model, "AROS", 4) == 0)
				addrom(rd, ":AROS");
			else if (rd->id == 63)
				addrom(rd, ":HRTMon");
		}
	}
}

static void ClearConfigFileList()
{
	for (const auto* config : ConfigFilesList)
	{
		delete config;
	}
	ConfigFilesList.clear();
}

void ReadConfigFileList(void)
{
	char path[MAX_DPATH];
	std::vector<std::string> files;
	const char *filter_rp9[] = { ".rp9", "\0" };
	const char *filter_uae[] = { ".uae", "\0" };

	ClearConfigFileList();

	// Read rp9 files
	get_rp9_path(path, MAX_DPATH);
	read_directory(path, nullptr, &files);
	FilterFiles(&files, filter_rp9);
	for (auto & file : files)
	{
		auto* tmp = new ConfigFileInfo();
		strncpy(tmp->FullPath, path, MAX_DPATH - 1);
		strncat(tmp->FullPath, file.c_str(), MAX_DPATH - 1);
		strncpy(tmp->Name, file.c_str(), MAX_DPATH - 1);
		remove_file_extension(tmp->Name);
		strncpy(tmp->Description, _T("rp9"), MAX_DPATH - 1);
		ConfigFilesList.emplace_back(tmp);
	}

	// Read standard config files
	get_configuration_path(path, MAX_DPATH);
	read_directory(path, nullptr, &files);
	FilterFiles(&files, filter_uae);
	for (auto & file : files)
	{
		auto* tmp = new ConfigFileInfo();
		strncpy(tmp->FullPath, path, MAX_DPATH - 1);
		strncat(tmp->FullPath, file.c_str(), MAX_DPATH - 1);
		strncpy(tmp->Name, file.c_str(), MAX_DPATH - 1);
		remove_file_extension(tmp->Name);
		// If the user has many (thousands) of configs, this will take a long time
		if (amiberry_options.read_config_descriptions)
		{
			auto p = cfgfile_open(tmp->FullPath, NULL);
			if (p) {
				cfgfile_get_description(p, NULL, tmp->Description, NULL, NULL, NULL, NULL, NULL);
				cfgfile_close(p);
			}
		}
		ConfigFilesList.emplace_back(tmp);
	}
}

ConfigFileInfo* SearchConfigInList(const char* name)
{
	for (const auto & i : ConfigFilesList)
	{
		if (!SDL_strncasecmp(i->Name, name, MAX_DPATH))
			return i;
	}
	return nullptr;
}

void disk_selection(const int drive, uae_prefs* prefs)
{
	std::string tmp;

	if (strlen(prefs->floppyslots[drive].df) > 0)
		tmp = std::string(prefs->floppyslots[drive].df);
	else
		tmp = current_dir;
	tmp = SelectFile("Select disk image file", tmp, diskfile_filter);
	if (!tmp.empty())
	{
		if (strncmp(prefs->floppyslots[drive].df, tmp.c_str(), MAX_DPATH) != 0)
		{
			strncpy(prefs->floppyslots[drive].df, tmp.c_str(), MAX_DPATH);
			disk_insert(drive, tmp.c_str());
			add_file_to_mru_list(lstMRUDiskList, tmp);
			current_dir = extract_path(tmp);
		}
	}
}

bool gui_ask_disk(int drv, TCHAR *name)
{
	_tcscpy(changed_prefs.floppyslots[drv].df, name);
	disk_selection(drv, &changed_prefs);
	_tcscpy(name, changed_prefs.floppyslots[drv].df);
	return true;
}

static void prefs_to_gui()
{
	//
	// GUI Theme section
	//
	if (amiberry_options.gui_theme[0])
		load_theme(amiberry_options.gui_theme);
	else
		load_default_theme();

	/* filesys hack */
	changed_prefs.mountitems = currprefs.mountitems;
	memcpy(&changed_prefs.mountconfig, &currprefs.mountconfig, MOUNT_CONFIG_SIZE * sizeof(struct uaedev_config_info));
}

static void gui_to_prefs(void)
{
	if (quit_program == -UAE_RESET_HARD) {
		// copy all if hard reset
		// disabled as this caused bug #1068
		//copy_prefs(&changed_prefs, &currprefs);
		memory_hardreset(2);
	}
	/* filesys hack */
	currprefs.mountitems = changed_prefs.mountitems;
	memcpy(&currprefs.mountconfig, &changed_prefs.mountconfig, MOUNT_CONFIG_SIZE * sizeof(struct uaedev_config_info));

#ifdef AMIBERRY
	if (currprefs.drawbridge_driver != changed_prefs.drawbridge_driver ||
		currprefs.drawbridge_serial_auto != changed_prefs.drawbridge_serial_auto ||
		_tcsicmp(currprefs.drawbridge_serial_port, changed_prefs.drawbridge_serial_port) != 0 ||
		currprefs.drawbridge_smartspeed != changed_prefs.drawbridge_smartspeed ||
		currprefs.drawbridge_autocache != changed_prefs.drawbridge_autocache ||
		currprefs.drawbridge_connected_drive_b != changed_prefs.drawbridge_connected_drive_b)
	{
		currprefs.drawbridge_driver = changed_prefs.drawbridge_driver;
		currprefs.drawbridge_serial_auto = changed_prefs.drawbridge_serial_auto;
		_tcscpy(currprefs.drawbridge_serial_port, changed_prefs.drawbridge_serial_port);
		currprefs.drawbridge_smartspeed = changed_prefs.drawbridge_smartspeed;
		currprefs.drawbridge_autocache = changed_prefs.drawbridge_autocache;
		currprefs.drawbridge_connected_drive_b = changed_prefs.drawbridge_connected_drive_b;
		if (quit_program != UAE_QUIT)
			drawbridge_update_profiles(&currprefs);
	}
#endif

	fixup_prefs(&changed_prefs, true);
	updatewinfsmode(0, &changed_prefs);
}

static void after_leave_gui()
{
	inputdevice_copyconfig(&changed_prefs, &currprefs);
	inputdevice_config_change_test();
}

int gui_init()
{
	emulating = 0;
	auto ret = 0;

	if (lstAvailableROMs.empty())
		RescanROMs();

	prefs_to_gui();
	run_gui();
	gui_to_prefs();

	if (quit_program < 0)
		quit_program = -quit_program;
	if (quit_program == UAE_QUIT)
		ret = -2; // Quit without start of emulator

	inputdevice_acquire (TRUE);

	after_leave_gui();
	emulating = 1;
	return ret;
}

void gui_exit()
{
	sync();
	close_sound();
	save_amiberry_settings();
	ClearConfigFileList();
	ClearAvailableROMList();
}

void gui_purge_events()
{
	keybuf_init();
}

int gui_update()
{
	std::string filename;
	std::string suffix = (current_state_num >= 1 && current_state_num <= 14) ?
		"-" + std::to_string(current_state_num) : "";

	if (strlen(currprefs.floppyslots[0].df) > 0)
		filename = extract_filename(currprefs.floppyslots[0].df);
	else if (currprefs.cdslots[0].inuse && strlen(currprefs.cdslots[0].name) > 0)
		filename = extract_filename(currprefs.cdslots[0].name);
	else
	{
		last_loaded_config[0] != '\0' ? filename = std::string(last_loaded_config) : filename = "default.uae";
	}

	get_savestate_path(savestate_fname, MAX_DPATH - 1);
	strncat(savestate_fname, filename.c_str(), MAX_DPATH - 1);
	remove_file_extension(savestate_fname);
	strncat(savestate_fname, (suffix + ".uss").c_str(), MAX_DPATH - 1);

	screenshot_filename = get_screenshot_path();
	screenshot_filename += filename;
	screenshot_filename = remove_file_extension(screenshot_filename);
	screenshot_filename.append(suffix + ".png");

	return 0;
}

/* if drive is -1, show the full GUI, otherwise file-requester for DF[drive] */
void gui_display(int shortcut)
{
	static int here;

	if (here)
		return;
	here++;
	gui_active++;

	if (setpaused(7)) {
		inputdevice_unacquire();
		//rawinput_release();
		wait_keyrelease();
		clearallkeys();
		setmouseactive(0, 0);
	}

	if (shortcut == -1)
	{
		prefs_to_gui();
		run_gui();
		gui_to_prefs();

		gui_update();
		gui_purge_events();
		gui_active--;
	}
	else if (shortcut >= 0 && shortcut < 4)
	{
		amiberry_gui_init();
		gui_widgets_init();
		disk_selection(shortcut, &changed_prefs);
		gui_widgets_halt();
		amiberry_gui_halt();
	}

	reset_sound();
	inputdevice_copyconfig(&changed_prefs, &currprefs);
	inputdevice_config_change_test();
	clearallkeys ();

	if (resumepaused(7)) {
		inputdevice_acquire(TRUE);
		setmouseactive(0, 1);
	}
	//rawinput_alloc();
	struct AmigaMonitor* mon = &AMonitors[0];
	SDL_SetWindowGrab(mon->amiga_window, SDL_TRUE);
	if (kmsdrm_detected && amiga_surface != nullptr)
	{
		target_graphics_buffer_update(mon->monitor_id, true);
	}
	fpscounter_reset();
	//screenshot_free();
	//write_disk_history();
	gui_active--;
	here--;
}

static void gui_flicker_led2(int led, int unitnum, int status)
{
	static int resetcounter[LED_MAX];
	uae_s8 old;
	uae_s8 *p;

	if (led == LED_HD)
		p = &gui_data.hd;
	else if (led == LED_CD)
		p = &gui_data.cd;
	else if (led == LED_MD)
		p = &gui_data.md;
	else if (led == LED_NET)
		p = &gui_data.net;
	else
		return;
	old = *p;
	if (status < 0) {
		if (old < 0) {
			gui_led(led, -1, -1);
		}
		else {
			gui_led(led, 0, -1);
		}
		return;
	}
	if (status == 0 && old < 0) {
		*p = 0;
		resetcounter[led] = 0;
		gui_led(led, 0, -1);
		return;
	}
	if (status == 0) {
		resetcounter[led]--;
		if (resetcounter[led] > 0)
			return;
	}
#ifdef RETROPLATFORM
	if (unitnum >= 0) {
		if (led == LED_HD) {
			rp_hd_activity(unitnum, status ? 1 : 0, status == 2 ? 1 : 0);
		}
		else if (led == LED_CD) {
			rp_cd_activity(unitnum, status);
		}
	}
#endif
	*p = status;
	resetcounter[led] = 4;
	if (old != *p)
		gui_led(led, *p, -1);
}

void gui_flicker_led(int led, int unitnum, int status)
{
	if (led < 0) {
		gui_flicker_led2(LED_HD, 0, 0);
		gui_flicker_led2(LED_CD, 0, 0);
		if (gui_data.net >= 0)
			gui_flicker_led2(LED_NET, 0, 0);
		if (gui_data.md >= 0)
			gui_flicker_led2(LED_MD, 0, 0);
	}
	else {
		gui_flicker_led2(led, unitnum, status);
	}
}

void gui_fps(int fps, int idle, int color)
{
	gui_data.fps = fps;
	gui_data.idle = idle;
	gui_data.fps_color = color;
	gui_led(LED_FPS, 0, -1);
	gui_led(LED_CPU, 0, -1);
	gui_led(LED_SND, (gui_data.sndbuf_status > 1 || gui_data.sndbuf_status < 0) ? 0 : 1, -1);
}

static int temp_fd = -1;
static int want_temp = 1; // Make this a negative number to disable Temperature reading
#define TEMPERATURE "/sys/class/thermal/thermal_zone0/temp"

void gui_led(int led, int on, int brightness)
{
	unsigned char kbd_led_status;

	// Check current prefs/ update if changed
	if (currprefs.kbd_led_num != changed_prefs.kbd_led_num) currprefs.kbd_led_num = changed_prefs.kbd_led_num;
	if (currprefs.kbd_led_scr != changed_prefs.kbd_led_scr) currprefs.kbd_led_scr = changed_prefs.kbd_led_scr;
	if (currprefs.kbd_led_cap != changed_prefs.kbd_led_cap) currprefs.kbd_led_cap = changed_prefs.kbd_led_cap;

#ifndef __MACH__
	// Temperature sensor initialization
	if (want_temp > 0 && temp_fd < 0) {
		temp_fd = open(TEMPERATURE, O_RDONLY);

		if (temp_fd < 0)
		{
			write_log("TEMPERATURE: Could not open %s for reading, disabling feature\n", TEMPERATURE);
			want_temp = 0;
		}
	}

	ioctl(0, KDGETLED, &kbd_led_status);

	// Handle floppy led status
	if (led >= LED_DF0 && led <= LED_DF3)
	{
		if (currprefs.kbd_led_num == led)
		{
			if (on) kbd_led_status |= LED_NUM;
			
			else kbd_led_status &= ~LED_NUM;
		}
		if (currprefs.kbd_led_scr == led)
		{
			if (on) kbd_led_status |= LED_SCR;
			else kbd_led_status &= ~LED_SCR;
		}
		if (currprefs.kbd_led_cap == led)
		{
			if (on) kbd_led_status |= LED_CAP;
			else kbd_led_status &= ~LED_CAP;
		}
#ifdef USE_GPIOD
		gpiod_line_set_value(lineRed, on);
#endif
	}

	// Handle power, hd/cd led status
	if (led == LED_POWER || led == LED_HD || led == LED_CD)
	{
		if (currprefs.kbd_led_num == led)
		{
			if (on) kbd_led_status |= LED_NUM;
			else kbd_led_status &= ~LED_NUM;
		}
		if (currprefs.kbd_led_scr == led)
		{
			if (on) kbd_led_status |= LED_SCR;
			else kbd_led_status &= ~LED_SCR;
		}
		if (currprefs.kbd_led_cap == led)
		{
			if (on) kbd_led_status |= LED_CAP;
			else kbd_led_status &= ~LED_CAP;
		}
#ifdef USE_GPIOD
		if (led == LED_HD)
			gpiod_line_set_value(lineYellow, on);
		if (led == LED_POWER)
			gpiod_line_set_value(lineGreen, on);
#endif
	}

	ioctl(0, KDSETLED, kbd_led_status);

	// Temperature reading
	if (static unsigned int temp_count = 0; temp_fd >= 0 && ++temp_count % 25 == 0) {
		static char tb[10] = { 0 };
		lseek(temp_fd, 0, SEEK_SET);
		if (int l = read(temp_fd, tb, sizeof tb - 1); l > 0) {
			tb[l] = '\0';
			gui_data.temperature = atoi(tb) / 1000;
		}
	}

#else
	// TODO
#endif
}

void gui_filename(int num, const char* name)
{
}

void gui_message(const char* format, ...)
{
	char msg[2048];
	va_list parms;

	va_start(parms, format);
	vsprintf(msg, format, parms);
	va_end(parms);

	ShowMessage("", msg, "", "", "Ok", "");
}

void notify_user(int msg)
{
	auto i = 0;
	while (gui_msglist[i].num >= 0)
	{
		if (gui_msglist[i].num == msg)
		{
			gui_message(gui_msglist[i].msg);
			break;
		}
		++i;
	}
}

void notify_user_parms(int msg, const TCHAR *parms, ...)
{
	TCHAR msgtxt[MAX_DPATH];
	TCHAR tmp[MAX_DPATH];
	auto c = 0;
	va_list parms2;

	auto i = 0;
	while (gui_msglist[i].num >= 0)
	{
		if (gui_msglist[i].num == msg)
		{
			strncpy(tmp, gui_msglist[i].msg, MAX_DPATH);
			va_start(parms2, parms);
			_vsntprintf(msgtxt, sizeof msgtxt / sizeof(TCHAR), tmp, parms2);
			gui_message(msgtxt);
			va_end(parms2);
			break;
		}
		++i;
	}
}

int translate_message(int msg, TCHAR* out)
{
	auto i = 0;
	while (gui_msglist[i].num >= 0)
	{
		if (gui_msglist[i].num == msg)
		{
			strncpy(out, gui_msglist[i].msg, MAX_DPATH);
			return 1;
		}
		++i;
	}
	return 0;
}


void FilterFiles(vector<string>* files, const char* filter[])
{
	for (auto q = 0; q < files->size(); q++)
	{
		auto tmp = (*files)[q];

		auto remove = true;
		for (auto f = 0; filter[f] != nullptr && strlen(filter[f]) > 0; ++f)
		{
			if (tmp.size() >= strlen(filter[f]))
			{
				if (!stricmp(tmp.substr(tmp.size() - strlen(filter[f])).c_str(), filter[f]))
				{
					remove = false;
					break;
				}
			}
		}

		if (remove)
		{
			files->erase(files->begin() + q);
			--q;
		}
	}
}


bool DevicenameExists(const char* name)
{
	for (auto i = 0; i < MAX_HD_DEVICES; ++i)
	{
		auto* uci = &changed_prefs.mountconfig[i];
		auto* const ci = &uci->ci;

		if (ci->devname[0])
		{
			if (!strcmp(ci->devname, name))
				return true;
			if (!strcmp(ci->volname, name))
				return true;
		}
	}
	return false;
}


void CreateDefaultDevicename(char* name)
{
	auto freeNum = 0;
	auto foundFree = false;

	while (!foundFree && freeNum < 10)
	{
		sprintf(name, "DH%d", freeNum);
		foundFree = !DevicenameExists(name);
		++freeNum;
	}
}


int tweakbootpri(int bp, int ab, int dnm)
{
	if (dnm)
		return BOOTPRI_NOAUTOMOUNT;
	if (!ab)
		return BOOTPRI_NOAUTOBOOT;
	if (bp < -127)
		bp = -127;
	return bp;
}

struct cddlg_vals current_cddlg;
struct tapedlg_vals current_tapedlg;
struct fsvdlg_vals current_fsvdlg;
struct hfdlg_vals current_hfdlg;

void hardfile_testrdb (struct hfdlg_vals* hdf) 
{
	uae_u8 id[512];
	struct hardfiledata hfd{};

	memset(id, 0, sizeof id);
	memset(&hfd, 0, sizeof hfd);
	hfd.ci.readonly = true;
	hfd.ci.blocksize = 512;
	hdf->rdb = 0;
	if (hdf_open(&hfd, hdf->ci.rootdir) > 0) {
		for (auto i = 0; i < 16; i++) {
			hdf_read_rdb(&hfd, id, i * 512, 512);
			auto babe = id[0] == 0xBA && id[1] == 0xBE; // A2090
			if (!memcmp(id, "RDSK\0\0\0", 7) || !memcmp(id, "CDSK\0\0\0", 7) || !memcmp(id, "DRKS\0\0", 6) ||
				(id[0] == 0x53 && id[1] == 0x10 && id[2] == 0x9b && id[3] == 0x13 && id[4] == 0 && id[5] == 0) || babe) {
				// RDSK or ADIDE "encoded" RDSK
				auto blocksize = 512;
				if (!babe)
					blocksize = (id[16] << 24) | (id[17] << 16) | (id[18] << 8) | (id[19] << 0);
				hdf->ci.cyls = hdf->ci.highcyl = hdf->forcedcylinders = 0;
				hdf->ci.sectors = 0;
				hdf->ci.surfaces = 0;
				hdf->ci.reserved = 0;
				hdf->ci.bootpri = 0;
				//hdf->ci.devname[0] = 0;
				if (blocksize >= 512)
					hdf->ci.blocksize = blocksize;
				hdf->rdb = 1;
				break;
			}
		}
		hdf_close(&hfd);
	}
}

void default_fsvdlg(struct fsvdlg_vals* f)
{
	memset(f, 0, sizeof(struct fsvdlg_vals));
	f->ci.type = UAEDEV_DIR;
}
void default_tapedlg(struct tapedlg_vals* f)
{
	memset(f, 0, sizeof(struct tapedlg_vals));
	f->ci.type = UAEDEV_TAPE;
}
void default_hfdlg(struct hfdlg_vals* f, bool rdb)
{
	int ctrl = f->ci.controller_type;
	int unit = f->ci.controller_unit;
	memset(f, 0, sizeof(struct hfdlg_vals));
	uci_set_defaults(&f->ci, rdb);
	f->original = true;
	f->ci.type = UAEDEV_HDF;
	f->ci.controller_type = ctrl;
	f->ci.controller_unit = unit;
	f->ci.unit_feature_level = 1;
}

void default_rdb_hfdlg(struct hfdlg_vals* f, const TCHAR* filename)
{
	default_hfdlg(f, true);
	_tcscpy(current_hfdlg.ci.rootdir, filename);
	hardfile_testrdb(f);
}

void updatehdfinfo(bool force, bool defaults, bool realdrive)
{
	uae_u8 id[512] = { 0 };
	uae_u32 i;
	bool phys = is_hdf_rdb();

	uae_u64 bsize = 0;
	if (force) {
		auto gotrdb = false;
		auto blocksize = 512;
		struct hardfiledata hfd {};
		memset(id, 0, sizeof id);
		memset(&hfd, 0, sizeof hfd);
		hfd.ci.readonly = true;
		hfd.ci.blocksize = blocksize;
		current_hfdlg.size = 0;
		current_hfdlg.dostype = 0;
		if (hdf_open(&hfd, current_hfdlg.ci.rootdir) > 0) {
			for (i = 0; i < 16; i++) {
				hdf_read(&hfd, id, static_cast<uae_u64>(i) * 512, 512);
				bsize = hfd.virtsize;
				current_hfdlg.size = hfd.virtsize;
				if (!memcmp(id, "RDSK", 4) || !memcmp(id, "CDSK", 4)) {
					blocksize = (id[16] << 24) | (id[17] << 16) | (id[18] << 8) | (id[19] << 0);
					gotrdb = true;
					break;
				}
			}
			if (i == 16) {
				hdf_read(&hfd, id, 0, 512);
				current_hfdlg.dostype = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | (id[3] << 0);
			}
		}
		if (defaults) {
			if (blocksize > 512) {
				hfd.ci.blocksize = blocksize;
			}
		}
		if (hfd.ci.chs) {
			current_hfdlg.ci.physical_geometry = true;
			current_hfdlg.ci.chs = true;
			current_hfdlg.ci.pcyls = hfd.ci.pcyls;
			current_hfdlg.ci.pheads = hfd.ci.pheads;
			current_hfdlg.ci.psecs = hfd.ci.psecs;
		}
		if (!current_hfdlg.ci.physical_geometry) {
			if (current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST) {
				getchspgeometry(bsize, &current_hfdlg.ci.pcyls, &current_hfdlg.ci.pheads, &current_hfdlg.ci.psecs, true);
			}
			else {
				getchspgeometry(bsize, &current_hfdlg.ci.pcyls, &current_hfdlg.ci.pheads, &current_hfdlg.ci.psecs, false);
			}
			if (defaults && !gotrdb && !realdrive) {
				gethdfgeometry(bsize, &current_hfdlg.ci);
				phys = false;
			}
		}
		else {
			current_hfdlg.forcedcylinders = current_hfdlg.ci.pcyls;
		}
		hdf_close(&hfd);
	}

	if (current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST) {
		if (current_hfdlg.ci.unit_feature_level == HD_LEVEL_ATA_1 && bsize >= 4 * static_cast<uae_u64>(0x40000000))
			current_hfdlg.ci.unit_feature_level = HD_LEVEL_ATA_2;
	}
}

void new_filesys(int entry)
{
	struct uaedev_config_data* uci;
	struct uaedev_config_info ci;
	memcpy(&ci, &current_fsvdlg.ci, sizeof(struct uaedev_config_info));
	uci = add_filesys_config(&changed_prefs, entry, &ci);
	if (uci) {
		if (uci->ci.rootdir[0])
			filesys_media_change(uci->ci.rootdir, 1, uci);
		else if (uci->configoffset >= 0)
			filesys_eject(uci->configoffset);
	}
}

void new_cddrive(int entry)
{
	struct uaedev_config_info ci = { 0 };
	ci.device_emu_unit = 0;
	ci.controller_type = current_cddlg.ci.controller_type;
	ci.controller_unit = current_cddlg.ci.controller_unit;
	ci.type = UAEDEV_CD;
	ci.readonly = true;
	ci.blocksize = 2048;
	add_filesys_config(&changed_prefs, entry, &ci);
}

void new_tapedrive(int entry)
{
	struct uaedev_config_data* uci;
	struct uaedev_config_info ci = { 0 };
	ci.controller_type = current_tapedlg.ci.controller_type;
	ci.controller_unit = current_tapedlg.ci.controller_unit;
	ci.readonly = current_tapedlg.ci.readonly;
	_tcscpy(ci.rootdir, current_tapedlg.ci.rootdir);
	ci.type = UAEDEV_TAPE;
	ci.blocksize = 512;
	uci = add_filesys_config(&changed_prefs, entry, &ci);
	if (uci && uci->unitnum >= 0) {
		tape_media_change(uci->unitnum, &ci);
	}
}

void new_hardfile(int entry)
{
	struct uaedev_config_data* uci;
	struct uaedev_config_info ci;
	memcpy(&ci, &current_hfdlg.ci, sizeof(struct uaedev_config_info));
	uci = add_filesys_config(&changed_prefs, entry, &ci);
	if (uci) {
		struct hardfiledata* hfd = get_hardfile_data(uci->configoffset);
		if (hfd)
			hardfile_media_change(hfd, &ci, true, false);
		pcmcia_disk_reinsert(&changed_prefs, &uci->ci, false);
	}
}

void new_harddrive(int entry)
{
	struct uaedev_config_data* uci;

	uci = add_filesys_config(&changed_prefs, entry, &current_hfdlg.ci);
	if (uci) {
		struct hardfiledata* hfd = get_hardfile_data(uci->configoffset);
		if (hfd)
			hardfile_media_change(hfd, &current_hfdlg.ci, true, false);
		pcmcia_disk_reinsert(&changed_prefs, &uci->ci, false);
	}
}

void addhdcontroller(const struct expansionromtype* erc, int firstid, int flags)
{
	TCHAR name[MAX_DPATH];
	name[0] = 0;
	if (erc->friendlymanufacturer && _tcsicmp(erc->friendlymanufacturer, erc->friendlyname)) {
		_tcscat(name, erc->friendlymanufacturer);
		_tcscat(name, _T(" "));
	}
	_tcscat(name, erc->friendlyname);
	if (changed_prefs.cpuboard_type && erc->romtype == ROMTYPE_CPUBOARD) {
		const struct cpuboardsubtype* cbt = &cpuboards[changed_prefs.cpuboard_type].subtypes[changed_prefs.cpuboard_subtype];
		if (!(cbt->deviceflags & flags))
			return;
		_tcscat(name, _T(" ("));
		_tcscat(name, cbt->name);
		_tcscat(name, _T(")"));
	}
	if (get_boardromconfig(&changed_prefs, erc->romtype, NULL) || get_boardromconfig(&changed_prefs, erc->romtype_extra, NULL)) {
		std::string name_string = std::string(name);
		controller.push_back({ firstid, name_string });
		for (int j = 1; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
			if (is_board_enabled(&changed_prefs, erc->romtype, j)) {
				TCHAR tmp[MAX_DPATH];
				_stprintf(tmp, _T("%s [%d]"), name, j + 1);
				std::string tmp_string = std::string(tmp);
				controller.push_back({ firstid + j * HD_CONTROLLER_NEXT_UNIT, tmp_string });
			}
		}
	}
}

void inithdcontroller(int ctype, int ctype_unit, int devtype, bool media)
{
	controller.clear();
	controller.push_back({ HD_CONTROLLER_TYPE_UAE, _T("UAE (uaehf.device)") });
	controller.push_back({ HD_CONTROLLER_TYPE_IDE_AUTO, _T("IDE (Auto)") });

	for (auto i = 0; expansionroms[i].name; i++) {
		const auto* const erc = &expansionroms[i];
		if (erc->deviceflags & EXPANSIONTYPE_IDE) {
			addhdcontroller(erc, HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST + i, EXPANSIONTYPE_IDE);
		}
	}

	controller.push_back({ HD_CONTROLLER_TYPE_SCSI_AUTO, _T("SCSI (Auto)") });
	for (int i = 0; expansionroms[i].name; i++) {
		const struct expansionromtype* erc = &expansionroms[i];
		if (erc->deviceflags & EXPANSIONTYPE_SCSI) {
			addhdcontroller(erc, HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST + i, EXPANSIONTYPE_SCSI);
		}
	}

	for (int i = 0; expansionroms[i].name; i++) {
		const struct expansionromtype* erc = &expansionroms[i];
		if (erc->deviceflags & EXPANSIONTYPE_CUSTOMDISK) {
			addhdcontroller(erc, HD_CONTROLLER_TYPE_CUSTOM_FIRST + i, EXPANSIONTYPE_CUSTOMDISK);
			break;
		}
	}

	controller_unit.clear();
	if (ctype >= HD_CONTROLLER_TYPE_IDE_FIRST && ctype <= HD_CONTROLLER_TYPE_IDE_LAST) {
		const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
		int ports = 2 + (ert ? ert->extrahdports : 0);
		for (int i = 0; i < ports; i += 2) {
			TCHAR tmp[100];
			_stprintf(tmp, _T("%d"), i + 0);
			controller_unit.push_back({ std::string(tmp) });
			_stprintf(tmp, _T("%d"), i + 1);
			controller_unit.push_back({ std::string(tmp) });
		}
		//if (media)
		//	ew(hDlg, IDC_HDF_CONTROLLER_UNIT, TRUE);
	}
	else if (ctype >= HD_CONTROLLER_TYPE_SCSI_FIRST && ctype <= HD_CONTROLLER_TYPE_SCSI_LAST) {
		const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
		controller_unit.emplace_back("0");
		controller_unit.emplace_back("1");
		if (!ert || !(ert->deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM))) {
			controller_unit.emplace_back("2");
			controller_unit.emplace_back("3");
			controller_unit.emplace_back("4");
			controller_unit.emplace_back("5");
			controller_unit.emplace_back("6");
			controller_unit.emplace_back("7");
			if (devtype == UAEDEV_HDF && ert && !_tcscmp(ert->name, _T("a2091")))
				controller_unit.emplace_back("XT");
			if (devtype == UAEDEV_HDF && ert && !_tcscmp(ert->name, _T("a2090a"))) {
				controller_unit.emplace_back("ST-506 #1");
				controller_unit.emplace_back("ST-506 #2");
			}
		}
		//if (media)
		//	ew(hDlg, IDC_HDF_CONTROLLER_UNIT, TRUE);
	}
	else if (ctype >= HD_CONTROLLER_TYPE_CUSTOM_FIRST && ctype <= HD_CONTROLLER_TYPE_CUSTOM_LAST) {
		//ew(hDlg, IDC_HDF_CONTROLLER_UNIT, FALSE);
	}
	else if (ctype == HD_CONTROLLER_TYPE_UAE) {
		for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
			TCHAR tmp[100];
			_stprintf(tmp, _T("%d"), i);
			controller_unit.push_back({ std::string(tmp) });
		}
		//if (media)
		//	ew(hDlg, IDC_HDF_CONTROLLER_UNIT, TRUE);
	}
	//else {
	//	ew(hDlg, IDC_HDF_CONTROLLER_UNIT, FALSE);
	//}

	controller_type.clear();
	controller_type.emplace_back("HD");
	controller_type.emplace_back("CF");

	controller_feature_level.clear();
	if (ctype >= HD_CONTROLLER_TYPE_IDE_FIRST && ctype <= HD_CONTROLLER_TYPE_IDE_LAST) {
		controller_feature_level.emplace_back("ATA-1");
		controller_feature_level.emplace_back("ATA-2+");
		controller_feature_level.emplace_back("ATA-2+ Strict");
	}
	else if (ctype >= HD_CONTROLLER_TYPE_SCSI_FIRST && ctype <= HD_CONTROLLER_TYPE_SCSI_LAST) {
		const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
		controller_feature_level.emplace_back("SCSI-1");
		controller_feature_level.emplace_back("SCSI-2");
		if (ert && (ert->deviceflags & (EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_CUSTOM_SECONDARY | EXPANSIONTYPE_SASI))) {
			controller_feature_level.emplace_back("SASI");
			controller_feature_level.emplace_back("SASI CHS");
		}
	}
}

bool isguiactive(void)
{
	return gui_active > 0;
}

int fromdfxtype(int num, int dfx, int subtype)
{
	switch (dfx)
	{
	case DRV_35_DD:
		return 0;
	case DRV_35_HD:
		return 1;
	case DRV_525_SD:
		return 2;
	case DRV_525_DD:
		return 3;
	case DRV_35_DD_ESCOM:
		return 4;
	}
	//if (num < 2) {
		if (dfx == DRV_FB) {
			return 5 + subtype;
		}
	//}
	//else {
	//	switch (dfx)
	//	{
	//	case DRV_PC_525_ONLY_40:
	//		return 5;
	//	case DRV_PC_525_40_80:
	//		return 6;
	//	case DRV_PC_35_ONLY_80:
	//		return 7;
	//	}
	//	if (dfx == DRV_FB) {
	//		return 8 + subtype;
	//	}
	//}
	return -1;
}

int todfxtype(int num, int dfx, int* subtype)
{
	*subtype = 0;

	switch (dfx)
	{
	case 0:
		return DRV_35_DD;
	case 1:
		return DRV_35_HD;
	case 2:
		return DRV_525_SD;
	case 3:
		return DRV_525_DD;
	case 4:
		return DRV_35_DD_ESCOM;
	}
	//if (num < 2) {
		if (dfx >= 5) {
			*subtype = dfx - 5;
			return DRV_FB;
		}
	//}
	//else {
	//	switch (dfx)
	//	{
	//	case 5:
	//		return DRV_PC_525_ONLY_40;
	//	case 6:
	//		return DRV_PC_525_40_80;
	//	case 7:
	//		return DRV_PC_35_ONLY_80;
	//	}
	//	if (dfx >= 8) {
	//		*subtype = dfx - 8;
	//		return DRV_FB;
	//	}
	//}
	return -1;
}

void DisplayDiskInfo(int num)
{
	struct diskinfo di {};
	char tmp1[MAX_DPATH];
	std::vector<std::string> infotext;
	char title[MAX_DPATH];
	char nameonly[MAX_DPATH];
	char linebuffer[512];

	DISK_examine_image(&changed_prefs, num, &di, true, nullptr);
	DISK_validate_filename(&changed_prefs, changed_prefs.floppyslots[num].df, num, tmp1, 0, NULL, NULL, NULL);
	extract_filename(tmp1, nameonly);
	snprintf(title, MAX_DPATH - 1, "Info for %s", nameonly);

	snprintf(linebuffer, sizeof(linebuffer) - 1, "Disk readable: %s", di.unreadable ? _T("No") : _T("Yes"));
	infotext.emplace_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Disk CRC32: %08X", di.imagecrc32);
	infotext.emplace_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Boot block CRC32: %08X", di.bootblockcrc32);
	infotext.emplace_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Boot block checksum valid: %s", di.bb_crc_valid ? _T("Yes") : _T("No"));
	infotext.emplace_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Boot block type: %s", di.bootblocktype == 0 ? _T("Custom") : (di.bootblocktype == 1 ? _T("Standard 1.x") : _T("Standard 2.x+")));
	infotext.emplace_back(linebuffer);
	if (di.diskname[0]) {
		snprintf(linebuffer, sizeof(linebuffer) - 1, "Label: '%s'", di.diskname);
		infotext.emplace_back(linebuffer);
	}
	infotext.emplace_back("");

	if (di.bootblockinfo[0]) {
		infotext.emplace_back("Amiga Bootblock Reader database detected:");
		snprintf(linebuffer, sizeof(linebuffer) - 1, "Name: '%s'", di.bootblockinfo);
		infotext.emplace_back(linebuffer);
		if (di.bootblockclass[0]) {
			snprintf(linebuffer, sizeof(linebuffer) - 1, "Class: '%s'", di.bootblockclass);
			infotext.emplace_back(linebuffer);
		}
		infotext.emplace_back("");
	}

	int w = 16;
	for (int i = 0; i < 1024; i += w) {
		for (int j = 0; j < w; j++) {
			uae_u8 b = di.bootblock[i + j];
			sprintf(linebuffer + j * 3, _T("%02X "), b);
			if (b >= 32 && b < 127)
				linebuffer[w * 3 + 1 + j] = (char)b;
			else
				linebuffer[w * 3 + 1 + j] = '.';
		}
		linebuffer[w * 3] = ' ';
		linebuffer[w * 3 + 1 + w] = 0;
		infotext.emplace_back(linebuffer);
	}

	ShowDiskInfo(title, infotext);
}

void save_mapping_to_file(const std::string& mapping)
{
	std::string filename = get_controllers_path();
	filename.append("gamecontrollerdb_user.txt");

	std::ofstream file_output; // out file stream
	file_output.open(filename, ios::app);
	if (file_output.is_open())
	{
		file_output << '\n' << mapping << '\n';
		file_output.close();
	}
}

void load_default_theme()
{
	gui_theme.font_name = "AmigaTopaz.ttf";
	gui_theme.font_size = 15;
	gui_theme.font_color = { 0, 0, 0 };
	gui_theme.base_color = { 170, 170, 170 };
	gui_theme.selector_inactive = { 170, 170, 170 };
	gui_theme.selector_active = { 103, 136, 187 };
	gui_theme.selection_color = { 195, 217, 217 };
	gui_theme.background_color = { 220, 220, 220 };
	gui_theme.foreground_color = { 0, 0, 0 };
}

// Get the path to the system fonts
std::string get_system_fonts_path()
{
	std::string path;
#ifdef _WIN32
	char buffer[MAX_DPATH];
	GetWindowsDirectoryA(buffer, MAX_DPATH);
	path = buffer;
	path.append("\\Fonts\\");
#else
	path = "/usr/share/fonts/truetype/";
#endif
	return path;
}

void apply_theme()
{
	gui_base_color = gui_theme.base_color;
	gui_foreground_color = gui_theme.foreground_color;
	gui_background_color = gui_theme.background_color;
	gui_selection_color = gui_theme.selection_color;
	gui_selector_inactive_color = gui_theme.selector_inactive;
	gui_selector_active_color = gui_theme.selector_active;
	gui_font_color = gui_theme.font_color;

	if (gui_theme.font_name.empty())
	{
		load_default_theme();
	}
	try
	{
		// Check if the font_name contains the full path to the file (e.g. in /usr/share/fonts)
		if (my_existsfile2(gui_theme.font_name.c_str()))
		{
			gui_font = new gcn::SDLTrueTypeFont(gui_theme.font_name, gui_theme.font_size);
		}
		else
		{
			// If only a font name was given, try to open it from the data directory
			std::string font = get_data_path();
			font.append(gui_theme.font_name);
			if (my_existsfile2(font.c_str()))
				gui_font = new gcn::SDLTrueTypeFont(font, gui_theme.font_size);
			else
			{
				// If the font file was not found in the data directory, fallback to a system font
				// TODO This needs a separate implementation for macOS!
				font = get_system_fonts_path();
				font.append("freefont/FreeSans.ttf");
			}
		}
		gui_font->setAntiAlias(true);
		gui_font->setColor(gui_font_color);
	}
	catch (gcn::Exception& e)
	{
		gui_running = false;
		std::cout << e.getMessage() << '\n';
		write_log("An error occurred while trying to open the GUI font! Exception: %s\n", e.getMessage().c_str());
		abort();
	}
	catch (std::exception& ex)
	{
		gui_running = false;
		cout << ex.what() << '\n';
		write_log("An error occurred while trying to open the GUI font! Exception: %s\n", ex.what());
		abort();
	}
	gcn::Widget::setGlobalFont(gui_font);
	gcn::Widget::setGlobalBaseColor(gui_base_color);
	gcn::Widget::setGlobalForegroundColor(gui_foreground_color);
	gcn::Widget::setGlobalBackgroundColor(gui_background_color);
	gcn::Widget::setGlobalSelectionColor(gui_selection_color);
}

// Extra theme settings, that should be called separately from the above function
void apply_theme_extras()
{
	if (selectors != nullptr)
	{
		selectors->setBaseColor(gui_base_color);
		selectors->setBackgroundColor(gui_base_color);
		selectors->setForegroundColor(gui_foreground_color);
	}
	if (selectorsScrollArea != nullptr)
	{
		selectorsScrollArea->setBaseColor(gui_base_color);
		selectorsScrollArea->setBackgroundColor(gui_base_color);
		selectorsScrollArea->setForegroundColor(gui_foreground_color);
	}
	for (int i = 0; categories[i].category != nullptr && categories[i].selector != nullptr; ++i)
	{
		categories[i].selector->setActiveColor(gui_selector_active_color);
		categories[i].selector->setInactiveColor(gui_selector_inactive_color);

		categories[i].panel->setBaseColor(gui_base_color);
		categories[i].panel->setForegroundColor(gui_foreground_color);
	}
}

void save_theme(const std::string& theme_filename)
{
	std::string filename = get_themes_path();
	filename.append(theme_filename);
	std::ofstream file_output; // out file stream
	file_output.open(filename, ios::out);
	if (file_output.is_open())
	{
		file_output << "font_name=" << gui_theme.font_name << '\n';
		file_output << "font_size=" << gui_theme.font_size << '\n';
		file_output << "font_color=" << gui_theme.font_color.r << "," << gui_theme.font_color.g << "," << gui_theme.font_color.b << '\n';
		file_output << "base_color=" << gui_theme.base_color.r << "," << gui_theme.base_color.g << "," << gui_theme.base_color.b << '\n';
		file_output << "selector_inactive=" << gui_theme.selector_inactive.r << "," << gui_theme.selector_inactive.g << "," << gui_theme.selector_inactive.b << '\n';
		file_output << "selector_active=" << gui_theme.selector_active.r << "," << gui_theme.selector_active.g << "," << gui_theme.selector_active.b << '\n';
		file_output << "selection_color=" << gui_theme.selection_color.r << "," << gui_theme.selection_color.g << "," << gui_theme.selection_color.b << '\n';
		file_output << "background_color=" << gui_theme.background_color.r << "," << gui_theme.background_color.g << "," << gui_theme.background_color.b << '\n';
		file_output << "foreground_color=" << gui_theme.foreground_color.r << "," << gui_theme.foreground_color.g << "," << gui_theme.foreground_color.b << '\n';
		file_output.close();
	}
}

void load_theme(const std::string& theme_filename)
{
	std::string filename = get_themes_path();
	filename.append(theme_filename);
	std::ifstream file_input(filename);
	if (file_input.is_open())
	{
		std::string line;
		while (std::getline(file_input, line))
		{
			std::string key = line.substr(0, line.find('='));
			std::string value = line.substr(line.find('=') + 1);
			if (key == "font_name")
				gui_theme.font_name = value;
			else if (key == "font_size")
				gui_theme.font_size = std::stoi(value);
			else if (key == "font_color")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.font_color = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
			else if (key == "base_color")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.base_color = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
			else if (key == "selector_inactive")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.selector_inactive = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
			else if (key == "selector_active")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.selector_active = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
			else if (key == "selection_color")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.selection_color = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
			else if (key == "background_color")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.background_color = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
			else if (key == "foreground_color")
			{
				const std::vector<int> rgb = parse_color_string(value);
				gui_theme.foreground_color = gcn::Color(rgb[0], rgb[1], rgb[2]);
			}
		}
		file_input.close();
	}
}
