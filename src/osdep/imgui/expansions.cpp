#include <algorithm>
#include <map>
#include <utility>
#include <vector>
#include <string>
#include "imgui.h"
#include "sysdeps.h"
#include "options.h"

#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "autoconf.h"
#include "cpuboard.h"
#include "rommgr.h"
#include "uae.h"
#include "expansion_display_name.h"
#include "settings/settings_definitions.h"

namespace settings = amiberry_settings;

// Categories for Expansion Boards (Matches WinUAE scsiromselectedmask)
static const char *ExpansionCategories[] = {
    "Built-in / Internal",
    "SCSI Controllers (AutoConfig)",
    "IDE Controllers",
    "SASI Controllers",
    "Custom / Other",
    "PCI Bridge Boards",
    "x86 Bridge Boards",
    "RTG Graphics Cards",
    "Sound Cards",
    "Network Cards",
    "Floppy Controllers",
    "x86 Expansion Boards"
};
static const int ExpansionCategoriesMask[] = {
    EXPANSIONTYPE_INTERNAL,
    EXPANSIONTYPE_SCSI,
    EXPANSIONTYPE_IDE,
    EXPANSIONTYPE_SASI,
    EXPANSIONTYPE_CUSTOM,
    EXPANSIONTYPE_PCI_BRIDGE,
    EXPANSIONTYPE_X86_BRIDGE,
    EXPANSIONTYPE_RTG,
    EXPANSIONTYPE_SOUND,
    EXPANSIONTYPE_NET,
    EXPANSIONTYPE_FLOPPY,
    EXPANSIONTYPE_X86_EXPANSION
};

static std::vector<int> displayed_rom_indices;

// Track which ROM browse button opened the file dialog to avoid cross-assignment
enum RomDialogSource {
    ROM_DIALOG_NONE = 0,
    ROM_DIALOG_EXPANSION,
    ROM_DIALOG_ACCELERATOR
};
static RomDialogSource rom_dialog_source = ROM_DIALOG_NONE;

// The accelerator rules (which variants this build emulates, what picking
// one changes) are the settings layer's; a board is picked as one of its
// variants.
static bool is_accelerator_subtype_available(int type, int subtype)
{
    const settings::context ctx{emulating != 0};
    return settings::is_choice_available(settings::require("cpuboard_type"), changed_prefs, ctx,
        cpuboards[type].subtypes[subtype].configname);
}

static bool is_accelerator_type_available(int type)
{
    if (type == 0 || !cpuboards[type].subtypes)
        return true;
    for (int i = 0; cpuboards[type].subtypes[i].name; i++) {
        if (is_accelerator_subtype_available(type, i))
            return true;
    }
    return false;
}

static int get_available_accelerator_subtype(int type)
{
    const int default_subtype = cpuboards[type].defaultsubtype;
    if (is_accelerator_subtype_available(type, default_subtype)) {
        return default_subtype;
    }

    for (int i = 0; cpuboards[type].subtypes[i].name; i++) {
        if (is_accelerator_subtype_available(type, i)) {
            return i;
        }
    }
    return 0;
}

static void select_accelerator(int type, int subtype)
{
    const settings::context ctx{emulating != 0};
    settings::set(settings::require("cpuboard_type"), changed_prefs, ctx,
        type && cpuboards[type].subtypes ? cpuboards[type].subtypes[subtype].configname : "none");
}

static void sanitize_accelerator_selection()
{
#ifndef WITH_PPC
    if (cpuboard_is_ppc_accelerator(&changed_prefs)) {
        changed_prefs.cpuboard_type = 0;
        changed_prefs.cpuboard_subtype = 0;
        changed_prefs.cpuboard_settings = 0;
        changed_prefs.ppc_mode = 0;
        changed_prefs.ppc_model[0] = 0;
        changed_prefs.ppc_implementation = 0;
        changed_prefs.ppc_cpu_idle = 0;
    }
#endif
}




