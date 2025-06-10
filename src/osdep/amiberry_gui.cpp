#include <algorithm>
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
#include "registry.h"
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
int recursiveromscan = 2;

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

struct romdataentry
{
	TCHAR* name;
	int priority;
};

void addromfiles(UAEREG* fkey, gcn::DropDown* d, const TCHAR* path, int type1, int type2)
{
	int idx;
	TCHAR tmp[MAX_DPATH];
	TCHAR tmp2[MAX_DPATH];
	TCHAR seltmp[MAX_DPATH];
	struct romdata* rdx = nullptr;
	struct romdataentry* rde = xcalloc(struct romdataentry, MAX_ROMMGR_ROMS);
	int ridx = 0;

	if (path)
		rdx = scan_single_rom(path);
	idx = 0;
	seltmp[0] = 0;
	for (; fkey;) {
		int size = sizeof(tmp) / sizeof(TCHAR);
		int size2 = sizeof(tmp2) / sizeof(TCHAR);
		if (!regenumstr(fkey, idx, tmp, &size, tmp2, &size2))
			break;
		if (_tcslen(tmp) == 7 || _tcslen(tmp) == 13) {
			int group = 0;
			int subitem = 0;
			int idx2 = _tstol(tmp + 4);
			if (_tcslen(tmp) == 13) {
				group = _tstol(tmp + 8);
				subitem = _tstol(tmp + 11);
			}
			if (idx2 >= 0) {
				struct romdata* rd = getromdatabyidgroup(idx2, group, subitem);
				for (int i = 0; i < 2; i++) {
					int type = i ? type2 : type1;
					if (type) {
						if (rd && ((((rd->type & ROMTYPE_GROUP_MASK) & (type & ROMTYPE_GROUP_MASK)) && ((rd->type & ROMTYPE_SUB_MASK) == (type & ROMTYPE_SUB_MASK) || !(type & ROMTYPE_SUB_MASK))) ||
							(rd->type & type) == ROMTYPE_NONE || (rd->type & type) == ROMTYPE_NOT)) {
							getromname(rd, tmp);
							int j;
							for (j = 0; j < ridx; j++) {
								if (!_tcsicmp(rde[j].name, tmp)) {
									break;
								}
							}
							if (j >= ridx) {
								rde[ridx].name = my_strdup(tmp);
								rde[ridx].priority = rd->sortpriority;
								ridx++;
							}
							if (rd == rdx)
								_tcscpy(seltmp, tmp);
							break;
						}
					}
				}
			}
		}
		idx++;
	}

	for (int i = 0; i < ridx; i++) {
		for (int j = i + 1; j < ridx; j++) {
			int ipri = rde[i].priority;
			const TCHAR* iname = rde[i].name;
			int jpri = rde[j].priority;
			const TCHAR* jname = rde[j].name;
			if ((ipri > jpri) || (ipri == jpri && _tcsicmp(iname, jname) > 0)) {
				struct romdataentry rdet{};
				memcpy(&rdet, &rde[i], sizeof(struct romdataentry));
				memcpy(&rde[i], &rde[j], sizeof(struct romdataentry));
				memcpy(&rde[j], &rdet, sizeof(struct romdataentry));
			}
		}
	}

	auto listmodel = d->getListModel();
	listmodel->clear();
	listmodel->add("");
	for (int i = 0; i < ridx; i++) {
		struct romdataentry* rdep = &rde[i];
		listmodel->add(rdep->name);
		xfree(rdep->name);
	}
	if (seltmp[0])
	{
		for (int i = 0; i < listmodel->getNumberOfElements(); i++) {
			if (!_tcsicmp(listmodel->getElementAt(i).c_str(), seltmp)) {
				d->setSelected(i);
				break;
			}
		}
	}
	else
	{
		if (path && path[0])
		{
			listmodel->add(path);
			d->setSelected(listmodel->getNumberOfElements() - 1);
		}
	}

	xfree(rde);
}

static int extpri(const TCHAR* p, int size)
{
	const TCHAR* s = _tcsrchr(p, '.');
	if (s == nullptr)
		return 80;
	// if archive: lowest priority
	if (!my_existsfile(p))
		return 100;
	int pri = 10;
	// prefer matching size
	struct mystat ms{};
	if (my_stat(p, &ms)) {
		if (ms.size == size) {
			pri--;
		}
	}
	return pri;
}

