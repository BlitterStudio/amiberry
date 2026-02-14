#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <filesystem>
#include <SDL_image.h>
#include <dpi_handler.hpp>
#include <unordered_map>
#include <cmath>
#include <functional>

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "fsdb_host.h"
#include "autoconf.h"
#include "blkdev.h"
#include "inputdevice.h"
#include "xwin.h"
#include "custom.h"
#include "disk.h"
#include "imgui_internal.h"
#include "savestate.h"
#include "target.h"
#include "tinyxml2.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#if defined(_WIN32) && defined(USE_OPENGL)
#include "imgui_impl_opengl3.h"
#include <SDL_opengl.h>
#endif
#include "ImGuiFileDialog.h"
#include <array>
#include <fstream>
#include <sstream>
#include "imgui/imgui_panels.h"
#include "imgui/imgui_help_text.h"

bool ctrl_state = false, shift_state = false, alt_state = false, win_state = false;
int last_x = 0;
int last_y = 0;

bool gui_running = false;
static int last_active_panel = 3;
bool joystick_refresh_needed = false;

// Touch-drag scrolling state (converts finger swipes to ImGui scroll wheel events)
static bool touch_scrolling = false;
static float touch_scroll_accum = 0.0f;
static SDL_FingerID touch_scroll_finger = 0;

static TCHAR startup_title[MAX_STARTUP_TITLE] = _T("");
static TCHAR startup_message[MAX_STARTUP_MESSAGE] = _T("");

#ifdef _WIN32
static int saved_emu_x = 0, saved_emu_y = 0, saved_emu_w = 0, saved_emu_h = 0;
static Uint32 saved_emu_flags = 0;
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
static bool gui_use_opengl = false;
#endif

// Texture helpers: abstract SDL_Texture (SDLRenderer2) vs GLuint (OpenGL3)
ImTextureID gui_create_texture(SDL_Surface* surface, int* out_w, int* out_h)
{
	if (!surface) return ImTextureID_Invalid;
	if (out_w) *out_w = surface->w;
	if (out_h) *out_h = surface->h;
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl) {
		SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
		if (!rgba) return ImTextureID_Invalid;
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
		SDL_FreeSurface(rgba);
		return (ImTextureID)(intptr_t)tex;
	}
#endif
	AmigaMonitor* mon = &AMonitors[0];
	return (ImTextureID)SDL_CreateTextureFromSurface(mon->gui_renderer, surface);
}

void gui_destroy_texture(ImTextureID tex)
{
	if (tex == ImTextureID_Invalid) return;
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl) {
		GLuint gl_tex = (GLuint)(intptr_t)tex;
		glDeleteTextures(1, &gl_tex);
		return;
	}
#endif
	SDL_DestroyTexture((SDL_Texture*)tex);
}

void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	_tcsncpy(startup_title, title, MAX_STARTUP_TITLE);
	_tcsncpy(startup_message, msg, MAX_STARTUP_MESSAGE);
}

// Forward declarations used in this early block
static void apply_imgui_theme();

float DISTANCE_BORDER = 10;
float DISTANCE_NEXT_X = 15;
float DISTANCE_NEXT_Y = 15;
float BUTTON_WIDTH = 80;
float BUTTON_HEIGHT = 30;
float SMALL_BUTTON_WIDTH = 30;
float SMALL_BUTTON_HEIGHT = 22;
float LABEL_HEIGHT = 20;
float TEXTFIELD_HEIGHT = 20;
float DROPDOWN_HEIGHT = 20;
float SLIDER_HEIGHT = 20;
float TITLEBAR_HEIGHT = 24;
float SELECTOR_WIDTH = 165;
float SELECTOR_HEIGHT = 24;
float SCROLLBAR_WIDTH = 20;
static ImVec4 rgb_to_vec4(int r, int g, int b, float a = 1.0f) { return ImVec4{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, a }; }
static ImVec4 lighten(const ImVec4& c, float f) { return ImVec4{ std::min(c.x + f, 1.0f), std::min(c.y + f, 1.0f), std::min(c.z + f, 1.0f), c.w }; }
static ImVec4 darken(const ImVec4& c, float f) { return ImVec4{ std::max(c.x - f, 0.0f), std::max(c.y - f, 0.0f), std::max(c.z - f, 0.0f), c.w }; }
static bool parse_rgb_csv(const std::string& s, int& r, int& g, int& b) {
	std::stringstream ss(s); std::string tok;
	try {
		if (!std::getline(ss, tok, ',')) return false; r = std::stoi(tok);
		if (!std::getline(ss, tok, ',')) return false; g = std::stoi(tok);
		if (!std::getline(ss, tok, ',')) return false; b = std::stoi(tok);
	} catch (...) { return false; }
	auto clamp = [](int v){ return std::max(0, std::min(255, v)); };
	r = clamp(r); g = clamp(g); b = clamp(b);
	return true;
}

static void apply_imgui_theme()
{
	ImVec4 col_base = rgb_to_vec4(gui_theme.base_color.r, gui_theme.base_color.g, gui_theme.base_color.b);
	ImVec4 col_bg   = rgb_to_vec4(gui_theme.background_color.r, gui_theme.background_color.g, gui_theme.background_color.b);
	ImVec4 col_fg   = rgb_to_vec4(gui_theme.foreground_color.r, gui_theme.foreground_color.g, gui_theme.foreground_color.b);
	ImVec4 col_sel  = rgb_to_vec4(gui_theme.selection_color.r, gui_theme.selection_color.g, gui_theme.selection_color.b);
	ImVec4 col_act  = rgb_to_vec4(gui_theme.selector_active.r, gui_theme.selector_active.g, gui_theme.selector_active.b);
	ImVec4 col_inact= rgb_to_vec4(gui_theme.selector_inactive.r, gui_theme.selector_inactive.g, gui_theme.selector_inactive.b);
	ImVec4 col_text = rgb_to_vec4(gui_theme.font_color.r, gui_theme.font_color.g, gui_theme.font_color.b);

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text]                 = col_text;
	colors[ImGuiCol_TextDisabled]         = darken(col_text, 0.40f);
	colors[ImGuiCol_WindowBg]             = col_base;
	colors[ImGuiCol_ChildBg]              = col_base;
	colors[ImGuiCol_PopupBg]              = col_base;
	colors[ImGuiCol_Border]               = darken(col_base, 0.45f);
	colors[ImGuiCol_BorderShadow]         = lighten(col_base, 0.45f);
	colors[ImGuiCol_FrameBg]              = darken(col_base, 0.1f);
	colors[ImGuiCol_FrameBgHovered]       = lighten(col_base, 0.05f);
	colors[ImGuiCol_FrameBgActive]        = lighten(col_base, 0.10f);
	colors[ImGuiCol_TitleBg]              = col_base;
	colors[ImGuiCol_TitleBgActive]        = col_act;
	colors[ImGuiCol_TitleBgCollapsed]     = col_base;
	colors[ImGuiCol_MenuBarBg]            = col_base;
	colors[ImGuiCol_ScrollbarBg]          = darken(col_base, 0.15f);
	colors[ImGuiCol_ScrollbarGrab]        = col_base;
	colors[ImGuiCol_ScrollbarGrabHovered] = lighten(col_base, 0.05f);
	colors[ImGuiCol_ScrollbarGrabActive]  = col_act;
	colors[ImGuiCol_CheckMark]            = darken(col_act, 0.15f);
	colors[ImGuiCol_SliderGrab]           = col_act;
	colors[ImGuiCol_SliderGrabActive]     = lighten(col_act, 0.15f);
	colors[ImGuiCol_Button]               = col_base;
	colors[ImGuiCol_ButtonHovered]        = lighten(col_base, 0.05f);
	colors[ImGuiCol_ButtonActive]         = col_act;
	colors[ImGuiCol_Header]               = col_base;
	colors[ImGuiCol_HeaderHovered]        = lighten(col_base, 0.05f);
	colors[ImGuiCol_HeaderActive]         = col_act;
	colors[ImGuiCol_TextSelectedBg]       = col_sel;
	colors[ImGuiCol_TableHeaderBg]        = col_base;

	style.FrameBorderSize = 0.0f; // We will draw our own bevels
	style.WindowBorderSize = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.ChildBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	if (SDL_GetNumTouchDevices() > 0) {
		style.ScrollbarSize = 24.0f;  // Wider for touch targets
		style.GrabMinSize = 20.0f;
	} else {
		style.ScrollbarSize = 16.0f;
		style.GrabMinSize = 10.0f;
	}
	style.TabRounding = 0.0f;

	// Add a bit more padding inside windows/child areas
	style.WindowPadding = ImVec2(10.0f, 10.0f);
	style.FramePadding = ImVec2(5.0f, 4.0f);
}

