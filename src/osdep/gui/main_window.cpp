#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <SDL_image.h>
#include <SDL_ttf.h>
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
#include "savestate.h"
#include "target.h"
#include "tinyxml2.h"

#ifdef USE_GUISAN
#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#endif

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "ImGuiFileDialog.h"
#include <array>
#include <fstream>
#include <sstream>
#include "imgui/imgui_panels.h"

bool ctrl_state = false, shift_state = false, alt_state = false, win_state = false;
int last_x = 0;
int last_y = 0;

bool gui_running = false;
static int last_active_panel = 3;
bool joystick_refresh_needed = false;

static TCHAR startup_title[MAX_STARTUP_TITLE] = _T("");
static TCHAR startup_message[MAX_STARTUP_MESSAGE] = _T("");

void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	_tcsncpy(startup_title, title, MAX_STARTUP_TITLE);
	_tcsncpy(startup_message, msg, MAX_STARTUP_MESSAGE);
}

// Forward declarations used in this early block
static void apply_imgui_theme_from_theme_file(const std::string& theme_file);
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

static void apply_imgui_theme_from_theme_file(const std::string& theme_file)
{
	// Build full theme file path
	std::string path = get_themes_path();
	path += theme_file;
	std::ifstream in(path);
	if (!in.is_open())
		return; // keep current ImGui style if theme not found

	// Defaults if keys are missing
	ImVec4 col_base = ImVec4(0.25f,0.25f,0.25f,1.0f);
	ImVec4 col_bg   = ImVec4(0.16f,0.16f,0.17f,1.0f);
	ImVec4 col_fg   = ImVec4(0.86f,0.86f,0.86f,1.0f);
	ImVec4 col_sel  = ImVec4(0.20f,0.45f,0.78f,1.0f);
	ImVec4 col_act  = col_sel;
	ImVec4 col_inact= col_base;
	ImVec4 col_text = col_fg;

	std::string line;
	while (std::getline(in, line)) {
		const auto eq = line.find('=');
		if (eq == std::string::npos) continue;
		std::string key = line.substr(0, eq);
		std::string val = line.substr(eq + 1);
		int r=0,g=0,b=0;
		if (key == "base_color" && parse_rgb_csv(val, r, g, b)) col_base = rgb_to_vec4(r,g,b);
		else if (key == "background_color" && parse_rgb_csv(val, r, g, b)) col_bg = rgb_to_vec4(r,g,b);
		else if (key == "foreground_color" && parse_rgb_csv(val, r, g, b)) col_fg = rgb_to_vec4(r,g,b);
		else if (key == "selection_color" && parse_rgb_csv(val, r, g, b)) col_sel = rgb_to_vec4(r,g,b);
		else if (key == "selector_active" && parse_rgb_csv(val, r, g, b)) col_act = rgb_to_vec4(r,g,b);
		else if (key == "selector_inactive" && parse_rgb_csv(val, r, g, b)) col_inact = rgb_to_vec4(r,g,b);
		else if (key == "font_color" && parse_rgb_csv(val, r, g, b)) col_text = rgb_to_vec4(r,g,b);
	}
	in.close();

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text]                 = col_text;
	colors[ImGuiCol_TextDisabled]         = darken(col_text, 0.40f);
	colors[ImGuiCol_WindowBg]             = col_bg;
	colors[ImGuiCol_ChildBg]              = darken(col_bg, 0.02f);
	colors[ImGuiCol_PopupBg]              = darken(col_bg, 0.05f);
	colors[ImGuiCol_Border]               = darken(col_base, 0.35f);
	colors[ImGuiCol_BorderShadow]         = darken(col_base, 0.50f);
	colors[ImGuiCol_FrameBg]              = col_base;
	colors[ImGuiCol_FrameBgHovered]       = lighten(col_base, 0.06f);
	colors[ImGuiCol_FrameBgActive]        = lighten(col_base, 0.10f);
	colors[ImGuiCol_TitleBg]              = darken(col_bg, 0.05f);
	colors[ImGuiCol_TitleBgActive]        = darken(col_bg, 0.10f);
	colors[ImGuiCol_TitleBgCollapsed]     = darken(col_bg, 0.08f);
	colors[ImGuiCol_MenuBarBg]            = darken(col_bg, 0.02f);
	colors[ImGuiCol_ScrollbarBg]          = darken(col_bg, 0.03f);
	colors[ImGuiCol_ScrollbarGrab]        = col_base;
	colors[ImGuiCol_ScrollbarGrabHovered] = lighten(col_base, 0.08f);
	colors[ImGuiCol_ScrollbarGrabActive]  = lighten(col_base, 0.12f);
	colors[ImGuiCol_CheckMark]            = col_sel;
	colors[ImGuiCol_SliderGrab]           = col_sel;
	colors[ImGuiCol_SliderGrabActive]     = lighten(col_sel, 0.10f);
	colors[ImGuiCol_Button]               = col_base;
	colors[ImGuiCol_ButtonHovered]        = lighten(col_base, 0.07f);
	colors[ImGuiCol_ButtonActive]         = lighten(col_base, 0.12f);
	colors[ImGuiCol_Header]               = col_sel;
	colors[ImGuiCol_HeaderHovered]        = lighten(col_sel, 0.10f);
	colors[ImGuiCol_HeaderActive]         = lighten(col_sel, 0.15f);
	colors[ImGuiCol_Separator]            = darken(col_base, 0.30f);
	colors[ImGuiCol_SeparatorHovered]     = lighten(col_base, 0.10f);
	colors[ImGuiCol_SeparatorActive]      = lighten(col_base, 0.15f);
	colors[ImGuiCol_ResizeGrip]           = col_inact;
	colors[ImGuiCol_ResizeGripHovered]    = lighten(col_inact, 0.10f);
	colors[ImGuiCol_ResizeGripActive]     = lighten(col_inact, 0.15f);
	colors[ImGuiCol_Tab]                  = darken(col_bg, 0.02f);
	colors[ImGuiCol_TabHovered]           = lighten(col_sel, 0.15f);
	colors[ImGuiCol_TabActive]            = lighten(col_sel, 0.10f);
	colors[ImGuiCol_TabUnfocused]         = darken(col_bg, 0.02f);
	colors[ImGuiCol_TabUnfocusedActive]   = darken(col_bg, 0.01f);
	colors[ImGuiCol_PlotLines]            = col_act;
	colors[ImGuiCol_PlotLinesHovered]     = lighten(col_act, 0.10f);
	colors[ImGuiCol_PlotHistogram]        = col_sel;
	colors[ImGuiCol_PlotHistogramHovered] = lighten(col_sel, 0.10f);
	colors[ImGuiCol_TextSelectedBg]       = ImVec4(col_sel.x, col_sel.y, col_sel.z, 0.35f);
	colors[ImGuiCol_NavHighlight]         = col_sel;
	colors[ImGuiCol_NavWindowingHighlight]= ImVec4(1,1,1,0.70f);
	colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0,0,0,0.20f);
	colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0,0,0,0.35f);
}