static void RefreshExpansionList() {
    displayed_rom_indices.clear();
    int first_match = -1;

    if (scsiromselectedcatnum < 0 || scsiromselectedcatnum >= IM_ARRAYSIZE(ExpansionCategoriesMask))
        scsiromselectedcatnum = 0;

    for (int i = 0; expansionroms[i].name; i++) {
        if (expansionroms[i].romtype & ROMTYPE_CPUBOARD) continue;

        int mask = ExpansionCategoriesMask[scsiromselectedcatnum];
        if (!(expansionroms[i].deviceflags & mask)) continue;

        if (scsiromselectedcatnum == 0 && (expansionroms[i].deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM)))
            continue;
        if ((expansionroms[i].deviceflags & EXPANSIONTYPE_X86_EXPANSION) && mask != EXPANSIONTYPE_X86_EXPANSION)
            continue;

        int cnt = 0;
        for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
            if (is_board_enabled(&changed_prefs, expansionroms[i].romtype, j)) {
                cnt++;
            }
        }

        if (cnt > 0 && first_match < 0) first_match = i;

        displayed_rom_indices.push_back(i);
    }

    std::sort(displayed_rom_indices.begin(), displayed_rom_indices.end(),
        [](int a, int b) {
            const struct expansionromtype *ea = &expansionroms[a];
            const struct expansionromtype *eb = &expansionroms[b];
            return _tcsicmp(expansion_display_name(ea->friendlyname, ea->friendlymanufacturer),
                expansion_display_name(eb->friendlyname, eb->friendlymanufacturer)) < 0;
        });

    bool current_valid = false;
    for (int idx: displayed_rom_indices) {
        if (idx == scsiromselected) {
            current_valid = true;
            break;
        }
    }
    if (!current_valid) {
        if (first_match >= 0) scsiromselected = first_match;
        else if (!displayed_rom_indices.empty()) scsiromselected = displayed_rom_indices[0];
        else scsiromselected = 0;
    }
}


static void InitializeExpansionSelection();

struct ROMOption {
    std::string name;
    std::string path;
    bool is_disabled_opt;
};

static const std::vector<ROMOption>& GetAvailableROMs(int romtype, int romtype_extra) {
    // The ROM database normally doesn't change frame-to-frame, and this is called once per
    // ROM selector every frame, so memoize the option list per (romtype, romtype_extra)
    // instead of rebuilding the vector each time. Invalidate whenever the rom list is
    // rebuilt - keyed on its generation, not its count, so a rescan that swaps ROMs for a
    // different set of the same size (scan_roms(true)) still refreshes the dropdowns.
    static std::map<std::pair<int, int>, std::vector<ROMOption>> cache;
    static int cached_gen = -1;

    const int gen = romlist_get_generation();
    if (gen != cached_gen) {
        cache.clear();
        cached_gen = gen;
    }

    const auto key = std::make_pair(romtype, romtype_extra);
    const auto it = cache.find(key);
    if (it != cache.end())
        return it->second;

    std::vector<ROMOption> options;

    // WinUAE adds "ROM Disabled" effectively by allowing NULL path or specific selection
    options.push_back({"ROM Disabled", "", true});

    const romlist *rl = romlist_getit();
    const int count = romlist_count();
    for (int i = 0; i < count; i++) {
        const romdata *rd = rl[i].rd;
        if (!rd) continue;

        bool match = false;
        if ((rd->type & ROMTYPE_MASK) == (romtype & ROMTYPE_MASK)) match = true;
        if (romtype_extra && (rd->type & ROMTYPE_MASK) == (romtype_extra & ROMTYPE_MASK)) match = true;

        if (match) {
            options.push_back({rd->name, rl[i].path, false});
        }
    }
    return cache.emplace(key, std::move(options)).first->second;
}

