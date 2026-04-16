#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "statusline.h"
#include "gui/gui_handling.h"
#include "target.h"

enum MiscSpecial { MISC_NORMAL = 0, MISC_DARK_THEME };

struct MiscListItem {
	const char* label;
	const char* tooltip;
	bool*       b;        // simple bool toggle (mutually exclusive with i)
	int*        i;        // int flag / bitmask toggle
	int         imask;    // mask applied to *i
	int         ival;     // value OR'd into *i when checked
	MiscSpecial special;  // non-zero for entries needing custom logic
};

static bool get_misc_checked(const MiscListItem& item)
{
	if (item.b) return *item.b;
	if (item.i) return (*item.i & item.imask) != 0;
	return false;
}

static void set_misc_checked(const MiscListItem& item, bool checked)
{
	if (item.b) {
		*item.b = checked;
	} else if (item.i) {
		*item.i &= ~item.imask;
		if (checked)
			*item.i |= item.ival & item.imask;
	}
}

// Ordered to match WinUAE misclist[]. Platform-neutral items first, Amiberry-specific at end.
static MiscListItem misc_items[] = {
	{"Untrap = middle button",
	 "Use middle mouse button to release mouse capture",
	 nullptr, &changed_prefs.input_mouse_untrap,
	 MOUSEUNTRAP_MIDDLEBUTTON, MOUSEUNTRAP_MIDDLEBUTTON, MISC_NORMAL},

	{"Show GUI on startup",
	 "Show the configuration GUI when Amiberry starts",
	 &changed_prefs.start_gui, nullptr, 0, 0, MISC_NORMAL},

	{"Always on top",
	 "Keep the emulator window on top of other windows",
	 &changed_prefs.main_alwaysontop, nullptr, 0, 0, MISC_NORMAL},

	{"GUI always on top",
	 "Keep the configuration GUI on top of other windows",
	 &changed_prefs.gui_alwaysontop, nullptr, 0, 0, MISC_NORMAL},

	{"Synchronize clock",
	 "Sync Amiga's clock with host system time",
	 &changed_prefs.tod_hack, nullptr, 0, 0, MISC_NORMAL},

	{"One second reboot pause",
	 "Add a 1 second delay when resetting the emulated Amiga",
	 &changed_prefs.reset_delay, nullptr, 0, 0, MISC_NORMAL},

	{"Faster RTG",
	 "Speed up RTG (graphics card) operations by disabling custom chipset emulation",
	 &changed_prefs.picasso96_nocustom, nullptr, 0, 0, MISC_NORMAL},

	{"Clipboard sharing",
	 "Share clipboard between Amiga and host system",
	 &changed_prefs.clipboard_sharing, nullptr, 0, 0, MISC_NORMAL},

	{"Allow native code",
	 "Allow execution of native code (JIT compiler required)",
	 &changed_prefs.native_code, nullptr, 0, 0, MISC_NORMAL},

	{"Native on-screen display",
	 "Show status line with LED indicators in native chipset modes",
	 nullptr, &changed_prefs.leds_on_screen,
	 STATUSLINE_CHIPSET, STATUSLINE_CHIPSET, MISC_NORMAL},

	{"RTG on-screen display",
	 "Show status line with LED indicators in RTG modes",
	 nullptr, &changed_prefs.leds_on_screen,
	 STATUSLINE_RTG, STATUSLINE_RTG, MISC_NORMAL},

	{"Log illegal memory accesses",
	 "Log attempts to access invalid memory addresses",
	 &changed_prefs.illegal_mem, nullptr, 0, 0, MISC_NORMAL},

	{"Blank unused displays",
	 "Blank secondary displays when running fullscreen",
	 &changed_prefs.blankmonitors, nullptr, 0, 0, MISC_NORMAL},

	{"Start mouse uncaptured",
	 "Start emulation with the mouse uncaptured",
	 &changed_prefs.start_uncaptured, nullptr, 0, 0, MISC_NORMAL},

	{"Start minimized",
	 "Start emulation with the window minimized",
	 &changed_prefs.start_minimized, nullptr, 0, 0, MISC_NORMAL},

	{"Minimize when focus is lost",
	 "Automatically minimize the emulator window when it loses focus",
	 &changed_prefs.minimize_inactive, nullptr, 0, 0, MISC_NORMAL},

	{"Master floppy write protection",
	 "Enable write protection for all floppy disk images",
	 &changed_prefs.floppy_read_only, nullptr, 0, 0, MISC_NORMAL},

	{"Master harddrive write protection",
	 "Enable write protection for all hard drive images",
	 &changed_prefs.harddrive_read_only, nullptr, 0, 0, MISC_NORMAL},

	{"Hide all UAE autoconfig boards",
	 "Hide UAE expansion boards from Amiga's autoconfig system",
	 &changed_prefs.uae_hide_autoconfig, nullptr, 0, 0, MISC_NORMAL},

	{"RCtrl = RAmiga",
	 "Map Right Control key to Right Amiga key",
	 &changed_prefs.right_control_is_right_win_key, nullptr, 0, 0, MISC_NORMAL},

	{"Power LED dims when audio filter is disabled",
	 "Dim the power LED indicator when the audio filter is off",
	 nullptr, &changed_prefs.power_led_dim, 255, 128, MISC_NORMAL},

	{"Capture mouse when window is activated",
	 "Automatically capture mouse when clicking on the emulator window",
	 &changed_prefs.capture_always, nullptr, 0, 0, MISC_NORMAL},

	{"Debug memory space",
	 "Enable debug memory space for diagnostic purposes",
	 &changed_prefs.debug_mem, nullptr, 0, 0, MISC_NORMAL},

	{"Force hard reset if CPU halted",
	 "Automatically reset the Amiga if the CPU becomes halted",
	 &changed_prefs.crash_auto_reset, nullptr, 0, 0, MISC_NORMAL},

	{"A600/A1200/A4000 IDE scsi.device disable",
	 "Disable scsi.device for built-in IDE controllers on these models",
	 &changed_prefs.scsidevicedisable, nullptr, 0, 0, MISC_NORMAL},

	{"Warp mode reset",
	 "Enable turbo/warp mode during reset for faster booting",
	 &changed_prefs.turbo_boot, nullptr, 0, 0, MISC_NORMAL},

	// Amiberry-specific
	{"Dark theme",
	 "Switch between light and dark GUI appearance",
	 nullptr, nullptr, 0, 0, MISC_DARK_THEME},

	{"Alt-Tab releases control",
	 "Reserve Alt-Tab for the host and release capture when focus is lost",
	 &changed_prefs.alt_tab_release, nullptr, 0, 0, MISC_NORMAL},

	{"Ctrl+Alt releases capture",
	 "Press Ctrl+Alt to release mouse capture (useful on laptops without a middle mouse button)",
	 &changed_prefs.ctrl_alt_release, nullptr, 0, 0, MISC_NORMAL},

	{"Use RetroArch Quit Button",
	 "Enable RetroArch-style quit button mapping",
	 &changed_prefs.use_retroarch_quit, nullptr, 0, 0, MISC_NORMAL},

	{"Use RetroArch Menu Button",
	 "Enable RetroArch-style menu button mapping",
	 &changed_prefs.use_retroarch_menu, nullptr, 0, 0, MISC_NORMAL},

	{"Use RetroArch Reset Button",
	 "Enable RetroArch-style reset button mapping",
	 &changed_prefs.use_retroarch_reset, nullptr, 0, 0, MISC_NORMAL},
};