void OpenDirDialogKey(const char* key, const std::string &initialPath)
{
	IGFD::FileDialogConfig config;
	config.path = initialPath;
	config.countSelectionMax = 1;
	config.flags = ImGuiFileDialogFlags_Modal;
	ImGuiFileDialog::Instance()->OpenDialog(key, "Choose Directory", nullptr, config);
}

namespace
{
	// Shared sizing/constraints used for both file/dir dialogs.
	// Generic consume helper: handles Display/IsOk/Close and delegates result extraction.
	bool ConsumeIGFDResult(const char* dialogKey, std::string& outPath,
		const std::function<std::string(ImGuiFileDialog*)>& getResult)
	{
		ImVec2 minSize(0,0), maxSize(FLT_MAX, FLT_MAX);
		ImGuiViewport* vp = ImGui::GetMainViewport();
		if (vp) {
			const float vw = vp->Size.x;
			const float vh = vp->Size.y;
			const float maxW = vw * 0.95f;
			const float maxH = vh * 0.90f;
			const float minW = vw * 0.50f;
			const float minH = vh * 0.50f;
			const float defW = vw * 0.70f;
			const float defH = vh * 0.70f;

			minSize = ImVec2(minW, minH);
			maxSize = ImVec2(maxW, maxH);

			ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(defW, defH), ImGuiCond_Appearing);
		}

		if (!ImGuiFileDialog::Instance()->Display(dialogKey, ImGuiWindowFlags_NoCollapse, minSize, maxSize))
			return false;

		bool ok = false;
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			outPath = getResult(ImGuiFileDialog::Instance());
			ok = true;
		}
		ImGuiFileDialog::Instance()->Close();
		return ok;
	}
}

bool ConsumeDirDialogResultKey(const char* key, std::string &outPath)
{
	return ConsumeIGFDResult(key, outPath,
		[](ImGuiFileDialog* dlg) { return dlg->GetCurrentPath(); });
}

bool IsDirDialogOpenKey(const char* key)
{
	return ImGuiFileDialog::Instance()->IsOpened(key);
}

void OpenFileDialogKey(const char* key, const char *title, const char *filters, const std::string &initialPath)
{
	IGFD::FileDialogConfig config;
	config.path = initialPath;
	config.countSelectionMax = 1;
	config.flags = ImGuiFileDialogFlags_Modal;
	ImGuiFileDialog::Instance()->OpenDialog(key, title, filters, config);
}

bool ConsumeFileDialogResultKey(const char* key, std::string &outPath)
{
	return ConsumeIGFDResult(key, outPath,
		[](ImGuiFileDialog* dlg) { return dlg->GetFilePathName(); });
}

bool IsFileDialogOpenKey(const char* key)
{
	return ImGuiFileDialog::Instance()->IsOpened(key);
}

void OpenDirDialog(const std::string &initialPath)
{
	OpenDirDialogKey("ChooseDirDlgKey", initialPath);
}

bool ConsumeDirDialogResult(std::string &outPath)
{
	return ConsumeDirDialogResultKey("ChooseDirDlgKey", outPath);
}

bool IsDirDialogOpen()
{
	return IsDirDialogOpenKey("ChooseDirDlgKey");
}

void OpenFileDialog(const char *title, const char *filters, const std::string &initialPath)
{
	OpenFileDialogKey("ChooseFileDlgKey", title, filters, initialPath);
}

bool ConsumeFileDialogResult(std::string &outPath)
{
	return ConsumeFileDialogResultKey("ChooseFileDlgKey", outPath);
}

bool IsFileDialogOpen()
{
	return IsFileDialogOpenKey("ChooseFileDlgKey");
}

#endif

// Helper: get usable bounds for a display index (fallback to full bounds)
static SDL_Rect get_display_usable_bounds(int display_index)
{
	SDL_Rect bounds{0,0,0,0};
	if (display_index < 0 || display_index >= SDL_GetNumVideoDisplays()) {
		display_index = 0;
	}
	if (SDL_GetDisplayUsableBounds(display_index, &bounds) != 0) {
		// Fallback if usable bounds fail
		SDL_GetDisplayBounds(display_index, &bounds);
	}
	return bounds;
}

// Helper: find the display index that contains the rect (by its topleft); fallback to primary
static int find_display_for_rect(const SDL_Rect& rect)
{
	const int nd = SDL_GetNumVideoDisplays();
	for (int i = 0; i < nd; ++i) {
		SDL_Rect b = get_display_usable_bounds(i);
		if (rect.x >= b.x && rect.x < b.x + b.w && rect.y >= b.y && rect.y < b.y + b.h)
			return i;
	}
	return 0;
}

// Helper: clamp rect position (and optionally size) into display usable bounds; returns true if changed
static bool clamp_rect_to_bounds(SDL_Rect& rect, const SDL_Rect& bounds, bool clamp_size)
{
	bool changed = false;
	SDL_Rect r = rect;
	if (clamp_size) {
		if (r.w > bounds.w) { r.w = bounds.w; changed = true; }
		if (r.h > bounds.h) { r.h = bounds.h; changed = true; }
	}
	if (r.x < bounds.x) { r.x = bounds.x; changed = true; }
	if (r.y < bounds.y) { r.y = bounds.y; changed = true; }
	if (r.x + r.w > bounds.x + bounds.w) { r.x = bounds.x + bounds.w - r.w; changed = true; }
	if (r.y + r.h > bounds.y + bounds.h) { r.y = bounds.y + bounds.h - r.h; changed = true; }
	if (changed) rect = r;
	return changed;
}

/*
* SDL Stuff we need
*/
SDL_Joystick* gui_joystick;
SDL_Surface* gui_screen;
SDL_Event gui_event;
SDL_Event touch_event;
SDL_Texture* gui_texture;
SDL_Rect gui_renderQuad;
SDL_Rect gui_window_rect{0, 0, GUI_WIDTH, GUI_HEIGHT};
static bool gui_window_moved = false; // track if user moved the GUI window

/* Flag for changes in rtarea:
  Bit 0: any HD in config?
  Bit 1: force because add/remove HD was clicked or new config loaded
  Bit 2: socket_emu on
  Bit 3: mousehack on
  Bit 4: rtgmem on
  Bit 5: chipmem larger than 2MB
  
  gui_rtarea_flags_onenter is set before GUI is shown, bit 1 may change during GUI display.
*/
static int gui_rtarea_flags_onenter;

