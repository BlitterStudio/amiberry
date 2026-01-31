#include "sysdeps.h"
#include "options.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"

// WinUAE-like helper to disable controls
static void BeginDisableableGroup(bool disabled)
{
	if (disabled) ImGui::BeginDisabled();
}

static void EndDisableableGroup(bool disabled)
{
	if (disabled) ImGui::EndDisabled();
}

void render_panel_adv_chipset()
{
	// Global padding for the whole panel
	ImGui::Indent(4.0f);

	ImGui::Text("Compatible Settings");
	ImGui::SameLine();
	bool compatible = changed_prefs.cs_compatible != 0;
	if (AmigaCheckbox("##CompatibleSettings", &compatible))
	{
		if (compatible) {
			if (!changed_prefs.cs_compatible)
				changed_prefs.cs_compatible = CP_GENERIC;
		} else {
			changed_prefs.cs_compatible = 0;
		}
		built_in_chipset_prefs(&changed_prefs);
	}
	
	// Most controls are disabled if Compatible Settings is ON
	bool controls_disabled = compatible;

	BeginGroupBox("Battery Backed Up Real Time Clock");
	{
		AmigaRadioButton("None", &changed_prefs.cs_rtc, 0); ImGui::SameLine();
		AmigaRadioButton("MSM6242B", &changed_prefs.cs_rtc, 1); ImGui::SameLine();
		AmigaRadioButton("RF5C01A", &changed_prefs.cs_rtc, 2); ImGui::SameLine();
		AmigaRadioButton("A2000 MSM6242B", &changed_prefs.cs_rtc, 3);
		
		BeginDisableableGroup(controls_disabled);
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Adj:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH);
		ImGui::InputInt("##RTCAdjust", &changed_prefs.cs_rtc_adjust, 1, 10);
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Battery Backed Up Real Time Clock");

	BeginGroupBox("CIA-A TOD Clock Source");
	{
		BeginDisableableGroup(controls_disabled);
		AmigaRadioButton("Vertical Sync", &changed_prefs.cs_ciaatod, 0); ImGui::SameLine();
		AmigaRadioButton("Power Supply 50Hz", &changed_prefs.cs_ciaatod, 1); ImGui::SameLine();
		AmigaRadioButton("Power Supply 60Hz", &changed_prefs.cs_ciaatod, 2);
		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("CIA-A TOD Clock Source");

	BeginGroupBox("Chipset Features");
	{
		BeginDisableableGroup(controls_disabled);
		
		if (ImGui::BeginTable("FeaturesTable", 3))
		{
			ImGui::TableNextColumn();
			AmigaCheckbox("CIA ROM Overlay", &changed_prefs.cs_ciaoverlay);
			ImGui::TableNextColumn();
			AmigaCheckbox("A1000 Boot RAM/ROM", &changed_prefs.cs_a1000ram);
			ImGui::TableNextColumn();
			AmigaCheckbox("DF0: ID Hardware", &changed_prefs.cs_df0idhw);

			ImGui::TableNextColumn();
			AmigaCheckbox("CD32 CD", &changed_prefs.cs_cd32cd);
			ImGui::TableNextColumn();
			AmigaCheckbox("CD32 C2P", &changed_prefs.cs_cd32c2p);
			ImGui::TableNextColumn();
			AmigaCheckbox("CD32 NVRAM", &changed_prefs.cs_cd32nvram);

			ImGui::TableNextColumn();
			AmigaCheckbox("CDTV CD", &changed_prefs.cs_cdtvcd);
			ImGui::TableNextColumn();
			AmigaCheckbox("CDTV SRAM", &changed_prefs.cs_cdtvram);
			ImGui::TableNextColumn();
			AmigaCheckbox("CDTV-CR", &changed_prefs.cs_cdtvcr);

			ImGui::TableNextColumn();
			bool ide_a600 = (changed_prefs.cs_ide & 1) != 0;
			if (AmigaCheckbox("A600/A1200 IDE", &ide_a600)) {
				if (ide_a600) changed_prefs.cs_ide = 1; else if (changed_prefs.cs_ide == 1) changed_prefs.cs_ide = 0;
			}
			ImGui::TableNextColumn();
			bool ide_a4000 = (changed_prefs.cs_ide & 2) != 0;
			if (AmigaCheckbox("A4000/A4000T IDE", &ide_a4000)) {
				if (ide_a4000) changed_prefs.cs_ide = 2; else if (changed_prefs.cs_ide == 2) changed_prefs.cs_ide = 0;
			}
			ImGui::TableNextColumn(); 
			bool pcmcia = changed_prefs.cs_pcmcia != 0;
			if (AmigaCheckbox("PCMCIA", &pcmcia)) changed_prefs.cs_pcmcia = pcmcia;

			ImGui::TableNextColumn();
			AmigaCheckbox("ROM Mirror (E0)", &changed_prefs.cs_ksmirror_e0);
			ImGui::TableNextColumn();
			AmigaCheckbox("ROM Mirror (A8)", &changed_prefs.cs_ksmirror_a8);
			ImGui::TableNextColumn();
			AmigaCheckbox("Composite color burst", &changed_prefs.cs_color_burst);

			ImGui::TableNextColumn();
			AmigaCheckbox("KB Reset Warning", &changed_prefs.cs_resetwarning);
			ImGui::TableNextColumn();
			bool z3auto = changed_prefs.cs_z3autoconfig != 0;
			if (AmigaCheckbox("Z3 Autoconfig", &z3auto)) changed_prefs.cs_z3autoconfig = z3auto;
			ImGui::TableNextColumn();
			bool cia_new = changed_prefs.cs_ciatype[0];
			if (AmigaCheckbox("CIA 391078-01", &cia_new)) {
				changed_prefs.cs_ciatype[0] = changed_prefs.cs_ciatype[1] = cia_new;
			}

			ImGui::TableNextColumn();
			AmigaCheckbox("CIA TOD bug", &changed_prefs.cs_ciatodbug);
			ImGui::TableNextColumn();
			AmigaCheckbox("Custom register byte write bug", &changed_prefs.cs_bytecustomwritebug);
			ImGui::TableNextColumn();
			AmigaCheckbox("Power up memory pattern", &changed_prefs.cs_memorypatternfill);

			ImGui::TableNextColumn();
			bool chip1m = changed_prefs.cs_1mchipjumper || changed_prefs.chipmem.size >= 0x100000;
			bool disable_1m = changed_prefs.chipmem.size >= 0x100000;
			ImGui::BeginDisabled(disable_1m);
			if (AmigaCheckbox("1M Chip / 0.5M+0.5M", &chip1m)) changed_prefs.cs_1mchipjumper = chip1m;
			ImGui::EndDisabled();

			ImGui::TableNextColumn();
			AmigaCheckbox("KS ROM has Chip RAM speed", &changed_prefs.cs_romisslow);
			
			ImGui::TableNextColumn();
			AmigaCheckbox("Toshiba Gary", &changed_prefs.cs_toshibagary);

			ImGui::EndTable();
		}
		
		ImGui::Spacing();
		
		// Combos with Labels on Left
		const char* unmapped_items[] = { "Floating", "Zero", "FF" };
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Unmapped address space:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH);
		if (ImGui::BeginCombo("##Unmapped", unmapped_items[changed_prefs.cs_unmapped_space])) {
			for (int n = 0; n < IM_ARRAYSIZE(unmapped_items); n++) {
				const bool is_selected = (changed_prefs.cs_unmapped_space == n);
				if (is_selected)
					ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
				if (ImGui::Selectable(unmapped_items[n], is_selected)) {
					changed_prefs.cs_unmapped_space = n;
				}
				if (is_selected) {
					ImGui::PopStyleColor();
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

		ImGui::SameLine(0, 20); // Keep reasonable spacing between separate control groups
		
		const char* ciasync_items[] = { "Autoselect", "68000", "Gayle", "68000 Alternate" };
		ImGui::AlignTextToFramePadding();
		ImGui::Text("CIA E-Clock Sync:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH);
		if (ImGui::BeginCombo("##CiaSync", ciasync_items[changed_prefs.cs_eclocksync])) {
			for (int n = 0; n < IM_ARRAYSIZE(ciasync_items); n++) {
				const bool is_selected = (changed_prefs.cs_eclocksync == n);
				if (is_selected)
					ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
				if (ImGui::Selectable(ciasync_items[n], is_selected)) {
					changed_prefs.cs_eclocksync = n;
				}
				if (is_selected) {
					ImGui::PopStyleColor();
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Chipset Features");

	BeginGroupBox("Internal SCSI Hardware");
	{
		BeginDisableableGroup(controls_disabled);
		
		// Reduce spacing for checkbox groups if desired, but default is usually fine here
		bool scsi_a3000 = (changed_prefs.cs_mbdmac & 1) != 0;
		if (AmigaCheckbox("A3000 WD33C93 SCSI", &scsi_a3000))
		{
			if (scsi_a3000) changed_prefs.cs_mbdmac |= 1; else changed_prefs.cs_mbdmac &= ~1;
		}
		ImGui::SameLine(0, 50);
		bool scsi_a4000 = (changed_prefs.cs_mbdmac & 2) != 0;
		if (AmigaCheckbox("A4000T NCR53C710 SCSI", &scsi_a4000))
		{
			if (scsi_a4000) changed_prefs.cs_mbdmac |= 2; else changed_prefs.cs_mbdmac &= ~2;
		}

		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Internal SCSI Hardware");

	BeginGroupBox("Chipset Revision");
	{
		BeginDisableableGroup(controls_disabled);

		// Ramsey
		{
			bool ramsey_en = changed_prefs.cs_ramseyrev >= 0;
			if (AmigaCheckbox("Ramsey revision:", &ramsey_en)) changed_prefs.cs_ramseyrev = ramsey_en ? 0x0f : -1;
			ImGui::SameLine();
			ImGui::BeginDisabled(!ramsey_en);
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			int v = changed_prefs.cs_ramseyrev;
			if (ImGui::InputInt("##RamseyRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_ramseyrev = v;
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
			ImGui::EndDisabled();
		}

		// Fat Gary
		{
			bool gary_en = changed_prefs.cs_fatgaryrev >= 0;
			if (AmigaCheckbox("Fat Gary revision:", &gary_en)) changed_prefs.cs_fatgaryrev = gary_en ? 0x00 : -1;
			ImGui::SameLine();
			ImGui::BeginDisabled(!gary_en);
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			int v = changed_prefs.cs_fatgaryrev;
			if (ImGui::InputInt("##GaryRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_fatgaryrev = v;
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
			ImGui::EndDisabled();
		}

		// Agnus
		{
			bool agnus_en = changed_prefs.cs_agnusrev >= 0;
			if (AmigaCheckbox("Agnus/Alice model:", &agnus_en)) changed_prefs.cs_agnusrev = agnus_en ? 0 : -1;
			ImGui::SameLine();
			ImGui::BeginDisabled(!agnus_en);
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			int v = changed_prefs.cs_agnusrev;
			if (ImGui::InputInt("##AgnusRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_agnusrev = v;
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
			ImGui::EndDisabled();

			ImGui::SameLine();
			const char* agnus_models[] = { "Auto", "Velvet", "A1000" };
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			if (ImGui::BeginCombo("##AgnusModel", agnus_models[changed_prefs.cs_agnusmodel])) {
				for (int n = 0; n < IM_ARRAYSIZE(agnus_models); n++) {
					const bool is_selected = (changed_prefs.cs_agnusmodel == n);
					if (is_selected)
						ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
					if (ImGui::Selectable(agnus_models[n], is_selected)) {
						changed_prefs.cs_agnusmodel = n;
					}
					if (is_selected) {
						ImGui::PopStyleColor();
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

			ImGui::SameLine();
			const char* agnus_sizes[] = { "Auto", "512k", "1M", "2M" };
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			if (ImGui::BeginCombo("##AgnusSize", agnus_sizes[changed_prefs.cs_agnussize])) {
				for (int n = 0; n < IM_ARRAYSIZE(agnus_sizes); n++) {
					const bool is_selected = (changed_prefs.cs_agnussize == n);
					if (is_selected)
						ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
					if (ImGui::Selectable(agnus_sizes[n], is_selected)) {
						changed_prefs.cs_agnussize = n;
					}
					if (is_selected) {
						ImGui::PopStyleColor();
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
		}

		// Denise
		{
			bool denise_en = changed_prefs.cs_deniserev >= 0;
			if (AmigaCheckbox("Denise/Lisa model:", &denise_en)) changed_prefs.cs_deniserev = denise_en ? 0 : -1;
			ImGui::SameLine();
			ImGui::BeginDisabled(!denise_en);
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			int v = changed_prefs.cs_deniserev;
			if (ImGui::InputInt("##DeniseRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_deniserev = v;
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
			ImGui::EndDisabled();

			ImGui::SameLine();
			const char* denise_models[] = { "Auto", "Velvet", "A1000 No-EHB", "A1000" };
			ImGui::SetNextItemWidth(BUTTON_WIDTH);
			if (ImGui::BeginCombo("##DeniseModel", denise_models[changed_prefs.cs_denisemodel])) {
				for (int n = 0; n < IM_ARRAYSIZE(denise_models); n++) {
					const bool is_selected = (changed_prefs.cs_denisemodel == n);
					if (is_selected)
						ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
					if (ImGui::Selectable(denise_models[n], is_selected)) {
						changed_prefs.cs_denisemodel = n;
					}
					if (is_selected) {
						ImGui::PopStyleColor();
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
		}
		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Chipset Revision");
}