void OpenDirDialog(const std::string &initialPath)
{
	IGFD::FileDialogConfig config;
	config.path = initialPath;
	config.countSelectionMax = 1;
	config.flags = ImGuiFileDialogFlags_Modal;
	ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose Directory", nullptr, config);
}

bool ConsumeDirDialogResult(std::string &outPath)
{
    // Ensure a reasonable initial and minimum size on first open
    // Apply only on first use so user-resized dimensions persist thereafter
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float vw = vp ? vp->Size.x : ImGui::GetIO().DisplaySize.x;
        const float vh = vp ? vp->Size.y : ImGui::GetIO().DisplaySize.y;
        const float maxW = std::max(320.0f, vw * 0.95f);
        const float maxH = std::max(240.0f, vh * 0.90f);
        const float minW = std::min(720.0f, maxW);
        const float minH = std::min(480.0f, maxH);
        const float defW = std::clamp(vw * 0.60f, minW, maxW);
        const float defH = std::clamp(vh * 0.60f, minH, maxH);
        if (vp)
            ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(defW, defH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(minW, minH), ImVec2(maxW, maxH));
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey"))
    {
        bool ok = false;
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            outPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            ok = true;
        }
        ImGuiFileDialog::Instance()->Close();
        return ok;
    }
    return false;
}

void OpenFileDialog(const char *title, const char *filters, const std::string &initialPath)
{
	IGFD::FileDialogConfig config;
	config.path = initialPath;
	config.countSelectionMax = 1;
	config.flags = ImGuiFileDialogFlags_Modal;
	ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", title, filters, config);
}

bool ConsumeFileDialogResult(std::string &outPath)
{
    // Ensure a reasonable initial and minimum size on first open
    // Apply only on first use so user-resized dimensions persist thereafter
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float vw = vp ? vp->Size.x : ImGui::GetIO().DisplaySize.x;
        const float vh = vp ? vp->Size.y : ImGui::GetIO().DisplaySize.y;
        const float maxW = std::max(320.0f, vw * 0.95f);
        const float maxH = std::max(240.0f, vh * 0.90f);
        const float minW = std::min(720.0f, maxW);
        const float minH = std::min(480.0f, maxH);
        const float defW = std::clamp(vw * 0.60f, minW, maxW);
        const float defH = std::clamp(vh * 0.60f, minH, maxH);
        if (vp)
            ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(defW, defH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(minW, minH), ImVec2(maxW, maxH));
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        bool ok = false;
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            outPath = ImGuiFileDialog::Instance()->GetFilePathName();
            ok = true;
        }
        ImGuiFileDialog::Instance()->Close();
        return ok;
    }
    return false;
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

#ifdef USE_GUISAN
ConfigCategory categories[] = {
	{"About", "amigainfo.png", nullptr, nullptr, InitPanelAbout, ExitPanelAbout, RefreshPanelAbout,
		HelpPanelAbout
	},
	{"Paths", "paths.png", nullptr, nullptr, InitPanelPaths, ExitPanelPaths, RefreshPanelPaths, HelpPanelPaths},
	{"Quickstart", "quickstart.png", nullptr, nullptr, InitPanelQuickstart, ExitPanelQuickstart,
		RefreshPanelQuickstart, HelpPanelQuickstart
	},
	{"Configurations", "file.png", nullptr, nullptr, InitPanelConfig, ExitPanelConfig, RefreshPanelConfig,
		HelpPanelConfig
	},
	{"CPU and FPU", "cpu.png", nullptr, nullptr, InitPanelCPU, ExitPanelCPU, RefreshPanelCPU, HelpPanelCPU},
	{"Chipset", "cpu.png", nullptr, nullptr, InitPanelChipset, ExitPanelChipset, RefreshPanelChipset,
		HelpPanelChipset
	},
	{"ROM", "chip.png", nullptr, nullptr, InitPanelROM, ExitPanelROM, RefreshPanelROM, HelpPanelROM},
	{"RAM", "chip.png", nullptr, nullptr, InitPanelRAM, ExitPanelRAM, RefreshPanelRAM, HelpPanelRAM},
	{"Floppy drives", "35floppy.png", nullptr, nullptr, InitPanelFloppy, ExitPanelFloppy, RefreshPanelFloppy,
		HelpPanelFloppy
	},
	{"Hard drives/CD", "drive.png", nullptr, nullptr, InitPanelHD, ExitPanelHD, RefreshPanelHD, HelpPanelHD},
	{"Expansions", "expansion.png", nullptr, nullptr, InitPanelExpansions, ExitPanelExpansions,
		RefreshPanelExpansions, HelpPanelExpansions},
	{"RTG board", "expansion.png", nullptr, nullptr, InitPanelRTG, ExitPanelRTG,
		RefreshPanelRTG, HelpPanelRTG
	},
	{"Hardware info", "expansion.png", nullptr, nullptr, InitPanelHWInfo, ExitPanelHWInfo, RefreshPanelHWInfo, HelpPanelHWInfo},
	{"Display", "screen.png", nullptr, nullptr, InitPanelDisplay, ExitPanelDisplay, RefreshPanelDisplay,
		HelpPanelDisplay
	},
	{"Sound", "sound.png", nullptr, nullptr, InitPanelSound, ExitPanelSound, RefreshPanelSound, HelpPanelSound},
	{"Input", "joystick.png", nullptr, nullptr, InitPanelInput, ExitPanelInput, RefreshPanelInput, HelpPanelInput},
	{"IO Ports", "port.png", nullptr, nullptr, InitPanelIO, ExitPanelIO, RefreshPanelIO, HelpPanelIO},
	{"Custom controls", "controller.png", nullptr, nullptr, InitPanelCustom, ExitPanelCustom,
		RefreshPanelCustom, HelpPanelCustom
	},
	{"Disk swapper", "35floppy.png", nullptr, nullptr, InitPanelDiskSwapper, ExitPanelDiskSwapper, RefreshPanelDiskSwapper, HelpPanelDiskSwapper},
	{"Miscellaneous", "misc.png", nullptr, nullptr, InitPanelMisc, ExitPanelMisc, RefreshPanelMisc, HelpPanelMisc},
	{ "Priority", "misc.png", nullptr, nullptr, InitPanelPrio, ExitPanelPrio, RefreshPanelPrio, HelpPanelPrio},
	{"Savestates", "savestate.png", nullptr, nullptr, InitPanelSavestate, ExitPanelSavestate,
		RefreshPanelSavestate, HelpPanelSavestate
	},
	{"Virtual Keyboard", "keyboard.png", nullptr, nullptr, InitPanelVirtualKeyboard, 
		ExitPanelVirtualKeyboard, RefreshPanelVirtualKeyboard, HelpPanelVirtualKeyboard
	},
	{"WHDLoad", "drive.png", nullptr, nullptr, InitPanelWHDLoad, ExitPanelWHDLoad, RefreshPanelWHDLoad, HelpPanelWHDLoad},

	{"Themes", "amigainfo.png", nullptr, nullptr, InitPanelThemes, ExitPanelThemes, RefreshPanelThemes, HelpPanelThemes},

	{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
};

/*
* Gui SDL stuff we need
*/
gcn::SDLInput* gui_input;
gcn::SDLGraphics* gui_graphics;
gcn::SDLImageLoader* gui_imageLoader;
gcn::SDLTrueTypeFont* gui_font;

/*
* Gui stuff we need
*/
gcn::Gui* uae_gui;
gcn::Container* gui_top;
gcn::Container* selectors;
gcn::ScrollArea* selectorsScrollArea;

// GUI Colors
gcn::Color gui_base_color;
gcn::Color gui_background_color;
gcn::Color gui_selector_inactive_color;
gcn::Color gui_selector_active_color;
gcn::Color gui_selection_color;
gcn::Color gui_foreground_color;
gcn::Color gui_font_color;

gcn::FocusHandler* focusHdl;
gcn::Widget* activeWidget;

// Main buttons
gcn::Button* cmdQuit;
gcn::Button* cmdReset;
gcn::Button* cmdRestart;
gcn::Button* cmdStart;
gcn::Button* cmdHelp;
gcn::Button* cmdShutdown;

#endif
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

#ifdef USE_GUISAN
void focus_bug_workaround(const gcn::Window* wnd)
{
	// When modal dialog opens via mouse, the dialog will not
	// have the focus unless there is a mouse click. We simulate the click...
	SDL_Event event{};
	event.type = SDL_MOUSEBUTTONDOWN;
	event.button.button = SDL_BUTTON_LEFT;
	event.button.state = SDL_PRESSED;
	event.button.x = wnd->getX() + 2;
	event.button.y = wnd->getY() + 2;
	gui_input->pushInput(event);
	event.type = SDL_MOUSEBUTTONUP;
	gui_input->pushInput(event);
}

static void show_help_requested()
{
	vector<string> helptext;
	if (categories[last_active_panel].HelpFunc != nullptr && categories[last_active_panel].HelpFunc(helptext))
	{
		//------------------------------------------------
		// Show help for current panel
		//------------------------------------------------
		char title[128];
		snprintf(title, 128, "Help for %s", categories[last_active_panel].category);
		ShowHelp(title, helptext);
	}
}

void update_gui_screen()
{
	const AmigaMonitor* mon = &AMonitors[0];

	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		gui_renderQuad = { 0, 0, gui_screen->w, gui_screen->h };
	else
		gui_renderQuad = { -(GUI_WIDTH - GUI_HEIGHT) / 2, (GUI_WIDTH - GUI_HEIGHT) / 2, gui_screen->w, gui_screen->h };
	
	SDL_RenderCopyEx(mon->gui_renderer, gui_texture, nullptr, &gui_renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
	SDL_RenderPresent(mon->gui_renderer);

	if (mon->amiga_window && !kmsdrm_detected)
		show_screen(0, 0);
}
#endif

#ifdef USE_IMGUI
// IMGUI runtime state and helpers
static bool show_message_box = false;
static char message_box_title[128] = {0};
static char message_box_message[1024] = {0};
static bool start_disabled = false; // Added for disable_resume logic

void BeginGroupBox(const char* name)
{
    ImGui::BeginGroup();
    ImGui::PushID(name);
    
    // Reserve space for the title and the top border part
    // The title will sit on the border line. We need space above the content.
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() + 2.0f)); 
    ImGui::Indent(10.0f); // Add internal padding
}

