#pragma once

#include <vector>
#include <string>
#include "imgui.h"

// Forward declaration to avoid pulling in SDL headers in every includer
struct SDL_Texture;

// Shared globals moved into the ImGui implementation files
extern std::vector<const char*> qs_models;
extern std::vector<const char*> qs_configs;
extern SDL_Texture* about_logo_texture;

// File dialog helpers used across imgui panels (defined in main_window.cpp)
void OpenFileDialog(const char* title, const char* filters, const std::string& initialPath);
bool ConsumeFileDialogResult(std::string& outPath);
void OpenDirDialog(const std::string& initialPath);
bool ConsumeDirDialogResult(std::string& outPath);

void BeginGroupBox(const char* name);
void EndGroupBox(const char* name);

void AmigaBevel(ImVec2 min, ImVec2 max, bool recessed);
void AmigaCircularBevel(ImVec2 center, float radius, bool recessed);
// Amiga-style widget wrappers
bool AmigaButton(const char* label, const ImVec2& size = ImVec2(0, 0));
bool AmigaCheckbox(const char* label, bool* v);
bool AmigaInputText(const char* label, char* buf, size_t buf_size);
bool AmigaRadioButton(const char* label, bool active);
bool AmigaRadioButton(const char* label, int* v, int v_button);

// Delayed tooltip on the previous widget (shown after hover delay)
void ShowHelpMarker(const char* desc);

void render_panel_about();
void render_panel_paths();
void render_panel_quickstart();
void Quickstart_ApplyDefaults(); // Helper to apply quickstart model defaults
void render_panel_configurations();
void render_panel_cpu();
void render_panel_chipset();
void render_panel_adv_chipset();
void render_panel_rom();
void render_panel_ram();
void render_panel_floppy();
void render_panel_hd();
void render_panel_expansions();
void copycpuboardmem(bool tomem);
void render_panel_rtg();
void render_panel_hwinfo();
void render_panel_display();
void render_panel_sound();
void render_panel_input();
void render_panel_io();
void render_panel_custom();
void render_panel_diskswapper();
void render_panel_misc();
void render_panel_prio();
void render_panel_savestates();
void render_panel_virtual_keyboard();
void render_panel_whdload();
void render_panel_themes();
void render_panel_filter();
