using System.Globalization;
using System.Text.Json.Nodes;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Automation;

namespace Amiberry.Windows;

public sealed partial class MainWindow
{
    private string newMachineName = "New machine";
    // The model the editor knows this machine was created from or switched to in this session.
    // Saved machine files do not record it, so it stays unknown (empty) for machines opened from the Library.
    private string modelPresetId = "", modelName = "", modelConfiguration = "";
    private TextBlock? overviewTitle, overviewState, overviewModel, overviewSummary;
    private readonly HashSet<string> collapsedSections = [];
    private async Task OpenEditorAsync(object request, string? name = null)
    {
        if (!await FlushDraftsAsync()) return;
        if (session && !await LeaveSessionAsync()) return;
        await EnsureSettingsOpenAsync();
        snapshot = await Core.CallAsync("open", request);
        // Reopening a running machine's editor (F12) starts from its pending preferences, so the bridge cannot tell
        // which of them were never written to the configuration file; the editor remembers that across Resume.
        bool reopening = request.GetType().GetProperty("path") is null && request.GetType().GetProperty("preset") is null;
        session = true; dirty = snapshot.B("changed");
        committedNotSaved = reopening && Emulating && resumedUnsavedPath is not null && resumedUnsavedPath == snapshot.S("currentConfiguration");
        if (!committedNotSaved) resumedUnsavedPath = null;
        currentPath = snapshot.S("currentConfiguration"); pendingApply = snapshot.I("apply");
        newMachineName = name ?? "New machine";
        SetModel(request.GetType().GetProperty("preset")?.GetValue(request) as string, (request.GetType().GetProperty("config")?.GetValue(request) as int?) ?? 0);
        if (Emulating && (request.GetType().GetProperty("path") is not null || request.GetType().GetProperty("preset") is not null))
            committedApply = Math.Max(committedApply, 2);
        drafts.Clear(); currentPage = "overview"; query = "";
        SetNavigation(true); await RenderEditorAsync();
    }
    private void SetModel(string? presetId, int config)
    {
        var preset = presetId is null ? null : catalog.Items("presets").FirstOrDefault(item => item.S("id") == presetId);
        modelPresetId = preset?.S("id") ?? "";
        modelName = preset?.S("label") ?? "";
        modelConfiguration = preset?.Items("configs").ElementAtOrDefault(config)?.S("label") ?? "";
    }
    private JsonNode Value(string id) => snapshot.Items("values").FirstOrDefault(item => item.S("id") == id) ?? new JsonObject { ["id"] = id, ["available"] = false };
    private SettingRow MakeSettingRow(JsonNode descriptor, JsonNode value, Func<string, Task> setter, string prefix = "")
    {
        string key = prefix + descriptor.S("id");
        var row = new SettingRow(descriptor, value, setter, (id, commit) =>
        {
            if (commit is null) drafts.Remove(prefix + id); else drafts[prefix + id] = commit;
            UpdateFooter(); return Task.CompletedTask;
        }, RunAsync, () => PickOptionPathAsync(descriptor, prefix), async (usage, modifiers) => (await Core.CallAsync("hotkey_format", new { usage, modifiers })).S("value"));
        rows[key] = row; return row;
    }
    private Task<string?> PickOptionPathAsync(JsonNode descriptor, string prefix)
    {
        string id = descriptor.S("id");
        if (id.EndsWith(":folder", StringComparison.Ordinal) || (prefix == "host:" && descriptor.S("type") == "path" && id != "logfile_path")) return PickFolderAsync();
        if (id is "rtc_file" or "flash_file") return PickFileAsync(true, id == "rtc_file" ? ".rtc" : ".nvr", id == "rtc_file" ? "Clock" : "Flash");
        return PickFileAsync();
    }
    private async Task SetOptionAsync(string id, string value)
    {
        ApplySnapshot(await Core.CallAsync("set", new { id, value }));
        if (id == "cdimage0") await RememberCdAsync(value);
        await RefreshCustomControlsAsync();
    }
    private void ApplySnapshot(JsonNode next)
    {
        snapshot = next; dirty = snapshot.B("changed"); pendingApply = snapshot.I("apply");
        foreach (var entry in rows)
            if (descriptors.ContainsKey(entry.Key)) entry.Value.Update(Value(entry.Key));
        UpdateFooter();
    }
    private async Task<bool> FlushDraftsAsync()
    {
        try
        {
            while (drafts.Count > 0)
                foreach (var commit in drafts.Values.ToArray()) await commit();
            return true;
        }
        catch (Exception error) { ShowError(error.Message); return false; }
    }
    private string SaveState()
    {
        bool unsaved = dirty || committedNotSaved || drafts.Count > 0;
        return unsaved ? "Unsaved changes" : currentPath.Length == 0 ? "Not saved to Library" : "All changes saved";
    }
    private void UpdateFooter()
    {
        editorBar.Visibility = session ? Visibility.Visible : Visibility.Collapsed;
        machineButton.Visibility = session ? Visibility.Collapsed : Visibility.Visible;
        int apply = Math.Max(pendingApply, committedApply);
        string action = !Emulating ? "Start Amiga" : apply >= 2 ? "Restart & Resume" : apply == 1 ? "Reset & Resume" : "Resume Amiga";
        startButton.Content = action;
        AutomationProperties.SetName(startButton, action);
        saveButton.IsEnabled = true;
        revertButton.IsEnabled = dirty || drafts.Count > 0;
        editStatus.Text = SaveState() + (Emulating && apply > 0 ? apply >= 2 ? " · Resuming will restart the Amiga." : " · Resuming will reset the Amiga." : "");
        var adjustments = snapshot.Items("adjustments").Select(item =>
        {
            string id = item.S("id");
            string label = descriptors.TryGetValue(id, out var descriptor) ? descriptor.S("label", id) : id;
            return label + ": " + item.S("reason");
        }).ToArray();
        if (adjustments.Length > 0) editStatus.Text += "\n" + string.Join("\n", adjustments);
        RefreshEditorHeader();
        RefreshOverview();
    }
    private async Task<bool> LeaveSessionAsync()
    {
        if (!await FlushDraftsAsync()) return false;
        if (dirty || committedNotSaved)
        {
            var result = await DialogAsync("Save machine changes?", "Save your edits before returning to the Library. Discard leaves the saved machine unchanged.", "Save", "Discard", "Cancel");
            if (result == ContentDialogResult.None) return false;
            if (result == ContentDialogResult.Primary && !await SaveAsync()) return false;
        }
        await Core.CallAsync("close"); session = false; dirty = false; committedNotSaved = false; resumedUnsavedPath = null; drafts.Clear(); rows.Clear(); pendingApply = 0;
        overviewTitle = overviewState = overviewModel = overviewSummary = null;
        return true;
    }
    private async Task<bool> SaveAsync(bool saveAs = false)
    {
        if (!await FlushDraftsAsync()) return false;
        string path = currentPath;
        if (saveAs || path.Length == 0)
        {
            var picked = await PickFileAsync(true, ".uae", path.Length > 0 ? Path.GetFileNameWithoutExtension(path) : SafeFileStem(newMachineName), initialFolder: currentPath.Length > 0 ? Path.GetDirectoryName(currentPath) : Folder(4));
            if (picked is null) return false;
            path = picked;
        }
        // save commits before writing; retain unsaved state if the file write fails.
        committedNotSaved = dirty || committedNotSaved;
        committedApply = Math.Max(committedApply, pendingApply);
        var result = await Core.CallAsync("save", new { path });
        committedApply = Math.Max(committedApply, result.I("apply"));
        currentPath = path; dirty = false; committedNotSaved = false; resumedUnsavedPath = null;
        snapshot = await Core.CallAsync("snapshot"); ApplySnapshot(snapshot);
        library = await Core.CallAsync("library"); UpdateFooter(); return true;
    }
    private async Task RevertAsync()
    {
        if (dirty || drafts.Count > 0 || committedNotSaved)
        {
            if (await DialogAsync("Revert editor changes?", "Discard edits made since the last save or apply. Saved machine files are not changed.", "Discard Changes", cancel: "Cancel", destructive: true) != ContentDialogResult.Primary) return;
        }
        drafts.Clear(); pendingApply = 0;
        ApplySnapshot(await Core.CallAsync("revert"));
        // Reverting returns to the last saved state, which may not be the model chosen in this session.
        SetModel(null, 0);
        await RenderEditorAsync();
    }
    private async Task StartAsync()
    {
        if (!await FlushDraftsAsync()) return;
        if (!Emulating && currentPath.Length > 0 && (dirty || committedNotSaved))
        {
            var answer = await DialogAsync("Save changes before starting?", "You can start with these edits without changing the saved machine.", "Save", "Start Without Saving", "Cancel");
            if (answer == ContentDialogResult.None) return;
            if (answer == ContentDialogResult.Primary && !await SaveAsync()) return;
        }
        bool unsaved = dirty || committedNotSaved;
        int apply = Math.Max(pendingApply, committedApply);
        if (Emulating && apply > 0 && await DialogAsync(apply >= 2 ? "Restart to apply changes?" : "Reset to apply changes?", "Unsaved work in the Amiga will be lost.", "Apply and Continue", cancel: "Cancel", destructive: true) != ContentDialogResult.Primary) return;
        var committed = await Core.CallAsync("commit");
        committedApply = Math.Max(committedApply, committed.I("apply"));
        apply = Math.Max(apply, committedApply);
        await Core.CallAsync("start", new { apply });
        await WaitForRunningAsync();
        committedApply = 0;
        await Core.CallAsync("close"); session = false; drafts.Clear(); dirty = false; pendingApply = 0;
        resumedUnsavedPath = unsaved ? currentPath : null;
        currentPage = "machines"; SetNavigation(false); await RenderLibraryAsync();
    }
    private string EditorMachineName() => currentPath.Length > 0 ? Path.GetFileNameWithoutExtension(currentPath) : newMachineName;
    private string EditorSubtitle() => currentPath.Length > 0 ? Path.GetFileNameWithoutExtension(currentPath) : newMachineName + " · Unsaved machine";
    // A first Save or a Save As changes the machine's identity while its page stays rendered; update the header in
    // place rather than rebuilding the page, which would drop the user's focus and scroll position.
    private void RefreshEditorHeader()
    {
        if (!session || pageSubtitle is null || !EditorPages.Any(item => item.Id == currentPage)) return;
        pageSubtitle.Text = EditorSubtitle();
        if (overviewTitle is not null) overviewTitle.Text = EditorMachineName();
    }
    private async Task RenderEditorAsync()
    {
        // Rebuilding drops every row; a rejected draft must be resolved while its row still shows the error.
        if (!await FlushDraftsAsync()) return;
        overviewTitle = overviewState = overviewModel = overviewSummary = null;
        if (!EditorPages.Any(item => item.Id == currentPage)) { currentPage = "overview"; SelectNavigation(currentPage); }
        BeginPage(EditorPages.First(item => item.Id == currentPage).Title,
            EditorSubtitle(), true);
        if (query.Length > 0 || currentPage == "all")
        {
            var matching = descriptors.Values.Where(item => Matches(item, query)).ToList();
            if (query.Length > 0)
            {
                if (matching.Count == 0)
                {
                    page.Children.Add(Card(Stack(SectionTitle("No matching settings"), Text("No setting of this machine matches “" + query + "”. The search covers every page; try a different name or keyword."),
                        Button("Clear Search", async () => { query = ""; await RenderEditorAsync(); }, "ClearSettingsSearchButton"))));
                }
                else page.Children.Add(Text($"{matching.Count} matching {(matching.Count == 1 ? "setting" : "settings")} across all pages"));
            }
            // A broad search or the full list stays collapsed so it can be scanned by group.
            bool collapse = query.Length == 0 || matching.Count > 40;
            foreach (var group in matching.GroupBy(item => item.S("group")))
                AddSection(SettingsGroupTitle(group.Key), group, collapse, true);
            if (query.Length == 0)
            {
                await AddMountsAsync(); await AddBoardsAsync(); await AddCustomControlsAsync();
            }
            return;
        }
        if (currentPage == "overview") { RenderOverview(); return; }
        foreach (var section in Layout.Sections.Where(item => item.Page == currentPage))
        {
            if (section.Kind == "mounts") { await AddMountsAsync(); continue; }
            if (section.Kind == "boards") { await AddBoardsAsync(); continue; }
            if (section.Kind == "controls") { await AddCustomControlsAsync(); continue; }
            var options = section.Ids.Where(descriptors.ContainsKey).Select(id => descriptors[id]).ToList();
            if (section.Title == "Accelerator board") options.AddRange(descriptors.Values.Where(item => item.S("id").StartsWith("cpuboard_settings:", StringComparison.Ordinal)));
            if (section.Title == "Priority") options = descriptors.Values.Where(item => item.S("group") == "priority").ToList();
            bool collapsed = section.Advanced;
            if (section.Ids.FirstOrDefault() is { } first && first.StartsWith("floppy", StringComparison.Ordinal) && first.EndsWith("type", StringComparison.Ordinal))
                collapsed = Value(first).S("value") == "-1";
            AddSection(section.Title, options, collapsed);
        }
        var placed = Layout.Sections.SelectMany(section => section.Ids).ToHashSet();
        foreach (var descriptor in descriptors.Values.Where(item => item.S("group") == "priority")) placed.Add(descriptor.S("id"));
        var groups = Layout.Groups[currentPage];
        AddSection("More", descriptors.Values.Where(item => groups.Contains(item.S("group")) && !placed.Contains(item.S("id")) && !item.S("id").StartsWith("cpuboard_settings:", StringComparison.Ordinal)), false);
    }

