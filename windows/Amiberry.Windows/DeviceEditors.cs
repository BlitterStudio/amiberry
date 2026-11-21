using System.Text.Json.Nodes;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Text;
using global::Windows.ApplicationModel.DataTransfer;

namespace Amiberry.Windows;

public sealed partial class MainWindow
{
    private readonly Dictionary<(int Port, bool Hotkey, string Id), ComboBox> mappings = [];
    private readonly Dictionary<(int Port, bool Hotkey, string Id), long> mappingRevisions = [];
    private readonly Dictionary<(int Port, bool Hotkey, string Id), (TextBlock Label, TextBlock Status)> mappingLabels = [];
    private readonly Dictionary<int, PortView> mappingPorts = [];
    private JsonNode latestControls = new JsonObject();
    private bool updatingMappings;
    private long mappingRevision;
    private sealed class PortView(TextBlock device, TextBlock empty, StackPanel mappings)
    {
        public TextBlock Device { get; } = device;
        public TextBlock Empty { get; } = empty;
        public StackPanel Mappings { get; } = mappings;
        public bool Built { get; set; }
    }

    // ---- shared building blocks -------------------------------------------------------------

    // Secondary text whose brush is a theme resource, so it follows live theme changes.
    private static TextBlock DeviceCaption(string text, string brush = "TextFillColorSecondaryBrush")
    {
        var block = (TextBlock)XamlReader.Load("<TextBlock xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Foreground=\"{ThemeResource " + brush + "}\" TextWrapping=\"Wrap\"/>");
        block.Text = text; block.FontSize = 12;
        return block;
    }

    private MenuFlyoutItem DeviceMenuItem(string text, Func<Task> action, string? id = null)
    {
        var item = new MenuFlyoutItem { Text = text };
        if (id is not null) AutomationProperties.SetAutomationId(item, id);
        item.Click += async (_, _) => await RunAsync(action);
        return item;
    }

    // Lays items out in as many equal columns as fit at minColumn each, wrapping to further rows.
    private static Grid ResponsiveGrid(double minColumn, params UIElement[] items)
    {
        var grid = new Grid { ColumnSpacing = 12, RowSpacing = 10 };
        foreach (var item in items) grid.Children.Add(item);
        int columns = 0;
        void Arrange(double width)
        {
            if (width <= 0) return;
            int next = Math.Clamp((int)((width + grid.ColumnSpacing) / (minColumn + grid.ColumnSpacing)), 1, Math.Max(1, items.Length));
            if (next == columns) return;
            columns = next;
            grid.ColumnDefinitions.Clear(); grid.RowDefinitions.Clear();
            for (int c = 0; c < columns; c++) grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            for (int r = 0; r < (items.Length + columns - 1) / columns; r++) grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            for (int i = 0; i < items.Length; i++) { Grid.SetRow((FrameworkElement)items[i], i / columns); Grid.SetColumn((FrameworkElement)items[i], i % columns); }
        }
        Arrange(minColumn * items.Length + grid.ColumnSpacing * items.Length);
        grid.SizeChanged += (_, e) => Arrange(e.NewSize.Width);
        return grid;
    }

    // Remembers an Expander's state across page renders; onOpen runs once content is first needed.
    private void TrackExpander(Expander expander, string key, bool defaultOpen, Action? onOpen = null)
    {
        bool open = defaultOpen ? !collapsedSections.Contains(key) : expanded.Contains(key);
        expander.Expanding += (_, _) => { expanded.Add(key); collapsedSections.Remove(key); onOpen?.Invoke(); };
        expander.Collapsed += (_, _) => { expanded.Remove(key); collapsedSections.Add(key); };
        expander.IsExpanded = open;
        if (open) onOpen?.Invoke();
    }

    private static void CopyToClipboard(string value)
    {
        var package = new DataPackage();
        package.SetText(value);
        Clipboard.SetContent(package);
    }

    // Windows' "Copy as path" wraps the path in quotes, which the mount entry cannot hold.
    private static string CleanPath(string value)
    {
        value = value.Trim();
        return value.Length >= 2 && value[0] == '"' && value[^1] == '"' ? value[1..^1].Trim() : value;
    }

    private double DialogWidth() => Math.Clamp(((FrameworkElement)Content).ActualWidth - 120, 300, 480);
    private double DialogBodyHeight() => Math.Clamp(((FrameworkElement)Content).ActualHeight - 260, 240, 640);

    // ---- hard drives and mounts -------------------------------------------------------------

    private static string MountKindLabel(string kind) => kind switch { "dir" => "Folder or archive", "hdf" => "Hardfile", "cd" => "CD drive", "tape" => "Tape drive", _ => kind };