static int gui_create_rtarea_flag(uae_prefs* p)
{
	auto flag = 0;

	if (p->mountitems > 0)
		flag |= 1;

	// We allow on-the-fly switching of this in Amiberry
#ifndef AMIBERRY
	if (p->input_tablet > 0)
		flag |= 8;
#endif

	return flag;
}

std::string get_xml_timestamp(const std::string& xml_filename)
{
	std::string result;
	tinyxml2::XMLDocument doc;
	auto error = false;

	auto* f = fopen(xml_filename.c_str(), _T("rb"));
	if (f)
	{
		auto err = doc.LoadFile(f);
		if (err != tinyxml2::XML_SUCCESS)
		{
			write_log(_T("Failed to parse '%s':  %d\n"), xml_filename.c_str(), err);
			error = true;
		}
		fclose(f);
	}
	else
	{
		error = true;
	}
	if (!error)
	{
		auto* whdbooter = doc.FirstChildElement("whdbooter");
		result = whdbooter->Attribute("timestamp");
	}
	if (!result.empty())
		return result;
	return "";
}

void gui_force_rtarea_hdchange()
{
	gui_rtarea_flags_onenter |= 2;
}

int disk_in_drive(int entry)
{
	for (int i = 0; i < 4; i++) {
		if (_tcslen(changed_prefs.dfxlist[entry]) > 0 && !_tcscmp(changed_prefs.dfxlist[entry], changed_prefs.floppyslots[i].df))
			return i;
	}
	return -1;
}

int disk_swap(int entry, int mode)
{
	int drv, i, drvs[4] = { -1, -1, -1, -1 };

	for (i = 0; i < MAX_SPARE_DRIVES; i++) {
		drv = disk_in_drive(i);
		if (drv >= 0)
			drvs[drv] = i;
	}
	if ((drv = disk_in_drive(entry)) >= 0) {
		if (mode < 0) {
			changed_prefs.floppyslots[drv].df[0] = 0;
			return 1;
		}

		if (_tcscmp(changed_prefs.floppyslots[drv].df, currprefs.floppyslots[drv].df)) {
			_tcscpy(changed_prefs.floppyslots[drv].df, currprefs.floppyslots[drv].df);
			disk_insert(drv, changed_prefs.floppyslots[drv].df);
		}
		else {
			changed_prefs.floppyslots[drv].df[0] = 0;
		}
		if (drvs[0] < 0 || drvs[1] < 0 || drvs[2] < 0 || drvs[3] < 0) {
			drv++;
			while (drv < 4 && drvs[drv] >= 0)
				drv++;
			if (drv < 4 && changed_prefs.floppyslots[drv].dfxtype >= 0) {
				_tcscpy(changed_prefs.floppyslots[drv].df, changed_prefs.dfxlist[entry]);
				disk_insert(drv, changed_prefs.floppyslots[drv].df);
			}
		}
		return 1;
	}
	for (i = 0; i < 4; i++) {
		if (drvs[i] < 0 && changed_prefs.floppyslots[i].dfxtype >= 0) {
			_tcscpy(changed_prefs.floppyslots[i].df, changed_prefs.dfxlist[entry]);
			disk_insert(i, changed_prefs.floppyslots[i].df);
			return 1;
		}
	}
	_tcscpy(changed_prefs.floppyslots[0].df, changed_prefs.dfxlist[entry]);
	disk_insert(0, changed_prefs.floppyslots[0].df);
	return 1;
}

void gui_restart()
{
	gui_running = false;
}

#ifdef USE_IMGUI
// IMGUI runtime state and helpers
static bool show_message_box = false;
static char message_box_title[128] = {0};
static std::string message_box_message;
static bool start_disabled = false; // Added for disable_resume logic

// Helper to draw Amiga 3D bevels
// recessed = true (In), recessed = false (Out)
void AmigaBevel(const ImVec2 min, const ImVec2 max, const bool recessed)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImU32 col_shine = ImGui::GetColorU32(ImGuiCol_BorderShadow); // Usually White
	const ImU32 col_shadow = ImGui::GetColorU32(ImGuiCol_Border);      // Usually Dark Gray

	ImU32 top_left = recessed ? col_shadow : col_shine;
	ImU32 bottom_right = recessed ? col_shine : col_shadow;

	// Top
	draw_list->AddLine(ImVec2(min.x, min.y), ImVec2(max.x - 1, min.y), top_left, 1.0f);
	// Left
	draw_list->AddLine(ImVec2(min.x, min.y), ImVec2(min.x, max.y - 1), top_left, 1.0f);
	// Bottom
	draw_list->AddLine(ImVec2(min.x, max.y - 1), ImVec2(max.x - 1, max.y - 1), bottom_right, 1.0f);
	// Right
	draw_list->AddLine(ImVec2(max.x - 1, min.y), ImVec2(max.x - 1, max.y - 1), bottom_right, 1.0f);
}

void AmigaCircularBevel(const ImVec2 center, const float radius, const bool recessed)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImU32 col_shine = ImGui::GetColorU32(ImGuiCol_BorderShadow); // Usually White
	const ImU32 col_shadow = ImGui::GetColorU32(ImGuiCol_Border);      // Usually Dark Gray

	ImU32 col_top_left = recessed ? col_shadow : col_shine;
	ImU32 col_bottom_right = recessed ? col_shine : col_shadow;

	// Top-Left Arc (135 deg to 315 deg, covering Left and Top)
	// 3PI/4 (135) to 7PI/4 (315)
	const float a_min_tl = 3.0f * IM_PI / 4.0f;
	const float a_max_tl = 7.0f * IM_PI / 4.0f;

	draw_list->PathArcTo(center, radius, a_min_tl, a_max_tl);
	draw_list->PathStroke(col_top_left, 0, 1.0f);

	// Bottom-Right Arc (315 deg to 135 deg, covering Right and Bottom)
	// -PI/4 (-45) to 3PI/4 (135)
	draw_list->PathArcTo(center, radius, -IM_PI / 4.0f, 3.0f * IM_PI / 4.0f);
	draw_list->PathStroke(col_bottom_right, 0, 1.0f);
}

void BeginGroupBox(const char* name)
{
	ImGui::BeginGroup();
	ImGui::PushID(name);

	// Reserve space for the title and the top border part
	ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() + 2.0f));
	ImGui::Indent(10.0f); // Add internal padding
}

void EndGroupBox(const char* name)
{
	ImGui::Unindent(10.0f);
	ImGui::PopID();
	ImGui::EndGroup();

	constexpr float text_padding = 8.0f;
	constexpr float box_padding = 4.0f;

	// Now draw the border and title
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 item_min = ImGui::GetItemRectMin();
	ImVec2 item_max = ImGui::GetItemRectMax();

	const float text_height = ImGui::GetTextLineHeight();
	const float border_y = item_min.y + text_height * 0.5f;

	ImU32 shadow_col = ImGui::GetColorU32(ImGuiCol_Border);
	ImU32 shine_col = ImGui::GetColorU32(ImGuiCol_BorderShadow);
	ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

	ImVec2 text_size = ImGui::CalcTextSize(name);
	float text_start_x = item_min.x + text_padding;
	item_min.x -= box_padding;

	ImVec2 content_max = ImGui::GetWindowContentRegionMax();
	ImVec2 window_pos = ImGui::GetWindowPos();
	float max_x = window_pos.x + content_max.x;

	if (item_max.x + box_padding > max_x)
		item_max.x = max_x;
	else
		item_max.x += box_padding;

	item_max.y += box_padding;

	// Draw Etched Group Border (Double line bevel)
	// Outer Shadow, Inner Shine
	draw_list->AddRect(ImVec2(item_min.x, border_y), ImVec2(item_max.x, item_max.y), shadow_col);
	draw_list->AddRect(ImVec2(item_min.x + 1, border_y + 1), ImVec2(item_max.x + 1, item_max.y + 1), shine_col);

	// Clear background for text
	draw_list->AddRectFilled(ImVec2(text_start_x - text_padding, border_y - text_height * 0.5f - 1), ImVec2(text_start_x + text_size.x + text_padding, border_y + text_height * 0.5f + 1), ImGui::GetColorU32(ImGuiCol_WindowBg));

	// Draw Title
	draw_list->AddText(ImVec2(text_start_x, item_min.y), text_col, name);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
}