static int addrom(UAEREG* fkey, struct romdata* rd, const TCHAR* name)
{
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH], tmp3[MAX_DPATH];
	char pathname[MAX_DPATH];

	_sntprintf(tmp1, sizeof tmp1, _T("ROM_%03d"), rd->id);
	if (rd->group) {
		TCHAR* p = tmp1 + _tcslen(tmp1);
		_sntprintf(p, sizeof p, _T("_%02d_%02d"), rd->group >> 16, rd->group & 65535);
	}
	getromname(rd, tmp2);
	pathname[0] = 0;

	if (name) {
		_tcscpy(pathname, name);
	}
	if (rd->crc32 == 0xffffffff) {
		if (rd->configname)
			_sntprintf(tmp2, sizeof tmp2, _T(":%s"), rd->configname);
		else
			_sntprintf(tmp2, sizeof tmp2, _T(":ROM_%03d"), rd->id);
	}
	int size = sizeof tmp3 / sizeof(TCHAR);
	if (regquerystr(fkey, tmp1, tmp3, &size)) {
		TCHAR* s = _tcschr(tmp3, '\"');
		if (s && _tcslen(s) > 1) {
			TCHAR* s2 = s + 1;
			s = _tcschr(s2, '\"');
			if (s)
				*s = 0;
			int pri1 = extpri(s2, rd->size);
			int pri2 = extpri(pathname, rd->size);
			if (pri2 >= pri1)
				return 1;
		}
	}
	fullpath(pathname, sizeof(pathname) / sizeof(TCHAR));
	if (pathname[0]) {
		_tcscat(tmp2, _T(" / \""));
		_tcscat(tmp2, pathname);
		_tcscat(tmp2, _T("\""));
	}
	if (!regsetstr(fkey, tmp1, tmp2))
		return 0;
	return 1;
}

struct romscandata {
	UAEREG* fkey;
	int got;
};

static struct romdata* scan_single_rom_2(struct zfile* f)
{
	uae_u8 buffer[20] = {};
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
		size = std::min(size, 262144);
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
		if (size < 4) {
			free(rombuf);
			return nullptr;
		}
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
		return nullptr;
	return scan_single_rom_2 (z);
}

static int isromext(const std::string& path, bool deepscan)
{
	if (path.empty())
		return 0;
	const auto ext_pos = path.find_last_of('.');
	if (ext_pos == std::string::npos)
		return 0;
	const std::string ext = path.substr(ext_pos + 1);

	static const std::vector<std::string> extensions = { "rom", "ROM", "roz", "ROZ", "bin", "BIN",  "a500", "A500", "a1200", "A1200", "a4000", "A4000", "cdtv", "CDTV", "cd32", "CD32" };
	if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end())
		return 1;

	if (ext.size() >= 2 && std::toupper(ext[0]) == 'U' && std::isdigit(ext[1]))
		return 1;
	if (!deepscan)
		return 0;
	for (auto i = 0; uae_archive_extensions[i]; i++)
	{
		if (strcasecmp(ext.c_str(), uae_archive_extensions[i]) == 0)
			return 1;
	}
	return 0;
}

static bool scan_rom_hook(const TCHAR* name, int line)
{
	// TODO
	return true;
	//MSG msg;
	//if (cdstate.status)
	//	return false;
	//if (!cdstate.active)
	//	return true;
	//if (name != NULL) {
	//	const TCHAR* s = NULL;
	//	if (line == 2) {
	//		s = _tcsrchr(name, '/');
	//		if (!s)
	//			s = _tcsrchr(name, '\\');
	//		if (s)
	//			s++;
	//	}
	//	SetWindowText(GetDlgItem(cdstate.hwnd, line == 1 ? IDC_INFOBOX_TEXT1 : (line == 2 ? IDC_INFOBOX_TEXT2 : IDC_INFOBOX_TEXT3)), s ? s : name);
	//}
	//while (PeekMessage(&msg, cdstate.hwnd, 0, 0, PM_REMOVE)) {
	//	if (!IsDialogMessage(cdstate.hwnd, &msg)) {
	//		TranslateMessage(&msg);
	//		DispatchMessage(&msg);
	//	}
	//}
	//return cdstate.active;
}