    private async Task AddMountsAsync()
    {
        var controllers = new Dictionary<string, List<JsonNode>>();
        var mounts = snapshot.Items("mounts").ToList();
        foreach (string kind in mounts.Select(mount => mount["fields"].S("kind")).Where(kind => kind is "hdf" or "cd" or "tape").Distinct())
            controllers[kind] = (await Core.CallAsync("mount_controllers", new { kind })).Items().ToList();

        var content = new StackPanel { Spacing = 12 };
        var add = new DropDownButton { Content = "Add Drive", HorizontalAlignment = HorizontalAlignment.Left };
        AutomationProperties.SetAutomationId(add, "StorageAddDrive");
        ToolTipService.SetToolTip(add, "Add a folder, hardfile or CD drive to this machine");
        var menu = new MenuFlyout();
        menu.Items.Add(DeviceMenuItem("Folder as drive…", () => NewMountAsync("dir", true), "StorageAddFolder"));
        menu.Items.Add(DeviceMenuItem("Hardfile or disk image…", () => NewMountAsync("hdf", false), "StorageAddHardfile"));
        menu.Items.Add(DeviceMenuItem("Create new hardfile…", CreateHardfileAsync, "StorageCreateHardfile"));
        menu.Items.Add(DeviceMenuItem("CD drive", () => NewMountAsync("cd", false), "StorageAddCd"));
        menu.Items.Add(new MenuFlyoutSeparator());
        var more = new MenuFlyoutSubItem { Text = "Other drive types" };
        more.Items.Add(DeviceMenuItem("Archive file as drive…", () => NewMountAsync("dir", false), "StorageAddArchive"));
        more.Items.Add(DeviceMenuItem("Tape drive from file…", () => NewMountAsync("tape", false), "StorageAddTapeFile"));
        more.Items.Add(DeviceMenuItem("Tape drive from folder…", () => NewMountAsync("tape", true), "StorageAddTapeFolder"));
        menu.Items.Add(more);
        add.Flyout = menu;
        content.Children.Add(add);

        if (mounts.Count == 0)
        {
            content.Children.Add(DeviceCaption("No drives are attached. Add a folder, hardfile or CD drive to make it appear in the Amiga."));
        }
        for (int index = 0; index < mounts.Count; index++)
            content.Children.Add(MountCard(mounts[index], index, controllers));
        var section = Disclosure(currentPage + ":mounts", "Hard Drives", content, true);
        AutomationProperties.SetAutomationId(section, "StorageHardDrives");
        page.Children.Add(section);
    }

