#include "sysdeps.h"
#include "gui.h"
#include "midiemu.h"
#include "options.h"
#include "filesys.h"
#include "disk.h"
#include "blkdev.h"
#include "rommgr.h"
#include "zfile.h"
#include "registry.h"
#include "fsdb.h"
#include "uae.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#ifdef LIBRETRO
unsigned int gui_ledstate = 0;
bool gui_running = false;
std::vector<std::string> midi_out_ports;
std::vector<std::string> midi_in_ports;
std::vector<std::string> lstMRUDiskList;
std::vector<std::string> lstMRUCDList;
std::vector<std::string> lstMRUWhdloadList;
struct hfdlg_vals current_hfdlg;
int emulating = 0;
bool config_loaded = false;
bool joystick_refresh_needed = false;

int gui_init(void)
{
	return 0;
}

void gui_update(void)
{
}

void gui_exit(void)
{
}

void gui_led(int led, int on, int brightness)
{
	(void)led;
	(void)on;
	(void)brightness;
}

void gui_handle_events(void)
{
}

void gui_filename(int num, const TCHAR* name)
{
	(void)num;
	(void)name;
}

void gui_fps(int fps, int lines, bool lace, int idle, int color)
{
	(void)fps;
	(void)lines;
	(void)lace;
	(void)idle;
	(void)color;
}

void gui_changesettings(void)
{
}

void gui_lock(void)
{
}

void gui_unlock(void)
{
}

void gui_flicker_led(int led, int unit, int status)
{
	(void)led;
	(void)unit;
	(void)status;
}

void gui_disk_image_change(int unit, const TCHAR* name, bool writeprotected)
{
	(void)unit;
	(void)name;
	(void)writeprotected;
}

void gui_display(int shortcut)
{
	(void)shortcut;
}

void gui_update_gfx(void)
{
}

void gui_restart(void)
{
}

void gui_message(const TCHAR* msg, ...)
{
	(void)msg;
}

void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	(void)title;
	(void)msg;
}

bool gui_ask_disk(int drv, TCHAR* name)
{
	(void)drv;
	(void)name;
	return false;
}

bool isguiactive(void)
{
	return false;
}

void notify_user(int msg)
{
	(void)msg;
}

void notify_user_parms(int msg, const TCHAR* parms, ...)
{
	(void)msg;
	(void)parms;
}

int translate_message(int msg, TCHAR* out)
{
	(void)msg;
	if (out) {
		*out = 0;
	}
	return 0;
}

#ifndef WITH_MT32EMU_SOURCES
int midi_emu = 0;

void midi_emu_close(void)
{
}

int midi_emu_open(const TCHAR* id)
{
	(void)id;
	return 0;
}

void midi_emu_parse(uae_u8* midi, int len)
{
	(void)midi;
	(void)len;
}

bool midi_emu_available(const TCHAR* id)
{
	(void)id;
	return false;
}

void midi_emu_reopen(void)
{
}

void midi_update_sound(float v)
{
	(void)v;
}
#endif

static bool is_hdf_rdb(void)
{
	return current_hfdlg.ci.sectors == 0 &&
		current_hfdlg.ci.surfaces == 0 &&
		current_hfdlg.ci.reserved == 0;
}

void hardfile_testrdb(struct hfdlg_vals* hdf)
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
				auto blocksize = 512;
				if (!babe)
					blocksize = (id[16] << 24) | (id[17] << 16) | (id[18] << 8) | (id[19] << 0);
				hdf->ci.cyls = hdf->ci.highcyl = hdf->forcedcylinders = 0;
				hdf->ci.sectors = 0;
				hdf->ci.surfaces = 0;
				hdf->ci.reserved = 0;
				hdf->ci.bootpri = 0;
				if (blocksize >= 512)
					hdf->ci.blocksize = blocksize;
				hdf->rdb = 1;
				break;
			}
		}
		hdf_close(&hfd);
	}
}

