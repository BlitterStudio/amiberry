#include "sysdeps.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "options.h"
#include "rommgr.h"
#include "memory.h"
#include "uae.h"
#include "gui/gui_handling.h"
#include "pcem/paths.h"
#include "settings/settings_definitions.h"

#include <cstdio>

namespace settings = amiberry_settings;

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules (MapROM vs. an accelerator board, the UAE board only
// while the emulator is stopped) and stores the paths verbatim.
struct rom_options {
    const settings::definition& kickstart_rom_file = settings::require("kickstart_rom_file");
    const settings::definition& kickstart_ext_rom_file = settings::require("kickstart_ext_rom_file");
    const settings::definition& cart_file = settings::require("cart_file");
    const settings::definition& boot_rom_uae = settings::require("boot_rom_uae");
    const settings::definition& uaeboard = settings::require("uaeboard");
    const settings::definition& flash_file = settings::require("flash_file");
    const settings::definition& rtc_file = settings::require("rtc_file");
    const settings::definition& maprom = settings::require("maprom");
    const settings::definition& kickshifter = settings::require("kickshifter");
};

const rom_options& options()
{
    static const rom_options o;
    return o;
}

bool available(const settings::definition& d, const settings::context& ctx)
{
    return settings::is_available(d, changed_prefs, ctx);
}

bool has_value(const settings::definition& d, const char* v)
{
    char current[AMIBERRY_SETTINGS_VALUE_MAX];
    settings::get(d, changed_prefs, current, sizeof current);
    return std::strcmp(current, v) == 0;
}

// Index of the choice the configuration holds, clamped for display when a
// configuration from a file holds a value this build does not know.
int choice_index_of(const settings::definition& d)
{
    const amiberry_setting_descriptor& desc = d.descriptor;
    for (size_t i = 0; i < desc.choice_count; i++) {
        char current[AMIBERRY_SETTINGS_VALUE_MAX];
        settings::get(d, changed_prefs, current, sizeof current);
        if (std::strcmp(current, desc.choices[i].value) == 0)
            return static_cast<int>(i);
    }
    return 0;
}

// A checkbox for a BOOL option, greyed out while unavailable.
void option_checkbox(const char* label, const settings::definition& d, const settings::context& ctx)
{
    ImGui::BeginDisabled(!available(d, ctx));
    bool checked = settings::get_bool(d, changed_prefs);
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked);
    ImGui::EndDisabled();
}

struct RomListEntry {
	std::string name;
	std::string path;
	std::string ident;
};

static std::vector<RomListEntry> main_rom_list;
static std::vector<RomListEntry> ext_rom_list;
static std::vector<RomListEntry> cart_rom_list;
static int customromselectnum = 0;

enum class RomPickType {
	None,
	Main,
	Extended,
	Cartridge,
	Flash,
	RTC,
	Custom
};

static RomPickType current_pick_type = RomPickType::None;

static void update_rom_list(std::vector<RomListEntry>& list, int romtype_mask) {
	list.clear();
	list.push_back({ "Select ROM...", "" });

	int count = romlist_count();
	const romlist* rl = romlist_getit();
	for (int i = 0; i < count; i++) {
		if (rl[i].rd->type & romtype_mask) {
			const char* name = rl[i].rd->name;
			const char* path = rl[i].path;
			if (path && *path) {
				list.push_back({ name ? name : path, path, rl[i].rd->configname ? (std::string(":") + rl[i].rd->configname) : "" });
			}
		}
	}
}

// Build the ROM combo lists once, and rebuild them whenever the ROM scan
// changes (e.g. after a rescan) so a new ROM appears without re-entering
// the GUI - like the chipset keyboard list.
static void refresh_rom_lists() {
	static int cached_rom_gen = -1;
	const int rom_gen = romlist_get_generation();
	if (rom_gen == cached_rom_gen)
		return;
	update_rom_list(main_rom_list, ROMTYPE_KICK | ROMTYPE_KICKCD32);
	update_rom_list(ext_rom_list, ROMTYPE_EXTCD32 | ROMTYPE_EXTCDTV | ROMTYPE_ARCADIABIOS | ROMTYPE_ALG);
	update_rom_list(cart_rom_list, ROMTYPE_FREEZER | ROMTYPE_ARCADIAGAME | ROMTYPE_CD32CART);
	cached_rom_gen = rom_gen;
}

