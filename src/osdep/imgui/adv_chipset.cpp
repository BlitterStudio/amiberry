#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "custom.h"

// Helper for bitfield checkboxes
static void CheckboxFlags(const char* label, int* flags, int bit_value)
{
	bool v = (*flags & bit_value) != 0;
	if (ImGui::Checkbox(label, &v))
	{
		if (v)
			*flags |= bit_value;
		else
			*flags &= ~bit_value;
	}
}

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
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Indent(5.0f); // Global indentation matching CPU panel

	ImGui::Text("Compatible Settings");
	ImGui::SameLine();
	bool compatible = changed_prefs.cs_compatible != 0;
	if (ImGui::Checkbox("##CompatibleSettings", &compatible))
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
		ImGui::RadioButton("None", &changed_prefs.cs_rtc, 0); ImGui::SameLine();
		ImGui::RadioButton("MSM6242B", &changed_prefs.cs_rtc, 1); ImGui::SameLine();
		ImGui::RadioButton("RF5C01A", &changed_prefs.cs_rtc, 2); ImGui::SameLine();
		ImGui::RadioButton("A2000 MSM6242B", &changed_prefs.cs_rtc, 3);
		
		BeginDisableableGroup(controls_disabled);
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Adj:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(60);
		ImGui::InputInt("##RTCAdjust", &changed_prefs.cs_rtc_adjust, 1, 10);
		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Battery Backed Up Real Time Clock");

	BeginGroupBox("CIA-A TOD Clock Source");
	{
		BeginDisableableGroup(controls_disabled);
		ImGui::RadioButton("Vertical Sync", &changed_prefs.cs_ciaatod, 0); ImGui::SameLine();
		ImGui::RadioButton("Power Supply 50Hz", &changed_prefs.cs_ciaatod, 1); ImGui::SameLine();
		ImGui::RadioButton("Power Supply 60Hz", &changed_prefs.cs_ciaatod, 2);
		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("CIA-A TOD Clock Source");

	BeginGroupBox("Chipset Features");
	{
		BeginDisableableGroup(controls_disabled);
		
		if (ImGui::BeginTable("FeaturesTable", 3))
		{
			ImGui::TableNextColumn();
			ImGui::Checkbox("CIA ROM Overlay", &changed_prefs.cs_ciaoverlay);
			ImGui::TableNextColumn();
			ImGui::Checkbox("A1000 Boot RAM/ROM", &changed_prefs.cs_a1000ram);
			ImGui::TableNextColumn();
			ImGui::Checkbox("DF0: ID Hardware", &changed_prefs.cs_df0idhw);

			ImGui::TableNextColumn();
			ImGui::Checkbox("CD32 CD", &changed_prefs.cs_cd32cd);
			ImGui::TableNextColumn();
			ImGui::Checkbox("CD32 C2P", &changed_prefs.cs_cd32c2p);
			ImGui::TableNextColumn();
			ImGui::Checkbox("CD32 NVRAM", &changed_prefs.cs_cd32nvram);

			ImGui::TableNextColumn();
			ImGui::Checkbox("CDTV CD", &changed_prefs.cs_cdtvcd);
			ImGui::TableNextColumn();
			ImGui::Checkbox("CDTV SRAM", &changed_prefs.cs_cdtvram);
			ImGui::TableNextColumn();
			ImGui::Checkbox("CDTV-CR", &changed_prefs.cs_cdtvcr);

			ImGui::TableNextColumn();
			bool ide_a600 = (changed_prefs.cs_ide & 1) != 0;
			if (ImGui::Checkbox("A600/A1200 IDE", &ide_a600)) {
				if (ide_a600) changed_prefs.cs_ide = 1; else if (changed_prefs.cs_ide == 1) changed_prefs.cs_ide = 0;
			}
			ImGui::TableNextColumn();
			bool ide_a4000 = (changed_prefs.cs_ide & 2) != 0;
			if (ImGui::Checkbox("A4000/A4000T IDE", &ide_a4000)) {
				if (ide_a4000) changed_prefs.cs_ide = 2; else if (changed_prefs.cs_ide == 2) changed_prefs.cs_ide = 0;
			}
			ImGui::TableNextColumn(); 
			bool pcmcia = changed_prefs.cs_pcmcia != 0;
			if (ImGui::Checkbox("PCMCIA", &pcmcia)) changed_prefs.cs_pcmcia = pcmcia;

			ImGui::TableNextColumn();
			ImGui::Checkbox("ROM Mirror (E0)", &changed_prefs.cs_ksmirror_e0);
			ImGui::TableNextColumn();
			ImGui::Checkbox("ROM Mirror (A8)", &changed_prefs.cs_ksmirror_a8);
			ImGui::TableNextColumn();
			ImGui::Checkbox("Composite color burst", &changed_prefs.cs_color_burst);

			ImGui::TableNextColumn();
			ImGui::Checkbox("KB Reset Warning", &changed_prefs.cs_resetwarning);
			ImGui::TableNextColumn();
			bool z3auto = changed_prefs.cs_z3autoconfig != 0;
			if (ImGui::Checkbox("Z3 Autoconfig", &z3auto)) changed_prefs.cs_z3autoconfig = z3auto;
			ImGui::TableNextColumn();
			bool cia_new = changed_prefs.cs_ciatype[0];
			if (ImGui::Checkbox("CIA 391078-01", &cia_new)) {
				changed_prefs.cs_ciatype[0] = changed_prefs.cs_ciatype[1] = cia_new;
			}

			ImGui::TableNextColumn();
			ImGui::Checkbox("CIA TOD bug", &changed_prefs.cs_ciatodbug);
			ImGui::TableNextColumn();
			ImGui::Checkbox("Custom register byte write bug", &changed_prefs.cs_bytecustomwritebug);
			ImGui::TableNextColumn();
			ImGui::Checkbox("Power up memory pattern", &changed_prefs.cs_memorypatternfill);

			ImGui::TableNextColumn();
			bool chip1m = changed_prefs.cs_1mchipjumper || changed_prefs.chipmem.size >= 0x100000;
			bool disable_1m = changed_prefs.chipmem.size >= 0x100000;
			ImGui::BeginDisabled(disable_1m);
			if (ImGui::Checkbox("1M Chip / 0.5M+0.5M", &chip1m)) changed_prefs.cs_1mchipjumper = chip1m;
			ImGui::EndDisabled();

			ImGui::TableNextColumn();
			ImGui::Checkbox("KS ROM has Chip RAM speed", &changed_prefs.cs_romisslow);
			
			ImGui::TableNextColumn();
			ImGui::Checkbox("Toshiba Gary", &changed_prefs.cs_toshibagary);

			ImGui::EndTable();
		}
		
		ImGui::Spacing();
		
		// Combos with Labels on Left
		const char* unmapped_items[] = { "Floating", "Zero", "FF" };
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Unmapped address space:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
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

		ImGui::SameLine(0, 20); // Keep reasonable spacing between separate control groups
		
		const char* ciasync_items[] = { "Autoselect", "68000", "Gayle", "68000 Alternate" };
		ImGui::AlignTextToFramePadding();
		ImGui::Text("CIA E-Clock Sync:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
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

		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Chipset Features");

	BeginGroupBox("Internal SCSI Hardware");
	{
		BeginDisableableGroup(controls_disabled);
		
		// Reduce spacing for checkbox groups if desired, but default is usually fine here
		bool scsi_a3000 = (changed_prefs.cs_mbdmac & 1) != 0;
		if (ImGui::Checkbox("A3000 WD33C93 SCSI", &scsi_a3000))
		{
			if (scsi_a3000) changed_prefs.cs_mbdmac |= 1; else changed_prefs.cs_mbdmac &= ~1;
		}
		ImGui::SameLine(0, 50);
		bool scsi_a4000 = (changed_prefs.cs_mbdmac & 2) != 0;
		if (ImGui::Checkbox("A4000T NCR53C710 SCSI", &scsi_a4000))
		{
			if (scsi_a4000) changed_prefs.cs_mbdmac |= 2; else changed_prefs.cs_mbdmac &= ~2;
		}

		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Internal SCSI Hardware");

	BeginGroupBox("Chipset Revision");
	{
		BeginDisableableGroup(controls_disabled);

		// Optimized Table: Fixed width for First column to prevent wasted space
		if (ImGui::BeginTable("RevTable", 2)) 
		{
			ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed, 210.0f); // Give just enough space for Checkbox+Input
			ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);       // Rest of space for Agnus/Denise
			// Note: We don't call TableHeadersRow() so header height is 0

			// -- Column 1: Ramsey & Gary --
			ImGui::TableNextColumn();
			
			// Ramsey
			{
				bool ramsey_en = changed_prefs.cs_ramseyrev >= 0;
				if (ImGui::Checkbox("Ramsey revision:", &ramsey_en)) changed_prefs.cs_ramseyrev = ramsey_en ? 0x0f : -1;
				ImGui::SameLine(0, 5); 
				ImGui::BeginDisabled(!ramsey_en);
				ImGui::SetNextItemWidth(40);
				int v = changed_prefs.cs_ramseyrev;
				if (ImGui::InputInt("##RamseyRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_ramseyrev = v;
				ImGui::EndDisabled();
			}
			
			// Fat Gary
			{
				bool gary_en = changed_prefs.cs_fatgaryrev >= 0;
				if (ImGui::Checkbox("Fat Gary revision:", &gary_en)) changed_prefs.cs_fatgaryrev = gary_en ? 0x00 : -1;
				ImGui::SameLine(0, 5);
				ImGui::BeginDisabled(!gary_en);
				ImGui::SetNextItemWidth(40);
				int v = changed_prefs.cs_fatgaryrev;
				if (ImGui::InputInt("##GaryRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_fatgaryrev = v;
				ImGui::EndDisabled();
			}

			// -- Column 2: Agnus & Denise --
			ImGui::TableNextColumn();
			
			// Agnus
			{
				bool agnus_en = changed_prefs.cs_agnusrev >= 0;
				if (ImGui::Checkbox("Agnus/Alice model:", &agnus_en)) changed_prefs.cs_agnusrev = agnus_en ? 0 : -1;
				ImGui::SameLine(0, 5);
				ImGui::BeginDisabled(!agnus_en);
				ImGui::SetNextItemWidth(40);
				int v = changed_prefs.cs_agnusrev;
				if (ImGui::InputInt("##AgnusRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_agnusrev = v;
				ImGui::EndDisabled();

				ImGui::SameLine(0, 5);
				const char* agnus_models[] = { "Auto", "Velvet", "A1000" };
				ImGui::SetNextItemWidth(60);
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
				
				ImGui::SameLine(0, 5);
				const char* agnus_sizes[] = { "Auto", "512k", "1M", "2M" };
				ImGui::SetNextItemWidth(60);
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
			}

			// Denise
			{
				bool denise_en = changed_prefs.cs_deniserev >= 0;
				if (ImGui::Checkbox("Denise/Lisa model:", &denise_en)) changed_prefs.cs_deniserev = denise_en ? 0 : -1;
				ImGui::SameLine(0, 5);
				ImGui::BeginDisabled(!denise_en);
				ImGui::SetNextItemWidth(40);
				int v = changed_prefs.cs_deniserev;
				if (ImGui::InputInt("##DeniseRev", &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) changed_prefs.cs_deniserev = v;
				ImGui::EndDisabled();

				ImGui::SameLine(0, 5);
				const char* denise_models[] = { "Auto", "Velvet", "A1000 No-EHB", "A1000" };
				ImGui::SetNextItemWidth(100);
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
			}

			ImGui::EndTable();
		}

		EndDisableableGroup(controls_disabled);
	}
	EndGroupBox("Chipset Revision");

	ImGui::Unindent(5.0f);
}