    // ---- Overview -----------------------------------------------------------

    private static readonly (string Title, string[] Ids)[] OverviewSections =
    [
        ("Processor & Chipset", ["cpu_model", "fpu_model", "chipset"]),
        ("Memory", ["chipmem_size", "bogomem_size", "fastmem_size", "z3mem_size"]),
        ("Kickstart & Disks", ["kickstart_rom_file", "floppy0", "floppy1"])
    ];
    private void RenderOverview()
    {
        var identity = new StackPanel { Spacing = 6 };
        var title = overviewTitle = Styled(EditorMachineName(), "SubtitleTextBlockStyle");
        AutomationProperties.SetAutomationId(title, "MachineIdentityName");
        identity.Children.Add(title);
        overviewState = Muted(""); overviewModel = Text(""); overviewSummary = Text("");
        overviewSummary.Opacity = 0.9;
        identity.Children.Add(overviewState); identity.Children.Add(overviewModel); identity.Children.Add(overviewSummary);
        var changeModel = Button("Change Model…", ChangeModelAsync, "ChangeModelButton");
        identity.Children.Add(Horizontal(changeModel, Button("All Hardware Settings", () => NavigateAsync("hardware")), Button("Storage & Disks", () => NavigateAsync("storage"))));
        page.Children.Add(Card(identity));
        RefreshOverview();
        AddSection("Library Details", descriptors.Values.Where(item => item.S("group") == "config"), false);
        foreach (var (sectionTitle, ids) in OverviewSections)
            AddSection(sectionTitle, ids.Where(descriptors.ContainsKey).Select(id => descriptors[id]), false);
    }
    private void RefreshOverview()
    {
        if (overviewState is null || overviewModel is null || overviewSummary is null || currentPage != "overview" || !session) return;
        overviewState.Text = (currentPath.Length > 0 ? "Saved machine" : "New machine") + " · " + SaveState();
        overviewModel.Text = modelName.Length > 0 ? "Based on " + modelName + (modelConfiguration.Length > 0 ? " · " + modelConfiguration : "") : "";
        overviewModel.Visibility = overviewModel.Text.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        var lines = new List<string>();
        string cpu = Choice("cpu_model"), fpu = Choice("fpu_model");
        if (cpu.Length > 0) lines.Add("Processor: " + cpu + (fpu.Length > 0 && fpu != "None" ? " + " + fpu : ""));
        if (Choice("chipset") is { Length: > 0 } chipset) lines.Add("Chipset: " + chipset);
        var memory = new List<string>();
        if (Choice("chipmem_size") is { Length: > 0 } chip) memory.Add(chip + " Chip");
        foreach (var (id, label) in new[] { ("bogomem_size", "Slow"), ("fastmem_size", "Fast"), ("z3mem_size", "Zorro III Fast") })
            if (Choice(id) is { Length: > 0 } size && size != "None") memory.Add(size + " " + label);
        if (memory.Count > 0) lines.Add("Memory: " + string.Join(" · ", memory));
        string rom = Choice("kickstart_rom_file");
        if (rom.Length > 0) lines.Add("Kickstart: " + Path.GetFileName(rom));
        string df0 = Choice("floppy0");
        if (df0.Length > 0) lines.Add("DF0: " + Path.GetFileName(df0));
        overviewSummary.Text = string.Join("\n", lines);
        overviewSummary.Visibility = lines.Count > 0 ? Visibility.Visible : Visibility.Collapsed;
    }
    // Human label of the current value of a setting (empty when the setting is unavailable or unset).
    private string Choice(string id)
    {
        if (!descriptors.TryGetValue(id, out var descriptor)) return "";
        var value = Value(id);
        string raw = value.S("value");
        if (raw.Length == 0) return "";
        var choices = value["choices"] is JsonArray ? value.Items("choices") : descriptor.Items("choices");
        return choices.FirstOrDefault(item => item.S("value") == raw)?.S("label") ?? raw;
    }