void ShowHelpMarker(const char* desc)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

bool AmigaButton(const char* label, const ImVec2& size)
{
	ImGui::PushStyleColor(ImGuiCol_Border, 0); // Hide default border if any
	const bool pressed = ImGui::Button(label, size);
	ImGui::PopStyleColor();

	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const bool active = ImGui::IsItemActive();

	AmigaBevel(min, max, active);

	return pressed;
}

bool AmigaCheckbox(const char* label, bool* v)
{
	// Use ImGui's standard checkbox (checkmark) and only add the Amiga-style bevel.
	const bool changed = ImGui::Checkbox(label, v);

	const float sz = ImGui::GetFrameHeight();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const ImVec2 box_min = pos;
	const ImVec2 box_max = ImVec2(pos.x + sz, pos.y + sz);

	AmigaBevel(box_min, box_max, false);

	return changed;
}

bool AmigaInputText(const char* label, char* buf, const size_t buf_size)
{
	// Recessed frame
	const bool changed = ImGui::InputText(label, buf, buf_size);
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	// InputText frame is slightly smaller than the item rect if there is a label,
	// but here we usually use ## labels.
	// Actually, ImGui::InputText draws the frame.
	AmigaBevel(min, max, true);

	return changed;
}

// Amiga-style radio button wrapper (bool variant)
// Returns true when pressed (matching ImGui::RadioButton semantics).
bool AmigaRadioButton(const char* label, const bool active)
{
	const bool pressed = ImGui::RadioButton(label, active);

	// Draw a circular recessed bevel around the radio indicator
	const float sz = ImGui::GetFrameHeight();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const float radius = (sz * 0.5f) - 0.5f;
	const ImVec2 center = ImVec2(floor(pos.x + sz * 0.5f + 0.5f), floor(pos.y + sz * 0.5f + 0.5f));
	
	AmigaCircularBevel(center, radius, true);

	return pressed;
}

// Amiga-style radio button wrapper (int* variant)
bool AmigaRadioButton(const char* label, int* v, const int v_button)
{
	const bool pressed = ImGui::RadioButton(label, v, v_button);

	// Draw a circular recessed bevel around the radio indicator
	const float sz = ImGui::GetFrameHeight();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const float radius = (sz * 0.5f) - 0.5f;
	const ImVec2 center = ImVec2(floor(pos.x + sz * 0.5f + 0.5f), floor(pos.y + sz * 0.5f + 0.5f));

	AmigaCircularBevel(center, radius, true);

	return pressed;
}

// Sidebar icons cache
struct IconTex { ImTextureID tex; int w; int h; };
static std::unordered_map<std::string, IconTex> g_sidebar_icons;

static void release_sidebar_icons()
{
	for (auto& kv : g_sidebar_icons) {
		if (kv.second.tex) {
			gui_destroy_texture(kv.second.tex);
			kv.second.tex = ImTextureID_Invalid;
		}
	}
	g_sidebar_icons.clear();
}

static void ensure_sidebar_icons_loaded()
{
	for (int i = 0; categories[i].category != nullptr; ++i) {
		if (!categories[i].imagepath) continue;
		std::string key = categories[i].imagepath;
		if (g_sidebar_icons.find(key) != g_sidebar_icons.end())
			continue;
		const auto full = prefix_with_data_path(categories[i].imagepath);
		SDL_Surface* surf = IMG_Load(full.c_str());
		if (!surf) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Sidebar icon load failed: %s: %s", full.c_str(), SDL_GetError());
			g_sidebar_icons.emplace(key, IconTex{ImTextureID_Invalid, 0, 0});
			continue;
		}
		ImTextureID tex = gui_create_texture(surf, nullptr, nullptr);
		IconTex it{ tex, surf->w, surf->h };
		SDL_FreeSurface(surf);
		g_sidebar_icons.emplace(std::move(key), it);
	}
}
#endif

