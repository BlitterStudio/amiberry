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

typedef struct {
  char Name[MAX_DPATH];
  char FullPath[MAX_DPATH];
  char Description[MAX_DPATH];
} ConfigFileInfo;
extern std::vector<ConfigFileInfo*> ConfigFilesList;

void InitPanelPaths(const struct _ConfigCategory& category);
void ExitPanelPaths(void);
void RefreshPanelPaths(void);
void InitPanelConfig(const struct _ConfigCategory& category);
void ExitPanelConfig(void);
void RefreshPanelConfig(void);
void InitPanelCPU(const struct _ConfigCategory& category);
void ExitPanelCPU(void);
void RefreshPanelCPU(void);
void InitPanelChipset(const struct _ConfigCategory& category);
void ExitPanelChipset(void);
void RefreshPanelChipset(void);
void InitPanelROM(const struct _ConfigCategory& category);
void ExitPanelROM(void);
void RefreshPanelROM(void);
void InitPanelRAM(const struct _ConfigCategory& category);
void ExitPanelRAM(void);
void RefreshPanelRAM(void);
void InitPanelFloppy(const struct _ConfigCategory& category);
void ExitPanelFloppy(void);
void RefreshPanelFloppy(void);
void InitPanelHD(const struct _ConfigCategory& category);
void ExitPanelHD(void);
void RefreshPanelHD(void);
void InitPanelDisplay(const struct _ConfigCategory& category);
void ExitPanelDisplay(void);
void RefreshPanelDisplay(void);
void InitPanelSound(const struct _ConfigCategory& category);
void ExitPanelSound(void);
void RefreshPanelSound(void);
void InitPanelInput(const struct _ConfigCategory& category);
void ExitPanelInput(void);
void RefreshPanelInput(void);
void InitPanelMisc(const struct _ConfigCategory& category);
void ExitPanelMisc(void);
void RefreshPanelMisc(void);
void InitPanelSavestate(const struct _ConfigCategory& category);
void ExitPanelSavestate(void);
void RefreshPanelSavestate(void);

void RefreshAllPanels(void);

void DisableResume(void);

void InGameMessage(const char *msg);
bool ShowMessage(const char *title, const char *line1, const char *line2, const char *button1, const char *button2);
bool SelectFolder(const char *title, char *value);
bool SelectFile(const char *title, char *value, const char *filter[], bool create = false);
bool EditFilesysVirtual(int unit_no);
bool EditFilesysHardfile(int unit_no);
bool CreateFilesysHardfile(void);

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
