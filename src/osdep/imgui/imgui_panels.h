#pragma once

#include <vector>
#include <string>
#include "imgui.h"
#include "file_dialog.h"
#include "gui/icons_fa.h"

// Forward declaration to avoid pulling in SDL headers in every includer
struct SDL_Texture;
union SDL_Event;

// Shared globals moved into the ImGui implementation files
extern std::vector<const char*> qs_models;
extern std::vector<const char*> qs_configs;
extern ImTextureID about_logo_texture;

// Shared catalogs used by panels that expose the same choices.
const std::vector<std::string>& get_sound_device_names();
const std::vector<std::string>& get_available_theme_names();
const std::vector<std::string>& get_available_bezel_names();

struct InputDeviceOption
{
	std::string label;
	std::string config_value;
	int id;
};

const std::vector<InputDeviceOption>& get_input_device_options();
bool InputDeviceCombo(const char* id, int current_index, const char* fallback_preview,
	int* selected_index);

inline constexpr int SOUND_BUFFER_SIZES[] = {
	1024, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 32768, 65536
};

inline int get_sound_buffer_size_index(const int size)
{
	if (size < SOUND_BUFFER_SIZES[0])
		return 0;

	int index = 0;
	while (index + 1 < IM_ARRAYSIZE(SOUND_BUFFER_SIZES) && SOUND_BUFFER_SIZES[index] < size)
		++index;
	return index + 1;
}

// Texture helpers that abstract SDL_Texture (SDLRenderer2) vs GLuint (OpenGL3)
struct SDL_Surface;
ImTextureID gui_create_texture(SDL_Surface* surface, int* out_w, int* out_h);
void gui_destroy_texture(ImTextureID tex);

bool BeginGroupBox(const char* name, bool collapsible = false, bool default_open = true);
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

void render_panel_play();
bool play_has_content_selection();
void play_clear_content_selection();
bool play_is_adjusting_selected_content_model();
bool play_prepare_selected_content_for_start();
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
void render_panel_global_settings();
// Shared hotkey picker used by panels that edit host keyboard shortcuts.
bool HotkeyPicker(const char* id, char* config_value, size_t config_value_size);
void HotkeyCapture_RenderPopup();
// Returns true while the hotkey-capture modal is open.
// Used by the GUI main loop to suppress its own keyboard shortcuts while
// the user is picking a key combination.
bool HotkeyCapture_IsActive();
void render_panel_prio();
void render_panel_savestates();
void render_panel_virtual_keyboard();
void render_panel_whdload();
void render_panel_themes();
void render_panel_filter();

// Controller mapping modal (ImGui)
void ControllerMap_Open(int device, bool map_touchpad);
bool ControllerMap_IsActive();
bool ControllerMap_ConsumeResult(std::string& out_mapping);
bool ControllerMap_HandleEvent(const SDL_Event& event);
void ControllerMap_RenderModal();
