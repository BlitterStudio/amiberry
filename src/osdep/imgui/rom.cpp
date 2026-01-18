#include "sysdeps.h"
#include "imgui.h"
#include "config.h"
#include "imgui_panels.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "rommgr.h"
#include "registry.h"
#include "uae.h"
#include "memory.h"

struct RomListEntry {
	std::string name;
	std::string path;
};

static std::vector<RomListEntry> main_rom_list;
static std::vector<RomListEntry> ext_rom_list;
static std::vector<RomListEntry> cart_rom_list;
static bool roms_initialized = false;
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
	struct romlist* rl = romlist_getit();
	for (int i = 0; i < count; i++) {
		if (rl[i].rd->type & romtype_mask) {
			const char* name = rl[i].rd->name;
			const char* path = rl[i].path;
			if (path && *path) {
				list.push_back({ name ? name : path, path });
			}
		}
	}
}

static void InitializeROMLists() {
	if (roms_initialized) return;
	update_rom_list(main_rom_list, ROMTYPE_KICK | ROMTYPE_KICKCD32);
	update_rom_list(ext_rom_list, ROMTYPE_EXTCD32 | ROMTYPE_EXTCDTV | ROMTYPE_ARCADIABIOS | ROMTYPE_ALG);
	update_rom_list(cart_rom_list, ROMTYPE_FREEZER | ROMTYPE_ARCADIAGAME | ROMTYPE_CD32CART);
	roms_initialized = true;
}

