using System.Diagnostics;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Markup;

namespace Amiberry.Windows;

public sealed partial class MainWindow
{
    private const int MediaLimit = 200;
    private string mediaKey = "";
    private int mediaShown = MediaLimit;
    private static readonly Regex InvalidNameRuns = new("\\s*[" + Regex.Escape(new string(Path.GetInvalidFileNameChars())) + "]+\\s*", RegexOptions.Compiled);
    private string? category;
    private TextBlock? downloadStatus, downloadDetail;
    private Button? installDownload, cancelDownload;
    private ProgressBar? downloadProgress;
    private string Folder(int id) => library.Items("folders").FirstOrDefault(item => item.I("id") == id).S("path");
    private JsonNode FolderNode(int id) => library.Items("folders").FirstOrDefault(item => item.I("id") == id) ?? new JsonObject { ["id"] = id, ["label"] = "Folder", ["path"] = "" };

    private async Task RenderLibraryAsync()
    {
        UpdateFooter();
        if (currentPage == "app") { await RenderAppSettingsAsync(); return; }
        library = await Core.CallAsync("library");
        if (currentPage == "new") { RenderNewMachinePage(); return; }
        string title = LibraryPages.FirstOrDefault(item => item.Id == currentPage).Title ?? "Library";
        int count = library.Items("machines").Count();
        bool firstRun = currentPage == "machines" && count == 0;
        (string subtitle, string placeholder) = currentPage switch
        {
            "machines" => (firstRun ? "No saved machines yet." : $"{count} saved {(count == 1 ? "machine" : "machines")}. Play one, edit its settings, or duplicate it as a starting point.", "Search machines"),
            "recent" => ("Floppy disks, CDs and WHDLoad archives you played recently.", "Search recent media"),
            "floppies" => ("Disk images from your floppy folder. Play one directly or in a saved machine.", "Search floppy disks"),
            "cds" => ("CD images from your CD folder. Play one directly or in a saved machine.", "Search CD images"),
            "whdload" => ("WHDLoad archives from your WHDLoad folder.", "Search WHDLoad archives"),
            "roms" => ("The Kickstart ROMs Amiberry can start machines with.", "Search Kickstart ROMs"),
            _ => ("Your Amiga machines and media", "Search this page")
        };
        BeginPage(title, subtitle, !firstRun, placeholder);
        page.Children.Add(Horizontal(PrimaryButton("New Machine", NewMachineAsync, "NewMachineButton"), Button("Play a File…", async () => { if (await PickFileAsync() is { } path) await PlayFileAsync(path); }, "PlayFileButton"), Button("Refresh", RenderLibraryAsync)));
        if (currentPage == "machines") RenderMachines();
        else if (currentPage == "roms") RenderRoms();
        else await RenderMediaAsync();
    }

    // ---- shared page pieces -------------------------------------------------

    // Every term must appear in at least one of the human-readable fields.
    private static bool FieldsMatch(string query, params string[] fields) =>
        query.Split(' ', StringSplitOptions.RemoveEmptyEntries).All(term => fields.Any(field => field.Contains(term, StringComparison.CurrentCultureIgnoreCase)));

    // A file-name stem that Windows accepts; invalid characters become a dash separator.
    private static string SafeFileStem(string text, string fallback = "New machine")
    {
        string stem = Regex.Replace(InvalidNameRuns.Replace(text, " - "), "\\s+", " ").Trim(' ', '.', '-');
        return stem.Length == 0 ? fallback : stem;
    }

    private static TextBlock ThemedText(string text, string brush, double size = 12)
    {
        var block = (TextBlock)XamlReader.Load("<TextBlock xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Foreground=\"{ThemeResource " + brush + "}\" TextWrapping=\"Wrap\"/>");
        block.Text = text; block.FontSize = size;
        return block;
    }
    private static Border Divided(UIElement child)
    {
        var border = (Border)XamlReader.Load("<Border xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" BorderBrush=\"{ThemeResource DividerStrokeColorDefaultBrush}\" BorderThickness=\"0,1,0,0\" Padding=\"0,12,0,0\"/>");
        border.Child = child;
        return border;
    }

