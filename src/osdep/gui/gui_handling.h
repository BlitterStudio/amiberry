#ifndef _GUI_HANDLING_H
#define _GUI_HANDLING_H

#define GUI_WIDTH  800
#define GUI_HEIGHT 480
#define DISTANCE_BORDER 15
#define DISTANCE_NEXT_X 15
#define DISTANCE_NEXT_Y 15
#define BUTTON_WIDTH 90
#define BUTTON_HEIGHT 30
#define SMALL_BUTTON_WIDTH 30
#define SMALL_BUTTON_HEIGHT 22
#define LABEL_HEIGHT 20
#define TEXTFIELD_HEIGHT 24
#define DROPDOWN_HEIGHT 22
#define SLIDER_HEIGHT 18
#define TITLEBAR_HEIGHT 24

typedef struct _ConfigCategory {
  const char *category;
  const char *imagepath;
  gcn::SelectorEntry *selector;
  gcn::Container *panel;
  void (*InitFunc) (const struct _ConfigCategory& category);
  void (*ExitFunc) (void);
  void (*RefreshFunc) (void);
  bool (*HelpFunc) (std::vector<std::string>&);
} ConfigCategory;

extern bool gui_running;
extern ConfigCategory categories[];
extern gcn::Gui* uae_gui;
extern gcn::Container* gui_top;
extern gcn::Color gui_baseCol;
extern gcn::SDLInput* gui_input;
extern SDL_Surface* gui_screen;

extern char currentDir[MAX_DPATH];
extern char last_loaded_config[MAX_DPATH];

extern int quickstart_start;
extern int quickstart_model;
extern int quickstart_conf;

typedef struct {
  char Name[MAX_DPATH];
  char FullPath[MAX_DPATH];
  char Description[MAX_DPATH];
} ConfigFileInfo;
extern std::vector<ConfigFileInfo*> ConfigFilesList;

void InitPanelPaths(const struct _ConfigCategory& category);
void ExitPanelPaths(void);
void RefreshPanelPaths(void);
bool HelpPanelPaths(std::vector<std::string> &helptext);
  
void InitPanelQuickstart(const struct _ConfigCategory& category);
void ExitPanelQuickstart(void);
void RefreshPanelQuickstart(void);
bool HelpPanelQuickstart(std::vector<std::string> &helptext);
  
void InitPanelConfig(const struct _ConfigCategory& category);
void ExitPanelConfig(void);
void RefreshPanelConfig(void);
bool HelpPanelConfig(std::vector<std::string> &helptext);
  
void InitPanelCPU(const struct _ConfigCategory& category);
void ExitPanelCPU(void);
void RefreshPanelCPU(void);
bool HelpPanelCPU(std::vector<std::string> &helptext);
  
void InitPanelChipset(const struct _ConfigCategory& category);
void ExitPanelChipset(void);
void RefreshPanelChipset(void);
bool HelpPanelChipset(std::vector<std::string> &helptext);
  
void InitPanelROM(const struct _ConfigCategory& category);
void ExitPanelROM(void);
void RefreshPanelROM(void);
bool HelpPanelROM(std::vector<std::string> &helptext);
  
void InitPanelRAM(const struct _ConfigCategory& category);
void ExitPanelRAM(void);
void RefreshPanelRAM(void);
bool HelpPanelRAM(std::vector<std::string> &helptext);
  
void InitPanelFloppy(const struct _ConfigCategory& category);
void ExitPanelFloppy(void);
void RefreshPanelFloppy(void);
bool HelpPanelFloppy(std::vector<std::string> &helptext);
  
void InitPanelHD(const struct _ConfigCategory& category);
void ExitPanelHD(void);
void RefreshPanelHD(void);
bool HelpPanelHD(std::vector<std::string> &helptext);
  
void InitPanelDisplay(const struct _ConfigCategory& category);
void ExitPanelDisplay(void);
void RefreshPanelDisplay(void);
bool HelpPanelDisplay(std::vector<std::string> &helptext);
  
void InitPanelSound(const struct _ConfigCategory& category);
void ExitPanelSound(void);
void RefreshPanelSound(void);
bool HelpPanelSound(std::vector<std::string> &helptext);

void InitPanelInput(const struct _ConfigCategory& category);
void ExitPanelInput(void);
void RefreshPanelInput(void);
bool HelpPanelInput(std::vector<std::string> &helptext);
  
void InitPanelMisc(const struct _ConfigCategory& category);
void ExitPanelMisc(void);
void RefreshPanelMisc(void);
bool HelpPanelMisc(std::vector<std::string> &helptext);
  
void InitPanelSavestate(const struct _ConfigCategory& category);
void ExitPanelSavestate(void);
void RefreshPanelSavestate(void);
bool HelpPanelSavestate(std::vector<std::string> &helptext);
  
void RefreshAllPanels(void);
void RegisterRefreshFunc(void (*func)(void));

void FocusBugWorkaround(gcn::Window *wnd);

void DisableResume(void);

bool ShowMessage(const char *title, const char *line1, const char *line2, const char *button1, const char *button2);
bool SelectFolder(const char *title, char *value);
bool SelectFile(const char *title, char *value, const char *filter[], bool create = false);
bool EditFilesysVirtual(int unit_no);
bool EditFilesysHardfile(int unit_no);
bool CreateFilesysHardfile(void);
void ShowHelp(const char *title, const std::vector<std::string>& text);
  
bool LoadConfigByName(const char *name);
ConfigFileInfo* SearchConfigInList(const char *name);

extern void ReadDirectory(const char *path, std::vector<std::string> *dirs, std::vector<std::string> *files);
extern void FilterFiles(std::vector<std::string> *files, const char *filter[]);

enum { DIRECTION_NONE, DIRECTION_UP, DIRECTION_DOWN, DIRECTION_LEFT, DIRECTION_RIGHT };
bool HandleNavigation(int direction);

#define MAX_HD_DEVICES 5
extern void CreateDefaultDevicename(char *name);
extern bool DevicenameExists(const char *name);
extern int tweakbootpri (int bp, int ab, int dnm);
  
extern char *screenshot_filename;
extern int currentStateNum;
extern int delay_savestate_frame;

#endif // _GUI_HANDLING_H