void amiberry_gui_init()
{
	AmigaMonitor* mon = &AMonitors[0];
	sdl_video_driver = SDL_GetCurrentVideoDriver();

#ifdef __ANDROID__
	if (mon->amiga_window && !mon->gui_window)
		mon->gui_window = mon->amiga_window;
	if (mon->amiga_renderer && !mon->gui_renderer)
		mon->gui_renderer = mon->amiga_renderer;
#endif

#ifdef _WIN32
	// On Windows, reuse the emulation window to avoid dual D3D11/OpenGL context
	// conflicts that cause crashes. This mirrors the Android/KMSDRM pattern.
	if (mon->amiga_window && !mon->gui_window)
		mon->gui_window = mon->amiga_window;
#ifndef USE_OPENGL
	if (mon->amiga_renderer && !mon->gui_renderer)
		mon->gui_renderer = mon->amiga_renderer;
#else
	// OpenGL path: keep GL context alive, destroy only emulation shaders.
	// ImGui will use the OpenGL3 backend directly instead of SDLRenderer2.
	if (mon->gui_window == mon->amiga_window && gl_context != nullptr) {
		destroy_shaders();
		gl_state_initialized = false;
	}
#endif
	if (mon->gui_window == mon->amiga_window) {
		SDL_GetWindowPosition(mon->gui_window, &saved_emu_x, &saved_emu_y);
		SDL_GetWindowSize(mon->gui_window, &saved_emu_w, &saved_emu_h);
		saved_emu_flags = SDL_GetWindowFlags(mon->gui_window);
		// Exit fullscreen for GUI if needed
		if (saved_emu_flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP))
			SDL_SetWindowFullscreen(mon->gui_window, 0);
		SDL_SetWindowSize(mon->gui_window, gui_window_rect.w, gui_window_rect.h);
		SDL_SetWindowPosition(mon->gui_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		SDL_SetWindowTitle(mon->gui_window, "Amiberry GUI");
		SDL_SetWindowResizable(mon->gui_window, SDL_TRUE);
		SDL_SetWindowGrab(mon->gui_window, SDL_FALSE);
	}
#endif

#if defined(_WIN32) && defined(USE_OPENGL)
	gui_use_opengl = (gl_context != nullptr && mon->gui_window == mon->amiga_window);
#endif

	if (sdl_video_driver != nullptr && strcmpi(sdl_video_driver, "KMSDRM") == 0)
	{
		kmsdrm_detected = true;
		if (!mon->gui_window && mon->amiga_window)
		{
			mon->gui_window = mon->amiga_window;
		}
		if (!mon->gui_renderer && mon->amiga_renderer)
		{
			mon->gui_renderer = mon->amiga_renderer;
		}
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}
	SDL_GetCurrentDisplayMode(0, &sdl_mode);

	if (!mon->gui_window)
	{
		write_log("Creating Amiberry GUI window...\n");
		int has_x = regqueryint(nullptr, _T("GUIPosX"), &gui_window_rect.x);
		int has_y = regqueryint(nullptr, _T("GUIPosY"), &gui_window_rect.y);
		if (!has_x || !has_y) {
			// Default to centered if we don't have stored position
			gui_window_rect.x = SDL_WINDOWPOS_CENTERED;
			gui_window_rect.y = SDL_WINDOWPOS_CENTERED;
		} else {
			// Clamp stored position/size to some display's usable bounds
			const int target_display = find_display_for_rect(gui_window_rect);
			SDL_Rect usable = get_display_usable_bounds(target_display);
			if (clamp_rect_to_bounds(gui_window_rect, usable, true)) {
				// Mark as adjusted so we persist corrected values
				gui_window_moved = true;
			}
		}

		Uint32 mode;
		if (!kmsdrm_detected)
		{
#ifdef __ANDROID__
			mode = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE;
			SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait PortraitUpsideDown");
#else
			// Only enable Windowed mode if we're running under a window environment
			mode = SDL_WINDOW_RESIZABLE;
#endif
		}
		else
		{
			// otherwise go for Full-window
			mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}

		if (currprefs.gui_alwaysontop)
			mode |= SDL_WINDOW_ALWAYS_ON_TOP;
		if (currprefs.start_minimized)
			mode |= SDL_WINDOW_HIDDEN;
		else
			mode |= SDL_WINDOW_SHOWN;
		// Set Window allow high DPI by default
		mode |= SDL_WINDOW_ALLOW_HIGHDPI;

		mon->gui_window = SDL_CreateWindow("Amiberry GUI",
										   SDL_WINDOWPOS_CENTERED,
										   SDL_WINDOWPOS_CENTERED,
										   gui_window_rect.w,
										   gui_window_rect.h,
										   mode);

		// Sync rect to actual window metrics (handles SDL_WINDOWPOS_CENTERED)
		int wx, wy, ww, wh;
		SDL_GetWindowPosition(mon->gui_window, &wx, &wy);
		SDL_GetWindowSize(mon->gui_window, &ww, &wh);
		gui_window_rect.x = wx;
		gui_window_rect.y = wy;
		gui_window_rect.w = ww;
		gui_window_rect.h = wh;

		// After creation, ensure window is fully visible; clamp to its current display
		int disp = SDL_GetWindowDisplayIndex(mon->gui_window);
		SDL_Rect usable = get_display_usable_bounds(disp);
		SDL_Rect clamped = gui_window_rect;
		bool adjusted = clamp_rect_to_bounds(clamped, usable, true);
		if (adjusted) {
			if (clamped.w != gui_window_rect.w || clamped.h != gui_window_rect.h)
				SDL_SetWindowSize(mon->gui_window, clamped.w, clamped.h);
			if (clamped.x != gui_window_rect.x || clamped.y != gui_window_rect.y)
				SDL_SetWindowPosition(mon->gui_window, clamped.x, clamped.y);
			gui_window_rect = clamped;
			gui_window_moved = true; // ensure we persist corrected values
		}

		// Center SDL window within the usable display area (respects taskbar)
		{
			int win_w, win_h; SDL_GetWindowSize(mon->gui_window, &win_w, &win_h);
			int cx = usable.x + (usable.w - win_w) / 2;
			int cy = usable.y + (usable.h - win_h) / 2;
			SDL_SetWindowPosition(mon->gui_window, cx, cy);
			gui_window_rect.x = cx;
			gui_window_rect.y = cy;
		}

		auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
		if (icon_surface != nullptr)
		{
			SDL_SetWindowIcon(mon->gui_window, icon_surface);
			SDL_FreeSurface(icon_surface);
		}
	}
	else if (kmsdrm_detected)
	{
		SDL_SetWindowSize(mon->gui_window, GUI_WIDTH, GUI_HEIGHT);
	}

#if defined(_WIN32) && defined(USE_OPENGL)
	if (!gui_use_opengl) {
#endif
		if (mon->gui_renderer == nullptr)
		{
			mon->gui_renderer = SDL_CreateRenderer(mon->gui_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			check_error_sdl(mon->gui_renderer == nullptr, "Unable to create a renderer:");
		}
		DPIHandler::set_render_scale(mon->gui_renderer);
#if defined(_WIN32) && defined(USE_OPENGL)
	}
#endif

#ifdef USE_IMGUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	if (SDL_GetNumTouchDevices() > 0)
		io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;      // Optimize for touch input

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	// Ensure WindowMinSize is valid after scaling (avoid ImGui assert)
	if (style.WindowMinSize.x < 1.0f || style.WindowMinSize.y < 1.0f)
		style.WindowMinSize = ImVec2(32.0f, 36.0f);

	// Load GUI theme (for font selection)
	load_theme(amiberry_options.gui_theme);
	// Apply theme colors to ImGui (map GUISAN theme fields)
	apply_imgui_theme();

	float scaling_factor = DPIHandler::get_layout_scale();

	// Apply scaling to GUI constants
	BUTTON_WIDTH = 90 * scaling_factor;
	BUTTON_HEIGHT = 30 * scaling_factor;
	SMALL_BUTTON_WIDTH = 30 * scaling_factor;
	SMALL_BUTTON_HEIGHT = 22 * scaling_factor;
	DISTANCE_BORDER = 10 * scaling_factor;
	DISTANCE_NEXT_X = 15 * scaling_factor;
	DISTANCE_NEXT_Y = 15 * scaling_factor;
	LABEL_HEIGHT = 20 * scaling_factor;
	TEXTFIELD_HEIGHT = 20 * scaling_factor;
	DROPDOWN_HEIGHT = 20 * scaling_factor;
	SLIDER_HEIGHT = 20 * scaling_factor;
	TITLEBAR_HEIGHT = 24 * scaling_factor;
	SELECTOR_WIDTH = 165 * scaling_factor;
	SELECTOR_HEIGHT = 24 * scaling_factor;
	SCROLLBAR_WIDTH = 20 * scaling_factor;
	style.ScaleAllSizes(scaling_factor);
	style.FontScaleDpi = scaling_factor;  // Enable DPI-aware font scaling (ImGui v1.92+)

	// Load custom font from data/ (default to AmigaTopaz.ttf)
	// Note: Don't manually scale font_px here - FontScaleDpi handles DPI scaling automatically
	const std::string font_file = gui_theme.font_name.empty() ? std::string("AmigaTopaz.ttf") : gui_theme.font_name;
	const std::string font_path = prefix_with_data_path(font_file);
	const float font_px = gui_theme.font_size > 0 ? static_cast<float>(gui_theme.font_size) : 15.0f;

	ImFont* loaded_font = nullptr;
	if (!font_path.empty() && std::filesystem::exists(font_path)) {
		loaded_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_px);
	}
	if (!loaded_font) {
		// Fallback to default font if loading failed
		loaded_font = io.Fonts->AddFontDefault();
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ImGui: failed to load font '%s', falling back to default", font_path.c_str());
	}
	io.FontDefault = loaded_font;

	// Setup Platform/Renderer backends
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl) {
		SDL_GL_MakeCurrent(mon->gui_window, gl_context);
		ImGui_ImplSDL2_InitForOpenGL(mon->gui_window, gl_context);
		ImGui_ImplOpenGL3_Init("#version 130");
	} else {
		ImGui_ImplSDL2_InitForSDLRenderer(mon->gui_window, mon->gui_renderer);
		ImGui_ImplSDLRenderer2_Init(mon->gui_renderer);
	}
#else
	ImGui_ImplSDL2_InitForSDLRenderer(AMonitors[0].gui_window, AMonitors[0].gui_renderer);
	ImGui_ImplSDLRenderer2_Init(AMonitors[0].gui_renderer);