bool hardfile_testrdb(const char* filename)
{
	(void)filename;
	return false;
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
		struct hardfiledata hfd{};
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
			} else {
				getchspgeometry(bsize, &current_hfdlg.ci.pcyls, &current_hfdlg.ci.pheads, &current_hfdlg.ci.psecs, false);
			}
			if (defaults && !gotrdb && !realdrive) {
				gethdfgeometry(bsize, &current_hfdlg.ci);
				phys = false;
			}
		} else {
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
		_sntprintf(tmp2, sizeof tmp2, _T(" %s [%02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X]"), idtmp,
			id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7],
			id[8], id[9], id[10], id[11], id[12], id[13], id[14], id[15]);
		if (!blocks) {
			_sntprintf(tmp, sizeof tmp, _T("%uMB"), (unsigned int)(bsize / (1024 * 1024)));
		} else if (blocks && !cyls) {
			_sntprintf(tmp, sizeof tmp, _T("%u blocks, %.1fMB"),
				blocks,
				(double)bsize / (1024.0 * 1024.0));
		} else {
			_sntprintf(tmp, sizeof tmp, _T("%u/%u/%u, %u/%u blocks, %.1fMB/%.1fMB"),
				cyls, heads, secs,
				blocks, (int)(bsize / current_hfdlg.ci.blocksize),
				(double)blocks * 1.0 * current_hfdlg.ci.blocksize / (1024.0 * 1024.0),
				(double)bsize / (1024.0 * 1024.0));
			if ((uae_u64)cyls * heads * secs > bsize / current_hfdlg.ci.blocksize) {
				_tcscat(tmp2, _T(" [Geometry larger than drive!]"));
			} else if (cyls > 65535) {
				_tcscat(tmp2, _T(" [Too many cyls]"));
			}
		}
		if (txtHdfInfo.empty() && txtHdfInfo2.empty()) {
			txtHdfInfo = std::string(tmp);
			txtHdfInfo2 = std::string(tmp2);
		}
	}
}

void add_file_to_mru_list(std::vector<std::string>& vec, const std::string& file)
{
	static const size_t kMaxMruList = 40;

	if (std::find(vec.begin(), vec.end(), file) == vec.end()) {
		vec.insert(vec.begin(), file);
	}

	while (vec.size() > kMaxMruList)
		vec.pop_back();
}

void load_default_theme()
{
}

void load_default_dark_theme()
{
}

void save_theme(const std::string& theme_filename)
{
	(void)theme_filename;
}

struct libretro_rom_scan_candidate
{
	std::string path;
	bool deepscan;
};

struct libretro_rom_scan_data
{
	UAEREG* fkey;
	int got;
};

static std::string libretro_path_join(const std::string& dir, const std::string& file)
{
	if (dir.empty())
		return file;
	if (file.empty())
		return dir;
	const char last = dir.back();
	if (last == '/' || last == '\\')
		return dir + file;
	return dir + "/" + file;
}

static void libretro_append_scan_candidate(std::vector<libretro_rom_scan_candidate>& candidates,
	const std::string& path, const bool deepscan)
{
	if (path.empty())
		return;

	TCHAR resolved[MAX_DPATH] = {};
#ifdef _WIN32
	if (_fullpath(resolved, path.c_str(), MAX_DPATH) == nullptr)
		return;
#else
	if (realpath(path.c_str(), resolved) == nullptr)
		return;
#endif

	for (auto& candidate : candidates) {
		if (_tcsicmp(candidate.path.c_str(), resolved) == 0) {
			candidate.deepscan = candidate.deepscan || deepscan;
			return;
		}
	}

	candidates.push_back({ resolved, deepscan });
}

