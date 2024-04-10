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
#include "disk.h"
#include "xwin.h"
#include "drawing.h"
#include "fsdb.h"
#include "parser.h"

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

static void addrom(struct romdata* rd, const char* path)
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

static int isromext(const std::string& path)
{
	if (path.empty())
		return 0;
	const auto ext_pos = path.find_last_of('.');
	if (ext_pos == std::string::npos)
		return 0;
	const std::string ext = path.substr(ext_pos + 1);

	static const std::vector<std::string> extensions = { "rom", "adf", "key", "a500", "a1200", "a4000" };
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
	if (!isromext(path)) {
		//write_log("ROMSCAN: skipping file '%s', unknown extension\n", path);
		return;
	}
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
		setcursorshape(0);
	}
	//rawinput_alloc();
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

void default_hfdlg(struct hfdlg_vals* f)
{
	auto ctrl = f->ci.controller_type;
	auto unit = f->ci.controller_unit;
	memset(f, 0, sizeof(struct hfdlg_vals));
	uci_set_defaults(&f->ci, false);
	f->original = true;
	f->ci.type = UAEDEV_HDF;
	f->ci.controller_type = ctrl;
	f->ci.controller_unit = unit;
	f->ci.unit_feature_level = 1;
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