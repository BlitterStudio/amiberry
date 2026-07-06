#include "sysdeps.h"
#include "imgui.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

#include "file_dialog.h"
#include "filesys.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "custom.h"
#include "disk.h"
#include "options.h"
#include "play_content_detection.h"
#include "play_setup.h"
#include "rommgr.h"
#include "target.h"

namespace {

PlayDisplayDefaults display_defaults;
bool display_defaults_initialized = false;
PlayContentDetection selected_content;
auto selected_content_choice = PlayContentType::Unknown;
bool has_selected_content = false;
bool selected_content_applied = false;
bool quickstart_override_tracking = false;
int quickstart_override_model = 0;
int quickstart_override_conf = 0;
int quickstart_override_compa = 0;
std::string applied_config_summary;

const char* shader_choice_name(const PlayShaderChoice choice)
{
	switch (choice) {
	case PlayShaderChoice::None:
		return "none";
	case PlayShaderChoice::Crt:
		return "tv";
	case PlayShaderChoice::Monitor1084:
		return "1084";
	case PlayShaderChoice::Custom:
		return nullptr;
	}

	return "none";
}

const char* follow_up_text(const PlayFollowUp follow_up)
{
	switch (follow_up) {
	case PlayFollowUp::None:
		return "Ready for the next step.";
	case PlayFollowUp::ChooseArchiveContent:
		return "Archive type is ambiguous. Choose whether it is a floppy archive or WHDLoad content.";
	case PlayFollowUp::ChooseDiskOrHardfile:
		return "Disk image type is ambiguous. Choose floppy disk or hardfile in Expert settings.";
	case PlayFollowUp::ChooseCdOrHardfile:
		return "CHD files can be CD media or hardfiles. Choose the matching content type.";
	case PlayFollowUp::ChooseCdMachine:
		return "CD media usually works best with a CD32 or CDTV-oriented setup.";
	case PlayFollowUp::AttachHardfileInExpert:
		return "A1200 expanded profile will be used, with the hardfile attached for editing in Hard drives/CD.";
	}

	return "";
}

const char* suggested_model_name(const PlaySuggestedModel model)
{
	switch (model) {
	case PlaySuggestedModel::KeepExisting:
		return "Current/configured machine";
	case PlaySuggestedModel::A500:
		return "Amiga 500";
	case PlaySuggestedModel::A1200:
		return "Amiga 1200";
	case PlaySuggestedModel::A1200Expanded:
		return "Amiga 1200 expanded";
	case PlaySuggestedModel::Cd32:
		return "CD32";
	}

	return "Current/configured machine";
}

const char* suggested_profile_text(const PlayContentType type, const PlaySuggestedModel model)
{
	if (type == PlayContentType::Configuration)
		return "Configuration file settings";
	if (type == PlayContentType::WhdLoad)
		return "WHDLoad autoload; the database/slave can adjust the exact machine";
	if (model == PlaySuggestedModel::A1200Expanded)
		return "A1200, 2 MB Chip RAM, extra Fast RAM";
	if (model == PlaySuggestedModel::A1200)
		return "A1200, AGA-compatible defaults";
	if (model == PlaySuggestedModel::A500)
		return "A500, most-compatible defaults";
	if (model == PlaySuggestedModel::Cd32)
		return "CD32 console defaults";

	return "Current settings";
}

PlaySuggestedModel suggested_model_for_action(const PlayContentType type)
{
	return play_suggested_model_for_action(selected_content, type);
}

void initialize_display_defaults()
{
	if (display_defaults_initialized)
		return;

	switch (changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen) {
	case GFX_FULLSCREEN:
		display_defaults.screen_mode = PlayScreenMode::Fullscreen;
		break;
	case GFX_FULLWINDOW:
		display_defaults.screen_mode = PlayScreenMode::FullWindow;
		break;
	default:
		display_defaults.screen_mode = PlayScreenMode::Windowed;
		break;
	}

	switch (changed_prefs.scaling_method) {
	case 2:
		display_defaults.scaling = PlayScalingMode::Integer;
		break;
	case 1:
		display_defaults.scaling = PlayScalingMode::Smooth;
		break;
	default:
		display_defaults.scaling = PlayScalingMode::Auto;
		break;
	}

	const auto is_custom_shader = [](const char* shader) {
		return shader[0] != '\0' &&
			strcmp(shader, "none") != 0 &&
			strcmp(shader, "tv") != 0 &&
			strcmp(shader, "1084") != 0;
	};

	if (is_custom_shader(changed_prefs.shader) || is_custom_shader(changed_prefs.shader_rtg))
		display_defaults.shader = PlayShaderChoice::Custom;
	else if (strcmp(changed_prefs.shader, "1084") == 0)
		display_defaults.shader = PlayShaderChoice::Monitor1084;
	else if (strcmp(changed_prefs.shader, "tv") == 0)
		display_defaults.shader = PlayShaderChoice::Crt;
	else
		display_defaults.shader = PlayShaderChoice::None;

	display_defaults.auto_crop = changed_prefs.gfx_auto_crop;
	display_defaults_initialized = true;
}

void copy_shader_name(char* destination, const size_t destination_size, const char* value)
{
	if (destination_size == 0)
		return;

	strncpy(destination, value, destination_size - 1);
	destination[destination_size - 1] = '\0';
}

void apply_display_defaults_to_changed_prefs()
{
	const PlayDisplayPrefs prefs = play_apply_display_defaults(display_defaults);

	changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = prefs.native_fullscreen;
	changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = prefs.rtg_fullscreen;
	changed_prefs.scaling_method = prefs.scaling_method;
	changed_prefs.gfx_autoresolution = prefs.gfx_autoresolution;
	changed_prefs.gfx_auto_crop = prefs.gfx_auto_crop;
	if (changed_prefs.gfx_auto_crop)
		changed_prefs.gfx_manual_crop = false;

	if (!prefs.preserve_shader) {
		const char* shader = shader_choice_name(display_defaults.shader);
		copy_shader_name(changed_prefs.shader, sizeof changed_prefs.shader, shader);
		copy_shader_name(changed_prefs.shader_rtg, sizeof changed_prefs.shader_rtg, shader);
	}
}

void save_display_defaults()
{
	const PlayDisplayPrefs prefs = play_apply_display_defaults(display_defaults);

	amiberry_options.default_fullscreen_mode = prefs.native_fullscreen;
	amiberry_options.default_scaling_method = prefs.scaling_method;
	amiberry_options.default_gfx_autoresolution = prefs.gfx_autoresolution;
	amiberry_options.default_auto_crop = prefs.gfx_auto_crop;

	if (prefs.preserve_shader) {
		copy_shader_name(amiberry_options.shader, sizeof amiberry_options.shader, changed_prefs.shader);
		copy_shader_name(amiberry_options.shader_rtg, sizeof amiberry_options.shader_rtg, changed_prefs.shader_rtg);
	} else {
		const char* shader = shader_choice_name(display_defaults.shader);
		copy_shader_name(amiberry_options.shader, sizeof amiberry_options.shader, shader);
		copy_shader_name(amiberry_options.shader_rtg, sizeof amiberry_options.shader_rtg, shader);
	}

	save_amiberry_settings();
}

void remember_parent_directory(const std::string& path)
{
	const auto separator = path.find_last_of("/\\");
	if (separator != std::string::npos)
		last_floppy_dir = path.substr(0, separator);
}

void copy_path_to_buffer(char* destination, const size_t destination_size, const std::string& path)
{
	if (destination_size == 0)
		return;

	strncpy(destination, path.c_str(), destination_size - 1);
	destination[destination_size - 1] = '\0';
}

void begin_quickstart_override_tracking()
{
	quickstart_override_model = quickstart_model;
	quickstart_override_conf = quickstart_conf;
	quickstart_override_compa = quickstart_compa;
	quickstart_override_tracking = true;
}

void reset_quickstart_override_tracking()
{
	quickstart_override_tracking = false;
}

bool quickstart_override_changed()
{
	return quickstart_override_tracking &&
		(quickstart_model != quickstart_override_model ||
			quickstart_conf != quickstart_override_conf ||
			quickstart_compa != quickstart_override_compa);
}

void apply_quickstart_model(const PlaySuggestedModel model)
{
	int model_index = -1;
	int config_index = 0;

	switch (model) {
	case PlaySuggestedModel::A500:
		model_index = 0;
		break;
	case PlaySuggestedModel::A1200:
		model_index = 4;
		break;
	case PlaySuggestedModel::A1200Expanded:
		model_index = 4;
		config_index = 1;
		break;
	case PlaySuggestedModel::Cd32:
		model_index = 8;
		break;
	case PlaySuggestedModel::KeepExisting:
		return;
	}

	quickstart_model = model_index;
	quickstart_conf = config_index;
	quickstart_compa = selected_content.suggested_compatibility;
	Quickstart_ApplyDefaults();
}

void apply_quickstart_model_unless_overridden(const PlaySuggestedModel model)
{
	if (quickstart_override_changed())
		return;

	apply_quickstart_model(model);
	reset_quickstart_override_tracking();
}

const char* machine_name_from_prefs()
{
	switch (changed_prefs.cs_compatible) {
	case CP_A500:
		return "Amiga 500";
	case CP_A500P:
		return "Amiga 500+";
	case CP_A600:
		return "Amiga 600";
	case CP_A1000:
		return "Amiga 1000";
	case CP_A1200:
		return "Amiga 1200";
	case CP_A3000:
		return "Amiga 3000";
	case CP_A3000T:
		return "Amiga 3000T";
	case CP_A4000:
		return "Amiga 4000";
	case CP_A4000T:
		return "Amiga 4000T";
	case CP_CD32:
		return "CD32";
	case CP_CDTV:
		return "CDTV";
	case CP_CDTVCR:
		return "CDTV-CR";
	default:
		break;
	}

	return "Custom Amiga";
}

const char* chipset_name_from_prefs()
{
	if (changed_prefs.chipset_mask & CSMASK_AGA)
		return "AGA";
	if ((changed_prefs.chipset_mask & CSMASK_ECS_AGNUS) || (changed_prefs.chipset_mask & CSMASK_ECS_DENISE))
		return "ECS";
	return "OCS";
}

std::string format_memory_size(const unsigned long long bytes)
{
	char buffer[64];
	if (bytes >= 1024ULL * 1024ULL)
		snprintf(buffer, sizeof buffer, "%llu MB", bytes / (1024ULL * 1024ULL));
	else
		snprintf(buffer, sizeof buffer, "%llu KB", bytes / 1024ULL);
	return buffer;
}

unsigned long long total_fast_memory()
{
	unsigned long long total = 0;
	for (int i = 0; i < MAX_RAM_BOARDS; ++i) {
		total += changed_prefs.fastmem[i].size;
		total += changed_prefs.z3fastmem[i].size;
	}
	total += changed_prefs.cpuboardmem1.size;
	total += changed_prefs.cpuboardmem2.size;
	total += changed_prefs.mbresmem_low.size;
	total += changed_prefs.mbresmem_high.size;
	return total;
}

std::string describe_current_config()
{
	char buffer[256];
	snprintf(buffer, sizeof buffer, "%s, %d, %s, %s Chip RAM",
		machine_name_from_prefs(),
		changed_prefs.cpu_model,
		chipset_name_from_prefs(),
		format_memory_size(changed_prefs.chipmem.size).c_str());

	std::string summary = buffer;
	const auto fast = total_fast_memory();
	if (fast > 0) {
		summary += ", ";
		summary += format_memory_size(fast);
		summary += " Fast RAM";
	}
	return summary;
}

void mark_content_applied()
{
	applied_config_summary = describe_current_config();
	selected_content_applied = true;
	reset_quickstart_override_tracking();
}

bool attach_selected_hardfile()
{
#ifdef FILESYS
	if (changed_prefs.mountitems >= MOUNT_CONFIG_SIZE) {
		ShowMessageBox("Hardfile", "The hardfile list is full. Remove an existing entry first.");
		return false;
	}

	default_hfdlg(&current_hfdlg, false);
	current_hfdlg.ci.type = UAEDEV_HDF;
	copy_path_to_buffer(current_hfdlg.ci.rootdir, sizeof current_hfdlg.ci.rootdir,
		selected_content.original_path);

	if (current_hfdlg.ci.devname[0] == 0) {
		char devname[256];
		CreateDefaultDevicename(devname);
		au_copy(current_hfdlg.ci.devname, sizeof current_hfdlg.ci.devname, devname);
	}

	hardfile_testrdb(&current_hfdlg);
	inithdcontroller(current_hfdlg.ci.controller_type, current_hfdlg.ci.controller_type_unit,
		UAEDEV_HDF, current_hfdlg.ci.rootdir[0] != 0);

	uaedev_config_info ci{};
	memcpy(&ci, &current_hfdlg.ci, sizeof ci);
	uaedev_config_data* uci = add_filesys_config(&changed_prefs, -1, &ci);
	if (!uci) {
		ShowMessageBox("Hardfile", "Failed to attach the selected hardfile.");
		return false;
	}

	if (auto* const hfd = get_hardfile_data(uci->configoffset))
		hardfile_media_change(hfd, &ci, true, false);
	gui_force_rtarea_hdchange();
	return true;
#else
	ShowMessageBox("Hardfile", "Hardfile support is not available in this build.");
	return false;
#endif
}

bool apply_configuration_content()
{
	if (!target_cfgfile_load(&changed_prefs, selected_content.original_path.c_str(), CONFIG_TYPE_DEFAULT, 0)) {
		ShowMessageBox("Load Configuration", "Failed to load configuration.");
		return false;
	}

	gui_config_mark_clean();
	mark_content_applied();
	return true;
}

bool apply_floppy_content()
{
	apply_quickstart_model_unless_overridden(suggested_model_for_action(PlayContentType::Floppy));
	apply_display_defaults_to_changed_prefs();
	remember_parent_directory(selected_content.original_path);

	const int archive_count = populate_diskswapper_from_archive(selected_content.original_path.c_str(), &changed_prefs);
	if (archive_count > 0) {
		copy_path_to_buffer(changed_prefs.floppyslots[0].df, sizeof changed_prefs.floppyslots[0].df,
			changed_prefs.dfxlist[0]);
		disk_insert(0, changed_prefs.floppyslots[0].df);
		add_file_to_mru_list(lstMRUDiskList, std::string(changed_prefs.dfxlist[0]));
		set_last_active_config(changed_prefs.dfxlist[0]);
		mark_content_applied();
		return true;
	}

	copy_path_to_buffer(changed_prefs.floppyslots[0].df, sizeof changed_prefs.floppyslots[0].df,
		selected_content.original_path);
	disk_insert(0, changed_prefs.floppyslots[0].df);
	add_file_to_mru_list(lstMRUDiskList, selected_content.original_path);
	set_last_active_config(selected_content.original_path.c_str());
	mark_content_applied();
	return true;
}

bool apply_whdload_content()
{
	apply_quickstart_model_unless_overridden(PlaySuggestedModel::A1200);
	whdload_prefs.whdload_filename = selected_content.original_path;
	add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
	whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
	apply_display_defaults_to_changed_prefs();
	set_last_active_config(whdload_prefs.whdload_filename.c_str());
	mark_content_applied();
	return true;
}

bool apply_hardfile_content()
{
	apply_quickstart_model_unless_overridden(PlaySuggestedModel::A1200Expanded);
	apply_display_defaults_to_changed_prefs();
	if (!attach_selected_hardfile())
		return false;

	set_last_active_config(selected_content.original_path.c_str());
	mark_content_applied();
	return true;
}

bool apply_cd_content()
{
	apply_quickstart_model_unless_overridden(PlaySuggestedModel::Cd32);

	char path[MAX_DPATH];
	copy_path_to_buffer(path, sizeof path, selected_content.original_path);
	cd_auto_prefs(&changed_prefs, path);
	apply_display_defaults_to_changed_prefs();
	add_file_to_mru_list(lstMRUCDList, selected_content.original_path);
	set_last_active_config(selected_content.original_path.c_str());
	mark_content_applied();
	return true;
}

bool apply_selected_content(const PlayContentType type)
{
	switch (type) {
	case PlayContentType::Configuration:
		return apply_configuration_content();
	case PlayContentType::Floppy:
		return apply_floppy_content();
	case PlayContentType::WhdLoad:
		return apply_whdload_content();
	case PlayContentType::Cd:
		return apply_cd_content();
	case PlayContentType::Hardfile:
		return apply_hardfile_content();
	case PlayContentType::Ambiguous:
	case PlayContentType::Unknown:
		ShowMessageBox("Play", "Choose a supported content type first.");
		return false;
	}

	return false;
}

PlayContentType selected_action_type()
{
	if (!has_selected_content)
		return PlayContentType::Unknown;
	if (selected_content_choice != PlayContentType::Unknown)
		return selected_content_choice;
	return selected_content.type;
}

void select_content_choice(const PlayContentType choice)
{
	if (selected_content_choice == choice)
		return;

	selected_content_choice = choice;
	selected_content_applied = false;
	applied_config_summary.clear();
}

void select_content_path(const std::string& selected_path)
{
	std::error_code ec;
	const bool is_directory = std::filesystem::is_directory(selected_path, ec);
	selected_content = play_detect_content(selected_path, is_directory);
	selected_content_choice = PlayContentType::Unknown;
	selected_content_applied = false;
	reset_quickstart_override_tracking();
	applied_config_summary.clear();
	has_selected_content = true;
}

void render_setup_status_row(const char* label, const char* value)
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine(BUTTON_WIDTH * 1.8f);
	ImGui::TextWrapped("%s", value);
}