static int scan_rom_2(struct zfile* f, void* vrsd)
{
	auto* rsd = static_cast<struct romscandata*>(vrsd);
	const TCHAR* path = zfile_getname(f);
	const TCHAR* romkey = _T("rom.key");
	struct romdata* rd;

	if (!isromext(path, true))
		return 0;
	rd = scan_single_rom_2(f);
	if (rd)
	{
		TCHAR name[MAX_DPATH];
		getromname(rd, name);
		addrom(rsd->fkey, rd, path);
		if (rd->type & ROMTYPE_KEY)
			addkeyfile(path);
		rsd->got = 1;
	} else if (_tcslen(path) > _tcslen(romkey) && !_tcsicmp(path + _tcslen(path) - _tcslen(romkey), romkey)) {
		addkeyfile(path);
	}
	return 0;
}

static int scan_rom(const std::string& path, UAEREG* fkey, bool deepscan)
{
	struct romscandata rsd = { fkey, 0 };
	struct romdata* rd;
	int cnt = 0;

	if (!isromext(path, deepscan)) {
		//write_log("ROMSCAN: skipping file '%s', unknown extension\n", path);
		return 0;
	}
#ifdef ARCADIA
	for (;;) {
		TCHAR tmp[MAX_DPATH];
		_tcscpy(tmp, path.c_str());
		rd = scan_arcadia_rom(tmp, cnt++);
		if (rd) {
			if (!addrom(fkey, rd, tmp))
				return 1;
			continue;
		}
		break;
	}
#endif
	zfile_zopen(path, scan_rom_2, (void*)&rsd);
	return rsd.got;
}

static int listrom(const int* roms)
{
	int i;

	i = 0;
	while (roms[i] >= 0) {
		struct romdata* rd = getromdatabyid(roms[i]);
		if (rd && romlist_get(rd))
			return 1;
		i++;
	}
	return 0;
}

static void show_rom_list()
{
	// TODO
	//TCHAR* p;
	//TCHAR* p1, * p2;
	//const int* rp;
	//bool first = true;
	//const int romtable[] = {
	//	5, 4, -1, -1, // A500 1.2
	//	6, 32, -1, -1, // A500 1.3
	//	7, -1, -1, // A500+
	//	8, 9, 10, -1, -1, // A600
	//	23, 24, -1, -1, // A1000
	//	11, 31, 15, -1, -1, // A1200
	//	59, 71, 61, -1, -1, // A3000
	//	16, 46, 31, 13, 12, -1, -1, // A4000
	//	17, -1, -1, // A4000T
	//	18, -1, 19, -1, -1, // CD32
	//	20, 21, 22, -1, 6, 32, -1, -1, // CDTV
	//	9, 10, -1, 107, 108, -1, -1, // CDTV-CR
	//	49, 50, 75, 51, 76, 77, -1, 5, 4, -1, -2, // ARCADIA

	//	18, -1, 19, -1, 74, 23, -1, -1,  // CD32 FMV

	//	69, 67, 70, 115, -1, -1, // nordic power
	//	65, 68, -1, -1, // x-power
	//	62, 60, -1, -1, // action cartridge
	//	116, -1, -1, // pro access
	//	52, 25, -1, -1, // ar 1
	//	26, 27, 28, -1, -1, // ar 2
	//	29, 30, -1, -1, // ar 3
	//	47, -1, -1, // action replay 1200

	//	0, 0, 0
	//};

	//p1 = _T("A500 Boot ROM 1.2\0A500 Boot ROM 1.3\0A500+\0A600\0A1000\0A1200\0A3000\0A4000\0A4000T\0")
	//	_T("CD32\0CDTV\0CDTV-CR\0Arcadia Multi Select\0")
	//	_T("CD32 Full Motion Video\0")
	//	_T("Nordic Power\0X-Power Professional 500\0Action Cartridge Super IV Professional\0")
	//	_T("Pro Access\0")
	//	_T("Action Replay MK I\0Action Replay MK II\0Action Replay MK III\0")
	//	_T("Action Replay 1200\0")
	//	_T("\0");

	//p = xmalloc(TCHAR, 100000);
	//if (!p)
	//	return;
	//WIN32GUI_LoadUIString(IDS_ROMSCANEND, p, 100);
	//_tcscat(p, _T("\n\n"));

	//rp = romtable;
	//while (rp[0]) {
	//	int ok = 1;
	//	p2 = p1 + _tcslen(p1) + 1;
	//	while (*rp >= 0) {
	//		if (ok) {
	//			ok = 0;
	//			if (listrom(rp))
	//				ok = 1;
	//		}
	//		while (*rp++ >= 0);
	//	}
	//	if (ok) {
	//		if (!first)
	//			_tcscat(p, _T(", "));
	//		first = false;
	//		_tcscat(p, p1);
	//	}
	//	if (*rp == -2) {
	//		_tcscat(p, _T("\n\n"));
	//		first = true;
	//	}
	//	rp++;
	//	p1 = p2;
	//}

	//pre_gui_message(p);
	//free(p);
}