void EndGroupBox(const char* name)
{
    ImGui::Unindent(10.0f);
    ImGui::PopID();
    ImGui::EndGroup(); // The group contains the content + dummy top space
    
    // Now draw the border and title
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 item_min = ImGui::GetItemRectMin();
    ImVec2 item_max = ImGui::GetItemRectMax();
    
    const float text_height = ImGui::GetTextLineHeight();
    const float border_y = item_min.y + text_height * 0.5f; // The border line y-coordinate
    
    ImU32 border_col = ImGui::GetColorU32(ImGuiCol_Border);
    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    
    ImVec2 text_size = ImGui::CalcTextSize(name);
    // Position text slightly indented from the left
    float text_start_x = item_min.x + 8.0f;
    float text_padding = 8.0f; // Gap around text

    // Expand the box slightly to wrap content comfortably
    const float box_padding = 4.0f;
    item_min.x -= box_padding; 
    // item_min.y is controlled by border_y

    // Ensure the right line doesn't fall outside the visible area
    // Use GetWindowContentRegionMax to find the rightmost edge of the content area
    ImVec2 content_max = ImGui::GetWindowContentRegionMax();
    ImVec2 window_pos = ImGui::GetWindowPos();
    float max_x = window_pos.x + content_max.x - 1.0f;

    if (item_max.x + box_padding > max_x)
        item_max.x = max_x;
    else
        item_max.x += box_padding;

    item_max.y += box_padding;

    // Draw Top-Left Line
    draw_list->AddLine(
        ImVec2(item_min.x, border_y), 
        ImVec2(text_start_x - text_padding, border_y), 
        border_col
    );
    
    // Draw Top-Right Line
    draw_list->AddLine(
        ImVec2(text_start_x + text_size.x + text_padding, border_y), 
        ImVec2(item_max.x, border_y), 
        border_col
    );
    
    // Draw Right Line
    draw_list->AddLine(ImVec2(item_max.x, border_y), ImVec2(item_max.x, item_max.y), border_col);
    
    // Draw Bottom Line
    draw_list->AddLine(ImVec2(item_max.x, item_max.y), ImVec2(item_min.x, item_max.y), border_col);
    
    // Draw Left Line
    draw_list->AddLine(ImVec2(item_min.x, item_max.y), ImVec2(item_min.x, border_y), border_col);
    
    // Draw Title
    // Adjust y to center text vertically on the line (standard text rendering is top-left)
    float text_y = item_min.y; // Should match the dummy space we reserved approx
    draw_list->AddText(ImVec2(text_start_x, text_y), text_col, name);

    // Add spacing after the box so the next one doesn't touch it
    ImGui::Dummy(ImVec2(0.0f, 10.0f)); 
}

// Sidebar icons cache
struct IconTex { SDL_Texture* tex; int w; int h; };
static std::unordered_map<std::string, IconTex> g_sidebar_icons;

static void release_sidebar_icons()
{
	for (auto& kv : g_sidebar_icons) {
		if (kv.second.tex) {
			SDL_DestroyTexture(kv.second.tex);
			kv.second.tex = nullptr;
		}
	}
	g_sidebar_icons.clear();
}

