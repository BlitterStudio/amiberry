#include "imgui.h"
#include "sysdeps.h"
#include "custom.h"
#include "options.h"
#include "imgui_panels.h"
#include "rommgr.h"
#include "specialmonitors.h"
#include "settings/settings_definitions.h"

#include <vector>
#include <string>

#include "gui/gui_handling.h"

namespace settings = amiberry_settings;

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules and the dependent changes a new value implies.
struct chipset_options {
    const settings::definition& chipset = settings::require("chipset");
    const settings::definition& ntsc = settings::require("ntsc");
    const settings::definition& cycle_exact = settings::require("cycle_exact");
    const settings::definition& immediate_blits = settings::require("immediate_blits");
    const settings::definition& waiting_blits = settings::require("waiting_blits");
    const settings::definition& keyboard_type = settings::require("keyboard_type");
    const settings::definition& keyboard_nkro = settings::require("keyboard_nkro");
    const settings::definition& display_optimizations = settings::require("display_optimizations");
    const settings::definition& collision_level = settings::require("collision_level");
    const settings::definition& chipset_compatible = settings::require("chipset_compatible");
    const settings::definition& hvcsync = settings::require("hvcsync");
    const settings::definition& genlock = settings::require("genlock");
    const settings::definition& genlockmode = settings::require("genlockmode");
    const settings::definition& genlock_image = settings::require("genlock_image");
    const settings::definition& genlock_video = settings::require("genlock_video");
    const settings::definition& genlock_mix = settings::require("genlock_mix");
    const settings::definition& genlock_alpha = settings::require("genlock_alpha");
    const settings::definition& genlock_aspect = settings::require("genlock_aspect");
};

const chipset_options& options()
{
    static const chipset_options o;
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

// A checkbox for a BOOL option.
void option_checkbox(const char* label, const settings::definition& d, const settings::context& ctx)
{
    bool checked = settings::get_bool(d, changed_prefs);
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked);
}

static std::vector<std::string> keyboard_items_strs;
static std::vector<const char *> keyboard_items_ptr;
static std::vector<std::string> special_monitor_strs;
static std::vector<const char *> special_monitor_ptr;

static void appendkbmcurom(std::string &s, bool hasrom) {
    if (!hasrom) {
        s += " [ROM not found]";
    }
}

