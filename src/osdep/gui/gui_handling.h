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
extern SDL_GameController* gui_controller;
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

extern void update_gui_screen();
extern void cap_fps(Uint64 start, int fps);
extern long get_file_size(const std::string& filename);
extern bool download_file(const std::string& source, std::string destination);
extern void download_rtb(std::string filename);

#endif // GUI_HANDLING_H
