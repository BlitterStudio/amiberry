#ifndef GUI_HANDLING_H
#define GUI_HANDLING_H

#include "amiberry_gfx.h"

#define DISTANCE_BORDER 15
#define DISTANCE_NEXT_X 15
#define DISTANCE_NEXT_Y 15
#define BUTTON_WIDTH 90
#define BUTTON_HEIGHT 30
#define SMALL_BUTTON_WIDTH 30
#define SMALL_BUTTON_HEIGHT 22
#define LABEL_HEIGHT 20
#define TEXTFIELD_HEIGHT 20
#define DROPDOWN_HEIGHT 20
#define SLIDER_HEIGHT 20
#define TITLEBAR_HEIGHT 24
#include <guisan/sdl/sdlinput.hpp>

#include "options.h"

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

static const unsigned long memsizes[] = {
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

static const int msi_chip[] = { 3, 4, 5, 16, 6, 7, 8 };
static const int msi_bogo[] = { 0, 4, 5, 16, 17 };
static const int msi_fast[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
static const int msi_z3fast[] = { 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const int msi_z3chip[] = { 0, 9, 10, 11, 12, 13, 19, 14, 20, 15 };
static const int msi_gfx[] = { 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const int msi_cpuboard[] = { 0, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
static const int msi_mb[] = { 0, 5, 6, 7, 8, 9, 10, 11, 12 };

#define MIN_CHIP_MEM 0
#define MAX_CHIP_MEM 6
#define MIN_FAST_MEM 0
#define MAX_FAST_MEM 8
#define MIN_SLOW_MEM 0
#define MAX_SLOW_MEM 4
#define MIN_Z3_MEM 0
#define MAX_Z3_MEM 11
#define MAX_Z3_CHIPMEM 9
#define MIN_P96_MEM 0
#define MAX_P96_MEM_Z3 ((max_z3fastmem >> 20) < 512 ? 8 : ((max_z3fastmem >> 20) < 1024 ? 9 : ((max_z3fastmem >> 20) < 2048) ? 10 : 11))
#define MAX_P96_MEM_Z2 4
#define MIN_MB_MEM 0
#define MAX_MBL_MEM 7
#define MAX_MBH_MEM 8
#define MIN_CB_MEM 0
#define MAX_CB_MEM_Z2 4
#define MAX_CB_MEM_16M 5
#define MAX_CB_MEM_32M 6
#define MAX_CB_MEM_64M 7
#define MAX_CB_MEM_128M 8
#define MAX_CB_MEM_256M 9

#define MIN_M68K_PRIORITY 1
#define MAX_M68K_PRIORITY 16
#define MIN_CACHE_SIZE 0
#define MAX_CACHE_SIZE 5
#define MIN_REFRESH_RATE 1
#define MAX_REFRESH_RATE 10
#define MIN_SOUND_MEM 0
#define MAX_SOUND_MEM 10

static const char* diskfile_filter[] = { ".adf", ".adz", ".fdi", ".ipf", ".zip", ".dms", ".gz", ".xz", ".scp", "\0" };
static const char* cdfile_filter[] = { ".cue", ".ccd", ".iso", ".mds", ".nrg", "\0" };
static const char* whdload_filter[] = { ".lha", "\0" };

typedef struct _ConfigCategory
{
	const char* category;
	const char* imagepath;
	gcn::SelectorEntry* selector;
	gcn::Container* panel;
	void (*InitFunc)(const struct _ConfigCategory& category);
	void (*ExitFunc)();
	void (*RefreshFunc)();
	bool (*HelpFunc)(std::vector<std::string>&);
} ConfigCategory;

extern bool gui_running;
extern ConfigCategory categories[];
extern gcn::Gui* uae_gui;
extern gcn::Container* gui_top;
extern gcn::Color gui_baseCol;
extern gcn::Color colTextboxBackground;
extern gcn::Color colSelectorActive;
extern gcn::SDLInput* gui_input;
extern gcn::SDLTrueTypeFont* gui_font;
extern SDL_Surface* gui_screen;
extern SDL_Joystick* gui_joystick;
extern gcn::SDLGraphics* gui_graphics;

#ifdef USE_DISPMANX
extern DISPMANX_RESOURCE_HANDLE_T gui_resource;
extern DISPMANX_RESOURCE_HANDLE_T black_gui_resource;
extern DISPMANX_ELEMENT_HANDLE_T gui_element;
extern int element_present;
#else
extern SDL_Texture* gui_texture;
extern SDL_Cursor* cursor;
extern SDL_Surface* cursor_surface;
#endif

extern char current_dir[MAX_DPATH];
extern char last_loaded_config[MAX_DPATH];

extern int quickstart_model;
extern int quickstart_conf;

typedef struct
{
	char Name[MAX_DPATH];
	char FullPath[MAX_DPATH];
	char Description[MAX_DPATH];
	int BuiltInID;
} ConfigFileInfo;

extern vector<ConfigFileInfo*> ConfigFilesList;

void InitPanelAbout(const struct _ConfigCategory& category);
void ExitPanelAbout();
void RefreshPanelAbout();
bool HelpPanelAbout(std::vector<std::string>& helptext);

void InitPanelPaths(const struct _ConfigCategory& category);
void ExitPanelPaths(void);
void RefreshPanelPaths(void);
bool HelpPanelPaths(std::vector<std::string>& helptext);

void InitPanelQuickstart(const struct _ConfigCategory& category);
void ExitPanelQuickstart(void);
void RefreshPanelQuickstart(void);
bool HelpPanelQuickstart(std::vector<std::string>& helptext);

void InitPanelConfig(const struct _ConfigCategory& category);
void ExitPanelConfig(void);
void RefreshPanelConfig(void);
bool HelpPanelConfig(std::vector<std::string>& helptext);

void InitPanelCPU(const struct _ConfigCategory& category);
void ExitPanelCPU(void);
void RefreshPanelCPU(void);
bool HelpPanelCPU(std::vector<std::string>& helptext);

void InitPanelChipset(const struct _ConfigCategory& category);
void ExitPanelChipset(void);
void RefreshPanelChipset(void);
bool HelpPanelChipset(std::vector<std::string>& helptext);

void InitPanelROM(const struct _ConfigCategory& category);
void ExitPanelROM(void);
void RefreshPanelROM(void);
bool HelpPanelROM(std::vector<std::string>& helptext);

void InitPanelRAM(const struct _ConfigCategory& category);
void ExitPanelRAM(void);
void RefreshPanelRAM(void);
bool HelpPanelRAM(std::vector<std::string>& helptext);

void InitPanelFloppy(const struct _ConfigCategory& category);
void ExitPanelFloppy(void);
void RefreshPanelFloppy(void);
bool HelpPanelFloppy(std::vector<std::string>& helptext);

void InitPanelHD(const struct _ConfigCategory& category);
void ExitPanelHD(void);
void RefreshPanelHD(void);
bool HelpPanelHD(std::vector<std::string>& helptext);

void InitPanelRTG(const struct _ConfigCategory& category);
void ExitPanelRTG(void);
void RefreshPanelRTG(void);
bool HelpPanelRTG(std::vector<std::string>& helptext);

void InitPanelHWInfo(const struct _ConfigCategory& category);
void ExitPanelHWInfo();
void RefreshPanelHWInfo();
bool HelpPanelHWInfo(std::vector<std::string>& helptext);

void InitPanelDisplay(const struct _ConfigCategory& category);
void ExitPanelDisplay(void);
void RefreshPanelDisplay(void);
bool HelpPanelDisplay(std::vector<std::string>& helptext);

void InitPanelSound(const struct _ConfigCategory& category);
void ExitPanelSound(void);
void RefreshPanelSound(void);
bool HelpPanelSound(std::vector<std::string>& helptext);

void InitPanelInput(const struct _ConfigCategory& category);
void ExitPanelInput(void);
void RefreshPanelInput(void);
bool HelpPanelInput(std::vector<std::string>& helptext);

void InitPanelCustom(const struct _ConfigCategory& category);
void ExitPanelCustom(void);
void RefreshPanelCustom(void);
bool HelpPanelCustom(std::vector<std::string>& helptext);

void InitPanelMisc(const struct _ConfigCategory& category);
void ExitPanelMisc(void);
void RefreshPanelMisc(void);
bool HelpPanelMisc(std::vector<std::string>& helptext);

void InitPanelPrio(const struct _ConfigCategory& category);
void ExitPanelPrio(void);
void RefreshPanelPrio(void);
bool HelpPanelPrio(std::vector<std::string>& helptext);

void InitPanelSavestate(const struct _ConfigCategory& category);
void ExitPanelSavestate(void);
void RefreshPanelSavestate(void);
bool HelpPanelSavestate(std::vector<std::string>& helptext);

#ifdef ANDROID
void InitPanelOnScreen(const struct _ConfigCategory& category);
void ExitPanelOnScreen(void);
void RefreshPanelOnScreen(void);
bool HelpPanelOnScreen(std::vector<std::string> &helptext);
#endif

void refresh_all_panels(void);
void register_refresh_func(void (*func)(void));

void disable_resume(void);

bool ShowMessage(const char* title, const char* line1, const char* line2, const char* button1, const char* button2);
amiberry_hotkey ShowMessageForInput(const char* title, const char* line1, const char* button1);
bool SelectFolder(const char* title, char* value);
bool SelectFile(const char* title, char* value, const char* filter[], bool create = false);
bool EditFilesysVirtual(int unit_no);
bool EditFilesysHardfile(int unit_no);
bool CreateFilesysHardfile(void);
void ShowHelp(const char* title, const std::vector<std::string>& text);
void ShowDiskInfo(const char* title, const std::vector<std::string>& text);

bool LoadConfigByName(const char* name);
ConfigFileInfo* SearchConfigInList(const char* name);

extern void read_directory(const char* path, vector<string>* dirs, vector<string>* files);
extern void FilterFiles(vector<string>* files, const char* filter[]);

enum
{
	DIRECTION_NONE,
	DIRECTION_UP,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_RIGHT
};

bool HandleNavigation(int direction);
void PushFakeKey(SDL_Keycode inKey);

#define MAX_HD_DEVICES 6
extern void CreateDefaultDevicename(char* name);
extern bool DevicenameExists(const char* name);
extern int tweakbootpri(int bp, int ab, int dnm);

extern char* screenshot_filename;
extern int currentStateNum;
extern int delay_savestate_frame;
extern const int joystick_dead_zone;
extern int last_x;
extern int last_y;

extern void init_dispmanx_gui();
extern void update_gui_screen();
extern void cap_fps(Uint64 start);
extern long get_file_size(const std::string& filename);
extern bool download_file(const std::string& source, std::string destination);
extern void download_rtb(std::string filename);

#endif // GUI_HANDLING_H