bool is_builtin_aros_rom(const romdata* rd)
{
	if (!rd)
		return false;
	if (rd->id == 66)
		return true;
	return rd->configname && _tcscmp(rd->configname, _T("AROS")) == 0;
}

bool has_real_kickstart_roms()
{
	const int count = romlist_count();
	const romlist* list = romlist_getit();
	for (int i = 0; i < count; ++i) {
		const auto& entry = list[i];
		if (!entry.rd || !entry.path || entry.path[0] == 0)
			continue;
		if (!(entry.rd->type & (ROMTYPE_KICK | ROMTYPE_KICKCD32)))
			continue;
		if (is_builtin_aros_rom(entry.rd))
			continue;
		return true;
	}

	return false;
}

void render_rom_setup()
{
	if (has_real_kickstart_roms())
		render_setup_status_row("Kickstart ROMs:", "Kickstart ROMs detected.");
	else
		render_setup_status_row("Kickstart ROMs:", "Only the bundled AROS fallback was detected. Add original Kickstart ROMs for better compatibility, then rescan.");
	render_setup_status_row("ROM folder:", get_rom_path().c_str());

	if (AmigaButton(ICON_FA_FOLDER_OPEN " Choose ROM folder...", ImVec2(BUTTON_WIDTH * 2.2f, BUTTON_HEIGHT)))
		OpenDirDialogKey("PLAY_ROM_DIR", get_rom_path());

	std::string selected_path;
	if (ConsumeDirDialogResultKey("PLAY_ROM_DIR", selected_path)) {
		set_rom_path(selected_path);
		save_amiberry_settings();
		scan_roms(true);
		symlink_roms(&changed_prefs);
		ShowMessageBox("ROMs", "ROM folder updated and scanned.");
	}

	ImGui::SameLine();
	if (AmigaButton(ICON_FA_ARROWS_ROTATE " Rescan ROMs", ImVec2(BUTTON_WIDTH * 1.7f, BUTTON_HEIGHT))) {
		scan_roms(true);
		symlink_roms(&changed_prefs);
		ShowMessageBox("ROMs", "ROM scan complete.");
	}

	ImGui::SameLine();
	if (AmigaButton(ICON_FA_SIM_CARD " Open ROM settings", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
		gui_show_panel("rom", true);
	ImGui::Spacing();
}

bool render_combo(const char* label, int* value, const char* const* items, const int item_count)
{
	bool changed = false;
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine(BUTTON_WIDTH * 1.5f);
	ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
	if (ImGui::BeginCombo((std::string("##") + label).c_str(), items[*value])) {
		for (int i = 0; i < item_count; ++i) {
			const bool selected = *value == i;
			if (ImGui::Selectable(items[i], selected)) {
				*value = i;
				changed = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	ImGui::Spacing();
	return changed;
}

void render_display_defaults()
{
	initialize_display_defaults();

	static const char* screen_items[] = { "Windowed", "Full-window", "Fullscreen" };
	static const char* scaling_items[] = { "Auto", "Integer", "Smooth" };
	static const char* shader_items[] = { "None", "CRT", "1084", "Custom" };

	int screen_mode = static_cast<int>(display_defaults.screen_mode);
	if (render_combo("Screen mode:", &screen_mode, screen_items, IM_ARRAYSIZE(screen_items))) {
		display_defaults.screen_mode = static_cast<PlayScreenMode>(screen_mode);
		apply_display_defaults_to_changed_prefs();
	}

	int scaling = static_cast<int>(display_defaults.scaling);
	if (render_combo("Scaling:", &scaling, scaling_items, IM_ARRAYSIZE(scaling_items))) {
		display_defaults.scaling = static_cast<PlayScalingMode>(scaling);
		apply_display_defaults_to_changed_prefs();
	}
	if (display_defaults.scaling == PlayScalingMode::Integer)
		ImGui::TextWrapped("Integer scaling works best with Resolution Autoswitch set to Always On. This flow enables that automatically.");

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("AutoCrop:");
	ImGui::SameLine(BUTTON_WIDTH * 1.5f);
	if (AmigaCheckbox("##AutoCrop", &display_defaults.auto_crop))
		apply_display_defaults_to_changed_prefs();
	ShowHelpMarker("Automatically crop black borders from the display.");

	int shader = static_cast<int>(display_defaults.shader);
	if (render_combo("Shader:", &shader, shader_items, IM_ARRAYSIZE(shader_items))) {
		display_defaults.shader = static_cast<PlayShaderChoice>(shader);
		apply_display_defaults_to_changed_prefs();
	}

	ImGui::Spacing();
	if (AmigaButton("Apply now", ImVec2(BUTTON_WIDTH * 1.2f, BUTTON_HEIGHT)))
		apply_display_defaults_to_changed_prefs();
	ImGui::SameLine();
	if (AmigaButton("Save defaults", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT))) {
		apply_display_defaults_to_changed_prefs();
		save_display_defaults();
		ShowMessageBox("Display Defaults", "Display defaults saved.");
	}
	ImGui::Spacing();
}

void render_content_picker()
{
	if (AmigaButton(ICON_FA_FOLDER_OPEN " Choose content...", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT))) {
		OpenFileDialogKey("PLAY_CONTENT", "Choose Amiga content",
			"Amiga Content (*.uae,*.adf,*.adz,*.dms,*.ipf,*.zip,*.7z,*.lha,*.lzh,*.lzx,*.cue,*.bin,*.iso,*.ccd,*.mds,*.chd,*.nrg,*.hdf,*.hdz,*.hda,*.vhd,*.img){.uae,.adf,.adz,.dms,.ipf,.zip,.7z,.lha,.lzh,.lzx,.cue,.bin,.iso,.ccd,.mds,.chd,.nrg,.hdf,.hdz,.hda,.vhd,.img},All Files (*){.*}",
			get_floppy_path());
	}
	if (AmigaButton(ICON_FA_FOLDER_OPEN " Choose folder...", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
		OpenDirDialogKey("PLAY_CONTENT_DIR", get_floppy_path());

	std::string selected_path;
	if (ConsumeFileDialogResultKey("PLAY_CONTENT", selected_path) ||
		ConsumeFileDialogResultKey("PLAY_CONTENT_DIR", selected_path))
		select_content_path(selected_path);

	if (has_selected_content) {
		ImGui::Spacing();
		render_setup_status_row("Selected:", selected_content.display_name.c_str());
		render_setup_status_row("Detected as:", play_content_type_name(selected_content.type));
		render_setup_status_row("Next step:", follow_up_text(selected_content.follow_up));

		if (selected_content.type == PlayContentType::Ambiguous) {
			ImGui::Spacing();
			for (const PlayContentType choice : selected_content.choices) {
				if (choice == PlayContentType::Hardfile) {
					if (AmigaButton("Use as hardfile...", ImVec2(BUTTON_WIDTH * 1.7f, BUTTON_HEIGHT))) {
						select_content_choice(PlayContentType::Hardfile);
						play_prepare_selected_content_for_start();
					}
				} else {
					std::string label = std::string("Use as ") + play_content_type_name(choice);
					if (AmigaButton(label.c_str(), ImVec2(BUTTON_WIDTH * 1.6f, BUTTON_HEIGHT))) {
						select_content_choice(choice);
					}
				}
				ImGui::SameLine();
			}
			ImGui::NewLine();
		}

		const PlayContentType action_type = selected_action_type();
		if (action_type != PlayContentType::Unknown && action_type != PlayContentType::Ambiguous) {
			const PlaySuggestedModel suggested_model = suggested_model_for_action(action_type);
			render_setup_status_row("Will use:", suggested_model_name(suggested_model));
			render_setup_status_row("Hardware:", suggested_profile_text(action_type, suggested_model));
			if (!applied_config_summary.empty())
				render_setup_status_row("Configured:", applied_config_summary.c_str());

			ImGui::Spacing();
			if (AmigaButton("Use this content", ImVec2(BUTTON_WIDTH * 1.7f, BUTTON_HEIGHT)))
				play_prepare_selected_content_for_start();
			ImGui::SameLine();
			if (AmigaButton(ICON_FA_ROCKET " Change model...", ImVec2(BUTTON_WIDTH * 1.8f, BUTTON_HEIGHT))) {
				begin_quickstart_override_tracking();
				gui_show_panel("quickstart", true);
			}
			if (action_type == PlayContentType::Hardfile) {
				ImGui::SameLine();
				if (AmigaButton(ICON_FA_HARD_DRIVE " Edit storage...", ImVec2(BUTTON_WIDTH * 1.8f, BUTTON_HEIGHT))) {
					if (play_prepare_selected_content_for_start())
						gui_show_panel("hd", true);
				}
			}
		}
	}
}

} // namespace

void render_panel_play()
{
	ImGui::Indent(4.0f);

	if (!play_setup_dismissed()) {
		if (BeginGroupBox("First-run setup", true, true)) {
			render_rom_setup();
			ImGui::Spacing();
			if (AmigaButton("Done", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
				play_set_setup_dismissed(true);
		}
		EndGroupBox("First-run setup");
	}

	if (BeginGroupBox("Play", true, true)) {
		render_content_picker();
		ImGui::Spacing();
		if (!has_selected_content &&
			AmigaButton(ICON_FA_ROCKET " Open Quickstart", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
			gui_show_panel("quickstart", true);
	}
	ImGui::Spacing();
	EndGroupBox("Play");

	if (BeginGroupBox("Display defaults", true, true)) {
		render_display_defaults();
	}
	EndGroupBox("Display defaults");

	if (play_setup_dismissed()) {
		ImGui::Spacing();
		if (AmigaButton("Show first-run setup", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
			play_set_setup_dismissed(false);
	}
}

bool play_has_content_selection()
{
	return has_selected_content;
}

bool play_prepare_selected_content_for_start()
{
	if (!has_selected_content || selected_content_applied)
		return true;

	const PlayContentType action_type = selected_action_type();
	if (action_type == PlayContentType::Unknown)
		return true;

	return apply_selected_content(action_type);
}