static void ensure_sidebar_icons_loaded()
{
	AmigaMonitor* mon = &AMonitors[0];
	for (int i = 0; categories[i].category != nullptr; ++i) {
		if (!categories[i].imagepath) continue;
		std::string key = categories[i].imagepath;
		if (g_sidebar_icons.find(key) != g_sidebar_icons.end())
			continue;
		const auto full = prefix_with_data_path(categories[i].imagepath);
		SDL_Surface* surf = IMG_Load(full.c_str());
		if (!surf) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Sidebar icon load failed: %s: %s", full.c_str(), SDL_GetError());
			g_sidebar_icons.emplace(key, IconTex{nullptr, 0, 0});
			continue;
		}
		SDL_Texture* tex = SDL_CreateTextureFromSurface(mon->gui_renderer, surf);
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
	}
	SDL_GetCurrentDisplayMode(0, &sdl_mode);

#ifdef USE_GUISAN
	//-------------------------------------------------
	// Create new screen for GUI
	//-------------------------------------------------
	if (!gui_screen)
	{
		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create GUI surface:");
	}
#endif

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
			// Only enable Windowed mode if we're running under x11
			mode = SDL_WINDOW_RESIZABLE;
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

        if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
        {
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				gui_window_rect.x,
				gui_window_rect.y,
				gui_window_rect.w,
				gui_window_rect.h,
				mode);
        }
        else
        {
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				gui_window_rect.y,
				gui_window_rect.x,
				gui_window_rect.h,
				gui_window_rect.w,
				mode);
        }
        check_error_sdl(mon->gui_window == nullptr, "Unable to create window:");

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

	if (mon->gui_renderer == nullptr)
	{
		mon->gui_renderer = SDL_CreateRenderer(mon->gui_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(mon->gui_renderer == nullptr, "Unable to create a renderer:");
	}

	DPIHandler::set_render_scale(mon->gui_renderer);

#ifdef USE_IMGUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	// Ensure WindowMinSize is valid after scaling (avoid ImGui assert)
	if (style.WindowMinSize.x < 1.0f || style.WindowMinSize.y < 1.0f)
		style.WindowMinSize = ImVec2(32.0f, 36.0f);

	// Load GUI theme (for font selection)
	load_theme(amiberry_options.gui_theme);
	// Apply theme colors to ImGui (map GUISAN theme fields)
	apply_imgui_theme_from_theme_file(amiberry_options.gui_theme);

	// Load custom font from data/ (default to AmigaTopaz.ttf), scale by DPI
	const std::string font_file = gui_theme.font_name.empty() ? std::string("AmigaTopaz.ttf") : gui_theme.font_name;
	const std::string font_path = prefix_with_data_path(font_file);
	const float font_px = gui_theme.font_size > 0 ? (float)gui_theme.font_size : 15.0f;

	ImFont* loaded_font = nullptr;
	if (!font_path.empty()) {
		loaded_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_px);
	}
	if (!loaded_font) {
		// Fallback to default font if loading failed
		loaded_font = io.Fonts->AddFontDefault();
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ImGui: failed to load font '%s', falling back to default", font_path.c_str());
	}
	io.FontDefault = loaded_font;

	// Setup Platform/Renderer backends
	ImGui_ImplSDLRenderer2_Init(AMonitors[0].gui_renderer);
	ImGui_ImplSDL2_InitForSDLRenderer(AMonitors[0].gui_window, AMonitors[0].gui_renderer);
#endif
#ifdef USE_GUISAN
	gui_texture = SDL_CreateTexture(mon->gui_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING, gui_screen->w,
									gui_screen->h);
	check_error_sdl(gui_texture == nullptr, "Unable to create GUI texture:");

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_HEIGHT, GUI_WIDTH);
#endif

	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);

	SDL_RaiseWindow(mon->gui_window);

#ifdef USE_GUISAN
	//-------------------------------------------------
	// Create helpers for GUI framework
	//-------------------------------------------------

	gui_imageLoader = new gcn::SDLImageLoader();
	gui_imageLoader->setRenderer(mon->gui_renderer);

	// The ImageLoader in use is static and must be set to be
	// able to load images
	gcn::Image::setImageLoader(gui_imageLoader);
	gui_graphics = new gcn::SDLGraphics();
	// Set the target for the graphics object to be the screen.
	// In other words, we will draw to the screen.
	// Note, any surface will do, it doesn't have to be the screen.
	gui_graphics->setTarget(gui_screen);
	gui_input = new gcn::SDLInput();
#endif

	// Quickstart models and configs initialization
	qs_models.clear();
	for (int i = 0; i < IM_ARRAYSIZE(amodels); ++i) {
		qs_models.push_back(amodels[i].name);
	}
	qs_configs.clear();
	for (int i = 0; i < IM_ARRAYSIZE(amodels[0].configs); ++i) {
		qs_configs.push_back(amodels[0].configs[i]);
	}
}

void amiberry_gui_halt()
{
	AmigaMonitor* mon = &AMonitors[0];

#ifdef USE_GUISAN
	delete gui_imageLoader;
	gui_imageLoader = nullptr;
	delete gui_input;
	gui_input = nullptr;
	delete gui_graphics;
	gui_graphics = nullptr;
#elif USE_IMGUI
	// Release any About panel resources
	if (about_logo_texture) {
		SDL_DestroyTexture(about_logo_texture);
		about_logo_texture = nullptr;
	}
	// Release sidebar icon textures
	release_sidebar_icons();
	// Properly shutdown ImGui backends/context
	ImGui_ImplSDLRenderer2_Shutdown();
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
		SDL_DestroyRenderer(mon->gui_renderer);
		mon->gui_renderer = nullptr;
	}

	if (mon->gui_window && !kmsdrm_detected) {
		if (gui_window_moved) {
			regsetint(nullptr, _T("GUIPosX"), gui_window_rect.x);
			regsetint(nullptr, _T("GUIPosY"), gui_window_rect.y);
		}
		SDL_DestroyWindow(mon->gui_window);
		mon->gui_window = nullptr;
	}
}