static constexpr int misc_items_count = IM_ARRAYSIZE(misc_items);

static char* target_hotkey_string = nullptr;
static bool  open_hotkey_popup = false;

static void ShowHotkeyPopup()
{
	if (open_hotkey_popup) {
		ImGui::OpenPopup("Set Hotkey");
		open_hotkey_popup = false;
	}

	if (ImGui::BeginPopupModal("Set Hotkey", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Press a key to map...");
		ImGui::Separator();

		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
			// Skip modifier keys — they should only act as prefixes, not as the
			// final key. Otherwise pressing Shift to type "Shift+F12" would
			// immediately close the popup with just "LeftShift".
			switch (key) {
				case ImGuiKey_LeftCtrl:  case ImGuiKey_RightCtrl:
				case ImGuiKey_LeftShift: case ImGuiKey_RightShift:
				case ImGuiKey_LeftAlt:   case ImGuiKey_RightAlt:
				case ImGuiKey_LeftSuper: case ImGuiKey_RightSuper:
					continue;
				default:
					break;
			}
			if (ImGui::IsKeyPressed((ImGuiKey)key)) {
				const char* key_name = ImGui::GetKeyName((ImGuiKey)key);
				if (key_name && target_hotkey_string) {
					// Emit side-specific modifier tokens (LCtrl/RCtrl/LShift/RShift/
					// LAlt/RAlt/LGUI/RGUI) to match the parser in
					// get_hotkey_from_config(). If both sides of a modifier are
					// held, prefer the left variant.
					std::string full_key;
					if      (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))   full_key += "LCtrl+";
					else if (ImGui::IsKeyDown(ImGuiKey_RightCtrl))  full_key += "RCtrl+";
					if      (ImGui::IsKeyDown(ImGuiKey_LeftShift))  full_key += "LShift+";
					else if (ImGui::IsKeyDown(ImGuiKey_RightShift)) full_key += "RShift+";
					if      (ImGui::IsKeyDown(ImGuiKey_LeftAlt))    full_key += "LAlt+";
					else if (ImGui::IsKeyDown(ImGuiKey_RightAlt))   full_key += "RAlt+";
					if      (ImGui::IsKeyDown(ImGuiKey_LeftSuper))  full_key += "LGUI+";
					else if (ImGui::IsKeyDown(ImGuiKey_RightSuper)) full_key += "RGUI+";
					full_key += key_name;
					strncpy(target_hotkey_string, full_key.c_str(), 255);
					ImGui::CloseCurrentPopup();
				}
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();

		if (AmigaButton(ICON_FA_XMARK " Cancel"))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}

void render_panel_misc()
{
	ImGui::Indent(4.0f);

	if (ImGui::BeginTable("MiscPanelTable", 2,
		ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Options", ImGuiTableColumnFlags_WidthStretch, 0.6f);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 0.4f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		const float list_height = ImGui::GetContentRegionAvail().y;

		if (ImGui::BeginTable("##MiscOptionsList", 1,
			ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter,
			ImVec2(0, list_height)))
		{
			for (int idx = 0; idx < misc_items_count; idx++)
			{
				const auto& item = misc_items[idx];

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::PushID(idx);

				if (item.special == MISC_DARK_THEME)
				{
					bool is_dark = (strcmp(amiberry_options.gui_theme, "Dark.theme") == 0);
					if (AmigaCheckbox(item.label, &is_dark))
					{
						const char* theme = is_dark ? "Dark.theme" : "Default.theme";
						strncpy(amiberry_options.gui_theme, theme,
							sizeof(amiberry_options.gui_theme) - 1);
						amiberry_options.gui_theme[sizeof(amiberry_options.gui_theme) - 1] = '\0';
						load_theme(amiberry_options.gui_theme);
						apply_imgui_theme();
						save_amiberry_settings();
					}
				}
				else
				{
					bool checked = get_misc_checked(item);
					if (AmigaCheckbox(item.label, &checked))
						set_misc_checked(item, checked);
				}

				if (item.tooltip)
					ShowHelpMarker(item.tooltip);

				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		ImGui::TableNextColumn();

		auto HotkeyRow = [&](const char* label, char* config_val) {
			ImGui::PushID(label);
			ImGui::Text("%s", label);

			if (ImGui::BeginTable("##HotkeyRow", 2, ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch, 1.0f);
				ImGui::TableSetupColumn("Btns",  ImGuiTableColumnFlags_WidthFixed, SMALL_BUTTON_WIDTH * 2 + ImGui::GetStyle().ItemSpacing.x);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::BeginDisabled();
				ImGui::InputText("##Display", config_val, 256, ImGuiInputTextFlags_ReadOnly);
				AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
				ImGui::EndDisabled();

				ImGui::TableNextColumn();
				if (AmigaButton("...")) {
					target_hotkey_string = config_val;
					open_hotkey_popup = true;
				}
				ImGui::SameLine();
				if (AmigaButton(ICON_FA_XMARK "##clear"))
					config_val[0] = '\0';

				ImGui::EndTable();
			}
			ImGui::PopID();
		};

		HotkeyRow("Open GUI:", changed_prefs.open_gui);
		ShowHelpMarker("Keyboard shortcut to open the configuration GUI");
		HotkeyRow("Quit Key:", changed_prefs.quit_amiberry);
		ShowHelpMarker("Keyboard shortcut to quit the emulator");
		HotkeyRow("Action Replay:", changed_prefs.action_replay);
		ShowHelpMarker("Keyboard shortcut to activate Action Replay cartridge");
		HotkeyRow("FullScreen:", changed_prefs.fullscreen_toggle);
		ShowHelpMarker("Keyboard shortcut to toggle fullscreen mode");
		HotkeyRow("Minimize:", changed_prefs.minimize);
		ShowHelpMarker("Keyboard shortcut to minimize the emulator window");
		HotkeyRow("Left Amiga:", changed_prefs.left_amiga);
		ShowHelpMarker("Keyboard shortcut to simulate Left Amiga key press");
		HotkeyRow("Right Amiga:", changed_prefs.right_amiga);
		ShowHelpMarker("Keyboard shortcut to simulate Right Amiga key press");
		HotkeyRow("Screenshot:", changed_prefs.screenshot);
		ShowHelpMarker("Keyboard shortcut or controller button to take a screenshot");
		HotkeyRow("Debugger:", changed_prefs.debugger_trigger);
		ShowHelpMarker("Keyboard shortcut or controller button to activate the built-in debugger");

		ImGui::Separator();

		if (ImGui::BeginTable("MiscLEDTable", 2, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
			ImGui::TableSetupColumn("Combo", ImGuiTableColumnFlags_WidthStretch);

			const char* led_items[] = {"none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD"};

			auto LedRow = [&](const char* label, int* val) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", label);
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
				const std::string combo_label = "##" + std::string(label);
				ImGui::Combo(combo_label.c_str(), val, led_items, IM_ARRAYSIZE(led_items));
				AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
			};

			LedRow("NumLock:", &changed_prefs.kbd_led_num);
			ShowHelpMarker("Map Amiga status LED to keyboard NumLock indicator");
			LedRow("ScrollLock:", &changed_prefs.kbd_led_scr);
			ShowHelpMarker("Map Amiga status LED to keyboard ScrollLock indicator");
			LedRow("CapsLock:", &changed_prefs.kbd_led_cap);
			ShowHelpMarker("Map Amiga status LED to keyboard CapsLock indicator");

			ImGui::EndTable();
		}

		ImGui::EndTable();
	}

	ShowHotkeyPopup();
}