#endif

	if (amiberry_options.quickstart_start && !emulating && strlen(last_loaded_config) == 0)
	{
		Quickstart_ApplyDefaults();
	}
#endif

	// Quickstart models and configs initialization
	qs_models.clear();
	for (const auto & amodel : amodels) {
		qs_models.push_back(amodel.name);
	}
	qs_configs.clear();
	for (const auto & config : amodels[0].configs) {
		qs_configs.push_back(config);
	}
}

void amiberry_gui_halt()
{
	AmigaMonitor* mon = &AMonitors[0];

#ifdef USE_IMGUI
	// Release any About panel resources
	if (about_logo_texture) {
		gui_destroy_texture(about_logo_texture);
		about_logo_texture = ImTextureID_Invalid;
	}
	// Release sidebar icon textures
	release_sidebar_icons();
	// Properly shutdown ImGui backends/context
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl)
		ImGui_ImplOpenGL3_Shutdown();
	else
		ImGui_ImplSDLRenderer2_Shutdown();
#else
	ImGui_ImplSDLRenderer2_Shutdown();
#endif
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
#endif

	if (gui_screen != nullptr)
	{
		SDL_FreeSurface(gui_screen);
		gui_screen = nullptr;
	}
	if (gui_texture != nullptr)
	{
		SDL_DestroyTexture(gui_texture);
		gui_texture = nullptr;
	}
	if (mon->gui_renderer && !kmsdrm_detected)
	{
#if defined(__ANDROID__)
		// Don't destroy the renderer on Android, as we reuse it
#elif defined(_WIN32)
		if (mon->gui_renderer == mon->amiga_renderer) {
			mon->gui_renderer = nullptr;  // Shared — don't destroy
		} else {
			SDL_DestroyRenderer(mon->gui_renderer);  // OpenGL path — separate renderer
			mon->gui_renderer = nullptr;
		}
#else
		if (mon->gui_renderer == SDL_GetRenderer(mon->amiga_window)) {
			mon->gui_renderer = nullptr;
		} else
		{
			SDL_DestroyRenderer(mon->gui_renderer);
			mon->gui_renderer = nullptr;
		}
#endif
	}

	if (mon->gui_window && !kmsdrm_detected) {
		if (gui_window_moved) {
			regsetint(nullptr, _T("GUIPosX"), gui_window_rect.x);
			regsetint(nullptr, _T("GUIPosY"), gui_window_rect.y);
		}
#if defined(__ANDROID__)
		// Don't destroy the window on Android, as we reuse it
#elif defined(_WIN32)
		if (mon->gui_window == mon->amiga_window) {
			mon->gui_window = nullptr;  // Shared — don't destroy
		} else {
			SDL_DestroyWindow(mon->gui_window);
			mon->gui_window = nullptr;
		}
#else
		SDL_DestroyWindow(mon->gui_window);
		mon->gui_window = nullptr;
#endif
	}