    private static StackPanel Actions(params UIElement[] items)
    {
        var panel = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, VerticalAlignment = VerticalAlignment.Center };
        foreach (var item in items) panel.Children.Add(item);
        return panel;
    }
    // Details on the left, actions on the right; the actions drop below when the card is narrow.
    private static Grid ActionRow(UIElement details, StackPanel actions)
    {
        var grid = new Grid { ColumnSpacing = 16, RowSpacing = 12 };
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        Grid.SetColumn(actions, 1);
        grid.Children.Add(details); grid.Children.Add(actions);
        grid.SizeChanged += (_, e) =>
        {
            bool narrow = e.NewSize.Width < 560;
            Grid.SetRow(actions, narrow ? 1 : 0); Grid.SetColumn(actions, narrow ? 0 : 1); Grid.SetColumnSpan(actions, narrow ? 2 : 1);
            actions.HorizontalAlignment = narrow ? HorizontalAlignment.Left : HorizontalAlignment.Right;
        };
        return grid;
    }
    private MenuFlyoutItem MenuAction(string text, Symbol symbol, Func<Task> action)
    {
        var item = new MenuFlyoutItem { Text = text, Icon = new SymbolIcon(symbol) };
        item.Click += async (_, _) => await RunAsync(action);
        return item;
    }
    private static Button MoreButton(MenuFlyout menu, string name, string tooltip, string? id = null)
    {
        var more = new Button { Content = new SymbolIcon(Symbol.More), Flyout = menu };
        AutomationProperties.SetName(more, name);
        if (id is not null) AutomationProperties.SetAutomationId(more, id);
        ToolTipService.SetToolTip(more, tooltip);
        return more;
    }
    private static void RevealInExplorer(string path)
    {
        string? arguments = File.Exists(path) ? $"/select,\"{path}\"" : Directory.Exists(path) ? $"\"{path}\"" : Path.GetDirectoryName(path) is { } parent && Directory.Exists(parent) ? $"\"{parent}\"" : null;
        if (arguments is null) throw new InvalidOperationException("That location no longer exists.");
        Process.Start(new ProcessStartInfo("explorer.exe") { Arguments = arguments, UseShellExecute = false });
    }
    private UIElement NoResultsCard(string what, string? id = null) =>
        Card(Stack(SectionTitle("No results"), Text($"Nothing in {what} matches “{query.Trim()}”."),
            Button("Clear Search", async () => { query = ""; await RenderLibraryAsync(); }, id ?? "ClearSearchButton")));

    // Label, current location and actions for one library folder.
    private UIElement FolderRow(JsonNode folder, Func<Task> refresh, Func<string, Task>? import = null)
    {
        int id = folder.I("id"); string label = folder.S("label"), path = folder.S("path");
        var details = new StackPanel { Spacing = 2 };
        details.Children.Add(Styled(label + " folder", "BodyStrongTextBlockStyle"));
        var location = Muted(path.Length == 0 ? "Not set" : path);
        location.TextWrapping = TextWrapping.NoWrap; location.TextTrimming = TextTrimming.CharacterEllipsis;
        if (path.Length > 0) ToolTipService.SetToolTip(location, path);
        details.Children.Add(location);
        bool exists = path.Length > 0 && Directory.Exists(path);
        if (path.Length > 0 && !exists) details.Children.Add(ThemedText("This folder doesn't exist yet. It is created when you import files.", "SystemFillColorCautionBrush"));
        var choose = Button("Choose Folder…", async () =>
        {
            if (!await FlushDraftsAsync()) return;
            if (await PickFolderAsync() is { } picked) { library = await Core.CallAsync("folder_set", new { id, path = picked }); await refresh(); }
        }, "FolderChoose_" + id);
        AutomationProperties.SetName(choose, "Choose " + label + " folder");
        var actions = Actions(choose);
        if (import is not null)
        {
            var importButton = Button("Import Files…", () => import(path), "FolderImport_" + id);
            importButton.IsEnabled = path.Length > 0;
            if (path.Length == 0) ToolTipService.SetToolTip(importButton, "Choose a folder first.");
            actions.Children.Add(importButton);
        }
        var open = Button("Open Folder", () => { RevealInExplorer(path); return Task.CompletedTask; }, "FolderOpen_" + id);
        open.IsEnabled = exists;
        AutomationProperties.SetName(open, "Open " + label + " folder in File Explorer");
        actions.Children.Add(open);
        return ActionRow(details, actions);
    }

    // ---- Machines -----------------------------------------------------------

    private void RenderMachines()
    {
        var machines = library.Items("machines").ToList();
        if (machines.Count == 0)
        {
            page.Children.Add(Card(Stack(
                SectionTitle("Two ways to start"),
                Text("Create a machine: pick an Amiga model, adjust its settings, and save it to launch again later."),
                Text("Play a file: start a floppy disk, CD, or archive straight away."))));
            page.Children.Add(SectionTitle("Choose a model"));
            page.Children.Add(PresetGrid());
            return;
        }
        var categories = machines.Select(item => item.S("category")).Distinct().Order().ToList();
        if (categories.Count < 2 || (category is not null && !categories.Contains(category))) category = null;
        if (categories.Count > 1)
        {
            var filter = new ComboBox { Width = 240, HorizontalAlignment = HorizontalAlignment.Left };
            AutomationProperties.SetName(filter, "Category");
            AutomationProperties.SetAutomationId(filter, "MachineCategoryFilter");
            filter.Items.Add(new ComboBoxItem { Content = "All categories", Tag = "" });
            foreach (string item in categories)
                filter.Items.Add(new ComboBoxItem { Content = CategoryName(item), Tag = item });
            filter.SelectedItem = filter.Items.OfType<ComboBoxItem>().FirstOrDefault(item => (string)item.Tag == (category ?? ""));
            filter.SelectionChanged += async (_, _) => await RunAsync(async () =>
            {
                category = filter.SelectedIndex == 0 || filter.SelectedItem is not ComboBoxItem selected ? null : selected.Tag as string;
                await RenderLibraryAsync();
            });
            page.Children.Add(filter);
        }
        var filtered = machines.Where(item => (category is null || item.S("category") == category) && (query.Length == 0 || FieldsMatch(query, MachineName(item), item.S("description"), item.S("category"), Path.GetFileNameWithoutExtension(item.S("path"))))).ToList();
        if (filtered.Count != machines.Count) page.Children.Add(Muted($"Showing {filtered.Count} of {machines.Count} machines"));
        foreach (var machine in filtered) page.Children.Add(MachineCard(machine));
        if (filtered.Count == 0)
        {
            string trimmed = query.Trim();
            string message = trimmed.Length > 0 ? $"No machines match “{trimmed}”" + (category is null ? "." : $" in {CategoryName(category)}.") : $"No machines in {CategoryName(category ?? "")}.";
            page.Children.Add(Card(Stack(SectionTitle("No matching machines"), Text(message),
                Button("Clear Search and Filter", async () => { query = ""; category = null; await RenderLibraryAsync(); }, "ClearMachineFilterButton"))));
        }
    }
    private static string CategoryName(string category) => category.Length == 0 ? "Other" : category;
    private static string MachineName(JsonNode machine) => machine.S("name").Length > 0 ? machine.S("name") : Path.GetFileNameWithoutExtension(machine.S("path"));

    private UIElement MachineCard(JsonNode machine)
    {
        string path = machine.S("path"), name = MachineName(machine), stem = Path.GetFileNameWithoutExtension(path);
        var details = new StackPanel { Spacing = 4 };
        details.Children.Add(Styled(name, "SubtitleTextBlockStyle"));
        if (machine.S("description").Length > 0) details.Children.Add(Text(machine.S("description")));
        var facts = new List<string>();
        if (machine.S("category").Length > 0) facts.Add(machine.S("category"));
        if (File.Exists(path)) facts.Add("Modified " + File.GetLastWriteTime(path).ToString("d"));
        if (facts.Count > 0) details.Children.Add(Muted(string.Join(" · ", facts)));

        var play = PrimaryButton("Play", async () => { await OpenEditorAsync(new { path }); await StartAsync(); }, "MachinePlay_" + stem);
        AutomationProperties.SetName(play, "Play " + name);
        var edit = Button("Edit", () => OpenEditorAsync(new { path }), "MachineEdit_" + stem);
        AutomationProperties.SetName(edit, "Edit " + name);

        var menu = new MenuFlyout();
        menu.Items.Add(MenuAction("Duplicate…", Symbol.Copy, () => DuplicateMachineAsync(machine)));
        menu.Items.Add(MenuAction("Rename…", Symbol.Rename, () => RenameMachineAsync(machine)));
        menu.Items.Add(MenuAction("Show in File Explorer", Symbol.Folder, () => { RevealInExplorer(path); return Task.CompletedTask; }));
        menu.Items.Add(new MenuFlyoutSeparator());
        menu.Items.Add(MenuAction("Delete…", Symbol.Delete, () => DeleteMachineAsync(machine)));
        var more = MoreButton(menu, "More actions for " + name, "Duplicate, rename or delete", "MachineMore_" + stem);
        return Card(ActionRow(details, Actions(play, edit, more)));
    }

    // ---- New machine --------------------------------------------------------

    private UIElement PresetGrid()
    {
        var grid = new VariableSizedWrapGrid { Orientation = Orientation.Horizontal, ItemWidth = 232 };
        foreach (var preset in catalog.Items("presets")) grid.Children.Add(PresetButton(preset));
        return grid;
    }
    private Button PresetButton(JsonNode preset)
    {
        var configurations = preset.Items("configs").ToList();
        int installed = configurations.Count(item => item.B("romInstalled"));
        string status = installed == configurations.Count && installed > 0 ? "Kickstart ROM installed"
            : installed == 0 ? "Kickstart ROM not installed"
            : $"Kickstart ROM installed for {installed} of {configurations.Count} configurations";
        var button = Button("", () => ChoosePresetAsync(preset), "Preset_" + preset.S("id"));
        button.Content = Stack(Styled(preset.S("label"), "BodyStrongTextBlockStyle"), Muted(status));
        ((StackPanel)button.Content).Spacing = 2;
        button.HorizontalAlignment = HorizontalAlignment.Stretch;
        button.HorizontalContentAlignment = HorizontalAlignment.Left;
        button.Padding = new Thickness(12);
        button.Margin = new Thickness(0, 0, 8, 8);
        AutomationProperties.SetName(button, $"Create {preset.S("label")}. {status}");
        return button;
    }
    private static TextBlock Styled(string text, string style) { var block = Text(text); block.Style = (Style)Application.Current.Resources[style]; return block; }
    private static TextBlock Muted(string text) { var block = Styled(text, "CaptionTextBlockStyle"); block.Opacity = 0.78; return block; }
    private static TextBlock SectionTitle(string text)
    {
        var block = Styled(text, "SubtitleTextBlockStyle");
        AutomationProperties.SetHeadingLevel(block, AutomationHeadingLevel.Level2);
        return block;
    }

    private Task NewMachineAsync()
    {
        currentPage = "new"; SelectNavigation(currentPage);
        RenderNewMachinePage(); return Task.CompletedTask;
    }
    private void RenderNewMachinePage()
    {
        BeginPage("New Machine", "Choose an Amiga model. You can customize every setting before starting.");
        page.Children.Add(Button("Back to Machines", async () => { currentPage = "machines"; SelectNavigation(currentPage); await RenderLibraryAsync(); }, "NewMachineBackButton"));
        page.Children.Add(PresetGrid());
    }
    private static int DefaultCompatibility(JsonNode preset) => Math.Min(1, preset.I("compatibilityMax"));

    // Model / configuration / compatibility controls shared by the New Machine and Change Model dialogs.
    private sealed class PresetChooser
    {
        private readonly JsonNode[] presets;
        private readonly ComboBox? models;
        private readonly ComboBox configs = new() { Header = "Configuration", HorizontalAlignment = HorizontalAlignment.Stretch };
        private readonly TextBlock singleConfig = Text("");
        private readonly Slider compatibility = new() { Minimum = 0, Maximum = 1, StepFrequency = 1 };
        private readonly TextBlock compatibilityRange = Muted("");
        private readonly InfoBar rom = new() { IsClosable = false, IsOpen = false };
        public StackPanel Content { get; } = Stack();
        public JsonNode? Preset => models is null ? presets.FirstOrDefault() : models.SelectedIndex >= 0 ? presets[models.SelectedIndex] : null;
        public int Config => Math.Max(0, configs.SelectedIndex);
        public int Compatibility => (int)compatibility.Value;

        public PresetChooser(JsonNode[] presets, bool chooseModel, string? selectedId)
        {
            this.presets = presets;
            if (chooseModel)
            {
                models = new ComboBox { Header = "Model", PlaceholderText = "Choose a model", HorizontalAlignment = HorizontalAlignment.Stretch };
                AutomationProperties.SetAutomationId(models, "ChangeModelPicker");
                foreach (var preset in presets) models.Items.Add(preset.S("label"));
                Content.Children.Add(models);
                models.SelectionChanged += (_, _) => Refresh();
            }
            Content.Children.Add(configs); Content.Children.Add(singleConfig); Content.Children.Add(rom);
            Content.Children.Add(compatibility); Content.Children.Add(compatibilityRange);
            configs.SelectionChanged += (_, _) => UpdateRom();
            compatibility.ValueChanged += (_, _) => UpdateCompatibility();
            if (models is null) Refresh();
            else
            {
                int index = Array.FindIndex(presets, preset => preset.S("id") == selectedId);
                if (index >= 0) models.SelectedIndex = index; else Refresh();
            }
        }

        private void Refresh()
        {
            var preset = Preset;
            var list = preset?.Items("configs").ToList() ?? [];
            configs.Items.Clear();
            foreach (var config in list) configs.Items.Add(config.S("label") + (config.B("romInstalled") ? " · ROM installed" : " · ROM not installed"));
            if (list.Count > 0) configs.SelectedIndex = 0;
            configs.Visibility = list.Count > 1 ? Visibility.Visible : Visibility.Collapsed;
            singleConfig.Text = list.Count == 1 ? list[0].S("label") : "";
            singleConfig.Visibility = list.Count == 1 ? Visibility.Visible : Visibility.Collapsed;
            int maximum = preset?.I("compatibilityMax") ?? 0;
            compatibility.Maximum = Math.Max(1, maximum);
            compatibility.Value = preset is null ? 0 : DefaultCompatibility(preset);
            compatibility.Visibility = compatibilityRange.Visibility = maximum > 0 ? Visibility.Visible : Visibility.Collapsed;
            UpdateRom(); UpdateCompatibility();
        }
        private void UpdateRom()
        {
            var preset = Preset;
            rom.IsOpen = preset is not null;
            if (preset is null) return;
            bool ready = preset.Items("configs").ElementAtOrDefault(Config)?.B("romInstalled") == true;
            rom.Severity = ready ? InfoBarSeverity.Success : InfoBarSeverity.Warning;
            rom.Title = ready ? "Kickstart ROM installed" : "Kickstart ROM not installed";
            rom.Message = ready ? "This configuration is ready to start." : "Import a matching ROM under Kickstart ROMs, or choose one in the machine's Hardware settings.";
        }
        private void UpdateCompatibility()
        {
            var preset = Preset;
            if (preset is null) { compatibility.Header = "Compatibility"; return; }
            var labels = preset.Items("compatibilityLabels").Select(item => item.ToString()).ToArray();
            compatibility.Header = "Compatibility: " + (labels.ElementAtOrDefault((int)compatibility.Value) ?? compatibility.Value.ToString("0"));
            compatibilityRange.Text = labels.Length > 1 ? $"From {labels[0]} to {labels[^1]}." : "";
        }
    }

    private async Task ChoosePresetAsync(JsonNode preset)
    {
        var chooser = new PresetChooser([preset], false, null);
        chooser.Content.Children.Add(Muted("Every setting can be changed after the machine is created."));
        if (await DialogAsync("New " + preset.S("label"), chooser.Content, "Create", cancel: "Cancel") != ContentDialogResult.Primary) return;
        await OpenEditorAsync(new { preset = preset.S("id"), config = chooser.Config, compatibility = chooser.Compatibility }, name: preset.S("label"));
    }

    // ---- Machine file operations -------------------------------------------

    private static TextBox NameBox(string header, string text, string id)
    {
        var box = new TextBox { Header = header, Text = text, MinWidth = 320 };
        AutomationProperties.SetAutomationId(box, id);
        box.Loaded += (_, _) => { box.Focus(FocusState.Programmatic); box.SelectAll(); };
        return box;
    }
    private static string MachineFileName(string input)
    {
        string stem = input.Trim();
        if (stem.EndsWith(".uae", StringComparison.OrdinalIgnoreCase)) stem = stem[..^4].TrimEnd();
        if (stem.Length == 0) throw new InvalidOperationException("Enter a name for the machine.");
        if (stem.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) throw new InvalidOperationException("A machine name can't contain \\ / : * ? \" < > |");
        if (stem.EndsWith('.')) throw new InvalidOperationException("A machine name can't end with a period.");
        return stem + ".uae";
    }

    private async Task RenameMachineAsync(JsonNode machine)
    {
        string source = machine.S("path");
        var name = NameBox("Machine name", Path.GetFileNameWithoutExtension(source), "RenameMachineName");
        var result = await DialogAsync("Rename machine", Stack(name, Muted("Renames the machine's file in your Library. Its settings and disk images are not changed.")), "Rename", cancel: "Cancel", primaryAction: async () =>
        {
            string target = Path.Combine(Path.GetDirectoryName(source)!, MachineFileName(name.Text));
            if (!string.Equals(target, source, StringComparison.OrdinalIgnoreCase) && File.Exists(target))
                throw new InvalidOperationException($"A machine named “{Path.GetFileNameWithoutExtension(target)}” already exists. Choose a different name.");
            if (string.Equals(target, source, StringComparison.Ordinal)) return;
            library = await Core.CallAsync("configuration_rename", new { path = source, newPath = target });
            currentPath = library.S("currentConfiguration");
        });
        if (result == ContentDialogResult.Primary) await RenderLibraryAsync();
    }
    private async Task DuplicateMachineAsync(JsonNode machine)
    {
        string source = machine.S("path"), folder = Path.GetDirectoryName(source)!, stem = Path.GetFileNameWithoutExtension(source);
        string suggestion = stem + " copy";
        for (int number = 2; File.Exists(Path.Combine(folder, suggestion + ".uae")); number++) suggestion = $"{stem} copy {number}";
        var name = NameBox("Name for the copy", suggestion, "DuplicateMachineName");
        var result = await DialogAsync("Duplicate machine", Stack(name, Muted("Creates a new machine with the same settings. The original is not changed.")), "Duplicate", cancel: "Cancel", primaryAction: async () =>
        {
            string target = Path.Combine(folder, MachineFileName(name.Text));
            if (File.Exists(target)) throw new InvalidOperationException($"A machine named “{Path.GetFileNameWithoutExtension(target)}” already exists. Choose a different name.");
            await Task.Run(() => File.Copy(source, target, false));
        });
        if (result == ContentDialogResult.Primary) await RenderLibraryAsync();
    }
    private async Task DeleteMachineAsync(JsonNode machine)
    {
        if (await DialogAsync("Delete " + MachineName(machine) + "?", "The configuration file will be moved to the Recycle Bin. Disk images are not deleted.", "Delete Machine", cancel: "Cancel", destructive: true) != ContentDialogResult.Primary) return;
        await Task.Run(() => Microsoft.VisualBasic.FileIO.FileSystem.DeleteFile(machine.S("path"), Microsoft.VisualBasic.FileIO.UIOption.OnlyErrorDialogs, Microsoft.VisualBasic.FileIO.RecycleOption.SendToRecycleBin));
        await RenderLibraryAsync();
    }
    private async Task PlayFileAsync(string path)
    {
        if (session && !await LeaveSessionAsync()) return;
        await Core.ActionAsync("quick_play", path);
        resumedUnsavedPath = null;
        committedApply = Math.Max(committedApply, 2);
        await Core.CallAsync("start", new { apply = 2 });
        await WaitForRunningAsync();
        committedApply = 0;
        currentPage = "machines"; SetNavigation(false); await RenderLibraryAsync();
    }

    // ---- Media --------------------------------------------------------------

    private async Task RenderMediaAsync()
    {
        if (currentPage == "recent") { RenderRecent(); return; }
        int kind = currentPage == "floppies" ? 0 : currentPage == "cds" ? 1 : 2;
        int folderId = kind == 0 ? 1 : kind == 1 ? 2 : 7;
        string noun = kind == 0 ? "floppy disk images" : kind == 1 ? "CD images" : "WHDLoad archives";
        string folder = Folder(folderId);
        page.Children.Add(Card(FolderRow(FolderNode(folderId), RenderLibraryAsync, ImportMediaAsync)));
        if (folder.Length == 0)
        {
            page.Children.Add(Card(Stack(SectionTitle("Choose a folder"), Text($"Pick the folder where your {noun} are kept, or play a file directly with Play a File."))));
            return;
        }
        string[] extensions = kind == 0 ? [".adf", ".adz", ".dms", ".ipf", ".zip", ".7z"] : kind == 1 ? [".cue", ".iso", ".chd"] : [".lha"];
        string search = query;
        var files = await Task.Run(() => Directory.Exists(folder)
            ? Directory.EnumerateFiles(folder, "*", new EnumerationOptions { RecurseSubdirectories = true, IgnoreInaccessible = true, AttributesToSkip = FileAttributes.Hidden | FileAttributes.System | FileAttributes.ReparsePoint })
                .Where(path => extensions.Contains(Path.GetExtension(path).ToLowerInvariant()))
                .Where(path => search.Length == 0 || FieldsMatch(search, Path.GetRelativePath(folder, path)))
                .OrderBy(Path.GetFileName, StringComparer.CurrentCultureIgnoreCase).ToList()
            : []);
        if (files.Count == 0)
        {
            if (query.Length > 0) page.Children.Add(NoResultsCard("this folder"));
            else page.Children.Add(Card(Stack(SectionTitle($"No {noun} found"), Text(Directory.Exists(folder) ? "Import files into this folder, choose a different folder, or play a file directly." : "The folder doesn't exist yet. Import files to create it, or choose a different folder."))));
            return;
        }
        page.Children.Add(Muted(query.Length > 0 ? $"{files.Count} {noun} match “{query.Trim()}”" : $"{files.Count} {noun}"));
        // The visible window resets whenever the page or the search changes; Show More / Show All reach every result.
        string key = currentPage + "|" + query;
        if (key != mediaKey) { mediaKey = key; mediaShown = MediaLimit; }
        foreach (string path in files.Take(mediaShown)) RenderMediaCard(path, kind, false, folder);
        if (files.Count > mediaShown)
        {
            int remaining = files.Count - mediaShown;
            page.Children.Add(Muted($"Showing {mediaShown} of {files.Count}."));
            page.Children.Add(Horizontal(
                Button($"Show {Math.Min(MediaLimit, remaining)} More", async () => { mediaShown += MediaLimit; await RenderLibraryAsync(); }, "MediaShowMoreButton"),
                Button($"Show All {files.Count}", async () => { mediaShown = files.Count; await RenderLibraryAsync(); }, "MediaShowAllButton")));
        }
    }
    private void RenderRecent()
    {
        (string Key, string Title)[] lists = [("recentFloppies", "Floppy disks"), ("recentCds", "CD images"), ("recentWhdload", "WHDLoad archives")];
        int total = 0, shown = 0;
        for (int list = 0; list < lists.Length; list++)
        {
            var all = library.Items(lists[list].Key).ToList(); total += all.Count;
            var items = all.Where(item => query.Length == 0 || FieldsMatch(query, item.S("name"), item.S("path"))).ToList();
            if (items.Count == 0) continue;
            shown += items.Count;
            page.Children.Add(SectionTitle(lists[list].Title));
            foreach (var item in items) RenderMediaCard(item.S("path"), list, true, null);
        }
        if (total == 0)
            page.Children.Add(Card(Stack(SectionTitle("Nothing played recently"), Text("Disks, CDs and WHDLoad archives you play show up here so you can start them again quickly."),
                Horizontal(Button("Browse Floppy Disks", () => NavigateAsync("floppies")), Button("Browse CD Images", () => NavigateAsync("cds"))))));
        else if (shown == 0) page.Children.Add(NoResultsCard("recent media"));
    }
    private void RenderMediaCard(string path, int kind, bool recent, string? root)
    {
        bool exists = File.Exists(path);
        string file = Path.GetFileName(path);
        var details = new StackPanel { Spacing = 2 };
        var title = Styled(file, "BodyStrongTextBlockStyle"); title.TextWrapping = TextWrapping.NoWrap; title.TextTrimming = TextTrimming.CharacterEllipsis;
        ToolTipService.SetToolTip(title, path);
        details.Children.Add(title);
        string location = root is null ? Path.GetDirectoryName(path) ?? "" : Path.GetDirectoryName(Path.GetRelativePath(root, path)) ?? "";
        if (location.Length > 0)
        {
            var place = Muted(location); place.TextWrapping = TextWrapping.NoWrap; place.TextTrimming = TextTrimming.CharacterEllipsis;
            ToolTipService.SetToolTip(place, path);
            details.Children.Add(place);
        }
        if (!exists) details.Children.Add(ThemedText("File not found. It may have been moved or deleted.", "SystemFillColorCriticalBrush"));

        var play = Button("Play", () => PlayFileAsync(path)); play.IsEnabled = exists;
        AutomationProperties.SetName(play, "Play " + file);
        var actions = Actions(play);
        if (kind < 2)
        {
            var inMachine = Button("Play in Machine…", () => PlayInMachineAsync(path, kind)); inMachine.IsEnabled = exists;
            AutomationProperties.SetName(inMachine, "Play in Machine… " + file);
            actions.Children.Add(inMachine);
        }
        var menu = new MenuFlyout();
        if (Emulating && kind < 2 && exists)
            menu.Items.Add(MenuAction(kind == 0 ? "Insert into DF0:" : "Insert CD", Symbol.Add, () => kind == 0 ? Core.ActionAsync("insert_floppy", path, drive: 0) : InsertCdAsync(path)));
        if (exists) menu.Items.Add(MenuAction("Show in File Explorer", Symbol.Folder, () => { RevealInExplorer(path); return Task.CompletedTask; }));
        if (recent) menu.Items.Add(MenuAction("Remove from Recent", Symbol.Delete, async () => { library = await Core.CallAsync("recent_remove", new { list = kind, path }); await RenderLibraryAsync(); }));
        if (menu.Items.Count > 0) actions.Children.Add(MoreButton(menu, "More actions for " + file, "More actions"));
        page.Children.Add(Card(ActionRow(details, actions)));
    }
    private async Task PlayInMachineAsync(string path, int kind)
    {
        var machine = new ComboBox { Header = "Saved machine", HorizontalAlignment = HorizontalAlignment.Stretch };
        string current = library.S("currentConfiguration");
        foreach (var entry in library.Items("machines")) machine.Items.Add(new ComboBoxItem { Content = MachineName(entry), Tag = entry.S("path") });
        if (machine.Items.Count == 0) { ShowError("Save a machine first to play media in it."); return; }
        machine.SelectedItem = machine.Items.OfType<ComboBoxItem>().FirstOrDefault(item => string.Equals((string)item.Tag, current, StringComparison.OrdinalIgnoreCase)) ?? machine.Items[0];
        var note = Muted(kind == 0 ? "Puts the disk in DF0: and starts the machine. The saved machine is only changed if you choose to save." : "Puts the CD in the machine's CD drive and starts it. The saved machine is only changed if you choose to save.");
        if (await DialogAsync("Play " + Path.GetFileName(path), Stack(machine, note), "Play", cancel: "Cancel") != ContentDialogResult.Primary) return;
        await OpenEditorAsync(new { path = (string)((ComboBoxItem)machine.SelectedItem).Tag });
        await SetOptionAsync(kind == 0 ? "floppy0" : "cdimage0", path);
        await StartAsync();
    }
    private async Task ImportMediaAsync(string folder)
    {
        if (string.IsNullOrEmpty(folder)) throw new InvalidOperationException("Choose the destination folder first.");
        var files = await PickFilesAsync();
        await Task.Run(() => { Directory.CreateDirectory(folder); foreach (string source in files) { string destination = Path.Combine(folder, Path.GetFileName(source)); if (!string.Equals(source, destination, StringComparison.OrdinalIgnoreCase)) File.Copy(source, destination, false); } });
        await RenderLibraryAsync();
    }

    // ---- Kickstart ROMs -----------------------------------------------------

    private void RenderRoms()
    {
        page.Children.Add(Text("Kickstart ROMs are copyrighted. Import ROMs you own, or use the built-in AROS replacement."));
        page.Children.Add(Card(FolderRow(FolderNode(0), RenderLibraryAsync)));
        page.Children.Add(Horizontal(Button("Import ROMs…", () => ImportRomsAsync(false)), Button("Import Folder…", () => ImportRomsAsync(true)), Button("Add rom.key…", async () =>
        {
            if (await PickFileAsync() is { } path) { library = await Core.CallAsync("rom_key", new { path }); await RenderLibraryAsync(); }
        }), Button("Rescan", async () => { library = await Core.CallAsync("rescan_roms"); catalog = await Core.CallAsync("catalog"); await RenderLibraryAsync(); })));

        var roms = library.Items("roms").ToList();
        var shownRoms = roms.Where(rom => query.Length == 0 || FieldsMatch(query, rom.S("name"), rom.S("path"))).ToList();
        var presets = catalog.Items("presets").Where(preset => query.Length == 0 || FieldsMatch(query, preset.S("label"), string.Join(" ", preset.Items("configs").Select(config => config.S("label"))))).ToList();
        if (roms.Count == 0)
            page.Children.Add(Card(Stack(SectionTitle("No Kickstart ROMs recognized"), Text("Import ROM files you own, import a folder that contains them, or choose the ROM folder above and rescan."))));
        else if (shownRoms.Count > 0)
        {
            page.Children.Add(SectionTitle(query.Length > 0 ? $"Installed ROMs ({shownRoms.Count} of {roms.Count})" : $"Installed ROMs ({roms.Count})"));
            foreach (var rom in shownRoms)
            {
                var details = new StackPanel { Spacing = 2 };
                details.Children.Add(Styled(rom.S("name"), "BodyStrongTextBlockStyle"));
                var location = Muted(rom.S("path")); location.TextWrapping = TextWrapping.NoWrap; location.TextTrimming = TextTrimming.CharacterEllipsis;
                ToolTipService.SetToolTip(location, rom.S("path"));
                details.Children.Add(location);
                page.Children.Add(Card(details));
            }
        }
        if (query.Length > 0 && shownRoms.Count == 0 && presets.Count == 0) { page.Children.Add(NoResultsCard("Kickstart ROMs or machine models")); return; }
        if (presets.Count == 0) return;

        var all = catalog.Items("presets").ToList();
        int ready = all.Count(preset => preset.Items("configs").Any() && preset.Items("configs").All(config => config.B("romInstalled")));
        page.Children.Add(SectionTitle("Which machines can start"));
        if (query.Length == 0) page.Children.Add(Muted($"{ready} of {all.Count} models have a ROM for every configuration."));
        var grid = new Grid { ColumnSpacing = 24, RowSpacing = 8 };
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        for (int row = 0; row < presets.Count; row++)
        {
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            var configs = presets[row].Items("configs").ToList();
            int installed = configs.Count(config => config.B("romInstalled"));
            var label = Text(presets[row].S("label"));
            var verdict = installed == configs.Count && installed > 0 ? ThemedText("Ready", "SystemFillColorSuccessBrush", 14)
                : installed > 0 ? ThemedText($"{installed} of {configs.Count} configurations ready", "SystemFillColorCautionBrush", 14)
                : ThemedText("ROM needed", "SystemFillColorCautionBrush", 14);
            Grid.SetRow(label, row); Grid.SetRow(verdict, row); Grid.SetColumn(verdict, 1);
            grid.Children.Add(label); grid.Children.Add(verdict);
        }
        page.Children.Add(Card(grid));
    }
    private async Task ImportRomsAsync(bool folder)
    {
        var paths = new List<string>();
        if (folder)
        {
            if (await PickFolderAsync() is not { } source) return;
            paths = await Task.Run(() => Directory.EnumerateFiles(source, "*", new EnumerationOptions { RecurseSubdirectories = true, IgnoreInaccessible = true, AttributesToSkip = FileAttributes.ReparsePoint }).Where(IsRom).ToList());
        }
        else
        {
            paths = (await PickFilesAsync()).ToList();
        }
        string destination = Folder(0);
        if (string.IsNullOrEmpty(destination)) throw new InvalidOperationException("Choose the ROMs folder first.");
        await Task.Run(() =>
        {
            Directory.CreateDirectory(destination);
            foreach (string path in paths)
            {
                string target = Path.Combine(destination, Path.GetFileName(path));
                if (!string.Equals(path, target, StringComparison.OrdinalIgnoreCase)) File.Copy(path, target, false);
            }
        });
        foreach (string path in paths.Where(path => Path.GetFileName(path).Equals("rom.key", StringComparison.OrdinalIgnoreCase))) await Core.CallAsync("rom_key", new { path = Path.Combine(destination, Path.GetFileName(path)) });
        library = await Core.CallAsync("rescan_roms"); catalog = await Core.CallAsync("catalog"); await RenderLibraryAsync();
    }
    private static bool IsRom(string path)
    {
        string extension = Path.GetExtension(path).TrimStart('.').ToLowerInvariant();
        return new FileInfo(path).Length <= 2 * 1024 * 1024 && (Path.GetFileName(path).Equals("rom.key", StringComparison.OrdinalIgnoreCase) || new[] { "rom", "bin", "a1000", "a500", "a600", "a1200", "a3000", "a4000", "cdtv", "cd32", "roz" }.Contains(extension) || (extension.Length >= 2 && extension[0] == 'u' && char.IsDigit(extension[1])));
    }

    // ---- App settings -------------------------------------------------------

    private async Task RenderAppSettingsAsync()
    {
        // Rebuilding drops every row; a rejected draft must be resolved while its row still shows the error.
        if (!await FlushDraftsAsync()) return;
        host = await Core.CallAsync("host");
        downloadStatus = downloadDetail = null; installDownload = cancelDownload = null; downloadProgress = null;
        BeginPage("App Settings", "Settings for Amiberry itself, saved to amiberry.conf as you change them. A machine's own settings are edited in that machine.", true, "Search app settings");
        bool any = false;
        if (query.Length == 0 || FieldsMatch(query, "Library folders", "folder path directory", string.Join(" ", library.Items("folders").Select(item => item.S("label") + " " + item.S("path")))))
        {
            any = true;
            var folders = new StackPanel { Spacing = 12 };
            bool first = true;
            foreach (var folder in library.Items("folders"))
            {
                var row = FolderRow(folder, RenderAppSettingsAsync);
                folders.Children.Add(first ? row : Divided(row)); first = false;
            }
            page.Children.Add(Disclosure("app:Library Folders", "Library Folders", folders, true));
        }
        foreach (var group in hostDescriptors.Values.GroupBy(item => item.S("group")))
        {
            foreach (var section in AppSections(group.Key, group))
            {
                var options = section.Options.Where(option => query.Length == 0 || FieldsMatch(query, section.Title, option.S("label"), option.S("id"), option.S("help"))).ToList();
                if (options.Count == 0) continue;
                any = true;
                var content = new StackPanel { Spacing = 10 };
                foreach (var descriptor in options)
                {
                    var row = MakeSettingRow(descriptor, HostValue(descriptor.S("id")), async value =>
                    {
                        host = await Core.CallAsync("host_set", new { id = descriptor.S("id"), value });
                        foreach (var visible in rows.Values) if (hostDescriptors.ContainsKey(visible.Id)) visible.Update(HostValue(visible.Id));
                    }, "host:");
                    content.Children.Add(row.Element);
                }
                page.Children.Add(Disclosure("app:" + section.Title, query.Length == 0 ? section.Title : $"{section.Title} ({options.Count})", content, query.Length > 0 || (!section.Collapsed && options.Count <= 6)));
            }
        }
        if (query.Length == 0 || FieldsMatch(query, "WHDBoot WHDLoad JST download install update boot files tools"))
        {
            any = true;
            page.Children.Add(SectionTitle("WHDBoot tools"));
            downloadStatus = Text("");
            AutomationProperties.SetLiveSetting(downloadStatus, AutomationLiveSetting.Polite);
            downloadDetail = Muted("");
            downloadProgress = new ProgressBar { Minimum = 0, Maximum = 100, Visibility = Visibility.Collapsed };
            installDownload = Button("Install", async () => { await Core.CallAsync("whdboot_download"); await RefreshDownloadAsync(); }, "WhdbootInstallButton");
            cancelDownload = Button("Cancel Download", async () => { await Core.CallAsync("whdboot_cancel"); await RefreshDownloadAsync(); }, "WhdbootCancelButton");
            cancelDownload.Visibility = Visibility.Collapsed;
            page.Children.Add(Card(Stack(Muted("Boot support files for WHDLoad games, downloaded and updated by Amiberry."), downloadStatus, downloadDetail, downloadProgress, Horizontal(installDownload, cancelDownload))));
            await RefreshDownloadAsync();
        }
        if (!any) page.Children.Add(NoResultsCard("app settings"));
    }
    private JsonNode HostValue(string id) => host.Items("values").FirstOrDefault(item => item.S("id") == id) ?? new JsonObject { ["id"] = id, ["available"] = false };
    private static string HostGroupTitle(string group) => group switch
    {
        "global_input" => "Input", "global_display" => "Display", "global_sound" => "Sound", "global_whdload" => "WHDLoad Games", "global_touch" => "On-Screen Controls", "global_visual" => "Appearance", "global_paths" => "Folders & Logging", "global_updates" => "Updates", "global_advanced" => "Advanced", _ => group.Replace('_', ' ')
    };
    private static string FormatBytes(double bytes)
    {
        string[] units = ["B", "KB", "MB", "GB"];
        int unit = 0;
        while (bytes >= 1024 && unit < units.Length - 1) { bytes /= 1024; unit++; }
        return (unit == 0 ? bytes.ToString("0") : bytes.ToString("0.#")) + " " + units[unit];
    }
    private async Task RefreshDownloadAsync()
    {
        if (currentPage != "app" || downloadStatus is null) return;
        var result = await Core.CallAsync("whdboot_status");
        var statusText = downloadStatus; var detailText = downloadDetail; var progressBar = downloadProgress; var install = installDownload; var cancel = cancelDownload;
        if (statusText is null || detailText is null) return;
        (string Label, string Key)[] components = [("WHDLoad", "whdload"), ("JST", "jst"), ("RTB", "rtb"), ("Host quit", "hostQuit"), ("Boot data", "bootData"), ("Database", "database"), ("Libraries", "libraries")];
        var missing = components.Where(item => !result.B(item.Key)).Select(item => item.Label).ToList();
        bool downloading = result.B("downloading");
        double done = result.D("downloaded"), total = result.D("totalBytes");
        string headline, detail;
        if (downloading)
        {
            headline = result.S("label").Length > 0 ? $"Downloading {result.S("label")}…" : "Starting download…";
            var parts = new List<string>();
            if (result.I("step") > 0 && result.I("totalSteps") > 0) parts.Add($"Step {result.I("step")} of {result.I("totalSteps")}");
            if (total > 0) parts.Add($"{FormatBytes(done)} of {FormatBytes(total)}");
            else if (done > 0) parts.Add(FormatBytes(done));
            detail = string.Join(" · ", parts);
        }
        else
        {
            headline = missing.Count == 0 ? "WHDBoot tools are installed." : missing.Count == components.Length ? "WHDBoot tools are not installed." : "WHDBoot tools are incomplete.";
            var parts = new List<string>();
            if (missing.Count > 0 && missing.Count < components.Length) parts.Add("Missing: " + string.Join(", ", missing) + ".");
            string completion = result.I("result") switch { 1 => "The last download couldn't fetch every file.", 2 => "The last download failed.", 3 => "The last download was cancelled.", 4 => "Downloads are unavailable.", 5 => "Another download or emulation is running.", _ => "" };
            if (completion.Length > 0) parts.Add(completion);
            if (!result.B("downloadsAvailable")) parts.Add("Downloads aren't available in this build.");
            if (result.B("emulationRunning")) parts.Add("Stop emulation before installing updates.");
            detail = string.Join(" ", parts);
        }
        statusText.Text = headline;
        detailText.Text = detail; detailText.Visibility = detail.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        if (install is not null)
        {
            install.Content = missing.Count == 0 ? "Update" : "Install";
            install.IsEnabled = result.B("downloadsAvailable") && !result.B("emulationRunning") && !downloading;
        }
        if (cancel is not null) cancel.Visibility = downloading ? Visibility.Visible : Visibility.Collapsed;
        if (progressBar is not null)
        {
            progressBar.Visibility = downloading ? Visibility.Visible : Visibility.Collapsed;
            progressBar.IsIndeterminate = downloading && total <= 0;
            progressBar.Value = total > 0 ? 100 * done / total : 0;
        }
    }
}