#ifdef USE_GUISAN
void check_input()
{
	const AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	didata* did = &di_joystick[0];
	didata* existing_did;

	while (SDL_PollEvent(&gui_event))
	{
		switch (gui_event.type)
		{
		case SDL_WINDOWEVENT:
			if (gui_event.window.windowID == SDL_GetWindowID(mon->gui_window))
			{
				switch (gui_event.window.event)
				{
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
			got_event = 1;
			break;

		case SDL_QUIT:
			got_event = 1;
			//-------------------------------------------------
			// Quit entire program
			//-------------------------------------------------
			uae_quit();
			gui_running = false;
			break;

		case SDL_JOYDEVICEADDED:
			// Check if we need to re-import joysticks
			existing_did = &di_joystick[gui_event.jdevice.which];
			if (existing_did->guid.empty())
			{
				write_log("GUI: SDL2 Controller/Joystick added, re-running import joysticks...\n");
				import_joysticks();
				joystick_refresh_needed = true;
				RefreshPanelInput();
			}
			return;
		case SDL_JOYDEVICEREMOVED:
			write_log("GUI: SDL2 Controller/Joystick removed, re-running import joysticks...\n");
			if (inputdevice_devicechange(&currprefs))
			{
				import_joysticks();
				joystick_refresh_needed = true;
				RefreshPanelInput();
			}
			return;

		case SDL_JOYHATMOTION:
		case SDL_JOYBUTTONDOWN:
			if (gui_joystick)
			{
				got_event = 1;
				const int hat = SDL_JoystickGetHat(gui_joystick, 0);

				if (gui_event.jbutton.button == static_cast<Uint8>(enter_gui_button) || (
					SDL_JoystickGetButton(gui_joystick, did->mapping.menu_button) &&
					SDL_JoystickGetButton(gui_joystick, did->mapping.hotkey_button)))
				{
					if (emulating && cmdStart->isEnabled())
					{
						//------------------------------------------------
						// Continue emulation
						//------------------------------------------------
						gui_running = false;
					}
					else
					{
						//------------------------------------------------
						// First start of emulator -> reset Amiga
						//------------------------------------------------
						uae_reset(0, 1);
						gui_running = false;
					}
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_UP]) || hat & SDL_HAT_UP)
				{
					if (handle_navigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
				{
					if (handle_navigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}

				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])))
				{
					for (auto z = 0; z < 10; ++z)
					{
						PushFakeKey(SDLK_UP);
					}
				}
				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])))
				{
					for (auto z = 0; z < 10; ++z)
					{
						PushFakeKey(SDLK_DOWN);
					}
				}

				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || hat & SDL_HAT_RIGHT)
				{
					if (handle_navigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
				{
					if (handle_navigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_LEFT);
					break;
				}
				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_A])
						|| SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_B]))
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_A]))
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_B])))
				{
					PushFakeKey(SDLK_RETURN);
					continue;
				}

				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.quit_button) &&
					SDL_JoystickGetButton(gui_joystick, did->mapping.hotkey_button))
				{
					// use the HOTKEY button
					uae_quit();
					gui_running = false;
					break;
				}

				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_GUIDE])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_GUIDE])))
				{
					// use the HOTKEY button
					gui_running = false;
				}
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				if (gui_event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
				{
					if (gui_event.jaxis.value > joystick_dead_zone && last_x != 1)
					{
						last_x = 1;
						if (handle_navigation(DIRECTION_RIGHT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_RIGHT);
					}
					else if (gui_event.jaxis.value < -joystick_dead_zone && last_x != -1)
					{
						last_x = -1;
						if (handle_navigation(DIRECTION_LEFT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_LEFT);
					}
					else if (gui_event.jaxis.value > -joystick_dead_zone && gui_event.jaxis.value < joystick_dead_zone)
					{
						last_x = 0;
					}
				}
				else if (gui_event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
				{
					if (gui_event.jaxis.value < -joystick_dead_zone && last_y != -1)
					{
						last_y = -1;
						if (handle_navigation(DIRECTION_UP))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_UP);
					}
					else if (gui_event.jaxis.value > joystick_dead_zone && last_y != 1)
					{
						last_y = 1;
						if (handle_navigation(DIRECTION_DOWN))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_DOWN);
					}
					else if (gui_event.jaxis.value > -joystick_dead_zone && gui_event.jaxis.value < joystick_dead_zone)
					{
						last_y = 0;
					}
				}
			}
			break;

		case SDL_KEYDOWN:
			got_event = 1;
			switch (gui_event.key.keysym.sym)
			{
			case SDLK_RCTRL:
			case SDLK_LCTRL:
				ctrl_state = true;
				break;
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
				shift_state = true;
				break;
			case SDLK_RALT:
			case SDLK_LALT:
				alt_state = true;
				break;
			case SDLK_RGUI:
			case SDLK_LGUI:
				win_state = true;
				break;
			default:
				break;
			}

			if (gui_event.key.keysym.scancode == enter_gui_key.scancode)
			{
				if ((enter_gui_key.modifiers.lctrl || enter_gui_key.modifiers.rctrl) == ctrl_state
					&& (enter_gui_key.modifiers.lshift || enter_gui_key.modifiers.rshift) == shift_state
					&& (enter_gui_key.modifiers.lalt || enter_gui_key.modifiers.ralt) == alt_state
					&& (enter_gui_key.modifiers.lgui || enter_gui_key.modifiers.rgui) == win_state)
				{
					if (emulating && cmdStart->isEnabled())
					{
						//------------------------------------------------
						// Continue emulation
						//------------------------------------------------
						gui_running = false;
					}
					else
					{
						//------------------------------------------------
						// First start of emulator -> reset Amiga
						//------------------------------------------------
						uae_reset(0, 1);
						gui_running = false;
					}
				}
			}
			else
			{
				switch (gui_event.key.keysym.sym)
				{
				case SDLK_q:
					//-------------------------------------------------
					// Quit entire program via Q on keyboard
					//-------------------------------------------------
					focusHdl = gui_top->_getFocusHandler();
					activeWidget = focusHdl->getFocused();
					if (dynamic_cast<gcn::TextField*>(activeWidget) == nullptr)
					{
						// ...but only if we are not in a Textfield...
						uae_quit();
						gui_running = false;
					}
					break;

				case VK_ESCAPE:
				case VK_Red:
					gui_running = false;
					break;

				case VK_Green:
				case VK_Blue:
					//------------------------------------------------
					// Simulate press of enter when 'X' pressed
					//------------------------------------------------
					gui_event.key.keysym.sym = SDLK_RETURN;
					gui_input->pushInput(gui_event); // Fire key down
					gui_event.type = SDL_KEYUP; // and the key up
					break;

				case VK_UP:
					if (handle_navigation(DIRECTION_UP))
						continue; // Don't change value when enter ComboBox -> don't send event to control
					break;

				case VK_DOWN:
					if (handle_navigation(DIRECTION_DOWN))
						continue; // Don't change value when enter ComboBox -> don't send event to control
					break;

				case VK_LEFT:
					if (handle_navigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					break;

				case VK_RIGHT:
					if (handle_navigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					break;

				case SDLK_F1:
					show_help_requested();
					cmdHelp->requestFocus();
					break;

				default:
					break;
				}
			}
			break;

	    case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &gui_event, sizeof gui_event);
			touch_event.type = (gui_event.type == SDL_FINGERDOWN) ? SDL_MOUSEBUTTONDOWN : (gui_event.type == SDL_FINGERUP) ? SDL_MOUSEBUTTONUP : SDL_MOUSEMOTION;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = (gui_event.type == SDL_FINGERDOWN) ? SDL_PRESSED : (gui_event.type == SDL_FINGERUP) ? SDL_RELEASED : 0;
			touch_event.button.x = gui_graphics->getTarget()->w * static_cast<int>(gui_event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * static_cast<int>(gui_event.tfinger.y);
			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = 1;
			if (gui_event.wheel.y > 0)
			{
				for (auto z = 0; z < gui_event.wheel.y; ++z)
				{
					PushFakeKey(SDLK_UP);
				}
			}
			else if (gui_event.wheel.y < 0)
			{
				for (auto z = 0; z > gui_event.wheel.y; --z)
				{
					PushFakeKey(SDLK_DOWN);
				}
			}
			break;

		case SDL_KEYUP:
			got_event = 1;
			switch (gui_event.key.keysym.sym)
			{
			case SDLK_RCTRL:
			case SDLK_LCTRL:
				ctrl_state = false;
				break;
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
				shift_state = false;
				break;
			case SDLK_RALT:
			case SDLK_LALT:
				alt_state = false;
				break;
			case SDLK_RGUI:
			case SDLK_LGUI:
				win_state = false;
				break;
			default:
				break;
			}
			break;

		default:
			got_event = 1;
			break;
		}

		//-------------------------------------------------
		// Send event to gui-controls
		//-------------------------------------------------
		gui_input->pushInput(gui_event);
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();

		SDL_RenderClear(mon->gui_renderer);

		// Now we let the Gui object draw itself.
		uae_gui->draw();
		update_gui_screen();
	}
}

