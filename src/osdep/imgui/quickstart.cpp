#include "sysdeps.h"
#include "blkdev.h"
#include "imgui.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "disk.h"
#include "imgui_panels.h"
#include "uae.h"

extern void ShowMessageBox(const char* title, const char* message);
extern void DisplayDiskInfo(int drv);

std::vector<const char*> qs_models;
std::vector<const char*> qs_configs;

// Quickstart MRU display helpers/state
static bool qs_ignore_list_change = false;
static std::vector<std::string> qs_disk_display;
static std::vector<std::string> qs_cd_display;
static std::vector<std::string> qs_whd_display;

static void qs_refresh_disk_list_model()
{
	qs_disk_display.clear();
	for (const auto &entry : lstMRUDiskList)
	{
		const std::string full_path = entry;
		const auto sep = full_path.find_last_of("/\\");
		const std::string filename = sep == std::string::npos ? full_path : full_path.substr(sep + 1);
		qs_disk_display.emplace_back(filename + " { " + full_path + " }");
	}
}

static void qs_refresh_cd_list_model()
{
	qs_cd_display.clear();
	auto cd_drives = get_cd_drives();
	for (const auto &drive : cd_drives)
		qs_cd_display.emplace_back(drive);
	for (const auto &entry : lstMRUCDList)
	{
		const std::string full_path = entry;
		const auto sep = full_path.find_last_of("/\\");
		const std::string filename = sep == std::string::npos ? full_path : full_path.substr(sep + 1);
		qs_cd_display.emplace_back(filename + " { " + full_path + " }");
	}
}

static void qs_refresh_whd_list_model()
{
	qs_whd_display.clear();
	for (const auto &entry : lstMRUWhdloadList)
	{
		const std::string full_path = entry;
		const auto sep = full_path.find_last_of("/\\");
		const std::string filename = sep == std::string::npos ? full_path : full_path.substr(sep + 1);
		qs_whd_display.emplace_back(filename + " { " + full_path + " }");
	}
}

static int qs_find_in_mru(const std::vector<std::string> &mru, const char *path)
{
	if (!path || !*path)
		return -1;
	for (int i = 0; i < static_cast<int>(mru.size()); ++i)
	{
		if (mru[i] == path)
			return i;
	}
	return -1;
}

static void qs_set_control_state(int model, bool &df1_visible, bool &cd_visible, bool &df0_editable)
{
	df1_visible = true;
	cd_visible = false;
	df0_editable = true;

	switch (model)
	{
		case 8: // CD32
		case 9: // CDTV
		case 10: // American Laser Games / Picmatic
		case 11: // Arcadia Multi Select system
			df0_editable = true;
			df1_visible = false;
			cd_visible = true;
			break;
		default:
			break;
	}
}

static void adjust_prefs() {
	built_in_prefs(&changed_prefs, quickstart_model, quickstart_conf, 0, 0);
	switch (quickstart_model)
	{
		case 0: // A500
		case 1: // A500+
		case 2: // A600
		case 3: // A1000
		case 4: // A1200
		case 5: // A3000
			// df0 always active
			changed_prefs.floppyslots[0].dfxtype = DRV_35_DD;
			changed_prefs.floppyslots[1].dfxtype = DRV_NONE;

			// No CD available
			changed_prefs.cdslots[0].inuse = false;
			changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;

			// Set joystick port to Default
			changed_prefs.jports[1].mode = 0;
			break;
		case 6: // A4000
		case 7: // A4000T
		case 12: // Macrosystem
			// df0 always active
			changed_prefs.floppyslots[0].dfxtype = DRV_35_HD;
			changed_prefs.floppyslots[1].dfxtype = DRV_NONE;

			// No CD available
			changed_prefs.cdslots[0].inuse = false;
			changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;

			// Set joystick port to Default
			changed_prefs.jports[1].mode = 0;
			break;

		case 8: // CD32
		case 9: // CDTV
		case 10:// American Laser Games / Picmatic
		case 11:// Arcadia Multi Select system
			// No floppy drive available, CD available
			changed_prefs.floppyslots[0].dfxtype = DRV_NONE;
			changed_prefs.floppyslots[1].dfxtype = DRV_NONE;
			changed_prefs.cdslots[0].inuse = true;
			changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
			changed_prefs.gfx_monitor[0].gfx_size.width = 720;
			changed_prefs.gfx_monitor[0].gfx_size.height = 568;
			// Set joystick port to CD32 mode
			changed_prefs.jports[1].mode = 7;
			break;
		default:
			break;
	}
}

