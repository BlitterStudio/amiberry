#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_misc()
{
	ImGui::Checkbox("Status Line native", (bool*)&changed_prefs.leds_on_screen);
	ImGui::Checkbox("Status Line RTG", (bool*)&changed_prefs.leds_on_screen);
	ImGui::Checkbox("Show GUI on startup", &changed_prefs.start_gui);
	ImGui::Checkbox("Untrap = middle button", (bool*)&changed_prefs.input_mouse_untrap);
	ImGui::Checkbox("Alt-Tab releases control", &changed_prefs.alt_tab_release);
	ImGui::Checkbox("Use RetroArch Quit Button", &changed_prefs.use_retroarch_quit);
	ImGui::Checkbox("Use RetroArch Menu Button", &changed_prefs.use_retroarch_menu);
	ImGui::Checkbox("Use RetroArch Reset Button", &changed_prefs.use_retroarch_reset);
	ImGui::Checkbox("Master floppy write protection", &changed_prefs.floppy_read_only);
	ImGui::Checkbox("Master harddrive write protection", &changed_prefs.harddrive_read_only);
	ImGui::Checkbox("Clipboard sharing", &changed_prefs.clipboard_sharing);
	ImGui::Checkbox("RCtrl = RAmiga", &changed_prefs.right_control_is_right_win_key);
	ImGui::Checkbox("Always on top", &changed_prefs.main_alwaysontop);
	ImGui::Checkbox("GUI Always on top", &changed_prefs.gui_alwaysontop);
	ImGui::Checkbox("Synchronize clock", &changed_prefs.tod_hack);
	ImGui::Checkbox("One second reboot pause", &changed_prefs.reset_delay);
	ImGui::Checkbox("Faster RTG", &changed_prefs.picasso96_nocustom);
	ImGui::Checkbox("Allow native code", &changed_prefs.native_code);
	ImGui::Checkbox("Log illegal memory accesses", &changed_prefs.illegal_mem);
	ImGui::Checkbox("Minimize when focus is lost", &changed_prefs.minimize_inactive);
	ImGui::Checkbox("Capture mouse when window is activated", &changed_prefs.capture_always);
	ImGui::Checkbox("Hide all UAE autoconfig boards", &changed_prefs.uae_hide_autoconfig);
	ImGui::Checkbox("A600/A1200/A4000 IDE scsi.device disable", &changed_prefs.scsidevicedisable);
	ImGui::Checkbox("Warp mode reset", &changed_prefs.turbo_boot);

	const char* led_items[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD" };
	ImGui::Combo("NumLock", &changed_prefs.kbd_led_num, led_items, IM_ARRAYSIZE(led_items));
	ImGui::Combo("ScrollLock", &changed_prefs.kbd_led_scr, led_items, IM_ARRAYSIZE(led_items));
	ImGui::Combo("CapsLock", &changed_prefs.kbd_led_cap, led_items, IM_ARRAYSIZE(led_items));

	ImGui::InputText("Open GUI", changed_prefs.open_gui, 256);
	ImGui::InputText("Quit Key", changed_prefs.quit_amiberry, 256);
	ImGui::InputText("Action Replay", changed_prefs.action_replay, 256);
	ImGui::InputText("FullScreen", changed_prefs.fullscreen_toggle, 256);
	ImGui::InputText("Minimize", changed_prefs.minimize, 256);
	ImGui::InputText("Right Amiga", changed_prefs.right_amiga, 256);
}