static int scan_roms_2(UAEREG* fkey, const TCHAR* path, bool deepscan, int level)
{
	struct dirent* entry;
	struct stat statbuf{};
	DIR* dp;
	int ret = 0;

	if (!path)
		return 0;

	write_log(_T("ROM scan directory '%s'\n"), path);

	dp = opendir(path);
	if (dp == nullptr)
		return 0;

	scan_rom_hook(path, 1);

    while ((entry = readdir(dp)) != nullptr) {
        TCHAR tmppath[MAX_DPATH];
        _sntprintf(tmppath, sizeof tmppath, _T("%s/%s"), path, entry->d_name);

        if (stat(tmppath, &statbuf) == -1)
            continue;

        if (S_ISREG(statbuf.st_mode) && statbuf.st_size < 10000000) {
            if (scan_rom(tmppath, fkey, deepscan))
                ret = 1;
        } else if (deepscan && S_ISDIR(statbuf.st_mode) && entry->d_name[0] != '.' && (recursiveromscan < 0 || recursiveromscan > level)) {
            scan_roms_2(fkey, tmppath, deepscan, level + 1);
        }

        if (!scan_rom_hook(nullptr, 0))
            break;
    }

	closedir(dp);
	return ret;
}

#define MAX_ROM_PATHS 10

static int scan_roms_3(UAEREG* fkey, TCHAR** paths, const TCHAR* path)
{
	int i, ret;
	TCHAR pathp[MAX_DPATH];
	bool deepscan = true;

	ret = 0;
	scan_rom_hook(nullptr, 0);
	pathp[0] = 0;
	realpath(path, pathp);
	if (!pathp[0])
		return ret;
	if (_tcsicmp(pathp, get_rom_path().c_str()) == 0)
		deepscan = false; // do not scan root dir archives
	for (i = 0; i < MAX_ROM_PATHS; i++) {
		if (paths[i] && !_tcsicmp(paths[i], pathp))
			return ret;
	}
	ret = scan_roms_2(fkey, pathp, deepscan, 0);
	for (i = 0; i < MAX_ROM_PATHS; i++) {
		if (!paths[i]) {
			paths[i] = my_strdup(pathp);
			break;
		}
	}
	return ret;
}

int scan_roms(int show)
{
	TCHAR path[MAX_DPATH];
	static int recursive;
	int id, i, ret, keys, cnt;
	UAEREG* fkey, * fkey2;
	TCHAR* paths[MAX_ROM_PATHS];

	if (recursive)
		return 0;
	recursive++;

	ret = 0;

	regdeletetree(nullptr, _T("DetectedROMs"));
	fkey = regcreatetree(nullptr, _T("DetectedROMs"));
	if (fkey == nullptr)
		goto end;

	cnt = 0;
	for (i = 0; i < MAX_ROM_PATHS; i++)
		paths[i] = nullptr;
	scan_rom_hook(nullptr, 0);
	while (scan_rom_hook(nullptr, 0)) {
		keys = get_keyring();
		get_rom_path(path, sizeof path / sizeof(TCHAR));
		cnt += scan_roms_3(fkey, paths, path);
		// We only have one ROM path, so no need to scan other paths
		//if (1) {
		//	static pathtype pt[] = { PATH_TYPE_DEFAULT, PATH_TYPE_WINUAE, PATH_TYPE_NEWWINUAE, PATH_TYPE_NEWAF, PATH_TYPE_AMIGAFOREVERDATA, PATH_TYPE_END };
		//	for (i = 0; pt[i] != PATH_TYPE_END; i++) {
		//		ret = get_rom_path(path, pt[i]);
		//		if (ret < 0)
		//			break;
		//		cnt += scan_roms_3(fkey, paths, path);
		//	}
		//	if (get_keyring() > keys) { /* more keys detected in previous scan? */
		//		write_log(_T("ROM scan: more keys found, restarting..\n"));
		//		for (i = 0; i < MAX_ROM_PATHS; i++) {
		//			xfree(paths[i]);
		//			paths[i] = NULL;
		//		}
		//		continue;
		//	}
		//}
		break;
	}
	if (cnt == 0)
		scan_roms_3(fkey, paths, changed_prefs.path_rom.path[0]);

	for (i = 0; i < MAX_ROM_PATHS; i++)
		xfree(paths[i]);

	fkey2 = regcreatetree(nullptr, _T("DetectedROMS"));
	if (fkey2) {
		id = 1;
		for (;;) {
			struct romdata* rd = getromdatabyid(id);
			if (!rd)
				break;
			if (rd->crc32 == 0xffffffff)
				addrom(fkey, rd, nullptr);
			id++;
		}
		regclosetree(fkey2);
	}

end:

	read_rom_list(false);
	if (show)
		show_rom_list();

	regclosetree(fkey);
	recursive--;
	return ret;
}

