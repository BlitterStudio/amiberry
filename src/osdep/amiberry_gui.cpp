#include <cstdio>
#include <strings.h>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

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

#ifdef AMIBERRY
#include <linux/kd.h>
#include <sys/ioctl.h>
#endif

int emulating = 0;
bool config_loaded = false;
int gui_active;

struct gui_msg
{
	int num;
	const char* msg;
};

struct gui_msg gui_msglist[] = {
  { NUMSG_NEEDEXT2,       "The software uses a non-standard floppy disk format. You may need to use a custom floppy disk image file instead of a standard one. This message will not appear again." },
  { NUMSG_NOROM,          "Could not load system ROM, trying system ROM replacement." },
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

void AddFileToDiskList(const char *file, int moveToTop)
{
	unsigned int i;

	for (i = 0; i < lstMRUDiskList.size(); ++i)
	{
		if (!stricmp(lstMRUDiskList[i].c_str(), file))
		{
			if (moveToTop)
			{
				lstMRUDiskList.erase(lstMRUDiskList.begin() + i);
				lstMRUDiskList.insert(lstMRUDiskList.begin(), file);
			}
			break;
		}
	}
	if (i >= lstMRUDiskList.size())
		lstMRUDiskList.insert(lstMRUDiskList.begin(), file);

	while (lstMRUDiskList.size() > MAX_MRU_DISKLIST)
		lstMRUDiskList.pop_back();
}

void AddFileToCDList(const char *file, int moveToTop)
{
	unsigned int i;

	for (i = 0; i < lstMRUCDList.size(); ++i)
	{
		if (!stricmp(lstMRUCDList[i].c_str(), file))
		{
			if (moveToTop)
			{
				lstMRUCDList.erase(lstMRUCDList.begin() + i);
				lstMRUCDList.insert(lstMRUCDList.begin(), file);
			}
			break;
		}
	}
	if (i >= lstMRUCDList.size())
		lstMRUCDList.insert(lstMRUCDList.begin(), file);

	while (lstMRUCDList.size() > MAX_MRU_CDLIST)
		lstMRUCDList.pop_back();
}

void AddFileToWHDLoadList(const char* file, int moveToTop)
{
	unsigned int i;

	for (i = 0; i < lstMRUWhdloadList.size(); ++i)
	{
		if (!stricmp(lstMRUWhdloadList[i].c_str(), file))
		{
			if (moveToTop)
			{
				lstMRUWhdloadList.erase(lstMRUWhdloadList.begin() + i);
				lstMRUWhdloadList.insert(lstMRUWhdloadList.begin(), file);
			}
			break;
		}
	}
	if (i >= lstMRUWhdloadList.size())
		lstMRUWhdloadList.insert(lstMRUWhdloadList.begin(), file);

	while (lstMRUWhdloadList.size() > MAX_MRU_WHDLOADLIST)
		lstMRUWhdloadList.pop_back();
}

void ClearAvailableROMList()
{
	while (!lstAvailableROMs.empty())
	{
		auto* const tmp = lstAvailableROMs[0];
		lstAvailableROMs.erase(lstAvailableROMs.begin());
		delete tmp;
	}
}

static void addrom(struct romdata* rd, const char* path)
{
	char tmpName[MAX_DPATH];
	auto* const tmp = new AvailableROM();
	getromname(rd, tmpName);
	strncpy(tmp->Name, tmpName, MAX_DPATH - 1);
	if (path != nullptr)
		strncpy(tmp->Path, path, MAX_DPATH - 1);
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
	int size = zfile_ftell(f);
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

static int isromext(char* path)
{
	if (!path)
		return 0;
	auto* ext = strrchr(path, '.');
	if (!ext)
		return 0;
	ext++;

	if (!stricmp(ext, "rom") || !stricmp(ext, "adf") || !stricmp(ext, "key")
		|| !stricmp(ext, "a500") || !stricmp(ext, "a1200") || !stricmp(ext, "a4000"))
		return 1;
	for (auto i = 0; uae_archive_extensions[i]; i++)
	{
		if (!stricmp(ext, uae_archive_extensions[i]))
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

static void scan_rom(char *path)
{
	if (!isromext(path)) {
		//write_log("ROMSCAN: skipping file '%s', unknown extension\n", path);
		return;
	}
	zfile_zopen(path, scan_rom_2, 0);
}

void SymlinkROMs()
{
	symlink_roms(&changed_prefs);
}

void RescanROMs()
{
	vector<string> files;
	char path[MAX_DPATH];

	romlist_clear();

	ClearAvailableROMList();
	get_rom_path(path, MAX_DPATH);

	load_keyring(&changed_prefs, path);
	read_directory(path, nullptr, &files);
	for (auto & file : files)
	{
		char tmppath[MAX_DPATH];
		strncpy(tmppath, path, MAX_DPATH - 1);
		strncat(tmppath, file.c_str(), MAX_DPATH - 1);
		scan_rom(tmppath);
	}

	auto id = 1;
	for (;;) {
		auto* rd = getromdatabyid(id);
		if (!rd)
			break;
		if (rd->crc32 == 0xffffffff && strncmp(rd->model, "AROS", 4) == 0)
			addrom(rd, ":AROS");
		if (rd->crc32 == 0xffffffff && rd->id == 63) {
			addrom(rd, ":HRTMon");
		}
		id++;
	}
}

static void ClearConfigFileList()
{
	while (!ConfigFilesList.empty())
	{
		auto* const tmp = ConfigFilesList[0];
		ConfigFilesList.erase(ConfigFilesList.begin());
		delete tmp;
	}
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
	for (auto & i : ConfigFilesList)
	{
		if (!SDL_strncasecmp(i->Name, name, MAX_DPATH))
			return i;
	}
	return nullptr;
}

void disk_selection(const int drive, uae_prefs* prefs)
{
	char tmp[MAX_DPATH];

	if (strlen(prefs->floppyslots[drive].df) > 0)
		strncpy(tmp, prefs->floppyslots[drive].df, MAX_DPATH);
	else
		strncpy(tmp, current_dir, MAX_DPATH);
	if (SelectFile("Select disk image file", tmp, diskfile_filter))
	{
		if (strncmp(prefs->floppyslots[drive].df, tmp, MAX_DPATH) != 0)
		{
			strncpy(prefs->floppyslots[drive].df, tmp, MAX_DPATH);
			disk_insert(drive, tmp);
			AddFileToDiskList(tmp, 1);
			extract_path(tmp, current_dir);
		}
	}
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
		memory_hardreset(2);
	}
	/* filesys hack */
	currprefs.mountitems = changed_prefs.mountitems;
	memcpy(&currprefs.mountconfig, &changed_prefs.mountconfig, MOUNT_CONFIG_SIZE * sizeof(struct uaedev_config_info));
	fixup_prefs(&changed_prefs, true);
	update_win_fs_mode(0, &changed_prefs);
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
	char tmp[MAX_DPATH];

	get_savestate_path(savestate_fname, MAX_DPATH - 1);
	get_screenshot_path(screenshot_filename, MAX_DPATH - 1);

	if (strlen(currprefs.floppyslots[0].df) > 0)
		extract_filename(currprefs.floppyslots[0].df, tmp);
	else
		strncpy(tmp, last_loaded_config, MAX_DPATH - 1);

	strncat(savestate_fname, tmp, MAX_DPATH - 1);
	strncat(screenshot_filename, tmp, MAX_DPATH - 1);
	remove_file_extension(savestate_fname);
	remove_file_extension(screenshot_filename);

  switch(currentStateNum)
  {
    case 1:
  		strncat(savestate_fname,"-1.uss", MAX_DPATH - 1);
	    strncat(screenshot_filename,"-1.png", MAX_DPATH - 1);
	    break;
    case 2:
  		strncat(savestate_fname,"-2.uss", MAX_DPATH - 1);
  		strncat(screenshot_filename,"-2.png", MAX_DPATH - 1);
  		break;
    case 3:
  		strncat(savestate_fname,"-3.uss", MAX_DPATH - 1);
  		strncat(screenshot_filename,"-3.png", MAX_DPATH - 1);
  		break;
	case 4:
		strncat(savestate_fname, "-4.uss", MAX_DPATH - 1);
		strncat(screenshot_filename, "-4.png", MAX_DPATH - 1);
		break;
	case 5:
		strncat(savestate_fname, "-5.uss", MAX_DPATH - 1);
		strncat(screenshot_filename, "-5.png", MAX_DPATH - 1);
		break;
	case 6:
		strncat(savestate_fname, "-6.uss", MAX_DPATH - 1);
		strncat(screenshot_filename, "-6.png", MAX_DPATH - 1);
		break;
	case 7:
		strncat(savestate_fname, "-7.uss", MAX_DPATH - 1);
		strncat(screenshot_filename, "-7.png", MAX_DPATH - 1);
		break;
	case 8:
		strncat(savestate_fname, "-8.uss", MAX_DPATH - 1);
		strncat(screenshot_filename, "-8.png", MAX_DPATH - 1);
		break;
	case 9:
		strncat(savestate_fname, "-9.uss", MAX_DPATH - 1);
		strncat(screenshot_filename, "-9.png", MAX_DPATH - 1);
		break;
    default: 
	   	strncat(savestate_fname,".uss", MAX_DPATH - 1);
  		strncat(screenshot_filename,".png", MAX_DPATH - 1);
  }
  return 0;
}

/* if drive is -1, show the full GUI, otherwise file-requester for DF[drive] */
void gui_display(int shortcut)
{
	if (quit_program != 0)
		return;
	gui_active++;

	if (setpaused(7)) {
		inputdevice_unacquire();
		wait_keyrelease();
		clearallkeys();
		setmouseactive(0, 0);
	}

	if (shortcut == -1)
	{
		graphics_subshutdown();

		prefs_to_gui();
		run_gui();
		gui_to_prefs();

		clearscreen();

		gui_update();
		gui_purge_events();
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
	update_display(&changed_prefs);
	if (resumepaused(7)) {
		inputdevice_acquire(TRUE);
		setmouseactive(0, 1);
	}
	
	fpscounter_reset();
	gui_active--;
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

void gui_led(int led, int on, int brightness)
{
	unsigned char kbd_led_status;

	// Check current prefs/ update if changed
	if (currprefs.kbd_led_num != changed_prefs.kbd_led_num) currprefs.kbd_led_num = changed_prefs.kbd_led_num;
	if (currprefs.kbd_led_scr != changed_prefs.kbd_led_scr) currprefs.kbd_led_scr = changed_prefs.kbd_led_scr;
	//if (currprefs.kbd_led_cap != changed_prefs.kbd_led_cap) currprefs.kbd_led_cap = changed_prefs.kbd_led_cap;

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
	}

	ioctl(0, KDSETLED, kbd_led_status);
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

	graphics_subshutdown();
	ShowMessage(_T(""), msg, _T(""), _T("Ok"), _T(""));
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