static bool RomCombo(const char* label, char* current_path, int max_len, std::vector<RomListEntry>& list) {
	std::string preview_value = "Select ROM...";
	bool match_found = false;
	bool value_changed = false;
	
	for (const auto& entry : list) {
		if (entry.path == current_path) {
			preview_value = entry.name;
			match_found = true;
			break;
		}
	}
	if (!match_found && current_path[0]) {
		preview_value = current_path;
	}

	if (ImGui::BeginCombo(label, preview_value.c_str())) {
		for (const auto& entry : list) {
			bool is_selected = (entry.path == current_path);
			if (is_selected)
			{
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			}

			if (ImGui::Selectable(entry.name.c_str(), is_selected)) {
				strncpy(current_path, entry.path.c_str(), max_len);
				value_changed = true;
			}

			if (is_selected)
			{
				ImGui::PopStyleColor(1);
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	return value_changed;
}

void render_panel_rom()
{
	InitializeROMLists();
	ImGui::Indent(5.0f);

	BeginGroupBox("System ROM Settings");
	// Main ROM
	ImGui::Text("Main ROM File:");
	if (RomCombo("##MainRomCombo", changed_prefs.romfile, MAX_DPATH, main_rom_list)) {
		read_kickstart_version(&changed_prefs);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", changed_prefs.romfile);
	ImGui::SameLine();
	if (ImGui::Button("...##MainRomFileButton"))
	{
		current_pick_type = RomPickType::Main;
		OpenFileDialog("Select Main ROM", ".rom,.bin,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", changed_prefs.romfile);
	}

	// Extended ROM
	ImGui::Text("Extended ROM File:");
	RomCombo("##ExtRomCombo", changed_prefs.romextfile, MAX_DPATH, ext_rom_list);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", changed_prefs.romextfile);
	ImGui::SameLine();
	if (ImGui::Button("...##ExtRomFileButton"))
	{
		current_pick_type = RomPickType::Extended;
		OpenFileDialog("Select Extended ROM", ".rom,.bin,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", changed_prefs.romextfile);
	}

    bool maprom_disabled = (changed_prefs.cpuboard_type != 0);
    ImGui::BeginDisabled(maprom_disabled);
	if (ImGui::Checkbox("MapROM emulation", (bool*)&changed_prefs.maprom)) {
		if (changed_prefs.maprom)
			changed_prefs.maprom = 0x0f000000;
	}
    ImGui::EndDisabled();
    
	ImGui::SameLine();
	ImGui::Checkbox("ShapeShifter support", &changed_prefs.kickshifter);
	EndGroupBox("System ROM Settings");

	BeginGroupBox("Advanced Custom ROM Settings");
	// Custom ROM SELECTOR
	const char* custom_rom_items[] = { "ROM #1", "ROM #2", "ROM #3", "ROM #4" };
	ImGui::SetNextItemWidth(100);
	if (ImGui::Combo("##CustomROM", &customromselectnum, custom_rom_items, IM_ARRAYSIZE(custom_rom_items))) {
		// Just changed selection, fields will update automatically in next frame based on selection
	}
	
	struct romboard* rb = &changed_prefs.romboards[customromselectnum];
	
	ImGui::SameLine();
	ImGui::Text("Address range");
	
	// Address From
	char addr_from[16];
	sprintf(addr_from, "%08x", rb->start_address);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::InputText("##AddressRangeFrom", addr_from, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
		rb->start_address = strtoul(addr_from, nullptr, 16);
		rb->start_address &= ~65535; // Logic from values_from_kickstartdlg2
	}

	// Address To
	char addr_to[16];
	if (!rb->end_address && !rb->start_address)
		strcpy(addr_to, "00000000");
	else
		sprintf(addr_to, "%08x", rb->end_address);
		
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	if (ImGui::InputText("##AddressRangeTo", addr_to, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
		rb->end_address = strtoul(addr_to, nullptr, 16);
		// Logic from values_from_kickstartdlg2
		rb->end_address = ((rb->end_address - 1) & ~65535) | 0xffff;
	}

	ImGui::InputText("##CustomRomFilename", rb->lf.loadfile, MAX_DPATH);
	ImGui::SameLine();
    if (ImGui::Button("...##CustomRomFileButton")) {
    	current_pick_type = RomPickType::Custom;
	    OpenFileDialog("Select Custom ROM", ".rom,.bin,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", rb->lf.loadfile);
    }
	EndGroupBox("Advanced Custom ROM Settings");

	BeginGroupBox("Miscellaneous");
	ImGui::Text("Cartridge ROM File:");
	RomCombo("##CartRomCombo", changed_prefs.cartfile, MAX_DPATH, cart_rom_list);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", changed_prefs.cartfile);
	ImGui::SameLine();
	if (ImGui::Button("...##CartRomFileButton")) {
		current_pick_type = RomPickType::Cartridge;
		OpenFileDialog("Select Cartridge ROM", ".rom,.bin,.a500,.a600,.a1200,.a3000,.a4000,.cdtv,.cd32", changed_prefs.cartfile);
	}

	ImGui::Text("Flash RAM or A2286/A2386SX BIOS CMOS RAM file:");
	ImGui::InputText("##FlashROM", changed_prefs.flashfile, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##FlashRomFileButton")) {
		current_pick_type = RomPickType::Flash;
		OpenFileDialog("Select Flash ROM File", ".*", changed_prefs.flashfile);
	}

	ImGui::Text("Real Time Clock file:");
	ImGui::InputText("##RTC", changed_prefs.rtcfile, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##RTCFileButton")) {
		current_pick_type = RomPickType::RTC;
		OpenFileDialog("Select RTC File", ".*", changed_prefs.rtcfile);
	}
	EndGroupBox("Miscellaneous");

	BeginGroupBox("Advanced UAE expansion board/Boot ROM Settings");
	const char* uae_items[] = { "ROM disabled", "Original UAE (FS + F0 ROM)", "New UAE (64k + F0 ROM)", "New UAE (128k, ROM, Direct)", "New UAE (128k, ROM, Indirect)" };
	
	int current_uaeboard_idx = 0;
	if (changed_prefs.boot_rom == 1) {
		current_uaeboard_idx = 0; // Disabled
	} else {
		current_uaeboard_idx = changed_prefs.uaeboard + 1;
	}
	
	bool uae_enabled = !emulating;
	
	ImGui::BeginDisabled(!uae_enabled);
	if (ImGui::Combo("Board type", &current_uaeboard_idx, uae_items, IM_ARRAYSIZE(uae_items))) {
		if (current_uaeboard_idx > 0) {
			changed_prefs.uaeboard = current_uaeboard_idx - 1;
			changed_prefs.boot_rom = 0;
		} else {
			changed_prefs.uaeboard = 0;
			changed_prefs.boot_rom = 1;
		}
	}
	ImGui::EndDisabled();
	EndGroupBox("Advanced UAE expansion board/Boot ROM Settings");

	ImGui::Unindent(5.0f);

	// Handle File Dialog Result
	std::string file_dialog_result;
	if (ConsumeFileDialogResult(file_dialog_result)) {
		switch(current_pick_type) {
			case RomPickType::Main:
				strncpy(changed_prefs.romfile, file_dialog_result.c_str(), MAX_DPATH);
				read_kickstart_version(&changed_prefs);
				break;
			case RomPickType::Extended:
				strncpy(changed_prefs.romextfile, file_dialog_result.c_str(), MAX_DPATH);
				break;
			case RomPickType::Cartridge:
				strncpy(changed_prefs.cartfile, file_dialog_result.c_str(), MAX_DPATH);
				break;
			case RomPickType::Flash:
				strncpy(changed_prefs.flashfile, file_dialog_result.c_str(), MAX_DPATH);
				break;
			case RomPickType::RTC:
				strncpy(changed_prefs.rtcfile, file_dialog_result.c_str(), MAX_DPATH);
				break;
			case RomPickType::Custom:
				if (rb) strncpy(rb->lf.loadfile, file_dialog_result.c_str(), MAX_DPATH);
				break;
			default:
				break;
		}
		current_pick_type = RomPickType::None;
	}
}