void amiberry_gui_run()
{
	const AmigaMonitor* mon = &AMonitors[0];

	if (amiberry_options.gui_joystick_control)
	{
		for (auto j = 0; j < SDL_NumJoysticks(); j++)
		{
			gui_joystick = SDL_JoystickOpen(j);
			// Some controllers (e.g. PS4) report a second instance with only axes and no buttons.
			// We ignore these and move on.
			if (SDL_JoystickNumButtons(gui_joystick) < 1)
			{
				SDL_JoystickClose(gui_joystick);
				continue;
			}
			if (gui_joystick)
				break;
		}
	}

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	//-------------------------------------------------
	// The main loop
	//-------------------------------------------------
	while (gui_running)
	{
		const auto start = SDL_GetPerformanceCounter();
		check_input();

		if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
			disable_resume();

		cap_fps(start);
	}

	if (gui_joystick)
	{
		SDL_JoystickClose(gui_joystick);
		gui_joystick = nullptr;
	}
}

class MainButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto source = actionEvent.getSource();
		if (source == cmdShutdown)
		{
			shutdown_host();
		}
		else if (source == cmdQuit)
		{
			quit_program();
		}
		else if (source == cmdReset)
		{
			reset_amiga();
		}
		else if (source == cmdRestart)
		{
			restart_emulator();
		}
		else if (source == cmdStart)
		{
			start_emulation();
		}
		else if (source == cmdHelp)
		{
			show_help();
		}
	}
private:
	static void shutdown_host()
	{
		uae_quit();
		gui_running = false;
		host_poweroff = true;
	}

	static void quit_program()
	{
		uae_quit();
		gui_running = false;
	}

	static void reset_amiga()
	{
		uae_reset(1, 1);
		gui_running = false;
	}

	static void restart_emulator()
	{
		char tmp[MAX_DPATH];
		get_configuration_path(tmp, sizeof tmp);
		if (strlen(last_loaded_config) > 0) {
			strncat(tmp, last_loaded_config, MAX_DPATH - 1);
			strncat(tmp, ".uae", MAX_DPATH - 10);
		}
		else {
			strncat(tmp, OPTIONSFILENAME, MAX_DPATH - 1);
			strncat(tmp, ".uae", MAX_DPATH - 10);
		}
		uae_restart(&changed_prefs, -1, tmp);
		gui_running = false;
	}

	static void start_emulation()
	{
		if (emulating && cmdStart->isEnabled())
		{
			gui_running = false;
		}
		else
		{
			uae_reset(0, 1);
			gui_running = false;
		}
	}

	static void show_help()
	{
		show_help_requested();
		cmdHelp->requestFocus();
	}
};

MainButtonActionListener* mainButtonActionListener;

class PanelFocusListener : public gcn::FocusListener
{
public:
	void focusGained(const gcn::Event& event) override
	{
		for (auto i = 0; categories[i].category != nullptr; ++i)
		{
			if (event.getSource() == categories[i].selector)
			{
				categories[i].selector->setActive(true);
				categories[i].panel->setVisible(true);
				last_active_panel = i;
				cmdHelp->setVisible(categories[last_active_panel].HelpFunc != nullptr);
			}
			else
			{
				categories[i].selector->setActive(false);
				categories[i].panel->setVisible(false);
			}
		}
	}
};

PanelFocusListener* panelFocusListener;