// The keyboard combo lists the layer's keyboard_type choices; the entries
// carry a "[ROM not found]" marker when the controller MCU ROM is missing,
// so the labels are built here instead of using the descriptor's.
static void init_lists() {
    keyboard_items_strs.clear();
    keyboard_items_strs.emplace_back("Keyboard disconnected");
    keyboard_items_strs.emplace_back("UAE High level emulation");

    int ids1[] = {321, -1};
    int ids2[] = {322, -1};
    int ids3[] = {323, -1};
    struct romlist *has65001 = nullptr;
    struct romlist *has657036 = getromlistbyids(ids1, nullptr);
    struct romlist *has6805 = getromlistbyids(ids2, nullptr);
    struct romlist *has8039 = getromlistbyids(ids3, nullptr);

    std::string tmp = "A500 / A500 + (6500 - 1 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    tmp = "A600 (6570 - 036 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    tmp = "A1000 (6500 - 1 MCU. ROM not yet dumped)";
    appendkbmcurom(tmp, has65001);
    keyboard_items_strs.push_back(tmp);

    tmp = "A1000 (6570 - 036 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    tmp = "A1200 (68HC05C MCU)";
    appendkbmcurom(tmp, has6805);
    keyboard_items_strs.push_back(tmp);

    tmp = "A2000 (Cherry, 8039 MCU)";
    appendkbmcurom(tmp, has8039);
    keyboard_items_strs.push_back(tmp);

    tmp = "A2000/A3000/A4000 (6570-036 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    keyboard_items_ptr.clear();
    for (const auto &s: keyboard_items_strs)
        keyboard_items_ptr.push_back(s.c_str());

    special_monitor_strs.clear();
    special_monitor_strs.emplace_back("-");
    special_monitor_strs.emplace_back("Autodetect");

    for (int i = 0; specialmonitorfriendlynames[i]; ++i)
        special_monitor_strs.emplace_back(specialmonitorfriendlynames[i]);

    special_monitor_ptr.clear();
    for (const auto &s: special_monitor_strs)
        special_monitor_ptr.push_back(s.c_str());
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

} // namespace


void render_panel_chipset() {
    const chipset_options& o = options();
    const settings::context ctx{emulating != 0};

    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    // Build the keyboard/special-monitor lists once, and rebuild only when the ROM list
    // changes (e.g. after a rescan) so the "[ROM not found]" labels stay accurate -
    // instead of rebuilding every frame.
    static int cached_rom_gen = -1;
    const int rom_gen = romlist_get_generation();
    if (rom_gen != cached_rom_gen) {
        init_lists();
        cached_rom_gen = rom_gen;
    }

    if (ImGui::BeginTable("chipset_table", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.56f);
        ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.44f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // LEFT COLUMN
        BeginGroupBox("Chipset");
        {
            // Chipset radios (the layer's chipset choices)
            ImGui::BeginGroup();
            if (AmigaRadioButton("A1000 (No EHB)", has_value(o.chipset, "a1000_noehb")))
                settings::set(o.chipset, changed_prefs, ctx, "a1000_noehb");
            ShowHelpMarker("Original A1000 chipset without Extra Half-Brite mode.");
            if (AmigaRadioButton("OCS + OCS Denise", has_value(o.chipset, "ocs")))
                settings::set(o.chipset, changed_prefs, ctx, "ocs");
            ShowHelpMarker("Original Chip Set with original Denise. Standard A500/A2000.");
            if (AmigaRadioButton("ECS + OCS Denise", has_value(o.chipset, "ecs_agnus")))
                settings::set(o.chipset, changed_prefs, ctx, "ecs_agnus");
            ShowHelpMarker("Enhanced Chip Set Agnus with original Denise.");
            if (AmigaRadioButton("AGA", has_value(o.chipset, "aga")))
                settings::set(o.chipset, changed_prefs, ctx, "aga");
            ShowHelpMarker("Advanced Graphics Architecture. A1200/A4000 chipset.");
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            if (AmigaRadioButton("A1000", has_value(o.chipset, "a1000")))
                settings::set(o.chipset, changed_prefs, ctx, "a1000");
            ShowHelpMarker("Original A1000 chipset with EHB support.");
            if (AmigaRadioButton("OCS + ECS Denise", has_value(o.chipset, "ecs_denise")))
                settings::set(o.chipset, changed_prefs, ctx, "ecs_denise");
            ShowHelpMarker("Original Agnus with Enhanced Denise chip.");
            if (AmigaRadioButton("Full ECS", has_value(o.chipset, "ecs")))
                settings::set(o.chipset, changed_prefs, ctx, "ecs");
            ShowHelpMarker("Full Enhanced Chip Set. A600/A3000.");

            option_checkbox("NTSC", o.ntsc, ctx);
            ShowHelpMarker("Enable NTSC mode (60Hz)");
            ImGui::EndGroup();

            ImGui::Spacing();

            // The two cycle-exact checkboxes are views of the layer's
            // composite cycle_exact choice: Full means true, DMA/Memory
            // means anything but false (full cycle exact implies it).
            bool full_exact = has_value(o.cycle_exact, "true");
            if (AmigaCheckbox("Cycle Exact (Full)", &full_exact)) {
                if (full_exact)
                    settings::set(o.cycle_exact, changed_prefs, ctx, "true");
                else
                    settings::set(o.cycle_exact, changed_prefs, ctx,
                        settings::is_choice_available(o.cycle_exact, changed_prefs, ctx, "memory")
                            ? "memory" : "false");
            }
            ShowHelpMarker("Bit-perfect emulation. Requires fast CPU.");

            bool memory_exact = !has_value(o.cycle_exact, "false");
            // Memory cycle exact is indistinguishable from full below a 68020.
            ImGui::BeginDisabled(!settings::is_choice_available(o.cycle_exact, changed_prefs, ctx, "memory"));
            if (AmigaCheckbox("Cycle Exact (DMA/Memory)", &memory_exact)) {
                settings::set(o.cycle_exact, changed_prefs, ctx, memory_exact ? "memory" : "false");
            }
            ShowHelpMarker("Cycle-exact DMA and memory access emulation.");
            ImGui::EndDisabled();
            ImGui::Spacing();
        }
        EndGroupBox("Chipset");

        BeginGroupBox("Keyboard");
        {
            ImGui::SetNextItemWidth(BUTTON_WIDTH * 3.0f);
            int kb_mode = choice_index_of(o.keyboard_type);
            if (ImGui::Combo("##Keyboard Layout", &kb_mode, keyboard_items_ptr.data(), keyboard_items_ptr.size())) {
                settings::set(o.keyboard_type, changed_prefs, ctx,
                              o.keyboard_type.descriptor.choices[kb_mode].value);
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
            ShowHelpMarker("Select keyboard controller emulation mode.");

            ImGui::Spacing();

            ImGui::BeginDisabled(!available(o.keyboard_nkro, ctx));
            option_checkbox("Keyboard N-key rollover", o.keyboard_nkro, ctx);
            ShowHelpMarker("Allow multiple simultaneous key presses.");
            ImGui::Spacing();
            ImGui::EndDisabled();
        }
        EndGroupBox("Keyboard");

        ImGui::TableNextColumn();

        // RIGHT COLUMN
        BeginGroupBox("Options");
        {
            option_checkbox("Immediate Blitter", o.immediate_blits, ctx);
            ShowHelpMarker("Blitter operations complete instantly. Faster but less accurate.");

            // Waiting for the blitter is a four-value option; the checkbox
            // maps on to Automatic and off to Disabled.
            ImGui::BeginDisabled(!available(o.waiting_blits, ctx));
            bool wait_blits = !has_value(o.waiting_blits, "disabled");
            if (AmigaCheckbox("Wait for Blitter", &wait_blits)) {
                settings::set(o.waiting_blits, changed_prefs, ctx, wait_blits ? "automatic" : "disabled");
            }
            ShowHelpMarker("CPU waits for Blitter to finish. More compatible than Immediate Blitter.");
            ImGui::EndDisabled();

            bool mt = multithread_enabled != 0;
            if (AmigaCheckbox("Multithreaded Denise", &mt)) {
                multithread_enabled = mt ? 1 : 0;
            }
            ShowHelpMarker("Run Denise graphics chip emulation on a separate thread.");

            ImGui::Spacing();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Optimizations");
            ShowHelpMarker("Keep at 'Full' for best compatibility.");

            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            int optimization_idx = choice_index_of(o.display_optimizations);
            if (ImGui::Combo("##Optimizations", &optimization_idx, o.display_optimizations.descriptor.choices,
                             static_cast<int>(o.display_optimizations.descriptor.choice_count))) {
                settings::set(o.display_optimizations, changed_prefs, ctx,
                              o.display_optimizations.descriptor.choices[optimization_idx].value);
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Chipset Extra");
            ShowHelpMarker("Select specialized chipset behavior.");

            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            int cs_compatible_idx = choice_index_of(o.chipset_compatible);
            if (ImGui::BeginCombo("##Chipset Extra",
                                  cs_compatible_idx < static_cast<int>(o.chipset_compatible.descriptor.choice_count)
                                      ? o.chipset_compatible.descriptor.choices[cs_compatible_idx].label
                                      : "Unknown")) {
                for (int n = 0; n < static_cast<int>(o.chipset_compatible.descriptor.choice_count); n++) {
                    const bool is_selected = (cs_compatible_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(o.chipset_compatible.descriptor.choices[n].label, is_selected)) {
                        settings::set(o.chipset_compatible, changed_prefs, ctx,
                                      o.chipset_compatible.descriptor.choices[n].value);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Monitor sync source:");

            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            int hvcsync_idx = choice_index_of(o.hvcsync);
            if (ImGui::BeginCombo("##SyncSource", o.hvcsync.descriptor.choices[hvcsync_idx].label)) {
                for (int n = 0; n < static_cast<int>(o.hvcsync.descriptor.choice_count); n++) {
                    const bool is_selected = (hvcsync_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(o.hvcsync.descriptor.choices[n].label, is_selected)) {
                        settings::set(o.hvcsync, changed_prefs, ctx, o.hvcsync.descriptor.choices[n].value);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            // Video port display hardware: not a settings-layer option (the
            // choice list is the run-time special monitor set).
            int monitoremu_idx = changed_prefs.monitoremu;
            if (monitoremu_idx < 0 || monitoremu_idx >= static_cast<int>(special_monitor_ptr.size()))
                monitoremu_idx = 0;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Video port display hardware");
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            if (ImGui::BeginCombo("##VideoPort", special_monitor_ptr[monitoremu_idx])) {
                for (int n = 0; n < static_cast<int>(special_monitor_ptr.size()); n++) {
                    const bool is_selected = (monitoremu_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(special_monitor_ptr[n], is_selected)) {
                        changed_prefs.monitoremu = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
            ImGui::Spacing();
        }
        EndGroupBox("Options");

        ImGui::EndTable();
    }

    ImGui::Indent(4.0f);

    BeginGroupBox("Collision Level");
    {
        ImGui::BeginGroup();
        if (AmigaRadioButton("None##Collision", has_value(o.collision_level, "none")))
            settings::set(o.collision_level, changed_prefs, ctx, "none");
        ShowHelpMarker("Disable all collision detection. Fastest.");
        ImGui::Spacing();
        if (AmigaRadioButton("Sprites only", has_value(o.collision_level, "sprites")))
            settings::set(o.collision_level, changed_prefs, ctx, "sprites");
        ShowHelpMarker("Detect sprite-to-sprite collisions only.");
        ImGui::Spacing();
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(BUTTON_WIDTH / 2, 0));
        ImGui::SameLine();

        ImGui::BeginGroup();
        if (AmigaRadioButton("Sprites and Sprites vs. Playfield", has_value(o.collision_level, "playfields")))
            settings::set(o.collision_level, changed_prefs, ctx, "playfields");
        ShowHelpMarker("Detect sprite-to-sprite and sprite-to-playfield collisions.");
        ImGui::Spacing();
        if (AmigaRadioButton("Full", has_value(o.collision_level, "full")))
            settings::set(o.collision_level, changed_prefs, ctx, "full");
        ShowHelpMarker("Full collision detection including playfield-to-playfield. Default.");
        ImGui::Spacing();
        ImGui::EndGroup();
    }
    EndGroupBox("Collision Level");

    // Genlock section
    BeginGroupBox("Genlock");
    {
        option_checkbox("Genlock connected", o.genlock, ctx);
        ShowHelpMarker("Connect a genlock device for video overlay mixing.");
        ImGui::SameLine();

        bool genlock_enable = changed_prefs.genlock || changed_prefs.genlock_effects;
        ImGui::BeginDisabled(!genlock_enable);

        int genlock_selection = choice_index_of(o.genlockmode);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - BUTTON_WIDTH - ImGui::GetStyle().ItemSpacing.x - 3.0f);
        if (ImGui::BeginCombo("##Genlock", o.genlockmode.descriptor.choices[genlock_selection].label)) {
            for (int n = 0; n < static_cast<int>(o.genlockmode.descriptor.choice_count); n++) {
                // The video sources need a VIDEOGRAB build; the layer marks
                // them unavailable instead of the panel silently dropping
                // the selection.
                const bool selectable = settings::is_choice_available(o.genlockmode, changed_prefs, ctx,
                    o.genlockmode.descriptor.choices[n].value);
                const ImGuiSelectableFlags flags = selectable
                    ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_Disabled;
                const bool is_selected = (genlock_selection == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(o.genlockmode.descriptor.choices[n].label, is_selected, flags)) {
                    settings::set(o.genlockmode, changed_prefs, ctx, o.genlockmode.descriptor.choices[n].value);
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

        const char *genlock_mix_items[] = {"100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%"};
        int genlock_mix_index = 10 - changed_prefs.genlock_mix / 25;
        if (genlock_mix_index < 0)
            genlock_mix_index = 0;
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::Combo("##GenlockMix", &genlock_mix_index, genlock_mix_items, IM_ARRAYSIZE(genlock_mix_items))) {
            settings::set_number(o.genlock_mix, changed_prefs, ctx, (10 - genlock_mix_index) * 25);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

        option_checkbox("Include alpha channels in screenshots and video captures.", o.genlock_alpha, ctx);
        option_checkbox("Keep aspect ratio", o.genlock_aspect, ctx);

        if (available(o.genlock_image, ctx) || available(o.genlock_video, ctx))
        // Image, Video or LaserDisc
        {
            const settings::definition& file_option =
                changed_prefs.genlock_image == 3 ? o.genlock_image : o.genlock_video;
            // Show File selector
            // Index 3 is Image, 4 and >= 6 are Video
            char *filename_ptr = (changed_prefs.genlock_image == 3)
                                     ? changed_prefs.genlock_image_file
                                     : changed_prefs.genlock_video_file;
            AmigaInputText("Genlock File", filename_ptr, MAX_DPATH);
            ImGui::SameLine();
            if (AmigaButton("...##GenlockFile")) {
                const char *filter = (changed_prefs.genlock_image == 3)
                    ? ".png,.jpg,.jpeg,.bmp"
#ifdef AMIBERRY_WITH_FFMPEG
                    : ".avi,.mp4,.mkv,.mov,.webm,.mpg,.mpeg,.m4v,.wmv";
#else
                    : ".avi";
#endif
                std::string title = (changed_prefs.genlock_image == 3)
                                        ? "Select Genlock Image"
                                        : "Select Genlock Video";
                OpenFileDialogKey("CHIPSET_FILE", title.c_str(), filter, filename_ptr);
            }

            std::string filePath;
            if (ConsumeFileDialogResultKey("CHIPSET_FILE", filePath)) {
                settings::set(file_option, changed_prefs, ctx, filePath.c_str());
            }
        }
        ImGui::EndDisabled();
        ImGui::Spacing();
    }
    EndGroupBox("Genlock");
}
