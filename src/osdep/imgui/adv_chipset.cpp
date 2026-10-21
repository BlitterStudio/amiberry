#include "sysdeps.h"
#include "options.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "settings/settings_definitions.h"

#include <cstdio>

namespace settings = amiberry_settings;

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules and the dependent changes a new value implies.
struct adv_chipset_options {
    const settings::definition& chipset_compatible = settings::require("chipset_compatible");
    const settings::definition& rtc = settings::require("rtc");
    const settings::definition& rtc_adjust = settings::require("chipset_rtc_adjust");
    const settings::definition& ciaatod = settings::require("ciaatod");
    const settings::definition& cia_overlay = settings::require("cia_overlay");
    const settings::definition& a1000ram = settings::require("a1000ram");
    const settings::definition& df0idhw = settings::require("df0idhw");
    const settings::definition& cd32cd = settings::require("cd32cd");
    const settings::definition& cd32c2p = settings::require("cd32c2p");
    const settings::definition& cd32nvram = settings::require("cd32nvram");
    const settings::definition& cdtvcd = settings::require("cdtvcd");
    const settings::definition& cdtvram = settings::require("cdtvram");
    const settings::definition& cdtvcr = settings::require("cdtv-cr");
    const settings::definition& ide = settings::require("ide");
    const settings::definition& pcmcia = settings::require("pcmcia");
    const settings::definition& ksmirror_e0 = settings::require("ksmirror_e0");
    const settings::definition& ksmirror_a8 = settings::require("ksmirror_a8");
    const settings::definition& color_burst = settings::require("color_burst");
    const settings::definition& resetwarning = settings::require("resetwarning");
    const settings::definition& z3_autoconfig = settings::require("z3_autoconfig");
    const settings::definition& ciaa_type = settings::require("ciaa_type");
    const settings::definition& cia_todbug = settings::require("cia_todbug");
    const settings::definition& bytecustomwritebug = settings::require("cs_bytecustomwritebug");
    const settings::definition& memory_pattern = settings::require("memory_pattern");
    const settings::definition& chip1m = settings::require("1mchipjumper");
    const settings::definition& rom_is_slow = settings::require("rom_is_slow");
    const settings::definition& toshiba_gary = settings::require("toshiba_gary");
    const settings::definition& unmapped_address_space = settings::require("unmapped_address_space");
    const settings::definition& eclocksync = settings::require("eclocksync");
    const settings::definition& scsi_a3000 = settings::require("scsi_a3000");
    const settings::definition& scsi_a4000t = settings::require("scsi_a4000t");
    const settings::definition& ramsey = settings::require("ramsey");
    const settings::definition& fatgary = settings::require("fatgary");
    const settings::definition& agnusrev = settings::require("agnusrev");
    const settings::definition& agnusmodel = settings::require("agnusmodel");
    const settings::definition& agnussize = settings::require("agnussize");
    const settings::definition& deniserev = settings::require("deniserev");
    const settings::definition& denisemodel = settings::require("denisemodel");
};

const adv_chipset_options& options()
{
    static const adv_chipset_options o;
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

// A checkbox for a BOOL option, greyed out while the option is unavailable.
void option_checkbox(const char* label, const settings::definition& d, const settings::context& ctx)
{
    ImGui::BeginDisabled(!available(d, ctx));
    bool checked = settings::get_bool(d, changed_prefs);
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked);
    ImGui::EndDisabled();
}

// A checkbox that stores one value of a CHOICE option (off_value when
// cleared), greyed out while the option, or the value a click would store,
// is unavailable.
void value_checkbox(const char* label, const settings::definition& d, const settings::context& ctx,
                    const char* on_value, const char* off_value)
{
    bool checked = has_value(d, on_value);
    ImGui::BeginDisabled(!available(d, ctx)
        || !settings::is_choice_available(d, changed_prefs, ctx, checked ? off_value : on_value));
    if (AmigaCheckbox(label, &checked))
        settings::set(d, changed_prefs, ctx, checked ? on_value : off_value);
    ImGui::EndDisabled();
}

// A radio button for one choice of a CHOICE option, greyed out when the
// configuration checks would not keep that value.
void option_radio(const char* label, const settings::definition& d, const settings::context& ctx,
                  const char* v)
{
    ImGui::BeginDisabled(!settings::is_choice_available(d, changed_prefs, ctx, v));
    if (AmigaRadioButton(label, has_value(d, v)))
        settings::set(d, changed_prefs, ctx, v);
    ImGui::EndDisabled();
}