void gui_widgets_init()
{
	int i;
	int yPos;

	//-------------------------------------------------
	// Create GUI
	//-------------------------------------------------
	uae_gui = new gcn::Gui();
	uae_gui->setGraphics(gui_graphics);
	uae_gui->setInput(gui_input);

	//-------------------------------------------------
	// Initialize fonts
	//-------------------------------------------------
	TTF_Init();

	load_theme(amiberry_options.gui_theme);
	apply_theme();

	//-------------------------------------------------
	// Create container for main page
	//-------------------------------------------------
	gui_top = new gcn::Container();
	gui_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
	gui_top->setBaseColor(gui_base_color);
	gui_top->setBackgroundColor(gui_base_color);
	gui_top->setForegroundColor(gui_foreground_color);
	uae_gui->setTop(gui_top);

	//--------------------------------------------------
	// Create main buttons
	//--------------------------------------------------
	mainButtonActionListener = new MainButtonActionListener();

	cmdQuit = new gcn::Button("Quit");
	cmdQuit->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdQuit->setBaseColor(gui_base_color);
	cmdQuit->setForegroundColor(gui_foreground_color);
	cmdQuit->setId("Quit");
	cmdQuit->addActionListener(mainButtonActionListener);

	cmdShutdown = new gcn::Button("Shutdown");
	cmdShutdown->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdShutdown->setBaseColor(gui_base_color);
	cmdShutdown->setForegroundColor(gui_foreground_color);
	cmdShutdown->setId("Shutdown");
	cmdShutdown->addActionListener(mainButtonActionListener);

	cmdReset = new gcn::Button("Reset");
	cmdReset->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdReset->setBaseColor(gui_base_color);
	cmdReset->setForegroundColor(gui_foreground_color);
	cmdReset->setId("Reset");
	cmdReset->addActionListener(mainButtonActionListener);

	cmdRestart = new gcn::Button("Restart");
	cmdRestart->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdRestart->setBaseColor(gui_base_color);
	cmdRestart->setForegroundColor(gui_foreground_color);
	cmdRestart->setId("Restart");
	cmdRestart->addActionListener(mainButtonActionListener);

	cmdStart = new gcn::Button("Start");
	if (emulating)
		cmdStart->setCaption("Resume");
	cmdStart->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdStart->setBaseColor(gui_base_color);
	cmdStart->setForegroundColor(gui_foreground_color);
	cmdStart->setId("Start");
	cmdStart->addActionListener(mainButtonActionListener);

	cmdHelp = new gcn::Button("Help");
	cmdHelp->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdHelp->setBaseColor(gui_base_color);
	cmdHelp->setForegroundColor(gui_foreground_color);
	cmdHelp->setId("Help");
	cmdHelp->addActionListener(mainButtonActionListener);

	//--------------------------------------------------
	// Create selector entries
	//--------------------------------------------------
	constexpr auto workAreaHeight = GUI_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y;
	selectors = new gcn::Container();
	selectors->setFrameSize(0);
	selectors->setBaseColor(gui_base_color);
	selectors->setBackgroundColor(gui_base_color);
	selectors->setForegroundColor(gui_foreground_color);

	constexpr auto selectorScrollAreaWidth = SELECTOR_WIDTH + 2;
	selectorsScrollArea = new gcn::ScrollArea();
	selectorsScrollArea->setContent(selectors);
	selectorsScrollArea->setBaseColor(gui_base_color);
	selectorsScrollArea->setBackgroundColor(gui_background_color);
	selectorsScrollArea->setForegroundColor(gui_foreground_color);
	selectorsScrollArea->setSize(selectorScrollAreaWidth, workAreaHeight);
	selectorsScrollArea->setFrameSize(1);

	const auto panelStartX = DISTANCE_BORDER + selectorsScrollArea->getWidth() + DISTANCE_BORDER;

	int selectorsHeight = 0;
	panelFocusListener = new PanelFocusListener();
	for (i = 0; categories[i].category != nullptr; ++i)
	{
		categories[i].selector = new gcn::SelectorEntry(categories[i].category, prefix_with_data_path(categories[i].imagepath));
		categories[i].selector->setActiveColor(gui_selector_active_color);
		categories[i].selector->setInactiveColor(gui_selector_inactive_color);
		categories[i].selector->setSize(SELECTOR_WIDTH, SELECTOR_HEIGHT);
		categories[i].selector->addFocusListener(panelFocusListener);

		categories[i].panel = new gcn::Container();
		categories[i].panel->setId(categories[i].category);
		categories[i].panel->setSize(GUI_WIDTH - panelStartX - DISTANCE_BORDER, workAreaHeight);
		categories[i].panel->setBaseColor(gui_base_color);
		categories[i].panel->setForegroundColor(gui_foreground_color);
		categories[i].panel->setFrameSize(1);
		categories[i].panel->setVisible(false);

		selectorsHeight += categories[i].selector->getHeight();
	}

	selectors->setSize(SELECTOR_WIDTH, selectorsHeight);

	// These need to be called after the selectors have been created
	apply_theme_extras();

	//--------------------------------------------------
	// Initialize panels
	//--------------------------------------------------
	for (i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].InitFunc != nullptr)
			(*categories[i].InitFunc)(categories[i]);
	}

	//--------------------------------------------------
	// Place everything on main form
	//--------------------------------------------------
	gui_top->add(cmdShutdown, DISTANCE_BORDER, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdQuit, DISTANCE_BORDER + BUTTON_WIDTH + DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdRestart, DISTANCE_BORDER + 2 * BUTTON_WIDTH + 2 * DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdHelp, DISTANCE_BORDER + 3 * BUTTON_WIDTH + 3 * DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdReset, DISTANCE_BORDER + 5 * BUTTON_WIDTH + 5 * DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdStart, GUI_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);

	gui_top->add(selectorsScrollArea, DISTANCE_BORDER + 1, DISTANCE_BORDER + 1);

	for (i = 0, yPos = 0; categories[i].category != nullptr; ++i, yPos += SELECTOR_HEIGHT)
	{
		selectors->add(categories[i].selector, 0, yPos);
		gui_top->add(categories[i].panel, panelStartX, DISTANCE_BORDER + 1);
	}

	//--------------------------------------------------
	// Activate last active panel
	//--------------------------------------------------
	if (!emulating && amiberry_options.quickstart_start)
		last_active_panel = 2;
	if (amiberry_options.disable_shutdown_button)
		cmdShutdown->setEnabled(false);
	categories[last_active_panel].selector->requestFocus();
	cmdHelp->setVisible(categories[last_active_panel].HelpFunc != nullptr);
}

void gui_widgets_halt()
{
	for (auto i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].ExitFunc != nullptr)
			(*categories[i].ExitFunc)();

		delete categories[i].selector;
		delete categories[i].panel;
	}

	delete panelFocusListener;
	delete selectors;
	delete selectorsScrollArea;

	delete cmdQuit;
	delete cmdShutdown;
	delete cmdReset;
	delete cmdRestart;
	delete cmdStart;
	delete cmdHelp;

	delete mainButtonActionListener;

	delete gui_font;
	gui_font = nullptr;
	delete gui_top;
	gui_top = nullptr;
	delete uae_gui;
	uae_gui = nullptr;
}

void refresh_all_panels()
{
	for (auto i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].RefreshFunc != nullptr)
			(*categories[i].RefreshFunc)();
	}
}

void disable_resume()
{
	if (emulating)
	{
		cmdStart->setEnabled(false);
	}
}

void run_gui()
{
	gui_running = true;
	gui_rtarea_flags_onenter = gui_create_rtarea_flag(&currprefs);

	expansion_generate_autoconfig_info(&changed_prefs);

	try
	{
		amiberry_gui_init();
		gui_widgets_init();
		if (_tcslen(startup_message) > 0)
		{
			ShowMessage(startup_title, startup_message, "", "", "Ok", "");
			_tcscpy(startup_title, _T(""));
			_tcscpy(startup_message, _T(""));
			cmdStart->requestFocus();
		}
		amiberry_gui_run();
		gui_widgets_halt();
		amiberry_gui_halt();
	}

	// Catch all GUI framework exceptions.
	catch (gcn::Exception& e)
	{
		std::cout << e.getMessage() << '\n';
		uae_quit();
	}

	// Catch all Std exceptions.
	catch (exception& e)
	{
		std::cout << "Std exception: " << e.what() << '\n';
		uae_quit();
	}

	// Catch all unknown exceptions.
	catch (...)
	{
		std::cout << "Unknown exception" << '\n';
		uae_quit();
	}

	expansion_generate_autoconfig_info(&changed_prefs);
	cfgfile_compatibility_romtype(&changed_prefs);

	if (quit_program > UAE_QUIT || quit_program < -UAE_QUIT)
	{
		//--------------------------------------------------
		// Prepare everything for Reset of Amiga
		//--------------------------------------------------
		currprefs.nr_floppies = changed_prefs.nr_floppies;

		if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
			quit_program = -UAE_RESET_HARD; // Hard reset required...
	}

	// Reset counter for access violations
	init_max_signals();
}
#elif USE_IMGUI
void ShowMessageBox(const char* title, const char* message)
{
    // Safely copy and ensure null-termination
    strncpy(message_box_title, title ? title : "Message", sizeof(message_box_title) - 1);
    message_box_title[sizeof(message_box_title) - 1] = '\0';
    strncpy(message_box_message, message ? message : "", sizeof(message_box_message) - 1);
    message_box_message[sizeof(message_box_message) - 1] = '\0';
    show_message_box = true;
}

