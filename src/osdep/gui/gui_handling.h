#ifndef GUI_HANDLING_H
#define GUI_HANDLING_H

#include <guisan/sdl/sdlinput.hpp>
#include "amiberry_gfx.h"
#include "amiberry_input.h"
#include "filesys.h"
#include "options.h"
#include "registry.h"

enum
{
	DISTANCE_BORDER = 10,
	DISTANCE_NEXT_X = 15,
	DISTANCE_NEXT_Y = 15,
	BUTTON_WIDTH = 90,
	BUTTON_HEIGHT = 30,
	SMALL_BUTTON_WIDTH = 30,
	SMALL_BUTTON_HEIGHT = 22,
	LABEL_HEIGHT = 20,
	TEXTFIELD_HEIGHT = 20,
	DROPDOWN_HEIGHT = 20,
	SLIDER_HEIGHT = 20,
	TITLEBAR_HEIGHT = 24,
	SELECTOR_WIDTH = 165,
	SELECTOR_HEIGHT = 24,
	SCROLLBAR_WIDTH = 20
};

static const std::vector<std::string> floppy_drive_types = {
	"Disabled", "3.5\" DD", "3.5\" HD", "5.25\" (40)",
	"5.25\" (80)", "3.5\" ESCOM", "FB: Normal", "FB: Compatible",
	"FB: Turbo", "FB: Stalling"
};

static const TCHAR* memsize_names[] = {
	/* 0 */ _T("none"),
	/* 1 */ _T("64 KB"),
	/* 2 */ _T("128 KB"),
	/* 3 */ _T("256 KB"),
	/* 4 */ _T("512 KB"),
	/* 5 */ _T("1 MB"),
	/* 6 */ _T("2 MB"),
	/* 7 */ _T("4 MB"),
	/* 8 */ _T("8 MB"),
	/* 9 */ _T("16 MB"),
	/* 10*/ _T("32 MB"),
	/* 11*/ _T("64 MB"),
	/* 12*/ _T("128 MB"),
	/* 13*/ _T("256 MB"),
	/* 14*/ _T("512 MB"),
	/* 15*/ _T("1 GB"),
	/* 16*/ _T("1.5MB"),
	/* 17*/ _T("1.8MB"),
	/* 18*/ _T("2 GB"),
	/* 19*/ _T("384 MB"),
	/* 20*/ _T("768 MB"),
	/* 21*/ _T("1.5 GB"),
	/* 22*/ _T("2.5 GB"),
	/* 23*/ _T("3 GB")
};

static constexpr unsigned long memsizes[] = {
	/* 0 */ 0,
	/* 1 */ 0x00010000, /*  64K */
	/* 2 */ 0x00020000, /* 128K */
	/* 3 */ 0x00040000, /* 256K */
	/* 4 */ 0x00080000, /* 512K */
	/* 5 */ 0x00100000, /* 1M */
	/* 6 */ 0x00200000, /* 2M */
	/* 7 */ 0x00400000, /* 4M */
	/* 8 */ 0x00800000, /* 8M */
	/* 9 */ 0x01000000, /* 16M */
	/* 10*/ 0x02000000, /* 32M */
	/* 11*/ 0x04000000, /* 64M */
	/* 12*/ 0x08000000, //128M
	/* 13*/ 0x10000000, //256M
	/* 14*/ 0x20000000, //512M
	/* 15*/ 0x40000000, //1GB
	/* 16*/ 0x00180000, //1.5MB
	/* 17*/ 0x001C0000, //1.8MB
	/* 18*/ 0x80000000, //2GB
	/* 19*/ 0x18000000, //384M
	/* 20*/ 0x30000000, //768M
	/* 21*/ 0x60000000, //1.5GB
	/* 22*/ 0xA8000000, //2.5GB
	/* 23*/ 0xC0000000, //3GB
};