static void libretro_append_scan_root(std::vector<libretro_rom_scan_candidate>& candidates,
	const char* root)
{
	if (!root || !*root)
		return;

	const std::string root_path = root;
	libretro_append_scan_candidate(candidates, root_path, false);
	libretro_append_scan_candidate(candidates, libretro_path_join(root_path, "roms"), true);
	libretro_append_scan_candidate(candidates, libretro_path_join(root_path, "Kickstarts"), true);
	libretro_append_scan_candidate(candidates, libretro_path_join(root_path, "save-data/Kickstarts"), true);
	libretro_append_scan_candidate(candidates, libretro_path_join(root_path, "whdboot/save-data/Kickstarts"), true);
	libretro_append_scan_candidate(candidates, libretro_path_join(root_path, "amiberry/whdboot/save-data/Kickstarts"), true);
}

static int libretro_rom_ext_priority(const TCHAR* path, const int size)
{
	const TCHAR* s = _tcsrchr(path, '.');
	if (s == nullptr)
		return 80;
	if (!my_existsfile(path))
		return 100;

	int pri = 10;
	struct mystat ms {};
	if (my_stat(path, &ms) && ms.size == size)
		pri--;
	return pri;
}

static int libretro_add_rom(UAEREG* fkey, struct romdata* rd, const TCHAR* name)
{
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH], tmp3[MAX_DPATH];
	char pathname[MAX_DPATH];

	_sntprintf(tmp1, sizeof tmp1, _T("ROM_%03d"), rd->id);
	if (rd->group) {
		TCHAR* p = tmp1 + _tcslen(tmp1);
		_sntprintf(p, sizeof tmp1 - _tcslen(tmp1), _T("_%02d_%02d"), rd->group >> 16, rd->group & 65535);
	}
	getromname(rd, tmp2);
	pathname[0] = 0;

	if (name)
		_tcscpy(pathname, name);
	if (rd->crc32 == 0xffffffff) {
		if (rd->configname)
			_sntprintf(tmp2, sizeof tmp2, _T(":%s"), rd->configname);
		else
			_sntprintf(tmp2, sizeof tmp2, _T(":ROM_%03d"), rd->id);
	}

	int size = sizeof tmp3 / sizeof(TCHAR);
	if (regquerystr(fkey, tmp1, tmp3, &size)) {
		TCHAR* s = _tcschr(tmp3, '"');
		if (s && _tcslen(s) > 1) {
			TCHAR* s2 = s + 1;
			s = _tcschr(s2, '"');
			if (s)
				*s = 0;
			const int pri1 = libretro_rom_ext_priority(s2, rd->size);
			const int pri2 = libretro_rom_ext_priority(pathname, rd->size);
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

static bool libretro_is_rom_ext(const std::string& path, const bool deepscan)
{
	if (path.empty())
		return false;
	const auto ext_pos = path.find_last_of('.');
	if (ext_pos == std::string::npos)
		return false;

	std::string ext = path.substr(ext_pos + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});

	static const char* extensions[] = {
		"rom", "roz", "bin", "a500", "a600", "a1200", "a3000", "a4000", "cdtv", "cd32"
	};
	for (const auto* candidate : extensions) {
		if (ext == candidate)
			return true;
	}
	if (ext.size() >= 2 && ext[0] == 'u' && std::isdigit(static_cast<unsigned char>(ext[1])))
		return true;
	if (!deepscan)
		return false;
	for (auto i = 0; uae_archive_extensions[i]; i++) {
		if (_tcsicmp(ext.c_str(), uae_archive_extensions[i]) == 0)
			return true;
	}
	return false;
}

static int libretro_scan_rom_entry(struct zfile* f, void* user)
{
	auto* rsd = static_cast<libretro_rom_scan_data*>(user);
	const TCHAR* path = zfile_getname(f);
	const TCHAR* romkey = _T("rom.key");

	if (!libretro_is_rom_ext(path, true))
		return 0;

	struct romdata* rd = scan_single_rom_file(f);
	if (rd) {
		libretro_add_rom(rsd->fkey, rd, path);
		if (rd->type & ROMTYPE_KEY)
			addkeyfile(path);
		rsd->got = 1;
	} else if (_tcslen(path) > _tcslen(romkey) && !_tcsicmp(path + _tcslen(path) - _tcslen(romkey), romkey)) {
		addkeyfile(path);
	}
	return 0;
}

static int libretro_scan_rom_file(const std::string& path, UAEREG* fkey, const bool deepscan)
{
	if (!libretro_is_rom_ext(path, deepscan))
		return 0;

	libretro_rom_scan_data rsd = { fkey, 0 };
	zfile_zopen(path, libretro_scan_rom_entry, &rsd);
	return rsd.got;
}

static int libretro_scan_rom_dir(UAEREG* fkey, const std::string& path, const bool deepscan, const int level)
{
	struct dirent* entry;
	struct stat statbuf {};
	int ret = 0;

	write_log(_T("libretro ROM scan directory '%s'\n"), path.c_str());

	DIR* dp = opendir(path.c_str());
	if (dp == nullptr)
		return 0;

	while ((entry = readdir(dp)) != nullptr) {
		if (entry->d_name[0] == '.' && (entry->d_name[1] == 0 ||
			(entry->d_name[1] == '.' && entry->d_name[2] == 0))) {
			continue;
		}

		const std::string tmppath = libretro_path_join(path, entry->d_name);
		if (stat(tmppath.c_str(), &statbuf) == -1)
			continue;

		if (S_ISREG(statbuf.st_mode) && statbuf.st_size < 10000000) {
			if (libretro_scan_rom_file(tmppath, fkey, deepscan))
				ret = 1;
		} else if (deepscan && S_ISDIR(statbuf.st_mode) && entry->d_name[0] != '.' && level < 2) {
			if (libretro_scan_rom_dir(fkey, tmppath, deepscan, level + 1))
				ret = 1;
		}
	}

	closedir(dp);
	return ret;
}

int scan_roms(int show)
{
	(void)show;

	static int recursive;
	if (recursive)
		return 0;
	recursive++;

	regdeletetree(nullptr, _T("DetectedROMs"));
	UAEREG* fkey = regcreatetree(nullptr, _T("DetectedROMs"));
	if (fkey == nullptr) {
		recursive--;
		return 0;
	}

	std::vector<libretro_rom_scan_candidate> candidates;
	const std::string rom_path = get_rom_path();
	libretro_append_scan_candidate(candidates, rom_path, false);
	libretro_append_scan_root(candidates, getenv("AMIBERRY_LIBRETRO_SYSTEM_DIR"));
	libretro_append_scan_root(candidates, getenv("AMIBERRY_LIBRETRO_SAVE_DIR"));
	libretro_append_scan_root(candidates, getenv("AMIBERRY_WHDBOOT_ASSETS_DIR"));
	libretro_append_scan_root(candidates, getenv("AMIBERRY_WHDBOOT_PATH"));
	libretro_append_scan_root(candidates, getenv("WHDBOOT_SAVE_DATA"));
	libretro_append_scan_candidate(candidates, changed_prefs.path_rom.path[0], false);

	int cnt = 0;
	for (const auto& candidate : candidates) {
		if (libretro_scan_rom_dir(fkey, candidate.path, candidate.deepscan, 0))
			cnt++;
	}

	for (int id = 1; ; id++) {
		struct romdata* rd = getromdatabyid(id);
		if (!rd)
			break;
		if (rd->crc32 == 0xffffffff)
			libretro_add_rom(fkey, rd, nullptr);
	}

	write_log(_T("libretro ROM scan: %d matching path(s) across %zu candidate(s)\n"), cnt, candidates.size());
	read_rom_list(false);
	regclosetree(fkey);
	recursive--;
	return cnt;
}

void cancel_async_update_check()
{
}

// No physical SCSI/IOCTL device support in libretro
struct device_functions devicefunc_scsi_ioctl = {
	_T("IOCTL"),
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr
};
#endif