// Provide IMGUI categories using centralized panel list
ConfigCategory categories[] = {
#define PANEL(id, label, icon) { label, icon, render_panel_##id, nullptr },
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
	AmigaMonitor* mon = &AMonitors[0];

	amiberry_gui_init();
	start_disabled = false;

	if (!emulating && amiberry_options.quickstart_start)
		last_active_panel = 2;


	// Main loop
	while (gui_running)
	{
		while (SDL_PollEvent(&gui_event))
		{
			// Make sure ImGui sees all events
			ImGui_ImplSDL2_ProcessEvent(&gui_event);

			if (gui_event.type == SDL_QUIT)
			{
				uae_quit();
				gui_running = false;
			}
			if (gui_event.type == SDL_WINDOWEVENT) {
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
		}

		// Start the Dear ImGui frame
		ImGui_ImplSDL2_NewFrame();
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui::NewFrame();

		ImGuiStyle& style = ImGui::GetStyle();
		// Compute button bar height from style
		const auto button_bar_height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;

		// Make the main window occupy the full SDL window (no smaller inner window)
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 work_pos = vp ? vp->WorkPos : ImVec2(0, 0);
		ImVec2 work_size = vp ? vp->WorkSize : ImGui::GetIO().DisplaySize;
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
		const float sidebar_width = std::clamp(content_width * 0.18f, 180.0f, 380.0f);

		// Sidebar
		ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, -button_bar_height), true);
		// Ensure icons are ready
		ensure_sidebar_icons_loaded();

 		ImGuiStyle& s = ImGui::GetStyle();
		const float base_text_h = ImGui::GetTextLineHeight();
		const float base_row_h = ImGui::GetTextLineHeightWithSpacing();
		// Make icons larger than text height using gui_scale
		const float icon_h_target = base_text_h;
		const float row_h = std::max(base_row_h, icon_h_target + 2.0f * s.FramePadding.y);
		for (int i = 0; categories[i].category != nullptr; ++i) {
			const bool selected = (last_active_panel == i);
			// Full-width selectable row with custom height
			if (ImGui::Selectable( (std::string("##cat_") + std::to_string(i)).c_str(), selected, 0, ImVec2(ImGui::GetContentRegionAvail().x, row_h))) {
				last_active_panel = i;
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
			SDL_Texture* icon_tex = nullptr;
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
				dl->AddImage((ImTextureID)icon_tex, icon_p0, icon_p1);
			} else {
				// Fallback: small filled square as placeholder
				dl->AddRectFilled(icon_p0, icon_p1, ImGui::GetColorU32(ImGuiCol_TextDisabled), 3.0f);
			}
			// Text baseline: vertically center with the row
			float text_x = icon_p1.x + pad_x;
			float text_y = pos.y + ((rmax.y - rmin.y) - ImGui::GetTextLineHeight()) * 0.5f;
			dl->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), categories[i].category);
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Content
		ImGui::BeginChild("Content", ImVec2(0, -button_bar_height));

		// Render the currently active panel
		if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr) {
			if (categories[last_active_panel].RenderFunc)
				categories[last_active_panel].RenderFunc();
		}
		ImGui::EndChild();

		// Button bar
		// Left-aligned buttons
		if (ImGui::Button("Reset", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			uae_reset(1, 1);
			gui_running = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Quit", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			uae_quit();
			gui_running = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			uae_reset(1, 1);
			gui_running = false;
		}
		
		// Right-aligned buttons
		float right_buttons_width = (BUTTON_WIDTH * 2) + style.ItemSpacing.x;
		ImGui::SameLine();
		// Push cursor to the right
		float cursor_x = ImGui::GetWindowWidth() - right_buttons_width - style.WindowPadding.x;
		if (cursor_x < ImGui::GetCursorPosX()) 
			cursor_x = ImGui::GetCursorPosX(); // Prevent overlap
		ImGui::SetCursorPosX(cursor_x);

		if (start_disabled)
			ImGui::BeginDisabled();
		if (emulating) {
			if (ImGui::Button("Resume", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
				gui_running = false;
		} else {
			if (ImGui::Button("Start", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
				gui_running = false;
		}
		if (start_disabled)
			ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Help", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			const char* help_ptr = nullptr;
			if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr)
				help_ptr = categories[last_active_panel].HelpText;
			if (help_ptr && *help_ptr)
				ShowMessageBox("Help", help_ptr);
		}

		if (show_message_box)
		{
            // Center the dialog on appear and enforce a consistent width
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            const float vw = vp ? vp->Size.x : ImGui::GetIO().DisplaySize.x;
            const float vh = vp ? vp->Size.y : ImGui::GetIO().DisplaySize.y;
            const float desired_w = std::clamp(vw * 0.56f, 520.0f, 760.0f); // ~56% of viewport, clamped
            ImGui::SetNextWindowPos(vp ? vp->GetCenter() : ImVec2(vw * 0.5f, vh * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(desired_w, 0.0f), ImGuiCond_Appearing);
            // Lock width to desired_w; allow height to grow up to 85% viewport
            ImGui::SetNextWindowSizeConstraints(ImVec2(desired_w, 0.0f), ImVec2(desired_w, vh * 0.85f));

            ImGui::OpenPopup(message_box_title);
            if (ImGui::BeginPopupModal(message_box_title, &show_message_box, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                // Scrollable message area with reasonable max height
                const float line_h = ImGui::GetTextLineHeightWithSpacing();
                const float max_child_h = std::clamp(line_h * 10.0f, 140.0f, vh * 0.5f);
                ImGui::BeginChild("MessageScroll", ImVec2(0, max_child_h), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextWrapped("%s", message_box_message);
                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::Separator();

                // Align OK button to the right
                const float btn_w = BUTTON_WIDTH;
                const float btn_h = BUTTON_HEIGHT;
                float avail = ImGui::GetContentRegionAvail().x;
                if (avail > btn_w)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w));
                if (ImGui::Button("OK", ImVec2(btn_w, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    show_message_box = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsWindowAppearing())
                    ImGui::SetItemDefaultFocus();

                ImGui::EndPopup();
            }
        }

		// Check for config changes that require a reset
		if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
			disable_resume();

		ImGui::End();

		// Rendering
		ImGui::Render();
		SDL_SetRenderDrawColor(mon->gui_renderer, (Uint8)(0.45f * 255), (Uint8)(0.55f * 255),
						   (Uint8)(0.60f * 255), (Uint8)(1.00f * 255));
		SDL_RenderClear(mon->gui_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);
		SDL_RenderPresent(mon->gui_renderer);
	}

	amiberry_gui_halt();

	// Reset counter for access violations
	init_max_signals();
}
#endif