// A combo over the scanned ROMs of one type. Returns the entry the user
// picked (the caller stores it through the settings layer); the preview
// shows the entry name, or the raw path for a file outside the scan.
static const RomListEntry* RomCombo(const char* label, const char* current_path,
                                    std::vector<RomListEntry>& list) {
	std::string preview_value = "Select ROM...";
	const RomListEntry* selected = nullptr;

	for (const auto& entry : list) {
		if (entry.path == current_path) {
			preview_value = entry.name;
			break;
		}
	}
	if (preview_value == "Select ROM..." && current_path[0]) {
		preview_value = current_path;
	}

	if (ImGui::BeginCombo(label, preview_value.c_str())) {
		for (const auto& entry : list) {
			bool is_selected = (entry.path == current_path);
			if (is_selected)
			{
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			}

			ImGui::PushID(entry.path.c_str());
			if (ImGui::Selectable(entry.name.c_str(), is_selected)) {
				selected = &entry;
			}
			ImGui::PopID();

			if (is_selected)
			{
				ImGui::PopStyleColor(1);
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	return selected;
}

// Re-sync the cartridge identifier with the newly picked file: a scanned
// dump carries its romdata configname, anything else gets a clean slate so
// a stale ident can't mis-type an unrecognized ROM.
static void sync_cartident(const std::string& path) {
	std::string cartident;
	for (const auto& entry : cart_rom_list) {
		if (entry.path == path) {
			cartident = entry.ident;
			break;
		}
	}
	snprintf(changed_prefs.cartident, sizeof(changed_prefs.cartident), "%s", cartident.c_str());
}

} // namespace

void render_panel_rom()
{
	const rom_options& o = options();
	const settings::context ctx{emulating != 0};

	refresh_rom_lists();
	ImGui::Indent(4.0f);

	const float browse_btn_w = SMALL_BUTTON_WIDTH;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;

	BeginGroupBox("System ROM Settings");
	// Main ROM
	ImGui::Text("Main ROM File:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
	if (const RomListEntry* entry = RomCombo("##MainRomCombo", changed_prefs.romfile, main_rom_list)) {
		settings::set(o.kickstart_rom_file, changed_prefs, ctx, entry->path.c_str());
		// The kickstart version is core run-time state, not configuration;
		// refresh it on interaction like fixup_prefs does on apply.
		read_kickstart_version(&changed_prefs);
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", changed_prefs.romfile);
	ImGui::SameLine();
	if (AmigaButton("...##MainRomFileButton"))
	{
		current_pick_type = RomPickType::Main;
		OpenFileDialogKey("ROM", "Select Main ROM", ".rom,.bin,.a1000,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", get_rom_path());
	}

	// Extended ROM
	ImGui::Text("Extended ROM File:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
	if (const RomListEntry* entry = RomCombo("##ExtRomCombo", changed_prefs.romextfile, ext_rom_list))
		settings::set(o.kickstart_ext_rom_file, changed_prefs, ctx, entry->path.c_str());
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", changed_prefs.romextfile);
	ImGui::SameLine();
	if (AmigaButton("...##ExtRomFileButton"))
	{
		current_pick_type = RomPickType::Extended;
		OpenFileDialogKey("ROM", "Select Extended ROM", ".rom,.bin,.a1000,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", get_rom_path());
	}

	// MapROM is the address token the configuration writes (0x0f000000 on).
	bool maprom_on = !has_value(o.maprom, "0x0");
	ImGui::BeginDisabled(!available(o.maprom, ctx));
	if (AmigaCheckbox("MapROM emulation", &maprom_on)) {
		settings::set(o.maprom, changed_prefs, ctx, maprom_on ? "0x0f000000" : "0x0");
	}
	ShowHelpMarker("Map the ROM into fast RAM for faster access");
	ImGui::EndDisabled();

	ImGui::SameLine();
	option_checkbox("ShapeShifter support", o.kickshifter, ctx);
	ShowHelpMarker("Enable support for the ShapeShifter Mac emulator");
	ImGui::Spacing();
	EndGroupBox("System ROM Settings");

	BeginGroupBox("Advanced Custom ROM Settings");
	// Custom ROM SELECTOR
	const char* custom_rom_items[] = { "ROM #1", "ROM #2", "ROM #3", "ROM #4" };
	ImGui::SetNextItemWidth(BUTTON_WIDTH);
	if (ImGui::Combo("##CustomROM", &customromselectnum, custom_rom_items, IM_ARRAYSIZE(custom_rom_items))) {
		// Just changed selection, fields will update automatically in next frame based on selection
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

	romboard* rb = &changed_prefs.romboards[customromselectnum];

	ImGui::SameLine();
	ImGui::Text("Address range");

	// Address From
	char addr_from[16];
	snprintf(addr_from, sizeof(addr_from), "%08x", rb->start_address);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH);
	if (ImGui::InputText("##AddressRangeFrom", addr_from, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
		rb->start_address = strtoul(addr_from, nullptr, 16);
		rb->start_address &= ~65535; // Logic from values_from_kickstartdlg2
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);

	// Address To
	char addr_to[16];
	if (!rb->end_address && !rb->start_address)
		strcpy(addr_to, "00000000");
	else
		snprintf(addr_to, sizeof(addr_to), "%08x", rb->end_address);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH);
	if (ImGui::InputText("##AddressRangeTo", addr_to, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
		rb->end_address = strtoul(addr_to, nullptr, 16);
		// Logic from values_from_kickstartdlg2
		rb->end_address = ((rb->end_address - 1) & ~65535) | 0xffff;
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
	AmigaInputText("##CustomRomFilename", rb->lf.loadfile, MAX_DPATH);
	ImGui::SameLine();
    if (AmigaButton("...##CustomRomFileButton")) {
    	current_pick_type = RomPickType::Custom;
	    OpenFileDialogKey("ROM", "Select Custom ROM", ".rom,.bin,.a1000,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", rb->lf.loadfile);
    }
	ImGui::Spacing();
	EndGroupBox("Advanced Custom ROM Settings");

	BeginGroupBox("Miscellaneous");
	static bool custom_cart_enabled = false;
	static char custom_cart_derived_from[MAX_DPATH] = "";
	static bool custom_cart_local_edit = false;
	ImGui::Text("Cartridge ROM File:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
	if (const RomListEntry* entry = RomCombo("##CartRomCombo", changed_prefs.cartfile, cart_rom_list)) {
		settings::set(o.cart_file, changed_prefs, ctx, entry->path.c_str());
		sync_cartident(entry->path);
		custom_cart_local_edit = true;
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", changed_prefs.cartfile);
	ImGui::SameLine();
	if (AmigaButton("...##CartRomFileButton")) {
		current_pick_type = RomPickType::Cartridge;
		OpenFileDialogKey("ROM", "Select Cartridge ROM", ".rom,.bin,.a1000,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", get_rom_path());
	}

	if (strcmp(custom_cart_derived_from, changed_prefs.cartfile) != 0) {
		snprintf(custom_cart_derived_from, sizeof(custom_cart_derived_from), "%s", changed_prefs.cartfile);
		if (!custom_cart_local_edit) {
			// The cartridge path changed outside this panel (config load, GUI
			// reopen): re-derive custom mode from the new value.
			bool known_cart = false;
			for (const auto& entry : cart_rom_list) {
				if (entry.path == changed_prefs.cartfile) {
					known_cart = true;
					break;
				}
			}
			custom_cart_enabled = changed_prefs.cartfile[0] != 0 && !known_cart;
		}
	}
	custom_cart_local_edit = false;
	AmigaCheckbox("Custom Cartridge ROM", &custom_cart_enabled);
	ShowHelpMarker("Use an arbitrary cartridge ROM file. For custom dumps, set the identifier (e.g. :DeMoNv1) so the core can recognize it.");
	if (custom_cart_enabled) {
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
		if (AmigaInputText("##CartFileCustom", changed_prefs.cartfile, MAX_DPATH))
			custom_cart_local_edit = true;
		ImGui::SameLine();
		if (AmigaButton("...##CartFileCustomButton")) {
			current_pick_type = RomPickType::Cartridge;
			OpenFileDialogKey("ROM", "Select Cartridge ROM", ".rom,.bin,.a1000,.a500,.a600,.a3000,.a4000,.cdtv,.cd32", get_rom_path());
		}
		ImGui::Text("Cartridge identifier:");
		ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
		AmigaInputText("##CartIdent", changed_prefs.cartident, sizeof(changed_prefs.cartident));
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Identifier suffix for custom cartridge dumps, e.g. :DeMoNv1, :DeMoNv2, :SuperIV, :XPower or :NPower");
	}

	ImGui::Text("Flash RAM or A2286/A2386SX BIOS CMOS RAM file:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
	AmigaInputText("##FlashROM", changed_prefs.flashfile, MAX_DPATH);
	ImGui::SameLine();
	if (AmigaButton("...##FlashRomFileButton")) {
		current_pick_type = RomPickType::Flash;
		OpenFileDialogKey("ROM", "Select Flash ROM File", ".*", changed_prefs.flashfile);
	}

	ImGui::Text("Real Time Clock file:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browse_btn_w - spacing * 2);
	AmigaInputText("##RTC", changed_prefs.rtcfile, MAX_DPATH);
	ImGui::SameLine();
	if (AmigaButton("...##RTCFileButton")) {
		current_pick_type = RomPickType::RTC;
		OpenFileDialogKey("ROM", "Select RTC File", ".*", changed_prefs.rtcfile);
	}
	ImGui::Spacing();
	EndGroupBox("Miscellaneous");

	BeginGroupBox("Advanced UAE expansion board/Boot ROM Settings");
	const char* uae_items[] = { "ROM disabled", "Original UAE (FS + F0 ROM)", "New UAE (64k + F0 ROM)", "New UAE (128k, ROM, Direct)", "New UAE (128k, ROM, Indirect)" };

	// One combo for the two configuration keys: entry 0 disables both,
	// entry 1 keeps the automatic boot ROM without a board, and the rest
	// pick a board with the automatic ROM.
	int current_uaeboard_idx = has_value(o.boot_rom_uae, "disabled") ? 0
		: choice_index_of(o.uaeboard) + 1;

	ImGui::BeginDisabled(!available(o.uaeboard, ctx));
	ImGui::Text("Board Type:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - spacing * 2);
	if (ImGui::Combo("##BoardTypeCombo", &current_uaeboard_idx, uae_items, IM_ARRAYSIZE(uae_items))) {
		if (current_uaeboard_idx > 0) {
			settings::set(o.uaeboard, changed_prefs, ctx, o.uaeboard.descriptor.choices[current_uaeboard_idx - 1].value);
			settings::set(o.boot_rom_uae, changed_prefs, ctx, "automatic");
		} else {
			settings::set(o.uaeboard, changed_prefs, ctx, "disabled");
			settings::set(o.boot_rom_uae, changed_prefs, ctx, "disabled");
		}
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	ShowHelpMarker("UAE-specific expansion features for compatibility");
	ImGui::EndDisabled();
	ImGui::Spacing();
	EndGroupBox("Advanced UAE expansion board/Boot ROM Settings");

	ImGui::Unindent(4.0f);

	// Handle File Dialog Result
	std::string file_dialog_result;
	if (ConsumeFileDialogResultKey("ROM", file_dialog_result)) {
		switch(current_pick_type) {
			case RomPickType::Main:
				settings::set(o.kickstart_rom_file, changed_prefs, ctx, file_dialog_result.c_str());
				read_kickstart_version(&changed_prefs);
				break;
			case RomPickType::Extended:
				settings::set(o.kickstart_ext_rom_file, changed_prefs, ctx, file_dialog_result.c_str());
				break;
			case RomPickType::Cartridge:
				settings::set(o.cart_file, changed_prefs, ctx, file_dialog_result.c_str());
				sync_cartident(file_dialog_result);
				custom_cart_enabled = true;
				custom_cart_local_edit = true;
				break;
			case RomPickType::Flash:
				settings::set(o.flash_file, changed_prefs, ctx, file_dialog_result.c_str());
				break;
			case RomPickType::RTC:
				settings::set(o.rtc_file, changed_prefs, ctx, file_dialog_result.c_str());
				break;
			case RomPickType::Custom:
				if (rb) snprintf(rb->lf.loadfile, MAX_DPATH, "%s", file_dialog_result.c_str());
				break;
			default:
				break;
		}
		current_pick_type = RomPickType::None;
	}
}