    // Summarises a mount from the typed fields the settings layer reports; the serialized entry is
    // only shown when the layer could not classify it, and is otherwise one menu command away.
    private UIElement MountCard(JsonNode mount, int mountIndex, Dictionary<string, List<JsonNode>> controllers)
    {
        var fields = mount["fields"]!;
        string kind = fields.S("kind"), path = fields.S("path"), device = fields.S("device"), volume = fields.S("volume");
        bool known = kind is "dir" or "hdf" or "cd" or "tape";
        string kindLabel = known ? MountKindLabel(kind) : "Unrecognised drive";
        string name = kind is "dir" or "hdf" && device.Length > 0 ? device + ":" : kindLabel;

        var headline = new TextBlock { TextWrapping = TextWrapping.Wrap };
        headline.Inlines.Add(new Run { Text = name, FontWeight = FontWeights.SemiBold });
        if (volume.Length > 0) headline.Inlines.Add(new Run { Text = "  " + volume });

        var details = new List<string>();
        if (name != kindLabel) details.Add(kindLabel);
        if (fields.B("read_only")) details.Add("Read-only");
        if (kind is "dir" or "hdf")
        {
            int priority = fields.I("boot_priority");
            details.Add(priority == -128 ? "Not bootable" : priority == -129 ? "Not mounted automatically" : "Boot priority " + priority);
        }
        if (kind == "hdf") details.Add(fields.I("sectors") == 0 ? "Geometry from RDB" : fields.I("sectors") + " sectors · " + fields.I("surfaces") + " surfaces");
        if (kind is "hdf" or "cd" or "tape" && fields.S("controller").Length > 0)
        {
            string controllerText = fields.S("controller"), unitText = fields.S("unit");
            if (controllers.TryGetValue(kind, out var list) && list.FirstOrDefault(item => item.S("id") == controllerText) is { } controller)
            {
                controllerText = controller.S("label");
                if (controller.Items("units").FirstOrDefault(item => item.S("value") == unitText) is { } unit) controllerText += " · " + unit.S("label");
            }
            details.Add(controllerText);
        }

        var summary = new StackPanel { Spacing = 2 };
        summary.Children.Add(headline);
        if (details.Count > 0) summary.Children.Add(DeviceCaption(string.Join(" · ", details)));
        string location = known ? path.Length > 0 ? path : kind == "cd" ? "Empty drive (no disc image)" : "" : mount.S("entry");
        if (location.Length > 0)
        {
            var locationText = DeviceCaption(location);
            locationText.IsTextSelectionEnabled = true;
            if (!known) locationText.FontFamily = new Microsoft.UI.Xaml.Media.FontFamily("Consolas");
            AutomationProperties.SetName(locationText, known ? "Path " + location : "Mount entry " + location);
            summary.Children.Add(locationText);
        }

        string title = name + (volume.Length > 0 ? " " + volume : "");
        var edit = Button("Edit…", () => EditMountAsync(mount.DeepClone(), mountIndex), "MountEdit_" + mountIndex);
        AutomationProperties.SetHelpText(edit, "Edit " + title);
        var remove = Button("Remove", async () =>
        {
            if (!await FlushDraftsAsync()) return;
            ApplySnapshot(await Core.CallAsync("mount_remove", new { index = mountIndex })); await RenderEditorAsync();
        }, "MountRemove_" + mountIndex);
        AutomationProperties.SetHelpText(remove, "Remove " + title + " from this machine");
        var overflow = new Button { Content = new FontIcon { Glyph = "\uE712", FontSize = 14 }, Padding = new Thickness(8, 5, 8, 5) };
        AutomationProperties.SetName(overflow, "More actions for " + title);
        AutomationProperties.SetAutomationId(overflow, "MountMore_" + mountIndex);
        ToolTipService.SetToolTip(overflow, "More actions");
        var actionsMenu = new MenuFlyout();
        if (path.Length > 0)
            actionsMenu.Items.Add(DeviceMenuItem("Copy path", () => { CopyToClipboard(path); return Task.CompletedTask; }, "MountCopyPath_" + mountIndex));
        actionsMenu.Items.Add(DeviceMenuItem("Copy configuration entry", () => { CopyToClipboard(mount.S("entry")); return Task.CompletedTask; }, "MountCopyEntry_" + mountIndex));
        overflow.Flyout = actionsMenu;

        var actions = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, VerticalAlignment = VerticalAlignment.Top };
        actions.Children.Add(edit); actions.Children.Add(remove); actions.Children.Add(overflow);
        var grid = new Grid { ColumnSpacing = 12, RowSpacing = 10 };
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        grid.Children.Add(summary);
        grid.Children.Add(actions);
        Grid.SetColumn(actions, 1);
        grid.SizeChanged += (_, e) =>
        {
            bool narrow = e.NewSize.Width < 520;
            Grid.SetRow(actions, narrow ? 1 : 0); Grid.SetColumn(actions, narrow ? 0 : 1); Grid.SetColumnSpan(actions, narrow ? 2 : 1);
            grid.RowSpacing = narrow ? 10 : 0;
        };
        AutomationProperties.SetAutomationId(grid, "Mount_" + mountIndex);
        return Card(grid);
    }

    private async Task NewMountAsync(string kind, bool folder)
    {
        string? path = kind == "cd" ? "" : folder ? await PickFolderAsync() : await PickFileAsync();
        if (path is null) return;
        var mount = await Core.CallAsync("mount_default", new { kind, path });
        await EditMountAsync(mount, null);
    }

    private async Task CreateHardfileAsync()
    {
        var problem = new InfoBar { Severity = InfoBarSeverity.Error, IsOpen = false, IsClosable = true };
        var size = new NumberBox { Header = "Size", Minimum = 1, Value = 512, SmallChange = 64, LargeChange = 512, SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Compact, HorizontalAlignment = HorizontalAlignment.Stretch };
        var units = new ComboBox { Header = "Unit", MinWidth = 96, VerticalAlignment = VerticalAlignment.Bottom };
        units.Items.Add("MiB"); units.Items.Add("GiB"); units.SelectedIndex = 0;
        AutomationProperties.SetAutomationId(size, "HardfileSize");
        AutomationProperties.SetAutomationId(units, "HardfileSizeUnit");
        var sizeRow = new Grid { ColumnSpacing = 8 };
        sizeRow.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        sizeRow.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        sizeRow.Children.Add(size); sizeRow.Children.Add(units); Grid.SetColumn(units, 1);

        var dynamic = new CheckBox { Content = "Dynamic VHD (grows as data is written)", IsChecked = false };
        AutomationProperties.SetAutomationId(dynamic, "HardfileDynamic");
        var location = new TextBox { Header = "Save as", PlaceholderText = "Choose where to create the file", HorizontalAlignment = HorizontalAlignment.Stretch };
        AutomationProperties.SetAutomationId(location, "HardfilePath");
        var choose = new Button { Content = "Browse…", VerticalAlignment = VerticalAlignment.Bottom };
        AutomationProperties.SetAutomationId(choose, "HardfileBrowse");
        AutomationProperties.SetName(choose, "Browse for the new hardfile location");
        choose.Click += async (_, _) =>
        {
            try
            {
                string? picked = await PickFileAsync(true, dynamic.IsChecked == true ? ".vhd" : ".hdf", "HardDisk");
                if (picked is not null) location.Text = picked;
            }
            catch (Exception error) { problem.Message = error.Message; problem.IsOpen = true; }
        };
        // Keep a chosen name but follow the format: a fixed hardfile is .hdf, a dynamic disk .vhd.
        dynamic.Click += (_, _) =>
        {
            string wanted = dynamic.IsChecked == true ? ".vhd" : ".hdf", other = dynamic.IsChecked == true ? ".hdf" : ".vhd";
            if (location.Text.EndsWith(other, StringComparison.OrdinalIgnoreCase)) location.Text = location.Text[..^other.Length] + wanted;
        };
        var locationRow = new Grid { ColumnSpacing = 8 };
        locationRow.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        locationRow.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        locationRow.Children.Add(location); locationRow.Children.Add(choose); Grid.SetColumn(choose, 1);

        var form = Stack(problem, sizeRow, dynamic, locationRow,
            DeviceCaption("A fixed hardfile reserves its full size on disk now. A dynamic VHD only uses the space the Amiga writes."));
        form.MinWidth = DialogWidth();
        string? created = null;
        async Task CreateAsync()
        {
            problem.IsOpen = false;
            string target = CleanPath(location.Text);
            if (!double.IsFinite(size.Value) || size.Value < 1) throw new InvalidOperationException("Enter a size of at least 1 " + (string)units.SelectedItem + ".");
            if (target.Length == 0) throw new InvalidOperationException("Choose where to save the new hardfile.");
            double bytes = size.Value * (units.SelectedIndex == 1 ? 1024.0 * 1024 * 1024 : 1024.0 * 1024);
            if (bytes >= ulong.MaxValue) throw new InvalidOperationException("The size is too large.");
            var result = await Core.CallAsync("hardfile_create", new { path = target, sizeBytes = (ulong)bytes, dynamic = dynamic.IsChecked == true });
            created = result.S("path");
        }
        if (await DialogAsync("Create hardfile", form, "Create", cancel: "Cancel", primaryAction: CreateAsync) != ContentDialogResult.Primary || created is null) return;
        await EditMountAsync(await Core.CallAsync("mount_default", new { kind = "hdf", path = created }), null);
    }

    private async Task EditMountAsync(JsonNode mount, int? index)
    {
        // The page is rebuilt afterwards; a rejected draft must be resolved while its row is still visible.
        if (!await FlushDraftsAsync()) return;
        var fields = mount["fields"]!.AsObject();
        string kind = fields.S("kind");
        var controllerList = kind == "dir" ? new List<JsonNode>() : (await Core.CallAsync("mount_controllers", new { kind })).Items().ToList();
        var editors = new Dictionary<string, Func<string>>();
        var inputs = new Dictionary<string, Control>();
        var problem = new InfoBar { Severity = InfoBarSeverity.Error, IsOpen = false, IsClosable = true };
        var panel = new StackPanel { Spacing = 12, Margin = new Thickness(0, 0, 12, 0), MinWidth = DialogWidth() };
        panel.Children.Add(problem);

        TextBox? Field(string id, string header, string description = "")
        {
            if (!fields.ContainsKey(id)) return null;
            var box = new TextBox { Header = header, Text = fields.S(id), HorizontalAlignment = HorizontalAlignment.Stretch, Description = description.Length > 0 ? description : null };
            AutomationProperties.SetAutomationId(box, "Mount_" + id);
            editors[id] = () => id == "path" ? CleanPath(box.Text) : box.Text.Trim();
            inputs[id] = box;
            return box;
        }
        void Report(Exception error) { problem.Message = error.Message; problem.IsOpen = true; }
        Button BrowseButton(TextBox target, string label, Func<Task<string?>> pick)
        {
            var button = new Button { Content = label, VerticalAlignment = VerticalAlignment.Bottom };
            AutomationProperties.SetAutomationId(button, "MountBrowse");
            button.Click += async (_, _) => { try { if (await pick() is { } picked) target.Text = picked; } catch (Exception error) { Report(error); } };
            return button;
        }

        // Path with its picker(s). Folder-or-file kinds offer both from one Browse menu.
        string pathHeader = kind switch { "dir" => "Folder or archive", "hdf" => "Hardfile or disk image", "cd" => "Disc image (optional)", "tape" => "Tape file or folder", _ => "Path" };
        string pathHelp = kind == "cd" ? "Leave empty to add a drive without a disc." : "";
        var pathBox = Field("path", pathHeader, pathHelp);
        TextBox? firstField = null;
        if (pathBox is not null)
        {
            var row = new Grid { ColumnSpacing = 8 };
            row.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            row.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            row.Children.Add(pathBox);
            FrameworkElement browse;
            if (kind is "dir" or "tape")
            {
                var drop = new DropDownButton { Content = "Browse", VerticalAlignment = VerticalAlignment.Bottom };
                AutomationProperties.SetAutomationId(drop, "MountBrowse");
                var choices = new MenuFlyout();
                var folderItem = new MenuFlyoutItem { Text = "Folder…" };
                folderItem.Click += async (_, _) => { try { if (await PickFolderAsync() is { } picked) pathBox.Text = picked; } catch (Exception error) { Report(error); } };
                var fileItem = new MenuFlyoutItem { Text = kind == "dir" ? "Archive file…" : "File…" };
                fileItem.Click += async (_, _) => { try { if (await PickFileAsync() is { } picked) pathBox.Text = picked; } catch (Exception error) { Report(error); } };
                choices.Items.Add(folderItem); choices.Items.Add(fileItem);
                drop.Flyout = choices; browse = drop;
            }
            else browse = BrowseButton(pathBox, "Browse…", () => PickFileAsync());
            AutomationProperties.SetName(browse, "Browse for " + pathHeader.ToLowerInvariant());
            row.Children.Add(browse); Grid.SetColumn(browse, 1);
            panel.Children.Add(row);
        }

        // Identity: names the Amiga sees.
        var names = new List<UIElement>();
        if (Field("device", "Device name", "The name AmigaOS gives the drive, for example DH0.") is { } deviceBox) { names.Add(deviceBox); firstField = deviceBox; }
        if (Field("volume", "Volume label", "The name shown on the Workbench disk icon.") is { } volumeBox) names.Add(volumeBox);
        if (names.Count > 0) panel.Children.Add(ResponsiveGrid(180, [.. names]));
        if (Field("boot_priority", "Boot priority", "-128 is not bootable. -129 is not mounted automatically.") is { } bootBox) panel.Children.Add(bootBox);
        if (fields.ContainsKey("read_only"))
        {
            var readOnly = new CheckBox { Content = "Read only", IsChecked = fields.S("read_only") == "true" };
            AutomationProperties.SetAutomationId(readOnly, "Mount_read_only");
            editors["read_only"] = () => readOnly.IsChecked == true ? "true" : "false";
            inputs["read_only"] = readOnly;
            panel.Children.Add(readOnly);
        }

        if (kind != "dir")
        {
            var controllers = new ComboBox { Header = "Controller", HorizontalAlignment = HorizontalAlignment.Stretch };
            var units = new ComboBox { Header = "Controller unit", HorizontalAlignment = HorizontalAlignment.Stretch };
            AutomationProperties.SetAutomationId(controllers, "Mount_controller");
            AutomationProperties.SetAutomationId(units, "Mount_unit");
            string currentController = fields.S("controller");
            // A controller the layer cannot offer now stays selectable so applying other edits keeps it.
            if (currentController.Length > 0 && !controllerList.Any(item => item.S("id") == currentController))
                controllers.Items.Add(new ComboBoxItem { Content = currentController + " (not available now)", Tag = new JsonObject { ["id"] = currentController, ["label"] = currentController } });
            foreach (var controller in controllerList) controllers.Items.Add(new ComboBoxItem { Content = controller.S("label"), Tag = controller });
            var capturedUnits = units;
            controllers.SelectionChanged += (_, _) =>
            {
                capturedUnits.Items.Clear();
                if (controllers.SelectedItem is not ComboBoxItem selected) return;
                foreach (var unit in ((JsonNode)selected.Tag).Items("units")) capturedUnits.Items.Add(new ComboBoxItem { Content = unit.S("label"), Tag = unit.S("value") });
                capturedUnits.SelectedItem = capturedUnits.Items.OfType<ComboBoxItem>().FirstOrDefault(item => (string)item.Tag == fields.S("unit"));
                if (capturedUnits.SelectedIndex < 0 && capturedUnits.Items.Count > 0) capturedUnits.SelectedIndex = 0;
            };
            controllers.SelectedItem = controllers.Items.OfType<ComboBoxItem>().FirstOrDefault(item => ((JsonNode)item.Tag).S("id") == currentController);
            if (controllers.SelectedIndex < 0 && controllers.Items.Count > 0) controllers.SelectedIndex = 0;
            editors["controller"] = () => controllers.SelectedItem is ComboBoxItem selected ? ((JsonNode)selected.Tag).S("id") : fields.S("controller");
            editors["unit"] = () => units.SelectedItem is ComboBoxItem selected ? (string)selected.Tag : fields.S("unit");
            inputs["controller"] = controllers; inputs["unit"] = units;
            panel.Children.Add(ResponsiveGrid(180, controllers, units));
            if (controllers.Items.Count == 0)
            {
                controllers.IsEnabled = units.IsEnabled = false;
                panel.Children.Add(DeviceCaption("No controller is available for this drive type with the current configuration."));
            }
        }

        // Geometry is only needed for hardfiles without an RDB, so it stays out of the normal flow.
        var geometry = new List<UIElement>();
        foreach (var (id, header, help) in new[] { ("sectors", "Sectors per track", "0 reads the disk's RDB partition table."), ("surfaces", "Surfaces", ""), ("reserved", "Reserved blocks", ""), ("block_size", "Block size (bytes)", "") })
            if (Field(id, header, help) is { } box) geometry.Add(box);
        if (geometry.Count > 0)
        {
            var advanced = Stack(DeviceCaption("Disk geometry is probed from the hardfile when you choose it. Change it only for images without an RDB."), ResponsiveGrid(180, [.. geometry]));
            var expander = new Expander { Header = "Geometry (advanced)", Content = advanced, HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
            AutomationProperties.SetAutomationId(expander, "MountGeometry");
            panel.Children.Add(expander);
        }
        if ((firstField ?? pathBox) is { } focusTarget && !index.HasValue) focusTarget.Loaded += (_, _) => focusTarget.Focus(FocusState.Programmatic);

        JsonNode? saved = null;
        string[] order = ["path", "device", "volume", "boot_priority", "read_only", "controller", "unit", "sectors", "surfaces", "reserved", "block_size"];
        // Runs while the dialog stays open: a rejected value is reported there and every input is kept.
        async Task ApplyMountAsync()
        {
            problem.IsOpen = false;
            var edited = mount;
            bool controllerChanged = editors.TryGetValue("controller", out var controllerValue) && controllerValue() != fields.S("controller");
            foreach (string id in order)
            {
                if (!editors.TryGetValue(id, out var read)) continue;
                string value = read();
                if (value == fields.S(id) && !(id == "unit" && controllerChanged)) continue;
                try { edited = await Core.CallAsync("mount_field", new { entry = edited.S("entry"), field = id, value }); }
                catch (NativeRequestException failure)
                {
                    if (inputs.TryGetValue(id, out var input)) DispatcherQueue.TryEnqueue(() => input.Focus(FocusState.Programmatic));
                    string reason = failure.Message.Replace("Setting mount field: ", "");
                    throw new InvalidOperationException(MountLabel(id) + ": " + reason + MountHint(id));
                }
            }
            string entry = edited.S("entry");
            saved = index.HasValue ? await Core.CallAsync("mount_replace", new { index = index.Value, entry }) : await Core.CallAsync("mount_add", new { entry });
        }

        string noun = kind switch { "dir" => "folder drive", "hdf" => "hardfile", "cd" => "CD drive", "tape" => "tape drive", _ => "drive" };
        var scroller = new ScrollViewer { Content = panel, MaxHeight = DialogBodyHeight(), HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled };
        if (await DialogAsync((index.HasValue ? "Edit " : "Add ") + noun, scroller, index.HasValue ? "Apply" : "Add", cancel: "Cancel", primaryAction: ApplyMountAsync) != ContentDialogResult.Primary || saved is null) return;
        ApplySnapshot(saved);
        await RenderEditorAsync();
    }

    private static string MountLabel(string id) => id switch { "path" => "Path", "device" => "Device name", "volume" => "Volume label", "boot_priority" => "Boot priority", "sectors" => "Sectors per track", "surfaces" => "Surfaces", "reserved" => "Reserved blocks", "block_size" => "Block size", "controller" => "Controller", "unit" => "Controller unit", "read_only" => "Read only", _ => id };
    private static string MountHint(string id) => id switch
    {
        "path" => " Check that the path is not empty and contains no quotation marks.",
        "device" or "volume" => " Use a non-empty name without colons, commas or quotation marks.",
        "boot_priority" => " Enter a whole number from -129 to 127.",
        "sectors" or "surfaces" or "reserved" or "block_size" => " Enter a whole number, 0 or greater.",
        _ => ""
    };

    // ---- expansion boards -------------------------------------------------------------------

    private async Task AddBoardsAsync()
    {
        var boards = await Core.CallAsync("boards");
        var content = new StackPanel { Spacing = 10 };
        content.Children.Add(new InfoBar { IsOpen = true, IsClosable = false, Severity = InfoBarSeverity.Informational, Title = "Applied when the Amiga restarts", Message = "Expansion boards are constructed when emulation starts; changes require restarting." });
        var active = new StackPanel { Spacing = 8 };
        content.Children.Add(active);
        var add = new ComboBox { Header = "Add expansion board", PlaceholderText = "Choose a board…", HorizontalAlignment = HorizontalAlignment.Stretch };
        AutomationProperties.SetAutomationId(add, "BoardAddChoice");
        foreach (var board in boards.Items())
        {
            int unitCount = board.Items("units").Count();
            foreach (var unit in board.Items("units"))
            {
                var enabled = unit.Items("fields").FirstOrDefault(field => field["descriptor"].S("id") == "enabled");
                if (enabled is null) continue;
                int unitNumber = unit.I("unit");
                string boardId = board.S("id"), numbered = board.S("label") + (unitCount > 1 ? " #" + (unitNumber + 1) : "");
                string reason = enabled["descriptor"].S("constraint", "Not available with the current configuration.");
                if (enabled.S("value") != "true")
                {
                    bool available = enabled.B("available");
                    var entry = new ComboBoxItem { Content = numbered + " · " + board.S("manufacturer") + (available ? "" : " (unavailable)"), Tag = new BoardTarget(boardId, unitNumber), IsEnabled = available };
                    if (!available) ToolTipService.SetToolTip(entry, reason);
                    add.Items.Add(entry);
                    continue;
                }
                var fields = new StackPanel { Spacing = 8 };
                foreach (var field in unit.Items("fields").Where(field => field["descriptor"].S("id") != "enabled"))
                {
                    var descriptor = field["descriptor"]!;
                    string fieldId = descriptor.S("id");
                    fields.Children.Add(MakeSettingRow(descriptor, field, async value =>
                    {
                        var updated = await Core.CallAsync("board_set", new { type = boardId, unit = unitNumber, field = fieldId, value });
                        ApplySnapshot(await Core.CallAsync("snapshot"));
                        foreach (var b in updated.Items()) foreach (var u in b.Items("units")) foreach (var f in u.Items("fields"))
                            if (rows.TryGetValue($"board:{b.S("id")}:{u.I("unit")}:{f["descriptor"].S("id")}", out var visible)) visible.Update(f);
                    }, $"board:{boardId}:{unitNumber}:").Element);
                }
                var remove = Button("Remove Board", async () =>
                {
                    if (!await FlushDraftsAsync()) return;
                    await Core.CallAsync("board_set", new { type = boardId, unit = unitNumber, field = "enabled", value = "false" });
                    ApplySnapshot(await Core.CallAsync("snapshot")); await RenderEditorAsync();
                }, $"BoardRemove_{boardId}_{unitNumber}");
                remove.IsEnabled = enabled.B("available");
                remove.HorizontalAlignment = HorizontalAlignment.Right;
                AutomationProperties.SetHelpText(remove, "Remove " + numbered);
                if (!remove.IsEnabled) ToolTipService.SetToolTip(remove, reason);
                fields.Children.Add(remove);
                var header = new StackPanel();
                header.Children.Add(new TextBlock { Text = numbered });
                if (board.S("manufacturer").Length > 0) header.Children.Add(DeviceCaption(board.S("manufacturer")));
                var expander = new Expander { Header = header, Content = fields, HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
                AutomationProperties.SetName(expander, numbered);
                AutomationProperties.SetAutomationId(expander, $"Board_{boardId}_{unitNumber}");
                TrackExpander(expander, $"boards:{boardId}:{unitNumber}", false);
                active.Children.Add(expander);
            }
        }
        if (active.Children.Count == 0) active.Children.Add(DeviceCaption("No expansion boards are added."));
        if (add.Items.Count == 0) content.Children.Add(DeviceCaption("Every available board is already added."));
        else
        {
            var addButton = Button("Add Board", async () =>
            {
                if (add.SelectedItem is not ComboBoxItem selected) return;
                if (!await FlushDraftsAsync()) return;
                var target = (BoardTarget)selected.Tag;
                await Core.CallAsync("board_set", new { type = target.Type, unit = target.Unit, field = "enabled", value = "true" });
                expanded.Add($"boards:{target.Type}:{target.Unit}");
                ApplySnapshot(await Core.CallAsync("snapshot")); await RenderEditorAsync();
            }, "BoardAdd");
            addButton.IsEnabled = false;
            addButton.VerticalAlignment = VerticalAlignment.Bottom;
            add.SelectionChanged += (_, _) => addButton.IsEnabled = add.SelectedItem is ComboBoxItem;
            var addRow = new Grid { ColumnSpacing = 8 };
            addRow.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            addRow.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            addRow.Children.Add(add); addRow.Children.Add(addButton); Grid.SetColumn(addButton, 1);
            content.Children.Add(addRow);
        }
        var section = new Expander { Header = "Expansion Boards", Content = content, IsExpanded = true, HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
        AutomationProperties.SetAutomationId(section, "HardwareExpansionBoards");
        page.Children.Add(section);
    }
    private sealed record BoardTarget(string Type, int Unit);

    // ---- custom controller mappings ---------------------------------------------------------

    private static string PortName(int port) => "Port " + (port + 1);

    // The device chosen for the port in the Input settings, by its label.
    private string PortDevice(int port)
    {
        var value = Value("joyport" + port);
        string current = value.S("value");
        string label = value.Items("choices").FirstOrDefault(item => item.S("value") == current)?.S("label") ?? current;
        return label.Length > 0 ? label : "No device selected";
    }

    private static string ControlStatus(JsonNode control)
    {
        string use = control.S("use");
        if (use.Length > 0)
            return "Reserved: " + use switch
            {
                "hotkey" => "this button is the controller's hotkey button.",
                "quit" => "this button quits while the hotkey button is held.",
                "menu" => "this button opens the Amiberry menu while the hotkey button is held.",
                "reset" => "this button resets while the hotkey button is held.",
                "vkbd" => "this button shows the on-screen keyboard.",
                _ => "used for " + use + "."
            };
        return control.B("available") ? "" : "This controller does not have this control.";
    }

    private static void ApplyControlState(ComboBox combo, TextBlock label, TextBlock status, JsonNode control)
    {
        string name = control.S("label") + (control.B("axis") ? " (axis)" : "");
        label.Text = name;
        AutomationProperties.SetName(combo, name);
        string reason = ControlStatus(control);
        status.Text = reason;
        status.Visibility = reason.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        AutomationProperties.SetHelpText(combo, reason);
        combo.IsEnabled = control.B("available");
    }

    // An event the list does not know (hand-edited configuration) stays visible instead of looking unset.
    private static void SelectMapping(ComboBox combo, string value)
    {
        var match = combo.Items.OfType<ComboBoxItem>().FirstOrDefault(item => (string)item.Tag == value);
        if (match is null && value.Length > 0)
        {
            match = new ComboBoxItem { Content = value, Tag = value };
            combo.Items.Add(match);
        }
        combo.SelectedItem = match;
    }

    private async Task AddCustomControlsAsync()
    {
        var controls = await Core.CallAsync("controls");
        latestControls = controls;
        mappings.Clear(); mappingRevisions.Clear(); mappingLabels.Clear(); mappingPorts.Clear();
        var content = new StackPanel { Spacing = 10 };
        content.Children.Add(DeviceCaption("Assign Amiga actions to the buttons and axes of the game controller on each port. Hotkey mappings apply while the controller's hotkey button is held."));
        foreach (var portEntries in controls.Items("ports").GroupBy(entry => entry.I("port")).OrderBy(byPort => byPort.Key))
            content.Children.Add(BuildPortExpander(portEntries.Key, portEntries.Any(entry => entry.Items("controls").Any(control => control.B("available") || control.S("use").Length > 0))));
        if (!controls.Items("ports").Any()) content.Children.Add(Text("No controller ports are reported. Choose input devices in the Port sections above."));
        var section = new Expander { Header = "Custom Controls", Content = content, IsExpanded = true, HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
        AutomationProperties.SetAutomationId(section, "InputCustomControls");
        page.Children.Add(section);
    }

    private Expander BuildPortExpander(int port, bool any)
    {
        var device = DeviceCaption(PortDevice(port) + (any ? "" : " · Nothing to map"));
        var header = new StackPanel();
        header.Children.Add(new TextBlock { Text = PortName(port) });
        header.Children.Add(device);
        var empty = DeviceCaption("Custom controls need a game controller on this port. Choose one for " + PortName(port) + " in the sections above; keyboards and mice have no buttons to remap here.");
        empty.Visibility = any ? Visibility.Collapsed : Visibility.Visible;
        var maps = new StackPanel { Spacing = 10, Visibility = any ? Visibility.Visible : Visibility.Collapsed };
        var body = new StackPanel { Spacing = 10 };
        body.Children.Add(empty); body.Children.Add(maps);
        var view = new PortView(device, empty, maps);
        mappingPorts[port] = view;
        var expander = new Expander { Header = header, Content = body, HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
        AutomationProperties.SetName(expander, PortName(port));
        AutomationProperties.SetAutomationId(expander, "ControlsPort_" + port);
        // The many action lists are built the first time the port is opened, from the latest reply.
        TrackExpander(expander, "controls:" + port, false, () =>
        {
            if (view.Built) return;
            view.Built = true;
            foreach (var entry in latestControls.Items("ports").Where(entry => entry.I("port") == port))
                maps.Children.Add(BuildMappingGroup(port, entry));
        });
        return expander;
    }

    private Expander BuildMappingGroup(int port, JsonNode entry)
    {
        bool hotkey = entry.B("hotkey");
        var list = new StackPanel { Spacing = 8 };
        foreach (var control in entry.Items("controls")) list.Children.Add(BuildMappingRow(port, hotkey, control));
        var expander = new Expander { Header = hotkey ? "While the hotkey button is held" : "Normal", Content = list, HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };
        AutomationProperties.SetAutomationId(expander, $"ControlsGroup_{port}_{(hotkey ? "hotkey" : "normal")}");
        TrackExpander(expander, $"controls:{port}:{hotkey}", !hotkey);
        return expander;
    }

    private UIElement BuildMappingRow(int port, bool hotkey, JsonNode control)
    {
        var key = (port, hotkey, control.S("id"));
        var label = new TextBlock { TextWrapping = TextWrapping.Wrap, VerticalAlignment = VerticalAlignment.Center };
        var status = DeviceCaption("");
        var combo = new ComboBox { Width = 240, HorizontalAlignment = HorizontalAlignment.Right };
        AutomationProperties.SetAutomationId(combo, $"ControlMapping_{port}_{(hotkey ? "hotkey" : "normal")}_{key.Item3}");
        foreach (var action in latestControls.Items("events")) combo.Items.Add(new ComboBoxItem { Content = action.S("label"), Tag = action.S("value") });
        SelectMapping(combo, control.S("value"));
        ApplyControlState(combo, label, status, control);
        mappings[key] = combo;
        mappingLabels[key] = (label, status);
        combo.SelectionChanged += async (_, _) =>
        {
            if (updatingMappings || combo.SelectedItem is not ComboBoxItem selected) return;
            long revision = ++mappingRevision;
            mappingRevisions[key] = revision;
            string selectedValue = (string)selected.Tag;
            await RunAsync(async () =>
            {
                var updated = await Core.CallAsync("control_set", new { port = key.Item1, hotkey = key.Item2, control = key.Item3, @event = selectedValue });
                if (mappingRevisions.GetValueOrDefault(key) == revision) mappingRevisions.Remove(key);
                UpdateCustomControls(updated);
                ApplySnapshot(await Core.CallAsync("snapshot"));
            });
        };
        var names = new StackPanel { VerticalAlignment = VerticalAlignment.Center };
        names.Children.Add(label); names.Children.Add(status);
        var row = new Grid { ColumnSpacing = 16, RowSpacing = 6 };
        row.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        row.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        row.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        row.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        row.Children.Add(names); row.Children.Add(combo); Grid.SetColumn(combo, 1);
        row.SizeChanged += (_, e) =>
        {
            bool narrow = e.NewSize.Width < 520;
            Grid.SetRow(combo, narrow ? 1 : 0); Grid.SetColumn(combo, narrow ? 0 : 1); Grid.SetColumnSpan(combo, narrow ? 2 : 1);
            combo.Width = narrow ? double.NaN : 240; combo.HorizontalAlignment = narrow ? HorizontalAlignment.Stretch : HorizontalAlignment.Right;
        };
        return row;
    }

    private async Task RefreshCustomControlsAsync()
    {
        if (mappings.Count > 0 || mappingPorts.Values.Any(view => view.Device.IsLoaded)) UpdateCustomControls(await Core.CallAsync("controls"));
    }
    private void UpdateCustomControls(JsonNode controls)
    {
        latestControls = controls;
        updatingMappings = true;
        try
        {
            foreach (var port in controls.Items("ports").GroupBy(entry => entry.I("port")))
            {
                bool any = false;
                foreach (var entry in port) foreach (var control in entry.Items("controls"))
                {
                    any |= control.B("available") || control.S("use").Length > 0;
                    var key = (port.Key, entry.B("hotkey"), control.S("id"));
                    if (!mappings.TryGetValue(key, out var visible) || !mappingLabels.TryGetValue(key, out var parts)) continue;
                    ApplyControlState(visible, parts.Label, parts.Status, control);
                    if (!mappingRevisions.ContainsKey(key)) SelectMapping(visible, control.S("value"));
                }
                if (mappingPorts.TryGetValue(port.Key, out var view))
                {
                    view.Device.Text = PortDevice(port.Key) + (any ? "" : " · Nothing to map");
                    view.Empty.Visibility = any ? Visibility.Collapsed : Visibility.Visible;
                    view.Mappings.Visibility = any ? Visibility.Visible : Visibility.Collapsed;
                }
            }
        }
        finally { updatingMappings = false; }
    }
}