void render_panel_quickstart()
{
	// Refresh MRU display lists once per frame
	qs_refresh_disk_list_model();
	qs_refresh_cd_list_model();
	qs_refresh_whd_list_model();

	// State for asynchronous file dialogs in this panel
	static int qs_pending_floppy_drive = -1;
	static bool qs_pending_cd = false;
	static bool qs_pending_whd = false;

	bool df1_visible = true;
	bool cd_visible = false;
	bool df0_editable = true;
	qs_set_control_state(quickstart_model, df1_visible, cd_visible, df0_editable);

	if (ImGui::BeginTable("QuickstartModelTable", 2, ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

		// Model row
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Model:");
		ImGui::TableSetColumnIndex(1);
		if (ImGui::Combo("##QuickstartModel", &quickstart_model, qs_models.data(), static_cast<int>(qs_models.size())))
		{
			qs_configs.clear();
			for (auto &config : amodels[quickstart_model].configs)
			{
				if (config[0] == '\0')
					break;
				qs_configs.push_back(config);
			}
			quickstart_conf = 0;
			adjust_prefs();
		}
		ImGui::SameLine();
		bool ntsc = changed_prefs.ntscmode != 0;
		if (ImGui::Checkbox("NTSC", &ntsc))
		{
			changed_prefs.ntscmode = ntsc;
			changed_prefs.chipset_refreshrate = ntsc ? 60 : 50;
		}

		// Configuration row
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Configuration:");
		ImGui::TableSetColumnIndex(1);

		if (!qs_configs.empty())
		{
			if (quickstart_conf < 0 || quickstart_conf >= static_cast<int>(qs_configs.size()))
				quickstart_conf = 0;
			if (ImGui::Combo("##QuickstartConf", &quickstart_conf, qs_configs.data(), static_cast<int>(qs_configs.size())))
			{
				adjust_prefs();
			}
		}

		ImGui::EndTable();
	}

	ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 2));

	auto render_floppy_drive = [&](int i, bool is_editable)
	{
		ImGui::PushID(i);
		char label[64];
		snprintf(label, sizeof(label), "DF%d:", i);

		bool drive_enabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
		bool disk_present = std::strlen(changed_prefs.floppyslots[i].df) > 0;

		if (!is_editable) ImGui::BeginDisabled();
		if (ImGui::Checkbox(label, &drive_enabled))
		{
			if (drive_enabled)
			{
				changed_prefs.floppyslots[i].dfxtype = DRV_35_DD;
			}
			else
			{
				changed_prefs.floppyslots[i].dfxtype = DRV_NONE;
				changed_prefs.floppyslots[i].df[0] = 0;
			}
		}
		if (!is_editable) ImGui::EndDisabled();
		ImGui::SameLine();

		int nn = fromdfxtype(i, changed_prefs.floppyslots[i].dfxtype, changed_prefs.floppyslots[i].dfxsubtype);
		int selectedFloppyType = nn + 1;
		ImGui::SetNextItemWidth(120.0f);
		snprintf(label, sizeof(label), "##QSFloppyType%d", i);
		if (!drive_enabled) ImGui::BeginDisabled();
		if (ImGui::Combo(label, &selectedFloppyType, floppy_drive_types, IM_ARRAYSIZE(floppy_drive_types)))
		{
			int sub = 0;
			int dfxtype = todfxtype(i, selectedFloppyType - 1, &sub);
			changed_prefs.floppyslots[i].dfxtype = dfxtype;
			changed_prefs.floppyslots[i].dfxsubtype = sub;
			if (dfxtype == DRV_FB)
			{
				TCHAR tmp[32];
				_sntprintf(tmp, sizeof tmp, _T("%d:%s"), selectedFloppyType - 5, drivebridgeModes[selectedFloppyType - 6].data());
				_tcscpy(changed_prefs.floppyslots[i].dfxsubtypeid, tmp);
			}
			else
			{
				changed_prefs.floppyslots[i].dfxsubtypeid[0] = 0;
			}
		}
		if (!drive_enabled) ImGui::EndDisabled();

		ImGui::SameLine();

		bool wp_enabled = drive_enabled && !changed_prefs.floppy_read_only && disk_present;
		if (!wp_enabled) ImGui::BeginDisabled();
		bool wp = disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i) != 0;
		snprintf(label, sizeof(label), "Write-protected##QSFloppyWriteProtected%d", i);
		if (ImGui::Checkbox(label, &wp))
		{
			disk_setwriteprotect(&changed_prefs, i, changed_prefs.floppyslots[i].df, wp);
			if (disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i) != wp)
			{
				wp = !wp;
				ShowMessageBox("Set/Clear write protect", "Failed to change write permission.\nMaybe underlying filesystem doesn't support this.");
			}
			DISK_reinsert(i);
		}
		if (!wp_enabled) ImGui::EndDisabled();

		ImGui::SameLine();

		bool info_enabled = drive_enabled && disk_present;
		if (!info_enabled) ImGui::BeginDisabled();
		snprintf(label, sizeof(label), "?##QSFloppyInfo%d", i);
		if (ImGui::Button(label, ImVec2(SMALL_BUTTON_WIDTH, 0)))
		{
			DisplayDiskInfo(i);
		}
		if (!info_enabled) ImGui::EndDisabled();

		const float button_width = BUTTON_WIDTH;
		const float buttons_total_width = button_width * 2 + ImGui::GetStyle().ItemSpacing.x;
		float offset = ImGui::GetWindowContentRegionMax().x - buttons_total_width;
		ImGui::SameLine(offset);

		bool eject_enabled = drive_enabled && disk_present;
		if (!eject_enabled) ImGui::BeginDisabled();
		if (ImGui::Button("Eject", ImVec2(button_width, 0)))
		{
			disk_eject(i);
			changed_prefs.floppyslots[i].df[0] = 0;
		}
		if (!eject_enabled) ImGui::EndDisabled();

		ImGui::SameLine();

		if (!drive_enabled) ImGui::BeginDisabled();
		if (ImGui::Button("Select file", ImVec2(button_width, 0)))
		{
			std::string tmp;
			if (disk_present)
				tmp = changed_prefs.floppyslots[i].df;
			else
				tmp = get_floppy_path();
			OpenFileDialog("Select disk image file", ".adf,.adz,.ipf,.dms,.fdi,.hdf,.img,.*", tmp);
			qs_pending_floppy_drive = i;
		}
		if (!drive_enabled) ImGui::EndDisabled();

		int selected_index = -1;
		if (disk_present)
			selected_index = qs_find_in_mru(lstMRUDiskList, changed_prefs.floppyslots[i].df);

		std::vector<const char*> items;
		items.push_back("<empty>");
		items.reserve(qs_disk_display.size() + 1);
		for (auto &s : qs_disk_display)
			items.push_back(s.c_str());

		int combo_index = selected_index + 1;
		snprintf(label, sizeof(label), "##QSFloppyImagePath%d", i);
		ImGui::PushItemWidth(-1);
		if (ImGui::Combo(label, &combo_index, items.data(), static_cast<int>(items.size())))
		{
			if (combo_index == 0)
			{
				disk_eject(i);
				changed_prefs.floppyslots[i].df[0] = 0;
			}
			else if (!qs_ignore_list_change && combo_index > 0 && combo_index <= static_cast<int>(lstMRUDiskList.size()))
			{
				std::string element = get_full_path_from_disk_list(qs_disk_display[combo_index - 1]);
				if (element != changed_prefs.floppyslots[i].df)
				{
					std::strncpy(changed_prefs.floppyslots[i].df, element.c_str(), MAX_DPATH);
					DISK_history_add(changed_prefs.floppyslots[i].df, -1, HISTORY_FLOPPY, 0);
					disk_insert(i, changed_prefs.floppyslots[i].df);
					if (!last_loaded_config[0])
						set_last_active_config(element.c_str());
				}
			}
		}
		ImGui::PopItemWidth();
		ImGui::PopID();
	};

	render_floppy_drive(0, df0_editable);

	ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing()));

	if (df1_visible)
	{
		render_floppy_drive(1, true);
	}
	else if (cd_visible)
	{
		ImGui::PushID("CD");
		bool cd_inuse = changed_prefs.cdslots[0].inuse;
		ImGui::BeginDisabled();
		ImGui::Checkbox("CD Drive", &cd_inuse);
		ImGui::EndDisabled();

		const float button_width = BUTTON_WIDTH;
		const float buttons_total_width = button_width * 2 + ImGui::GetStyle().ItemSpacing.x;
		float offset = ImGui::GetWindowContentRegionMax().x - buttons_total_width;
		ImGui::SameLine(offset);

		bool cd_controls_enabled = changed_prefs.cdslots[0].inuse;
		if (!cd_controls_enabled) ImGui::BeginDisabled();
		if (ImGui::Button("Eject", ImVec2(button_width, 0)))
		{
			changed_prefs.cdslots[0].name[0] = 0;
			changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
		}
		ImGui::SameLine();
		if (ImGui::Button("Select file", ImVec2(button_width, 0)))
		{
			std::string tmp;
			if (std::strlen(changed_prefs.cdslots[0].name) > 0)
				tmp = changed_prefs.cdslots[0].name;
			else
				tmp = get_cdrom_path();

			OpenFileDialog("Select CD image file", ".cue,.bin,.iso,.ccd,.mds,.chd,.*", tmp);
			qs_pending_cd = true;
		}
		if (!cd_controls_enabled) ImGui::EndDisabled();

		int cd_index = -1;
		if (changed_prefs.cdslots[0].inuse && changed_prefs.cdslots[0].type == SCSI_UNIT_DEFAULT && std::strlen(changed_prefs.cdslots[0].name) > 0)
			cd_index = qs_find_in_mru(lstMRUCDList, changed_prefs.cdslots[0].name);

		std::vector<const char*> cd_items;
		cd_items.push_back("<empty>");
		cd_items.reserve(qs_cd_display.size() + 1);
		for (auto &s : qs_cd_display)
			cd_items.push_back(s.c_str());

		int combo_index = cd_index + 1;
		ImGui::PushItemWidth(-1);
		if (ImGui::Combo("##QSCDFile", &combo_index, cd_items.data(), static_cast<int>(cd_items.size())))
		{
			if (combo_index == 0)
			{
				changed_prefs.cdslots[0].name[0] = 0;
				changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
			}
			else if (!qs_ignore_list_change && combo_index > 0 && combo_index <= static_cast<int>(qs_cd_display.size()))
			{
				const std::string &selected = qs_cd_display[combo_index - 1];
				if (selected.rfind("/dev/", 0) == 0)
				{
					std::strncpy(changed_prefs.cdslots[0].name, selected.c_str(), MAX_DPATH);
					changed_prefs.cdslots[0].inuse = true;
					changed_prefs.cdslots[0].type = SCSI_UNIT_IOCTL;
				}
				else
				{
					std::string element = get_full_path_from_disk_list(selected);
					if (element != changed_prefs.cdslots[0].name)
					{
						std::strncpy(changed_prefs.cdslots[0].name, element.c_str(), MAX_DPATH);
						DISK_history_add(changed_prefs.cdslots[0].name, -1, HISTORY_CD, 0);
						changed_prefs.cdslots[0].inuse = true;
						changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
						if (!last_loaded_config[0])
							set_last_active_config(element.c_str());
					}
				}
			}
		}
		ImGui::PopItemWidth();
		ImGui::PopID();
	}

	ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 2));

	bool qs_mode = amiberry_options.quickstart_start;
	if (ImGui::Checkbox("Start in Quickstart mode", &qs_mode))
		amiberry_options.quickstart_start = qs_mode;

	ImGui::SameLine();
	if (ImGui::Button("Set configuration", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
	{
		adjust_prefs();
	}

	ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 2));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 0.5f));

	ImGui::TextUnformatted("WHDLoad auto-config:");

	const float whd_button_width = BUTTON_WIDTH;
	const float whd_buttons_total_width = whd_button_width * 2 + ImGui::GetStyle().ItemSpacing.x;
	float whd_offset = ImGui::GetWindowContentRegionMax().x - whd_buttons_total_width;
	ImGui::SameLine(whd_offset);

	if (ImGui::Button("Eject##QSWHD", ImVec2(whd_button_width, 0)))
	{
		whdload_prefs.whdload_filename.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Select file##QSWHD", ImVec2(whd_button_width, 0)))
	{
		std::string tmp;
		if (!whdload_prefs.whdload_filename.empty())
			tmp = whdload_prefs.whdload_filename;
		else
			tmp = get_whdload_arch_path();
		OpenFileDialog("Select WHDLoad LHA file", ".lha,.lzh,.*", tmp);
		qs_pending_whd = true;
	}
	std::vector<const char*> whd_items;
	whd_items.push_back("<empty>");
	whd_items.reserve(qs_whd_display.size() + 1);
	for (auto &s : qs_whd_display)
		whd_items.push_back(s.c_str());

	int whd_index = -1;
	if (!whdload_prefs.whdload_filename.empty())
		whd_index = qs_find_in_mru(lstMRUWhdloadList, whdload_prefs.whdload_filename.c_str());

	int combo_whd_index = whd_index + 1;
	ImGui::PushItemWidth(-1);
	if (ImGui::Combo("##QSWHDList", &combo_whd_index, whd_items.data(), static_cast<int>(whd_items.size()))) {
		if (combo_whd_index == 0)
		{
			whdload_prefs.whdload_filename.clear();
		}
		else if (!qs_ignore_list_change && combo_whd_index > 0 && combo_whd_index <= static_cast<int>(qs_whd_display.size())) {
			std::string element = get_full_path_from_disk_list(qs_whd_display[combo_whd_index - 1]);
			if (element != whdload_prefs.whdload_filename)
			{
				whdload_prefs.whdload_filename = element;
				add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
			}
			whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
			set_last_active_config(whdload_prefs.whdload_filename.c_str());
		}
	}
	ImGui::PopItemWidth();

	// Only change the current prefs if we're not already emulating
	if (!emulating && !config_loaded)
		adjust_prefs();

	{
		std::string filePath;
		if (ConsumeFileDialogResult(filePath))
		{
			if (qs_pending_floppy_drive >= 0 && qs_pending_floppy_drive < 2)
			{
				int i = qs_pending_floppy_drive;
				if (!filePath.empty())
				{
					if (std::strncmp(changed_prefs.floppyslots[i].df, filePath.c_str(), MAX_DPATH) != 0)
					{
						std::strncpy(changed_prefs.floppyslots[i].df, filePath.c_str(), MAX_DPATH);
						disk_insert(i, filePath.c_str());
						add_file_to_mru_list(lstMRUDiskList, filePath);
						if (!last_loaded_config[0])
							set_last_active_config(filePath.c_str());
					}
				}
			}
			else if (qs_pending_cd)
			{
				if (!filePath.empty())
				{
					if (std::strncmp(changed_prefs.cdslots[0].name, filePath.c_str(), MAX_DPATH) != 0)
					{
						std::strncpy(changed_prefs.cdslots[0].name, filePath.c_str(), MAX_DPATH);
						changed_prefs.cdslots[0].inuse = true;
						changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
						add_file_to_mru_list(lstMRUCDList, filePath);
						if (!last_loaded_config[0])
							set_last_active_config(filePath.c_str());
					}
				}
			}
			else if (qs_pending_whd)
			{
				if (!filePath.empty())
				{
					whdload_prefs.whdload_filename = filePath;
					add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
					whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
					set_last_active_config(whdload_prefs.whdload_filename.c_str());
				}
			}

			qs_pending_floppy_drive = -1;
			qs_pending_cd = false;
			qs_pending_whd = false;
		}
	}
}