    private static string SettingsGroupTitle(string group) => group switch
    {
        "cpu" => "CPU", "mmu" => "MMU", "fpu" => "FPU", "cpu_speed" => "CPU Speed",
        "ppc" => "PowerPC", "x86" => "x86 Bridgeboard", "jit" => "JIT", "rom" => "ROMs",
        "rtg" => "RTG", "io" => "Ports & MIDI", "adv_chipset" => "Advanced Chipset",
        "diskswapper" => "Disk Swapper", "filesystem" => "File System", "drawbridge" => "DrawBridge",
        "onscreen" => "On-screen Controls", "config" => "Library Details", "misc" => "Miscellaneous",
        _ => CultureInfo.CurrentCulture.TextInfo.ToTitleCase(group.Replace('_', ' '))
    };

    private Expander Disclosure(string key, string header, UIElement content, bool open)
    {
        var expander = new Expander { Header = header, Content = content, IsExpanded = expanded.Contains(key) || (open && !collapsedSections.Contains(key)), HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
        expander.Expanding += (_, _) => { expanded.Add(key); collapsedSections.Remove(key); };
        expander.Collapsed += (_, _) => { expanded.Remove(key); collapsedSections.Add(key); };
        return expander;
    }
    private void AddSection(string title, IEnumerable<JsonNode> options, bool collapsed, bool showCount = false)
    {
        var content = new StackPanel { Spacing = 4 };
        foreach (var descriptor in options)
        {
            string id = descriptor.S("id");
            content.Children.Add(MakeSettingRow(descriptor, Value(id), value => SetOptionAsync(id, value)).Element);
        }
        if (content.Children.Count == 0) return;
        page.Children.Add(Disclosure(currentPage + ":" + title, showCount ? $"{title} ({content.Children.Count})" : title, content, !collapsed));
    }

    // ---- Change model -------------------------------------------------------

    private async Task ChangeModelAsync()
    {
        if (!await FlushDraftsAsync()) return;
        var presets = catalog.Items("presets").ToArray();
        if (presets.Length == 0) throw new InvalidOperationException("No machine models are available.");
        // The model is only known when this session created or switched it; otherwise the user must choose one.
        var chooser = new PresetChooser(presets, true, modelPresetId.Length > 0 ? modelPresetId : null);
        chooser.Content.Children.Insert(0, Text(modelName.Length > 0
            ? "This machine is based on " + modelName + "."
            : "This machine's model isn't recorded in its settings. Choose the model to switch to."));
        chooser.Content.Children.Add(Muted("The machine's description and category are kept. All other settings are replaced by the model's defaults."));
        string description = Value("config_description").S("value"), categoryValue = Value("config_category").S("value");
        JsonNode? applied = null, chosen = null;
        int configIndex = 0;
        var result = await DialogAsync("Change model", chooser.Content, "Change Model", cancel: "Cancel", destructive: true, primaryAction: async () =>
        {
            chosen = chooser.Preset ?? throw new InvalidOperationException("Choose a model to continue.");
            configIndex = chooser.Config;
            applied = await Core.CallAsync("preset", new { id = chosen.S("id"), config = configIndex, compatibility = chooser.Compatibility });
        });
        if (result != ContentDialogResult.Primary || applied is null || chosen is null) return;
        ApplySnapshot(applied);
        SetModel(chosen.S("id"), configIndex);
        if (currentPath.Length == 0) newMachineName = modelName;
        if (descriptors.ContainsKey("config_description")) await SetOptionAsync("config_description", description);
        if (descriptors.ContainsKey("config_category")) await SetOptionAsync("config_category", categoryValue);
        await RenderEditorAsync();
    }
}