void render_panel_expansions() {
    ImGui::Indent(4.0f);

    // Initialize if needed
    if (displayed_rom_indices.empty()) {
        InitializeExpansionSelection();
    }

    // Enable/Disable based on emulation state (enable_for_expansion2dlg logic)
    bool gui_enabled = !emulating;

    BeginGroupBox("Expansion Board Settings");
    ImGui::BeginDisabled(!gui_enabled);

    // Category Selector
    ImGui::PushItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
    if (ImGui::BeginCombo("##Category", ExpansionCategories[scsiromselectedcatnum])) {
        for (int i = 0; i < IM_ARRAYSIZE(ExpansionCategories); i++) {
            const bool is_selected = (scsiromselectedcatnum == i);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(ExpansionCategories[i], is_selected)) {
                scsiromselectedcatnum = i;
                scsiromselected = 0; // Reset board selection
                RefreshExpansionList();
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Filter expansion boards by category");
    ImGui::PopItemWidth();

    // Board Selection Logic
    int current_combo_idx = -1;
    for (size_t i = 0; i < displayed_rom_indices.size(); ++i) {
        if (displayed_rom_indices[i] == scsiromselected) {
            current_combo_idx = i;
            break;
        }
    }
    const char *preview_val = "Select Board...";
    if (current_combo_idx >= 0) {
        const struct expansionromtype *ert = &expansionroms[displayed_rom_indices[current_combo_idx]];
        preview_val = expansion_display_name(ert->friendlyname, ert->friendlymanufacturer);
    }

    float avail_w = ImGui::GetContentRegionAvail().x;
    float enable_w = BUTTON_WIDTH * 1.5f;
    float unit_w = BUTTON_WIDTH;
    float board_w = avail_w - enable_w - unit_w - BUTTON_WIDTH / 4;

    ImGui::PushItemWidth(board_w);
    if (ImGui::BeginCombo("##ExpansionBoard", preview_val)) {
        for (size_t i = 0; i < displayed_rom_indices.size(); i++) {
            int global_idx = displayed_rom_indices[i];
            const bool is_selected = (current_combo_idx == i);

            // Format name with count if multiple
            int cnt = 0;
            for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
                if (is_board_enabled(&changed_prefs, expansionroms[global_idx].romtype, j)) cnt++;
            }
            const struct expansionromtype *ert = &expansionroms[global_idx];
            const char *display_name = expansion_display_name(ert->friendlyname, ert->friendlymanufacturer);
            std::string name_label = display_name;
            if (ert->friendlymanufacturer
                && _tcsicmp(ert->friendlymanufacturer, display_name) != 0) {
                name_label += " (";
                name_label += ert->friendlymanufacturer;
                name_label += ")";
            }
            if (cnt > 0) name_label = (cnt > 1 ? "[" + std::to_string(cnt) + "] " : "* ") + name_label;

            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(name_label.c_str(), is_selected)) {
                scsiromselected = global_idx;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Select an expansion board type. * indicates enabled boards");
    ImGui::PopItemWidth();

    ImGui::SameLine();

    const expansionromtype *ert = (scsiromselected >= 0) ? &expansionroms[scsiromselected] : nullptr;
    // Verify ert is valid (expansionroms is null-terminated)
    if (ert && !ert->name) ert = nullptr;

    // The settings layer owns the board rules: how many of it fit, the
    // fields a unit has, and what enabling one does (built-in controllers
    // are chipset options; a ROM board keeps a ROM file, so it survives a
    // save).
    const amiberry_settings_board_type *board = ert ? settings::board_type_find(ert->name) : nullptr;
    const settings::context board_ctx{emulating != 0};
    if (!board || scsiromselectednum >= board->units)
        scsiromselectednum = 0;
    const int unit = scsiromselectednum;
    auto board_value = [&](const char *field) {
        char text[MAX_DPATH] = "";
        if (board)
            settings::board_get(changed_prefs, board->id, unit, field, text, sizeof text);
        return std::string(text);
    };
    auto board_has = [&](const char *field) {
        return board && settings::board_field_available(changed_prefs, board_ctx, board->id, unit, field);
    };
    auto board_put = [&](const char *field, const std::string &value) {
        if (board)
            settings::board_set(changed_prefs, board_ctx, board->id, unit, field, value);
    };

    // Unit Selector
    ImGui::PushItemWidth(unit_w);
    ImGui::BeginDisabled(!board || board->units < 2);
    if (ImGui::BeginCombo("##Unit", std::to_string(unit + 1).c_str())) {
        for (int i = 0; board && i < board->units; i++) {
            const bool is_selected = (unit == i);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(std::to_string(i + 1).c_str(), is_selected)) {
                scsiromselectednum = i;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Unit number for boards that support multiple instances");
    ImGui::EndDisabled();
    ImGui::PopItemWidth();

    ImGui::SameLine();

    // Enable Checkbox
    bool enabled = board_value("enabled") == "true";
    ImGui::BeginDisabled(!board_has("enabled"));
    if (board && AmigaCheckbox("Enable Board", &enabled)) {
        board_put("enabled", enabled ? "true" : "false");
        enabled = board_value("enabled") == "true";
    }
    ImGui::EndDisabled();
    ShowHelpMarker("Enable this expansion board in the emulated system");

    // Detailed Settings: the unit's fields, as the layer describes them.
    if (board) {
        ImGui::BeginDisabled(!enabled);
        ImGui::Spacing();
        bool settings_header = false;
        for (size_t field_index = 1; field_index < board->field_count; field_index++) {
            const amiberry_setting_descriptor *d = settings::board_field_at(board, field_index);
            const std::string id = d->id;
            const bool available = board_has(d->id);
            const std::string current = board_value(d->id);

            if (id == "rom_file") {
                const std::vector<ROMOption>& rom_options = GetAvailableROMs(ert->romtype, ert->romtype_extra);
                char rom_path[MAX_DPATH] = "";
                strncpy(rom_path, current.c_str(), MAX_DPATH - 1);

                ImGui::Text("ROM Image:");
                ImGui::SameLine();
                ImGui::PushItemWidth(-40);
                if (rom_options.size() > 1) {
                    std::string current_preview = current.empty() ? rom_options[0].name : current;
                    for (const auto &opt: rom_options) {
                        if (!opt.is_disabled_opt && opt.path == current)
                            current_preview = opt.name;
                    }
                    if (ImGui::BeginCombo("##ROMSelector", current_preview.c_str())) {
                        for (const auto &opt: rom_options) {
                            const bool is_selected = opt.is_disabled_opt ? current.empty() : opt.path == current;
                            if (is_selected)
                                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                            if (ImGui::Selectable(opt.name.c_str(), is_selected))
                                board_put("rom_file", opt.is_disabled_opt ? std::string() : opt.path);
                            if (is_selected) {
                                ImGui::PopStyleColor();
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
                } else {
                    ImGui::InputText("##ROMPath", rom_path, MAX_DPATH, ImGuiInputTextFlags_ReadOnly);
                    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (AmigaButton("...")) {
                    rom_dialog_source = ROM_DIALOG_EXPANSION;
                    OpenFileDialogKey("EXPANSIONS_ROM", "Select ROM Image", "ROM Files (*.rom,*.bin){.rom,.bin}", get_rom_path());
                }
                std::string result_path;
                if (rom_dialog_source == ROM_DIALOG_EXPANSION && ConsumeFileDialogResultKey("EXPANSIONS_ROM", result_path)) {
                    rom_dialog_source = ROM_DIALOG_NONE;
                    if (!result_path.empty()) {
                        // Picking a file turns the board on too.
                        board_put("enabled", "true");
                        board_put("rom_file", result_path);
                    }
                }
                continue;
            }

            // The PCMCIA and 24-bit DMA flags only show on the variants
            // that have them, as before (greyed out while the board is off).
            if (id == "inserted" || id == "dma24bit") {
                int flags = ert->deviceflags;
                if (ert->subtypes) {
                    for (size_t c = 0; c < board->field_count; c++) {
                        const amiberry_setting_descriptor *sd = settings::board_field_at(board, c);
                        if (!strcmp(sd->id, "subtype")) {
                            const std::string variant = board_value("subtype");
                            for (int v = 0; ert->subtypes[v].name; v++) {
                                if (variant == ert->subtypes[v].configname)
                                    flags |= ert->subtypes[v].deviceflags;
                            }
                        }
                    }
                }
                if (!(flags & (id == "inserted" ? EXPANSIONTYPE_PCMCIA : EXPANSIONTYPE_DMA24)))
                    continue;
            }
            if (id.rfind("setting:", 0) == 0 && !settings_header) {
                ImGui::Separator();
                ImGui::Text("Board Settings:");
                settings_header = true;
            }
            ImGui::BeginDisabled(!available);
            if (d->type == AMIBERRY_SETTING_BOOL) {
                bool checked = current == "true";
                if (AmigaCheckbox(d->label, &checked))
                    board_put(d->id, checked ? "true" : "false");
            } else if (d->type == AMIBERRY_SETTING_CHOICE) {
                const char *preview = current.c_str();
                for (size_t c = 0; c < d->choice_count; c++) {
                    if (current == d->choices[c].value)
                        preview = d->choices[c].label;
                }
                ImGui::Text("%s:", d->label);
                ImGui::SameLine();
                ImGui::PushItemWidth(id == "scsi_id" ? 80.0f : -1.0f);
                if (ImGui::BeginCombo((std::string("##") + id).c_str(), preview)) {
                    for (size_t c = 0; c < d->choice_count; c++) {
                        const bool is_selected = current == d->choices[c].value;
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        if (ImGui::Selectable(d->choices[c].label, is_selected))
                            board_put(d->id, d->choices[c].value);
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
                ImGui::PopItemWidth();
            } else if (d->type == AMIBERRY_SETTING_STRING) {
                char text[ROMCONFIG_CONFIGTEXT_LEN] = "";
                strncpy(text, current.c_str(), sizeof text - 1);
                if (AmigaInputText(d->label, text, sizeof text))
                    board_put(d->id, text);
            }
            ImGui::EndDisabled();
            ShowHelpMarker(d->help);
        }
        ImGui::EndDisabled();
    }
    ImGui::EndDisabled();
    EndGroupBox("Expansion Board Settings");

    // Accelerator Settings
    BeginGroupBox("Accelerator Board Settings");
    sanitize_accelerator_selection();
    const int type = changed_prefs.cpuboard_type;
    const cpuboardsubtype *st = &cpuboards[type].subtypes[changed_prefs.cpuboard_subtype];

    if (ImGui::BeginTable("AccelTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Left Column: Type & Subtype
        ImGui::Text("Accelerator Board:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##AccelType", cpuboards[changed_prefs.cpuboard_type].name)) {
            for (int i = 0; cpuboards[i].name; i++) {
                if (!is_accelerator_type_available(i)) {
                    continue;
                }
                const bool is_selected = (changed_prefs.cpuboard_type == i);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(cpuboards[i].name, is_selected) && !is_selected) {
                    select_accelerator(i, i ? get_available_accelerator_subtype(i) : 0);
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("CPU accelerator board. Provides faster CPU and additional memory");

        if (cpuboards[type].subtypes) {
            ImGui::Text("Subtype:");
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            const cpuboardsubtype *subtypes = cpuboards[type].subtypes;
            if (ImGui::BeginCombo("##AccelSub", subtypes[changed_prefs.cpuboard_subtype].name)) {
                int i = 0;
                while (subtypes[i].name) {
                    if (!is_accelerator_subtype_available(type, i)) {
                        i++;
                        continue;
                    }
                    const bool is_selected = (changed_prefs.cpuboard_subtype == i);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(subtypes[i].name, is_selected)) {
                        select_accelerator(type, i);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                    i++;
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ShowHelpMarker("Specific variant or revision of the accelerator board");
        }

        ImGui::TableNextColumn();

        if (changed_prefs.cpuboard_type != 0) {
            int idx = 0;
            boardromconfig *brc = get_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, &idx);

            ImGui::Text("Boot ROM:");
            char rom_path[MAX_DPATH] = "";
            if (brc) strncpy(rom_path, brc->roms[idx].romfile, MAX_DPATH);

            const std::vector<ROMOption>& accel_rom_options = GetAvailableROMs(st->romtype, st->romtype_extra);

            ImGui::PushItemWidth(-BUTTON_WIDTH / 2);
            ImGui::BeginDisabled(!gui_enabled);

            if (accel_rom_options.size() > 1) {
                std::string current_preview = "Select ROM...";
                for (const auto &opt : accel_rom_options) {
                    if (opt.is_disabled_opt) {
                        if (rom_path[0] == 0) current_preview = opt.name;
                    } else if (strcmp(opt.path.c_str(), rom_path) == 0) {
                        current_preview = opt.name;
                    }
                }
                if (current_preview == "Select ROM..." && rom_path[0] != 0) {
                    current_preview = rom_path;
                }

                if (ImGui::BeginCombo("##AccelROM", current_preview.c_str())) {
                    for (const auto &opt : accel_rom_options) {
                        bool is_selected = opt.is_disabled_opt
                            ? (rom_path[0] == 0)
                            : (strcmp(opt.path.c_str(), rom_path) == 0);

                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        if (ImGui::Selectable(opt.name.c_str(), is_selected)) {
                            int new_idx = 0;
                            boardromconfig *new_brc = get_device_rom_new(&changed_prefs, ROMTYPE_CPUBOARD, 0, &new_idx);
                            if (new_brc) {
                                if (opt.is_disabled_opt) {
                                    new_brc->roms[new_idx].romfile[0] = 0;
                                } else {
                                    strncpy(new_brc->roms[new_idx].romfile, opt.path.c_str(), MAX_DPATH);
                                }
                            }
                        }
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            } else {
                ImGui::InputText("##AccelROM", rom_path, MAX_DPATH, ImGuiInputTextFlags_ReadOnly);
                AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (AmigaButton("...##Accel")) {
                rom_dialog_source = ROM_DIALOG_ACCELERATOR;
                OpenFileDialogKey("EXPANSIONS_ROM", "Select Boot ROM", "ROM Files (*.rom,*.bin){.rom,.bin}", get_rom_path());
            }
            ImGui::EndDisabled();

            std::string result_path;
            if (rom_dialog_source == ROM_DIALOG_ACCELERATOR && ConsumeFileDialogResultKey("EXPANSIONS_ROM", result_path)) {
                rom_dialog_source = ROM_DIALOG_NONE;
                if (!result_path.empty()) {
                    int new_idx = 0;
                    boardromconfig *new_brc = get_device_rom_new(&changed_prefs, ROMTYPE_CPUBOARD, 0, &new_idx);
                    if (new_brc) {
                        strncpy(new_brc->roms[new_idx].romfile, result_path.c_str(), MAX_DPATH);
                    }
                }
            }
        } else {
            ImGui::Text("Boot ROM: Not Required");
        }

        // Memory Slider
        static const int mem_sizes[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256};
        static const char *mem_labels[] = {
            "0 MB", "1 MB", "2 MB", "4 MB", "8 MB", "16 MB", "32 MB", "64 MB", "128 MB", "256 MB"
        };
        int current_size = changed_prefs.cpuboardmem1.size >> 20;
        int current_idx = 0;
        for (int i = 0; i < IM_ARRAYSIZE(mem_sizes); ++i) {
            if (mem_sizes[i] == current_size) {
                current_idx = i;
                break;
            }
        }

        const settings::context mem_ctx{emulating != 0};
        const settings::definition& mem_option = settings::require("cpuboardmem1_size");
        int max_idx = 0;
        for (int i = 0; i < IM_ARRAYSIZE(mem_sizes); ++i) {
            if (settings::is_choice_available(mem_option, changed_prefs, mem_ctx, std::to_string(mem_sizes[i])))
                max_idx = i;
        }

        if (max_idx > 0) {
            ImGui::Text("Board Memory:");
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            // WinUAE sync: Memory slider enabled based on cpuboard_type > 0, not just !emulating
            ImGui::BeginDisabled(!gui_enabled || changed_prefs.cpuboard_type == 0);
            if (ImGui::SliderInt("##BoardMem", &current_idx, 0, max_idx, mem_labels[current_idx])) {
                // The layer puts the memory where the board has it.
                settings::set(mem_option, changed_prefs, mem_ctx, std::to_string(mem_sizes[current_idx]));
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
            ShowHelpMarker("On-board memory provided by the accelerator. Faster than standard Fast RAM");
            ImGui::EndDisabled();
        } else {
            ImGui::Text("Board Memory: None / Fixed");
        }

        ImGui::EndTable();
    }

    // Accelerator Settings (Jumpers) - Full width below columns
    if (st->settings) {
        ImGui::Separator();
        ImGui::Text("Jumper Settings:");
        // Every switch and selector is a settings layer option, which owns
        // the bit layout the configuration uses.
        const settings::context jumper_ctx{emulating != 0};
        const expansionboardsettings *ebs = st->settings;
        for (int item_idx = 0; ebs[item_idx].name; item_idx++) {
            const expansionboardsettings *s = &ebs[item_idx];
            if (s->type != EXPANSIONBOARD_CHECKBOX && s->type != EXPANSIONBOARD_MULTI)
                continue;
            // A selector's configuration name list starts with its own name.
            const std::string jumper_id = std::string("cpuboard_settings:") + s->configname;
            const settings::definition *jumper = settings::find(jumper_id);
            if (s->type == EXPANSIONBOARD_MULTI && jumper && jumper->descriptor.type != AMIBERRY_SETTING_CHOICE)
                jumper = settings::find(jumper_id + "_choice");
            if (!jumper)
                continue;
            char current[64] = "";
            settings::get(*jumper, changed_prefs, current, sizeof current);

            if (s->type == EXPANSIONBOARD_CHECKBOX) {
                bool checked = !strcmp(current, "true");
                if (AmigaCheckbox(s->name, &checked))
                    settings::set(*jumper, changed_prefs, jumper_ctx, checked ? "true" : "false");
                continue;
            }

            // This board's positions, as it names them ("Label\0Pos1\0...",
            // "id\0pos1\0...").
            std::vector<std::pair<const char *, const char *>> positions;
            const char *label = s->name;
            const char *nm = s->name + strlen(s->name) + 1;
            for (const char *cfg = s->configname + strlen(s->configname) + 1; *cfg; cfg += strlen(cfg) + 1) {
                positions.emplace_back(cfg, *nm ? nm : cfg);
                if (*nm)
                    nm += strlen(nm) + 1;
            }
            const char *preview = current;
            for (const auto &pos : positions) {
                if (!strcmp(pos.first, current))
                    preview = pos.second;
            }
            if (ImGui::BeginCombo(label, preview)) {
                for (const auto &pos : positions) {
                    const bool is_selected = !strcmp(pos.first, current);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(pos.second, is_selected))
                        settings::set(*jumper, changed_prefs, jumper_ctx, pos.first);
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        }
    }
    EndGroupBox("Accelerator Board Settings");

    BeginGroupBox("Miscellaneous Expansions");
    if (ImGui::BeginTable("MiscTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::BeginDisabled(!gui_enabled);
        // WinUAE disables misc expansions when running? logic says "ew(hDlg, IDC_SOCKETS, workprefs.socket_emu)" - check enable_for_expansion2dlg

        AmigaCheckbox("bsdsocket.library", &changed_prefs.socket_emu);
        ShowHelpMarker("Emulate TCP/IP networking via host OS network stack");

        bool scsi_enabled = changed_prefs.scsi != 0;
        if (AmigaCheckbox("uaescsi.device", &scsi_enabled)) {
            changed_prefs.scsi = scsi_enabled ? 1 : 0;
        }
        ShowHelpMarker("Emulate SCSI device for CD-ROM and hard drive access");

        ImGui::TableNextColumn();
        // CD32 FMV Card removed for WinUAE parity
        AmigaCheckbox("uaenet.device", &changed_prefs.sana2);
        ShowHelpMarker("Direct network device emulation using host network interface");

        ImGui::EndDisabled();
        ImGui::EndTable();
    }
    EndGroupBox("Miscellaneous Expansions");
}

static void InitializeExpansionSelection() {
    load_keyring(&changed_prefs, nullptr);

    scsiromselected = 0;
    scsiromselectedcatnum = 0;
    scsiromselectednum = 0;

    // Check all categories
    for (int cat = 0; cat < IM_ARRAYSIZE(ExpansionCategoriesMask); cat++) {
        int mask = ExpansionCategoriesMask[cat];
        for (int i = 0; expansionroms[i].name; i++) {
            if (expansionroms[i].romtype & ROMTYPE_CPUBOARD) continue;
            if (!(expansionroms[i].deviceflags & mask)) continue;
            if (cat == 0 && (expansionroms[i].deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM))) continue;
            if ((expansionroms[i].deviceflags & EXPANSIONTYPE_X86_EXPANSION) && mask != EXPANSIONTYPE_X86_EXPANSION)
                continue;

            // Is this board enabled?
            if (is_board_enabled(&changed_prefs, expansionroms[i].romtype, 0)) {
                scsiromselectedcatnum = cat;
                scsiromselected = i;
                goto found_active;
            }
        }
    }
found_active:
    RefreshExpansionList();
}