static void ClearConfigFileList()
{
	for (const auto* config : ConfigFilesList)
	{
		delete config;
	}
	ConfigFilesList.clear();
}

void ReadConfigFileList()
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
			auto p = cfgfile_open(tmp->FullPath, nullptr);
			if (p) {
				cfgfile_get_description(p, nullptr, tmp->Description, nullptr, nullptr, nullptr, nullptr, nullptr);
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

void disk_selection(const int shortcut, uae_prefs* prefs)
{
	// Select Floppy Disk Image
	if (shortcut >= 0 && shortcut < 4)
	{
		std::string tmp;
		if (strlen(prefs->floppyslots[shortcut].df) > 0)
			tmp = std::string(prefs->floppyslots[shortcut].df);
		else
			tmp = get_floppy_path();
		tmp = SelectFile("Select disk image file", tmp, diskfile_filter);
		if (!tmp.empty())
		{
			if (strncmp(prefs->floppyslots[shortcut].df, tmp.c_str(), MAX_DPATH) != 0)
			{
				strncpy(prefs->floppyslots[shortcut].df, tmp.c_str(), MAX_DPATH);
				disk_insert(shortcut, tmp.c_str());
				add_file_to_mru_list(lstMRUDiskList, tmp);
			}
		}
	}
	else if (shortcut == 4)
	{
		// Load a Save state
		TCHAR tmp[MAX_DPATH];
		get_savestate_path(tmp, sizeof tmp / sizeof(TCHAR));

		const std::string selected = SelectFile("Load a save state file", tmp, statefile_filter);
		if (!selected.empty())
		{
			_tcscpy(savestate_fname, selected.c_str());
			savestate_initsave(savestate_fname, 1, true, true);
			savestate_state = STATE_DORESTORE;

			const auto filename = extract_filename(savestate_fname);
			screenshot_filename = get_screenshot_path();
			screenshot_filename += filename;
			screenshot_filename = remove_file_extension(screenshot_filename);
			screenshot_filename += ".png";
		}
		else {
			savestate_fname[0] = 0;
		}
	}
	else if (shortcut == 5)
	{
		// Save a state
		TCHAR tmp[MAX_DPATH];
		get_savestate_path(tmp, sizeof tmp / sizeof(TCHAR));

		std::string selected = SelectFile("Save a save state file", tmp, statefile_filter, true);
		if (!selected.empty())
		{
			// ensure the selected filename ends with .uss
			if (selected.size() < 4 || selected.substr(selected.size() - 4) != ".uss")
			{
				selected += ".uss";
			}

			_tcscpy(savestate_fname, selected.c_str());
			_tcscat(tmp, savestate_fname);
			save_state(savestate_fname, _T("Description!"));
			if (create_screenshot())
			{
				const auto filename = extract_filename(savestate_fname);
				screenshot_filename = get_screenshot_path();
				screenshot_filename += filename;
				screenshot_filename = remove_file_extension(screenshot_filename);
				screenshot_filename += ".png";
				save_thumb(screenshot_filename);
			}
		}
	}
	// Select CD Image
	else if (shortcut == 6)
	{
		std::string tmp;
		if (prefs->cdslots[0].inuse && strlen(prefs->cdslots[0].name) > 0)
			tmp = std::string(prefs->cdslots[0].name);
		else
			tmp = get_cdrom_path();
		tmp = SelectFile("Select CD image file", tmp, cdfile_filter);
		if (!tmp.empty())
		{
			if (strncmp(prefs->cdslots[0].name, tmp.c_str(), MAX_DPATH) != 0)
			{
				strncpy(prefs->cdslots[0].name, tmp.c_str(), MAX_DPATH);
				changed_prefs.cdslots[0].inuse = true;
				changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
				add_file_to_mru_list(lstMRUCDList, tmp);
			}
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
		currprefs.drawbridge_drive_cable != changed_prefs.drawbridge_drive_cable)
	{
		currprefs.drawbridge_driver = changed_prefs.drawbridge_driver;
		currprefs.drawbridge_serial_auto = changed_prefs.drawbridge_serial_auto;
		_tcscpy(currprefs.drawbridge_serial_port, changed_prefs.drawbridge_serial_port);
		currprefs.drawbridge_smartspeed = changed_prefs.drawbridge_smartspeed;
		currprefs.drawbridge_autocache = changed_prefs.drawbridge_autocache;
		currprefs.drawbridge_drive_cable = changed_prefs.drawbridge_drive_cable;
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

	read_rom_list(false);

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
}

void gui_purge_events()
{
	keybuf_init();
}

void gui_update()
{
	if (current_state_num == 99) return;

	std::string filename;
	const std::string suffix = current_state_num >= 1 && current_state_num <= 14 ?
		"-" + std::to_string(current_state_num) : "";

	if (strlen(currprefs.floppyslots[0].df) > 0)
		filename = extract_filename(currprefs.floppyslots[0].df);
	else if (currprefs.cdslots[0].inuse && strlen(currprefs.cdslots[0].name) > 0)
		filename = extract_filename(currprefs.cdslots[0].name);
	else if (!whdload_prefs.whdload_filename.empty())
		filename = extract_filename(whdload_prefs.whdload_filename);
	else if (strlen(last_active_config) > 0)
		filename = std::string(last_active_config) + ".uss";
	else
		return;

	get_savestate_path(savestate_fname, MAX_DPATH - 1);
	strncat(savestate_fname, filename.c_str(), MAX_DPATH - 1);
	remove_file_extension(savestate_fname);
	strncat(savestate_fname, (suffix + ".uss").c_str(), MAX_DPATH - 1);

	screenshot_filename = remove_file_extension(get_screenshot_path() + filename);
	screenshot_filename.append(suffix + ".png");
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
	else if (shortcut >= 0 && shortcut <= 6)
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
	resetcounter[led] = 15;
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

void gui_fps(int fps, int lines, bool lace, int idle, int color)
{
	gui_data.fps = fps;
	gui_data.lines = lines;
	gui_data.lace = lace;
	gui_data.idle = idle;
	gui_data.fps_color = color;
	gui_led(LED_FPS, 0, -1);
	gui_led(LED_LINES, 0, -1);
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
	_vsntprintf(msg, sizeof(msg), format, parms);
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
		_sntprintf(name, sizeof name, "DH%d", freeNum);
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

void updatehdfinfo(bool force, bool defaults, bool realdrive, std::string& txtHdfInfo, std::string& txtHdfInfo2)
{
	uae_u8 id[512] = { };
	uae_u32 blocks, cyls, i;
	TCHAR tmp[200], tmp2[200];
	TCHAR idtmp[17];
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

	cyls = phys ? current_hfdlg.ci.pcyls : current_hfdlg.forcedcylinders;
	int heads = phys ? current_hfdlg.ci.pheads : current_hfdlg.ci.surfaces;
	int secs = phys ? current_hfdlg.ci.psecs : current_hfdlg.ci.sectors;
	if (!cyls && current_hfdlg.ci.blocksize && secs && heads) {
		cyls = (uae_u32)(bsize / ((uae_u64)current_hfdlg.ci.blocksize * secs * heads));
	}
	blocks = cyls * (secs * heads);
	if (!blocks && current_hfdlg.ci.blocksize)
		blocks = (uae_u32)(bsize / current_hfdlg.ci.blocksize);
	if (current_hfdlg.ci.max_lba)
		blocks = (uae_u32)current_hfdlg.ci.max_lba;

	for (i = 0; i < sizeof (idtmp) / sizeof (TCHAR) - 1; i++) {
		TCHAR c = id[i];
		if (c < 32 || c > 126)
			c = '.';
		idtmp[i] = c;
		idtmp[i + 1] = 0;
	}

	tmp[0] = 0;
	if (bsize) {
		_sntprintf (tmp2, sizeof tmp2, _T(" %s [%02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X]"), idtmp,
			id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7],
			id[8], id[9], id[10], id[11], id[12], id[13], id[14], id[15]);
		if (!blocks) {
			_sntprintf (tmp, sizeof tmp, _T("%uMB"), (unsigned int)(bsize / (1024 * 1024)));
		} else if (blocks && !cyls) {
			_sntprintf (tmp, sizeof tmp, _T("%u blocks, %.1fMB"),
				blocks,
				(double)bsize / (1024.0 * 1024.0));
		} else {
			_sntprintf (tmp, sizeof tmp, _T("%u/%u/%u, %u/%u blocks, %.1fMB/%.1fMB"),
				cyls, heads, secs,
				blocks, (int)(bsize / current_hfdlg.ci.blocksize),
				(double)blocks * 1.0 * current_hfdlg.ci.blocksize / (1024.0 * 1024.0),
				(double)bsize / (1024.0 * 1024.0));
			if ((uae_u64)cyls * heads * secs > bsize / current_hfdlg.ci.blocksize) {
				_tcscat (tmp2, _T(" [Geometry larger than drive!]"));
			} else if (cyls > 65535) {
				_tcscat (tmp2, _T(" [Too many cyls]"));
			}
		}
		if (txtHdfInfo.empty() && txtHdfInfo2.empty()) {
			txtHdfInfo = std::string(tmp);
			txtHdfInfo2 = std::string(tmp2);
		}
	}
}

void new_filesys(int entry)
{
	struct uaedev_config_data* uci;
	struct uaedev_config_info ci{};
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
	struct uaedev_config_info ci{};
	ci.device_emu_unit = 0;
	ci.controller_type = current_cddlg.ci.controller_type;
	ci.controller_unit = current_cddlg.ci.controller_unit;
#ifdef AMIBERRY
	_tcscpy(ci.rootdir, current_cddlg.ci.rootdir);
#endif
	ci.type = UAEDEV_CD;
	ci.readonly = true;
	ci.blocksize = 2048;
	add_filesys_config(&changed_prefs, entry, &ci);
}

void new_tapedrive(int entry)
{
	struct uaedev_config_data* uci;
	struct uaedev_config_info ci{};
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
	struct uaedev_config_info ci{};
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
	if (get_boardromconfig(&changed_prefs, erc->romtype, nullptr) || get_boardromconfig(&changed_prefs, erc->romtype_extra, nullptr)) {
		auto name_string = std::string(name);
		controller.push_back({ firstid, name_string });
		for (int j = 1; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
			if (is_board_enabled(&changed_prefs, erc->romtype, j)) {
				TCHAR tmp[MAX_DPATH];
				_sntprintf(tmp, sizeof tmp, _T("%s [%d]"), name, j + 1);
				auto tmp_string = std::string(tmp);
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
			_sntprintf(tmp, sizeof tmp, _T("%d"), i + 0);
			controller_unit.push_back({ std::string(tmp) });
			_sntprintf(tmp, sizeof tmp, _T("%d"), i + 1);
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
			_sntprintf(tmp, sizeof tmp, _T("%d"), i);
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

bool isguiactive()
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
	DISK_validate_filename(&changed_prefs, changed_prefs.floppyslots[num].df, num, tmp1, 0, nullptr, nullptr, nullptr);
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
			_sntprintf(linebuffer + j * 3, sizeof(linebuffer) - j * 3, "%02X ", b);
			if (b >= 32 && b < 127)
				linebuffer[w * 3 + 1 + j] = static_cast<char>(b);
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

void load_default_dark_theme()
{
	gui_theme.font_name = "AmigaTopaz.ttf";
	gui_theme.font_size = 15;
	gui_theme.font_color = { 200, 200, 200 };
	gui_theme.base_color = { 32, 32, 37 };
	gui_theme.selector_inactive = { 32, 32, 37 };
	gui_theme.selector_active = { 50, 100, 200 };
	gui_theme.selection_color = { 50, 100, 200 };
	gui_theme.background_color = { 45, 45, 47 };
	gui_theme.foreground_color = { 200, 200, 200 };
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
	gcn::Widget::setWidgetsBaseColor(gui_base_color);
	gcn::Widget::setWidgetsForegroundColor(gui_foreground_color);
	gcn::Widget::setWidgetsBackgroundColor(gui_background_color);
	gcn::Widget::setWidgetsSelectionColor(gui_selection_color);
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