#ifdef _WIN32
	// Restore emulation window state
	if (mon->amiga_window && saved_emu_w > 0) {
		SDL_SetWindowTitle(mon->amiga_window, "Amiberry");
		SDL_SetWindowSize(mon->amiga_window, saved_emu_w, saved_emu_h);
		SDL_SetWindowPosition(mon->amiga_window, saved_emu_x, saved_emu_y);
		if (saved_emu_flags & SDL_WINDOW_FULLSCREEN)
			SDL_SetWindowFullscreen(mon->amiga_window, SDL_WINDOW_FULLSCREEN);
		else if (saved_emu_flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
			SDL_SetWindowFullscreen(mon->amiga_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		saved_emu_w = saved_emu_h = 0;
	}
#ifdef USE_OPENGL
	// GL context was kept alive — force full GL state reset for emulation
	if (gui_use_opengl && mon->amiga_window && gl_context != nullptr) {
		SDL_GL_MakeCurrent(mon->amiga_window, gl_context);
		gl_state_initialized = false;
		clear_loaded_shader_name();
	}
#endif
#endif
}

std::string get_filename_extension(const TCHAR* filename);

static void handle_drop_file_event(const SDL_Event& event)
{
	char* dropped_file = event.drop.file;
	const auto ext = get_filename_extension(dropped_file);

	if (strcasecmp(ext.c_str(), ".uae") == 0)
	{
		// Load configuration file
		uae_restart(&currprefs, -1, dropped_file);
		gui_running = false;
	}
	else if (strcasecmp(ext.c_str(), ".adf") == 0 || strcasecmp(ext.c_str(), ".adz") == 0 || strcasecmp(ext.c_str(), ".dms") == 0 || strcasecmp(ext.c_str(), ".ipf") == 0 || strcasecmp(ext.c_str(), ".zip") == 0)
	{
		// Insert floppy image
		disk_insert(0, dropped_file);
	}
	else if (strcasecmp(ext.c_str(), ".lha") == 0)
	{
		// WHDLoad archive
		whdload_auto_prefs(&currprefs, dropped_file);
		uae_restart(&currprefs, -1, nullptr);
		gui_running = false;
	}
	else if (strcasecmp(ext.c_str(), ".cue") == 0 || strcasecmp(ext.c_str(), ".iso") == 0 || strcasecmp(ext.c_str(), ".chd") == 0)
	{
		// CD image
		cd_auto_prefs(&currprefs, dropped_file);
		uae_restart(&currprefs, -1, nullptr);
		gui_running = false;
	}
	SDL_free(dropped_file);
}

#ifdef USE_IMGUI
// IMGUI runtime state and helpers

static bool show_disk_info = false;
static char disk_info_title[128] = {0};
static std::vector<std::string> disk_info_text;

void ShowMessageBox(const char* title, const char* message)
{
    // Safely copy and ensure null-termination
    strncpy(message_box_title, title ? title : "Message", sizeof(message_box_title) - 1);
    message_box_title[sizeof(message_box_title) - 1] = '\0';
    message_box_message = message ? message : "";
    show_message_box = true;
}

void ShowDiskInfo(const char* title, const std::vector<std::string>& text)
{
    strncpy(disk_info_title, title ? title : "Disk Info", sizeof(disk_info_title) - 1);
    disk_info_title[sizeof(disk_info_title) - 1] = '\0';
    disk_info_text = text;
    show_disk_info = true;
}

// Provide IMGUI categories using centralized panel list
ConfigCategory categories[] = {
#define PANEL(id, label, icon) { label, icon, render_panel_##id, help_text_##id },
	IMGUI_PANEL_LIST
#undef PANEL
	{ nullptr, nullptr, nullptr, nullptr }
};

void disable_resume()
{
	if (emulating)
	{
		start_disabled = true;
	}
}

void run_gui()
{
	gui_running = true;
	const AmigaMonitor* mon = &AMonitors[0];

	amiberry_gui_init();
	start_disabled = false;

	if (!emulating && amiberry_options.quickstart_start)
		last_active_panel = 2;

	// Main loop
	while (gui_running) {
		while (SDL_PollEvent(&gui_event)) {
			// Make sure ImGui sees all events
			ImGui_ImplSDL2_ProcessEvent(&gui_event);

			if (ControllerMap_HandleEvent(gui_event)) {
				continue;
			}

			if (gui_event.type == SDL_QUIT)	{
				uae_quit();
				gui_running = false;
			}
			else if (gui_event.type == SDL_KEYDOWN && !ImGui::GetIO().WantTextInput) {
				if (gui_event.key.keysym.sym == SDLK_F1) {
					// Show Help when F1 is pressed
					const char* help_ptr = nullptr;
					if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr)
						help_ptr = categories[last_active_panel].HelpText;
					if (help_ptr && *help_ptr)
						ShowMessageBox("Help", help_ptr);
				}
				else if (gui_event.key.keysym.sym == SDLK_q) {
					// Quit when Q is pressed
					uae_quit();
					gui_running = false;
				}
			}
			else if (gui_event.type == SDL_DROPFILE) {
				// Handle dropped files
				handle_drop_file_event(gui_event);
			}
			else if (gui_event.type == SDL_WINDOWEVENT) {
				// Handle window events, keep track of position and size
				if (gui_event.window.windowID == SDL_GetWindowID(mon->gui_window)) {
					switch (gui_event.window.event) {
					case SDL_WINDOWEVENT_MOVED:
						gui_window_rect.x = gui_event.window.data1;
						gui_window_rect.y = gui_event.window.data2;
						// Clamp move to current display usable bounds
						{
							int disp = SDL_GetWindowDisplayIndex(mon->gui_window);
							SDL_Rect usable = get_display_usable_bounds(disp);
							if (clamp_rect_to_bounds(gui_window_rect, usable, false)) {
								SDL_SetWindowPosition(mon->gui_window, gui_window_rect.x, gui_window_rect.y);
							}
						}
						gui_window_moved = true;
						break;
					case SDL_WINDOWEVENT_RESIZED:
						gui_window_rect.w = gui_event.window.data1;
						gui_window_rect.h = gui_event.window.data2;
						break;
					default:
						break;
					}
				}
				if (gui_event.window.event == SDL_WINDOWEVENT_CLOSE && gui_event.window.windowID == SDL_GetWindowID(mon->gui_window))
					gui_running = false;
			}
			// Touch-drag scrolling: convert finger swipes into ImGui scroll wheel events
			// SDL2 synthesizes mouse events from touch, but not mouse-wheel, so ImGui
			// child windows won't scroll from finger drags without this.
			else if (gui_event.type == SDL_FINGERDOWN) {
				touch_scroll_finger = gui_event.tfinger.fingerId;
				touch_scrolling = false;
				touch_scroll_accum = 0.0f;
			}
			else if (gui_event.type == SDL_FINGERMOTION
				&& gui_event.tfinger.fingerId == touch_scroll_finger) {
				int wh = 0;
				SDL_GetWindowSize(mon->gui_window, nullptr, &wh);
				const float dy_pixels = gui_event.tfinger.dy * static_cast<float>(wh);
				touch_scroll_accum += dy_pixels;

				// Dead-zone: require a minimum drag distance before scrolling,
				// so taps on buttons don't accidentally scroll
				const float drag_threshold = 8.0f * DPIHandler::get_layout_scale();
				if (!touch_scrolling && std::abs(touch_scroll_accum) > drag_threshold) {
					touch_scrolling = true;
					// Cancel the synthetic mouse-down so ImGui doesn't treat
					// the ongoing touch as a widget click-drag
					ImGui::GetIO().AddMouseButtonEvent(0, false);
				}
				if (touch_scrolling) {
					const float line_height = ImGui::GetTextLineHeightWithSpacing();
					if (line_height > 0.0f) {
						const float scroll_lines = dy_pixels / (line_height * 3.0f);
						ImGui::GetIO().AddMouseWheelEvent(0.0f, scroll_lines);
					}
				}
			}
			else if (gui_event.type == SDL_FINGERUP
				&& gui_event.tfinger.fingerId == touch_scroll_finger) {
				touch_scrolling = false;
			}
		}

#ifdef __ANDROID__
		// Toggle native soft keyboard based on ImGui's text input state.
		// ImGui sets WantTextInput when a text field is active.
		{
			bool want_text = ImGui::GetIO().WantTextInput;
			bool text_active = SDL_IsTextInputActive();
			if (want_text && !text_active)
				SDL_StartTextInput();
			else if (!want_text && text_active)
				SDL_StopTextInput();
		}
#endif

		// Skip rendering when minimized to save CPU
		if (SDL_GetWindowFlags(mon->gui_window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
#if defined(_WIN32) && defined(USE_OPENGL)
		if (gui_use_opengl)
			ImGui_ImplOpenGL3_NewFrame();
		else
			ImGui_ImplSDLRenderer2_NewFrame();
#else
		ImGui_ImplSDLRenderer2_NewFrame();
#endif
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// While touch-scrolling, suppress hover highlight so items don't
		// visually react to the finger position during a scroll gesture
		if (touch_scrolling) {
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		// Compute button bar height from style
		const auto button_bar_height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;

		// Make the main window occupies the full SDL window (no smaller inner window) and set its size
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 work_pos = vp->WorkPos;
		ImVec2 work_size = vp->WorkSize;
		ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(work_size, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::Begin("Amiberry", &gui_running, hostFlags);
		ImGui::PopStyleVar(2);

		// Determine sidebar width based on current window content width, with sane clamps
		const float content_width = ImGui::GetContentRegionAvail().x;
		const float sidebar_width = content_width * 0.22f;

		// Sidebar
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
		ImGui::BeginChild("Sidebar", ImVec2(sidebar_width - 2.0f, -button_bar_height));
		ImGui::Indent(4.0f);
		ImGui::Dummy(ImVec2(0, 2.0f));

		// Ensure icons are ready
		ensure_sidebar_icons_loaded();

 		const ImGuiStyle& s = ImGui::GetStyle();
		const float base_text_h = ImGui::GetTextLineHeight();
		const float base_row_h = ImGui::GetTextLineHeightWithSpacing();
		// Make icons larger than text height using gui_scale
		const float icon_h_target = base_text_h;
		const float row_h = std::max(base_row_h, icon_h_target + 2.0f * s.FramePadding.y);
		const ImVec4 col_act = rgb_to_vec4(gui_theme.selector_active.r, gui_theme.selector_active.g, gui_theme.selector_active.b);
		for (int i = 0; categories[i].category != nullptr; ++i) {
			const bool selected = (last_active_panel == i);
			// Full-width selectable row with custom height
			if (selected) {
				ImGui::PushStyleColor(ImGuiCol_Header, col_act);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, lighten(col_act, 0.05f));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, lighten(col_act, 0.10f));
			}
			if (ImGui::Selectable( (std::string("##cat_") + std::to_string(i)).c_str(), selected, 0, ImVec2(ImGui::GetContentRegionAvail().x, row_h))) {
				last_active_panel = i;
			}
			if (selected) {
				ImGui::PopStyleColor(3);
			}
			// Draw icon + text inside the selectable rect
			ImVec2 rmin = ImGui::GetItemRectMin();
			ImVec2 rmax = ImGui::GetItemRectMax();
			ImVec2 pos = rmin;
			float pad_y = s.FramePadding.y;
			float pad_x = s.FramePadding.x;
			float avail_h = (rmax.y - rmin.y) - 2.0f * pad_y;
			float icon_h = std::min(icon_h_target, avail_h > 0.0f ? avail_h : (row_h - 2.0f * pad_y));
			float icon_w = icon_h; // default square
			ImTextureID icon_tex = ImTextureID_Invalid;
			int tex_w = 0, tex_h = 0;
			if (categories[i].imagepath) {
				auto it = g_sidebar_icons.find(categories[i].imagepath);
				if (it != g_sidebar_icons.end() && it->second.tex) {
					icon_tex = it->second.tex; tex_w = it->second.w; tex_h = it->second.h;
					if (tex_w > 0 && tex_h > 0) {
						float aspect = (float)tex_w / (float)tex_h;
						icon_w = icon_h * aspect;
					}
				}
			}
			ImVec2 icon_p0 = ImVec2(pos.x + pad_x, pos.y + pad_y);
			ImVec2 icon_p1 = ImVec2(icon_p0.x + icon_w, icon_p0.y + icon_h);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			if (icon_tex) {
				dl->AddImage(icon_tex, icon_p0, icon_p1);
			} else {
				// Fallback: small filled square as placeholder
				dl->AddRectFilled(icon_p0, icon_p1, ImGui::GetColorU32(ImGuiCol_TextDisabled), 3.0f);
			}
			// Text baseline: vertically center with the row
			float text_x = icon_p1.x + pad_x;
			float text_y = pos.y + ((rmax.y - rmin.y) - ImGui::GetTextLineHeight()) * 0.5f;
			dl->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), categories[i].category);
		}
		ImGui::Unindent(4.0f);
		ImGui::EndChild();
		// Draw Sidebar bevel (outside child to avoid clipping)
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		AmigaBevel(ImVec2(min.x - 1, min.y - 1), ImVec2(max.x + 1, max.y + 1), false);

		ImGui::SameLine();

		// Content
		ImGui::BeginChild("Content", ImVec2(0, -button_bar_height));

		// Render the currently active panel
		if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr) {
			if (categories[last_active_panel].RenderFunc)
				categories[last_active_panel].RenderFunc();
		}
		ImGui::EndChild();

		// Controller mapping modal (if active)
		ControllerMap_RenderModal();

		ImGui::Dummy(ImVec2(0, 2.0f));

		// Button bar
		// Left-aligned buttons (Shutdown, Reset, Quit, Restart, Help)
		if (!amiberry_options.disable_shutdown_button) {
			if (AmigaButton("Shutdown", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
				uae_quit();
				gui_running = false;
				host_poweroff = true;
			}
			ImGui::SameLine();
		}
		if (AmigaButton("Reset", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			uae_reset(1, 1);
			gui_running = false;
		}
		ImGui::SameLine();
		if (AmigaButton("Quit", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			uae_quit();
			gui_running = false;
		}
		ImGui::SameLine();
		if (AmigaButton("Restart", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			char tmp[MAX_DPATH] = {0};
			get_configuration_path(tmp, sizeof tmp);
			if (strlen(last_loaded_config) > 0) {
				strncat(tmp, last_loaded_config, MAX_DPATH - 1);
				strncat(tmp, ".uae", MAX_DPATH - 10);
			} else {
				strncat(tmp, OPTIONSFILENAME, MAX_DPATH - 1);
				strncat(tmp, ".uae", MAX_DPATH - 10);
			}
			uae_restart(&changed_prefs, -1, tmp);
			gui_running = false;
		}

		// Right-aligned buttons
		float right_buttons_width = (BUTTON_WIDTH * 2) + style.ItemSpacing.x;
		ImGui::SameLine();
		float cursor_x = ImGui::GetWindowWidth() - right_buttons_width - style.WindowPadding.x;
		if (cursor_x < ImGui::GetCursorPosX()) cursor_x = ImGui::GetCursorPosX(); // Prevent overlap
		ImGui::SetCursorPosX(cursor_x);

		if (start_disabled)
			ImGui::BeginDisabled();

		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, lighten(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive], 0.05f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, lighten(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive], 0.10f));
		if (emulating) {
			if (AmigaButton("Resume", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
				gui_running = false;
			}
		} else {
			if (AmigaButton("Start", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
				uae_reset(0, 1);
				gui_running = false;
			}
		}
		ImGui::PopStyleColor(3);

		if (start_disabled)
			ImGui::EndDisabled();
		ImGui::SameLine();
		if (AmigaButton("Help", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			const char* help_ptr = nullptr;
			if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr && categories[last_active_panel].HelpText != nullptr)
				help_ptr = categories[last_active_panel].HelpText;
			if (help_ptr && *help_ptr)
				ShowMessageBox("Help", help_ptr);
		}

		if (show_message_box) {
            // Center the dialog on appearing; use viewport-relative sizes for proper scaling on all platforms
            const float vw = vp->Size.x;
            const float vh = vp->Size.y;
            const float desired_w = vw * 0.56f;
            ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(desired_w, 0.0f), ImGuiCond_Appearing);
            // Lock width to desired_w; allow height to grow up to 85% viewport
            ImGui::SetNextWindowSizeConstraints(ImVec2(desired_w, 0.0f), ImVec2(desired_w, vh * 0.85f));

            ImGui::OpenPopup(message_box_title);
            if (ImGui::BeginPopupModal(message_box_title, &show_message_box, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                // Scrollable message area; height scales with viewport
                const float max_child_h = vh * 0.65f;
                ImGui::BeginChild("MessageScroll", ImVec2(0, max_child_h), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextWrapped("%s", message_box_message.c_str());
                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::Separator();

                // Align OK button to the right
                const float btn_w = BUTTON_WIDTH;
                const float btn_h = BUTTON_HEIGHT;
                float avail = ImGui::GetContentRegionAvail().x;
                if (avail > btn_w)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w));
                if (AmigaButton("OK", ImVec2(btn_w, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    show_message_box = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsWindowAppearing())
                    ImGui::SetItemDefaultFocus();

                ImGui::EndPopup();
            }
        }

		if (show_disk_info) {
            const float vw = vp->Size.x;
            const float vh = vp->Size.y;
            const float desired_w = vw * 0.7f;

            ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(desired_w, vh * 0.8f), ImGuiCond_Appearing);

            ImGui::OpenPopup(disk_info_title);
            if (ImGui::BeginPopupModal(disk_info_title, &show_disk_info, ImGuiWindowFlags_NoSavedSettings))
            {
                // Child region for text
                const float footer_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
                ImGui::BeginChild("DiskInfoText", ImVec2(0, -footer_h), true);

                for (const auto& line : disk_info_text) {
                    ImGui::TextUnformatted(line.c_str());
                }

                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::Separator();

                // Button alignment logic
                const float btn_w = BUTTON_WIDTH;
                float avail = ImGui::GetContentRegionAvail().x;
                if (avail > btn_w)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w));

                if (ImGui::Button("Close", ImVec2(btn_w, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    show_disk_info = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

		ImGui::End();

		// Pop touch-scrolling hover suppression colors
		if (touch_scrolling) {
			ImGui::PopStyleColor(3);
		}

		// Rendering
		ImGui::Render();
#if defined(_WIN32) && defined(USE_OPENGL)
		if (gui_use_opengl) {
			glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
			glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(mon->gui_window);
		} else {
			const ImGuiIO& render_io = ImGui::GetIO();
			SDL_RenderSetScale(mon->gui_renderer, render_io.DisplayFramebufferScale.x, render_io.DisplayFramebufferScale.y);
			SDL_SetRenderDrawColor(mon->gui_renderer, static_cast<Uint8>(0.45f * 255), static_cast<Uint8>(0.55f * 255),
							   static_cast<Uint8>(0.60f * 255), static_cast<Uint8>(1.00f * 255));
			SDL_RenderClear(mon->gui_renderer);
			ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);
			SDL_RenderPresent(mon->gui_renderer);
		}
#else
		const ImGuiIO& render_io = ImGui::GetIO();
		SDL_RenderSetScale(mon->gui_renderer, render_io.DisplayFramebufferScale.x, render_io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(mon->gui_renderer, static_cast<Uint8>(0.45f * 255), static_cast<Uint8>(0.55f * 255),
						   static_cast<Uint8>(0.60f * 255), static_cast<Uint8>(1.00f * 255));
		SDL_RenderClear(mon->gui_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);
		SDL_RenderPresent(mon->gui_renderer);
#endif
	}

	amiberry_gui_halt();

	// Reset counter for access violations
	init_max_signals();
}
#endif