static const int msi_chip[] = {3, 4, 5, 16, 6, 7, 8};
static const int msi_bogo[] = {0, 4, 5, 16, 17};
static const int msi_fast[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
static const int msi_z3fast[] = {0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const int msi_z3chip[] = {0, 9, 10, 11, 12, 13, 19, 14, 20, 15};
static const int msi_gfx[] = {0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const int msi_cpuboard[] = {0, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static const int msi_mb[] = {0, 5, 6, 7, 8, 9, 10, 11, 12};

enum
{
	MIN_CHIP_MEM = 0,
	MAX_CHIP_MEM = 6,
	MIN_FAST_MEM = 0,
	MAX_FAST_MEM = 8,
	MIN_SLOW_MEM = 0,
	MAX_SLOW_MEM = 4,
	MIN_Z3_MEM = 0,
	MAX_Z3_MEM = 11,
	MAX_Z3_CHIPMEM = 9,
	MIN_P96_MEM = 0
};

#define MAX_P96_MEM_Z3 ((max_z3fastmem >> 20) < 512 ? 8 : ((max_z3fastmem >> 20) < 1024 ? 9 : ((max_z3fastmem >> 20) < 2048) ? 10 : 11))

enum
{
	MAX_P96_MEM_Z2 = 4,
	MIN_MB_MEM = 0,
	MAX_MBL_MEM = 7,
	MAX_MBH_MEM = 8,
	MIN_CB_MEM = 0,
	MAX_CB_MEM_Z2 = 4,
	MAX_CB_MEM_16M = 5,
	MAX_CB_MEM_32M = 6,
	MAX_CB_MEM_64M = 7,
	MAX_CB_MEM_128M = 8,
	MAX_CB_MEM_256M = 9
};

enum
{
	MIN_M68K_PRIORITY = 1,
	MAX_M68K_PRIORITY = 16,
	MIN_CACHE_SIZE = 0,
	MAX_CACHE_SIZE = 5,
	MIN_REFRESH_RATE = 1,
	MAX_REFRESH_RATE = 10,
	MIN_SOUND_MEM = 0,
	MAX_SOUND_MEM = 10
};

static const char* diskfile_filter[] = {
	".adf", ".adz", ".fdi", ".ipf", ".zip", ".dms", ".gz", ".xz", ".scp",
	".7z", ".lha", ".lzh", ".lzx", "\0"
};
static const char* harddisk_filter[] = {".hdf", ".hdz", ".lha", ".zip", ".vhd", ".chd", ".7z", "\0"};
static const char* archive_filter[] = { ".zip", ".7z", ".rar", ".lha", ".lzh", ".lzx", "\0" };
static const char* cdfile_filter[] = { ".cue", ".ccd", ".iso", ".mds", ".nrg", ".chd", "\0" };
static const char* whdload_filter[] = { ".lha", "\0" };
static const char* statefile_filter[] = { ".uss", ".sav", "\0" };
static string drivebridgeModes[] =
{
	"Normal",
	"Compatible",
	"Turbo",
	"Stalling"
};

using ConfigCategory = struct config_category
{
	const char* category;
	const char* imagepath;
	gcn::SelectorEntry* selector;
	gcn::Container* panel;

	void (*InitFunc)(const config_category& category);
	void (*ExitFunc)();
	void (*RefreshFunc)();
	bool (*HelpFunc)(std::vector<std::string>&);
};

extern bool gui_running;
extern gcn::Container* selectors;
extern gcn::ScrollArea* selectorsScrollArea;
extern ConfigCategory categories[];
extern gcn::Gui* uae_gui;
extern gcn::Container* gui_top;

// GUI Colors
extern amiberry_gui_theme gui_theme;
extern gcn::Color gui_base_color;
extern gcn::Color gui_background_color;
extern gcn::Color gui_selector_inactive_color;
extern gcn::Color gui_selector_active_color;
extern gcn::Color gui_selection_color;
extern gcn::Color gui_foreground_color;
extern gcn::Color gui_font_color;

extern gcn::SDLInput* gui_input;
extern SDL_Surface* gui_screen;
extern SDL_Joystick* gui_joystick;

extern gcn::SDLGraphics* gui_graphics;
extern gcn::SDLTrueTypeFont* gui_font;
extern SDL_Texture* gui_texture;


extern std::string current_dir;
extern char last_loaded_config[MAX_DPATH];
extern char last_active_config[MAX_DPATH];

extern int quickstart_model;
extern int quickstart_conf;

typedef struct {
	char Name[MAX_DPATH];
	char FullPath[MAX_DPATH];
	char Description[MAX_DPATH];
	int BuiltInID;
} ConfigFileInfo;

extern vector<ConfigFileInfo*> ConfigFilesList;

void InitPanelAbout(const struct config_category& category);
void ExitPanelAbout();
void RefreshPanelAbout();
bool HelpPanelAbout(std::vector<std::string>& helptext);

void InitPanelPaths(const struct config_category& category);
void ExitPanelPaths();
void RefreshPanelPaths();
bool HelpPanelPaths(std::vector<std::string>& helptext);

void InitPanelQuickstart(const struct config_category& category);
void ExitPanelQuickstart();
void RefreshPanelQuickstart();
bool HelpPanelQuickstart(std::vector<std::string>& helptext);

void InitPanelConfig(const struct config_category& category);
void ExitPanelConfig();
void RefreshPanelConfig();
bool HelpPanelConfig(std::vector<std::string>& helptext);

void InitPanelCPU(const struct config_category& category);
void ExitPanelCPU();
void RefreshPanelCPU();
bool HelpPanelCPU(std::vector<std::string>& helptext);

void InitPanelChipset(const struct config_category& category);
void ExitPanelChipset();
void RefreshPanelChipset();
bool HelpPanelChipset(std::vector<std::string>& helptext);

void InitPanelROM(const struct config_category& category);
void ExitPanelROM();
void RefreshPanelROM();
bool HelpPanelROM(std::vector<std::string>& helptext);

void InitPanelRAM(const struct config_category& category);
void ExitPanelRAM();
void RefreshPanelRAM();
bool HelpPanelRAM(std::vector<std::string>& helptext);

void InitPanelFloppy(const struct config_category& category);
void ExitPanelFloppy();
void RefreshPanelFloppy();
bool HelpPanelFloppy(std::vector<std::string>& helptext);

void InitPanelHD(const struct config_category& category);
void ExitPanelHD();
void RefreshPanelHD();
bool HelpPanelHD(std::vector<std::string>& helptext);

void InitPanelExpansions(const struct config_category& category);
void ExitPanelExpansions();
void RefreshPanelExpansions();
bool HelpPanelExpansions(std::vector<std::string>& helptext);

void InitPanelRTG(const struct config_category& category);
void ExitPanelRTG();
void RefreshPanelRTG();
bool HelpPanelRTG(std::vector<std::string>& helptext);

void InitPanelHWInfo(const struct config_category& category);
void ExitPanelHWInfo();
void RefreshPanelHWInfo();
bool HelpPanelHWInfo(std::vector<std::string>& helptext);

void InitPanelDisplay(const struct config_category& category);
void ExitPanelDisplay();
void RefreshPanelDisplay();
bool HelpPanelDisplay(std::vector<std::string>& helptext);

void InitPanelSound(const struct config_category& category);
void ExitPanelSound();
void RefreshPanelSound();
bool HelpPanelSound(std::vector<std::string>& helptext);

void InitPanelInput(const struct config_category& category);
void ExitPanelInput();
void RefreshPanelInput();
bool HelpPanelInput(std::vector<std::string>& helptext);

void InitPanelIO(const struct config_category& category);
void ExitPanelIO();
void RefreshPanelIO();
bool HelpPanelIO(std::vector<std::string>& helptext);

void InitPanelCustom(const struct config_category& category);
void ExitPanelCustom();
void RefreshPanelCustom();
bool HelpPanelCustom(std::vector<std::string>& helptext);

void InitPanelDiskSwapper(const struct config_category& category);
void ExitPanelDiskSwapper();
void RefreshPanelDiskSwapper();
bool HelpPanelDiskSwapper(std::vector<std::string>& helptext);

void InitPanelMisc(const struct config_category& category);
void ExitPanelMisc();
void RefreshPanelMisc();
bool HelpPanelMisc(std::vector<std::string>& helptext);

void InitPanelPrio(const struct config_category& category);
void ExitPanelPrio();
void RefreshPanelPrio();
bool HelpPanelPrio(std::vector<std::string>& helptext);

void InitPanelSavestate(const struct config_category& category);
void ExitPanelSavestate();
void RefreshPanelSavestate();
bool HelpPanelSavestate(std::vector<std::string>& helptext);

void InitPanelVirtualKeyboard(const struct config_category& category);
void ExitPanelVirtualKeyboard();
void RefreshPanelVirtualKeyboard();
bool HelpPanelVirtualKeyboard(std::vector<std::string>& helptext);

void InitPanelWHDLoad(const struct config_category& category);
void ExitPanelWHDLoad();
void RefreshPanelWHDLoad();
bool HelpPanelWHDLoad(std::vector<std::string>& helptext);

void InitPanelThemes(const struct config_category& category);
void ExitPanelThemes();
void RefreshPanelThemes();
bool HelpPanelThemes(std::vector<std::string>& helptext);

void refresh_all_panels();
void focus_bug_workaround(const gcn::Window* wnd);
void disable_resume();

bool ShowMessage(const std::string& title, const std::string& line1, const std::string& line2, const std::string& line3,
                 const std::string& button1, const std::string& button2);
amiberry_hotkey ShowMessageForInput(const char* title, const char* line1, const char* button1);

std::string SelectFolder(const std::string& title, std::string value);
std::string SelectFile(const std::string& title, std::string value, const char* filter[], bool create = false);
bool Create_Folder(const std::string& path);

bool EditFilesysVirtual(int unit_no);
bool EditFilesysHardfile(int unit_no);
bool EditFilesysHardDrive(int unit_no);
bool EditCDDrive(int unit_no);
bool EditTapeDrive(int unit_no);
bool CreateFilesysHardfile();
void ShowHelp(const char* title, const std::vector<std::string>& text);
void ShowDiskInfo(const char* title, const std::vector<std::string>& text);
void ShowCustomFields();

std::string show_controller_map(int device, bool map_touchpad);
extern void read_directory(const std::string& path, vector<string>* dirs, vector<string>* files);
extern void FilterFiles(vector<string>* files, const char* filter[]);

enum
{
	DIRECTION_NONE,
	DIRECTION_UP,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_RIGHT
};

bool handle_navigation(int direction);
void PushFakeKey(SDL_Keycode inKey);

extern bool handle_keydown(SDL_Event& event, bool& dialog_finished, bool& nav_left, bool& nav_right);
extern bool handle_joybutton(didata* did, bool& dialog_finished, bool& nav_left, bool& nav_right);
extern bool handle_joyaxis(const SDL_Event& event, bool& nav_left, bool& nav_right);
extern bool handle_finger(const SDL_Event& event, SDL_Event& touch_event);
extern bool handle_mousewheel(const SDL_Event& event);

enum
{
	MAX_HD_DEVICES = 8
};

extern void CreateDefaultDevicename(char* name);
extern bool DevicenameExists(const char* name);
extern int tweakbootpri(int bp, int ab, int dnm);

struct controller_map
{
	int type;
	std::string display;
};
extern std::vector<controller_map> controller;
extern std::vector<string> controller_unit;
extern std::vector<string> controller_type;
extern std::vector<string> controller_feature_level;

STATIC_INLINE bool is_hdf_rdb()
{
	return current_hfdlg.ci.sectors == 0 && current_hfdlg.ci.surfaces == 0 && current_hfdlg.ci.reserved == 0;
}

extern void new_filesys(int entry);
extern void new_cddrive(int entry);
extern void new_tapedrive(int entry);
extern void new_hardfile(int entry);
extern void new_harddrive(int entry);

extern void inithdcontroller(int ctype, int ctype_unit, int devtype, bool media);

extern int current_state_num;
extern int delay_savestate_frame;
extern int last_x;
extern int last_y;

extern struct romdata *scan_single_rom (const TCHAR *path);
extern void update_gui_screen();
extern void cap_fps(Uint64 start);
extern long get_file_size(const std::string& filename);
extern bool download_file(const std::string& source, const std::string& destination, bool keep_backup);
extern void download_rtb(const std::string& filename);
extern int fromdfxtype(int num, int dfx, int subtype);
extern int todfxtype(int num, int dfx, int* subtype);
extern void DisplayDiskInfo(int num);
extern std::string get_full_path_from_disk_list(std::string element);
extern amiberry_hotkey get_hotkey_from_config(std::string config_option);
extern void save_mapping_to_file(const std::string& mapping);
extern void clear_whdload_prefs();
extern void create_startup_sequence();

extern std::vector<int> parse_color_string(const std::string& input);
extern void save_theme(const std::string& theme_filename);
extern void load_theme(const std::string& theme_filename);
extern void load_default_theme();
extern void apply_theme();
extern void apply_theme_extras();

extern void SetLastLoadedConfig(const char* filename);
extern void set_last_active_config(const char* filename);
extern void disk_selection(const int shortcut, uae_prefs* prefs);

extern void addromfiles(UAEREG* fkey, gcn::DropDown* d, const TCHAR* path, int type1, int type2);

#endif // GUI_HANDLING_H