// A combo over a CHOICE option's values; refused choices are greyed out
// instead of silently snapping back to what the configuration checks keep.
void option_combo(const char* label, const settings::definition& d, const settings::context& ctx,
                  float width)
{
    const int current = choice_index_of(d);
    ImGui::BeginDisabled(!available(d, ctx));
    ImGui::SetNextItemWidth(width);
    if (ImGui::BeginCombo(label, d.descriptor.choices[current].label)) {
        for (int n = 0; n < static_cast<int>(d.descriptor.choice_count); n++) {
            const bool selectable = settings::is_choice_available(d, changed_prefs, ctx,
                d.descriptor.choices[n].value);
            const ImGuiSelectableFlags flags = selectable
                ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_Disabled;
            const bool is_selected = (current == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(d.descriptor.choices[n].label, is_selected, flags))
                settings::set(d, changed_prefs, ctx, d.descriptor.choices[n].value);
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
    ImGui::EndDisabled();
}

// The hex revision inputs of the Chipset Revision group: an enable checkbox
// (on writes the power-up default revision, off the -1 override marker)
// with the revision input sub-disabled while off.
void revision_control(const char* label, const char* id, const settings::definition& d,
                      const settings::context& ctx, int enable_rev,
                      const char* help_enable, const char* help_rev)
{
    bool enabled = settings::get_number(d, changed_prefs) >= 0;
    // The A4000 IDE controller keeps Gary and Ramsey present; disabling
    // them (-1) is refused then.
    const bool can_disable = !d.accepts || d.accepts(changed_prefs, settings::value{"-1", -1});
    ImGui::BeginDisabled(!available(d, ctx) || (enabled && !can_disable));
    if (AmigaCheckbox(label, &enabled))
        settings::set_number(d, changed_prefs, ctx, enabled ? enable_rev : -1);
    ImGui::EndDisabled();
    ShowHelpMarker(help_enable);

    ImGui::SameLine();
    ImGui::BeginDisabled(!(enabled && available(d, ctx)));
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    int v = static_cast<int>(settings::get_number(d, changed_prefs));
    if (ImGui::InputInt(id, &v, 0, 0, ImGuiInputTextFlags_CharsHexadecimal))
        settings::set_number(d, changed_prefs, ctx, v);
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    ImGui::EndDisabled();
    ShowHelpMarker(help_rev);
}

} // namespace

void render_panel_adv_chipset()
{
    const adv_chipset_options& o = options();
    const settings::context ctx{emulating != 0};

    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    // Compatible Settings maps to the chipset extra: every option below is
    // unavailable (greyed out) while an extra is active, because selecting
    // it rewrites the advanced chipset options.
    ImGui::Text("Compatible Settings");
    ImGui::SameLine();
    bool compatible = !has_value(o.chipset_compatible, "-");
    if (AmigaCheckbox("##CompatibleSettings", &compatible))
        settings::set(o.chipset_compatible, changed_prefs, ctx, compatible ? "Generic" : "-");
    ShowHelpMarker("Force safe defaults for all chipset settings to maximize compatibility");

    BeginGroupBox("Battery Backed Up Real Time Clock");
    {
        option_radio("None", o.rtc, ctx, "none");
        ShowHelpMarker("No real-time clock");
        ImGui::SameLine();
        option_radio("MSM6242B", o.rtc, ctx, "MSM6242B");
        ShowHelpMarker("MSM6242B RTC chip (used in A2000)");
        ImGui::SameLine();
        option_radio("RF5C01A", o.rtc, ctx, "RP5C01A");
        ShowHelpMarker("RF5C01A RTC chip (used in A3000/A4000)");
        ImGui::SameLine();
        option_radio("A2000 MSM6242B", o.rtc, ctx, "MSM6242B_A2000");
        ShowHelpMarker("MSM6242B with A2000-specific behavior");

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Adj:");
        ImGui::SameLine();
        ImGui::BeginDisabled(!available(o.rtc_adjust, ctx));
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        int adjust = static_cast<int>(settings::get_number(o.rtc_adjust, changed_prefs));
        if (ImGui::InputInt("##RTCAdjust", &adjust, 1, 10))
            settings::set_number(o.rtc_adjust, changed_prefs, ctx, adjust);
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        ImGui::EndDisabled();
        ShowHelpMarker("RTC time adjustment in seconds");
        ImGui::Spacing();
    }
    EndGroupBox("Battery Backed Up Real Time Clock");

    BeginGroupBox("CIA-A TOD Clock Source");
    {
        option_radio("Vertical Sync", o.ciaatod, ctx, "vblank");
        ShowHelpMarker("CIA Time of Day clock from vertical blank interrupt");
        ImGui::SameLine();
        option_radio("Power Supply 50Hz", o.ciaatod, ctx, "50hz");
        ShowHelpMarker("CIA TOD clock from 50Hz power line frequency");
        ImGui::SameLine();
        option_radio("Power Supply 60Hz", o.ciaatod, ctx, "60hz");
        ShowHelpMarker("CIA TOD clock from 60Hz power line frequency");
        ImGui::Spacing();
    }
    EndGroupBox("CIA-A TOD Clock Source");

    BeginGroupBox("Chipset Features");
    {
        // Use SizingFixedFit so each column is sized to its widest content.
        // Without this, the table stretches to the full window width, which
        // creates huge horizontal gaps between the 3 checkbox columns when the
        // GUI window is resized wider than the default (see issue #1970).
        if (ImGui::BeginTable("FeaturesTable", 3, ImGuiTableFlags_SizingFixedFit))
        {
            // Add some spacing between columns for readability
            ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableNextColumn();
            option_checkbox("CIA ROM Overlay", o.cia_overlay, ctx);
            ShowHelpMarker("Enable CIA-based Kickstart ROM overlay mechanism");
            ImGui::TableNextColumn();
            option_checkbox("A1000 Boot RAM/ROM", o.a1000ram, ctx);
            ShowHelpMarker("Enable A1000-style boot RAM at $C00000");
            ImGui::TableNextColumn();
            option_checkbox("DF0: ID Hardware", o.df0idhw, ctx);
            ShowHelpMarker("Enable DF0 drive ID hardware detection");

            ImGui::TableNextColumn();
            option_checkbox("CD32 CD", o.cd32cd, ctx);
            ShowHelpMarker("Enable CD32 CD-ROM drive");
            ImGui::TableNextColumn();
            option_checkbox("CD32 C2P", o.cd32c2p, ctx);
            ShowHelpMarker("Enable CD32 chunky-to-planar hardware acceleration");
            ImGui::TableNextColumn();
            option_checkbox("CD32 NVRAM", o.cd32nvram, ctx);
            ShowHelpMarker("Enable CD32 non-volatile RAM for saved data");

            ImGui::TableNextColumn();
            option_checkbox("CDTV CD", o.cdtvcd, ctx);
            ShowHelpMarker("Enable CDTV CD-ROM drive");
            ImGui::TableNextColumn();
            option_checkbox("CDTV SRAM", o.cdtvram, ctx);
            ShowHelpMarker("Enable CDTV static RAM for saved data");
            ImGui::TableNextColumn();
            option_checkbox("CDTV-CR", o.cdtvcr, ctx);
            ShowHelpMarker("Enable CDTV card reader");

            // The IDE controller is one configuration key (ide=) with the two
            // hardware variants as values, so the checkboxes exclude each
            // other exactly like the configuration they store.
            ImGui::TableNextColumn();
            value_checkbox("A600/A1200 IDE", o.ide, ctx, "a600/a1200", "none");
            ShowHelpMarker("Enable A600/A1200 Gayle IDE controller");
            ImGui::TableNextColumn();
            value_checkbox("A4000/A4000T IDE", o.ide, ctx, "a4000", "none");
            ShowHelpMarker("Enable A4000/A4000T IDE controller");
            ImGui::TableNextColumn();
            option_checkbox("PCMCIA", o.pcmcia, ctx);
            ShowHelpMarker("Enable PCMCIA slot (A600/A1200)");

            ImGui::TableNextColumn();
            option_checkbox("ROM Mirror (E0)", o.ksmirror_e0, ctx);
            ShowHelpMarker("Enable Kickstart ROM mirror at $E00000");
            ImGui::TableNextColumn();
            option_checkbox("ROM Mirror (A8)", o.ksmirror_a8, ctx);
            ShowHelpMarker("Enable Kickstart ROM mirror at $A80000");
            ImGui::TableNextColumn();
            option_checkbox("Composite color burst", o.color_burst, ctx);
            ShowHelpMarker("Enable color burst signal on composite video output");

            ImGui::TableNextColumn();
            option_checkbox("KB Reset Warning", o.resetwarning, ctx);
            ShowHelpMarker("Show warning when keyboard reset is pressed");
            ImGui::TableNextColumn();
            option_checkbox("Z3 Autoconfig", o.z3_autoconfig, ctx);
            ShowHelpMarker("Enable Zorro III AutoConfig for expansion boards");
            ImGui::TableNextColumn();
            // The single checkbox sets both CIA sockets, like the panel always did.
            value_checkbox("CIA 391078-01", o.ciaa_type, ctx, "391078-01", "default");
            ShowHelpMarker("Use newer CIA chip revision 391078-01");

            ImGui::TableNextColumn();
            option_checkbox("CIA TOD bug", o.cia_todbug, ctx);
            ShowHelpMarker("Emulate CIA Time of Day hardware bug");
            ImGui::TableNextColumn();
            option_checkbox("Custom register byte write bug", o.bytecustomwritebug, ctx);
            ShowHelpMarker("Emulate byte write bug in custom chip registers");
            ImGui::TableNextColumn();
            option_checkbox("Power up memory pattern", o.memory_pattern, ctx);
            ShowHelpMarker("Fill memory with pattern on power-up like real hardware");

            ImGui::TableNextColumn();
            ImGui::BeginDisabled(!available(o.chip1m, ctx));
            // With 1 MB chip RAM or more the core fixes the jumper on, so the
            // checkbox reflects that even while greyed out.
            bool chip1m = settings::get_bool(o.chip1m, changed_prefs)
                || changed_prefs.chipmem.size >= 0x100000;
            if (AmigaCheckbox("1M Chip / 0.5M+0.5M", &chip1m))
                settings::set_bool(o.chip1m, changed_prefs, ctx, chip1m);
            ImGui::EndDisabled();
            ShowHelpMarker("Jumper for 1MB chip RAM or 512KB+512KB configuration");

            ImGui::TableNextColumn();
            option_checkbox("KS ROM has Chip RAM speed", o.rom_is_slow, ctx);
            ShowHelpMarker("Kickstart ROM runs at Chip RAM speed instead of faster");

            ImGui::TableNextColumn();
            option_checkbox("Toshiba Gary", o.toshiba_gary, ctx);
            ShowHelpMarker("Use Toshiba Gary instead of Commodore Gary");

            ImGui::EndTable();
        }

        ImGui::Spacing();

        // Combos with Labels on Left
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Unmapped address space:");
        ImGui::SameLine();
        option_combo("##Unmapped", o.unmapped_address_space, ctx, BUTTON_WIDTH);
        ShowHelpMarker("What value unmapped memory reads return");

        ImGui::SameLine(0, 20); // Keep reasonable spacing between separate control groups

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CIA E-Clock Sync:");
        ImGui::SameLine();
        option_combo("##CiaSync", o.eclocksync, ctx, BUTTON_WIDTH);
        ShowHelpMarker("CIA E-Clock synchronization method");
        ImGui::Spacing();
    }
    EndGroupBox("Chipset Features");

    BeginGroupBox("Internal SCSI Hardware");
    {
        option_checkbox("A3000 WD33C93 SCSI", o.scsi_a3000, ctx);
        ShowHelpMarker("Enable A3000 built-in WD33C93 SCSI controller");
        ImGui::SameLine(0, 50);
        option_checkbox("A4000T NCR53C710 SCSI", o.scsi_a4000t, ctx);
        ShowHelpMarker("Enable A4000T built-in NCR53C710 SCSI controller");
        ImGui::Spacing();
    }
    EndGroupBox("Internal SCSI Hardware");

    BeginGroupBox("Chipset Revision");
    {
        revision_control("Ramsey revision:", "##RamseyRev", o.ramsey, ctx, 0x0f,
            "Enable Ramsey memory controller with specific revision",
            "Ramsey revision number (hexadecimal)");

        revision_control("Fat Gary revision:", "##GaryRev", o.fatgary, ctx, 0x00,
            "Enable Fat Gary chip with specific revision",
            "Fat Gary revision number (hexadecimal)");

        // Agnus/Alice
        {
            revision_control("Agnus/Alice model:", "##AgnusRev", o.agnusrev, ctx, 0,
                "Enable Agnus/Alice chip with specific revision",
                "Agnus/Alice revision number (hexadecimal)");
            ImGui::SameLine();
            option_combo("##AgnusModel", o.agnusmodel, ctx, BUTTON_WIDTH);
            ShowHelpMarker("Agnus/Alice model variant");
            ImGui::SameLine();
            option_combo("##AgnusSize", o.agnussize, ctx, BUTTON_WIDTH);
            ShowHelpMarker("Agnus/Alice addressable memory size");
        }

        // Denise/Lisa
        {
            revision_control("Denise/Lisa model:", "##DeniseRev", o.deniserev, ctx, 0,
                "Enable Denise/Lisa chip with specific revision",
                "Denise/Lisa revision number (hexadecimal)");
            ImGui::SameLine();
            option_combo("##DeniseModel", o.denisemodel, ctx, BUTTON_WIDTH);
            ShowHelpMarker("Denise/Lisa model variant");
        }
        ImGui::Spacing();
    }
    EndGroupBox("Chipset Revision");
}
