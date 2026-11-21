using System.Text.Json.Nodes;
using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Input;
using global::Windows.System;
using global::Windows.Foundation;
using global::Windows.ApplicationModel.DataTransfer;
using Microsoft.Windows.Storage.Pickers;
using WinRT.Interop;

namespace Amiberry.Windows;

public sealed partial class MainWindow : Window
{
    private NativeClient? client;
    private readonly string? pipeName;
    private readonly NavigationView navigation = new() { IsSettingsVisible = false, PaneDisplayMode = NavigationViewPaneDisplayMode.Auto, OpenPaneLength = 240, IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed, IsPaneToggleButtonVisible = false };
    private readonly TitleBar appTitleBar = new() { Title = "Amiberry", IsBackButtonVisible = true, IsPaneToggleButtonVisible = true };
    private readonly StackPanel page = new() { Spacing = 20, Margin = new Thickness(28, 24, 28, 24), MaxWidth = 1040, HorizontalAlignment = HorizontalAlignment.Stretch };
    private readonly ScrollViewer pageScroll = new() { HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled, HorizontalContentAlignment = HorizontalAlignment.Stretch };
    private readonly AutoSuggestBox searchBox = new() { QueryIcon = new SymbolIcon(Symbol.Find), HorizontalAlignment = HorizontalAlignment.Stretch };
    private readonly DispatcherTimer searchTimer = new() { Interval = TimeSpan.FromMilliseconds(250) };
    private readonly Grid runtimePanel = new() { Padding = new Thickness(24, 0, 12, 0), Visibility = Visibility.Collapsed };
    private readonly StackPanel runtimeBar = new() { Orientation = Orientation.Horizontal, Spacing = 8, VerticalAlignment = VerticalAlignment.Center };
    private readonly MenuFlyout runtimeMenu = new();
    private readonly Grid editorBar = new() { Padding = new Thickness(24, 12, 24, 16), ColumnSpacing = 16, RowSpacing = 10 };
    private readonly TextBlock runtimeStatus = new() { TextWrapping = TextWrapping.Wrap, VerticalAlignment = VerticalAlignment.Center };
    private readonly InfoBar notice = new() { IsClosable = true, Severity = InfoBarSeverity.Error, Margin = new Thickness(16, 4, 16, 4) };
    private readonly TextBlock editStatus = new() { TextWrapping = TextWrapping.Wrap, VerticalAlignment = VerticalAlignment.Center, FontSize = 12 };
    private readonly ProgressBar progress = new() { IsIndeterminate = true, Visibility = Visibility.Collapsed };
    private readonly SemaphoreSlim uiOperations = new(1, 1);
    private readonly CancellationTokenSource lifetime = new();
    private readonly Dictionary<string, SettingRow> rows = [];
    private readonly Dictionary<string, Func<Task>> drafts = [];
    private readonly Dictionary<string, JsonNode> descriptors = [];
    private readonly Dictionary<string, JsonNode> hostDescriptors = [];
    private readonly HashSet<string> expanded = [];
    private JsonNode catalog = new JsonObject(), library = new JsonObject(), snapshot = new JsonObject(), status = new JsonObject(), host = new JsonObject();
    private string currentPage = "machines", query = "", currentPath = "";
    private bool session, dirty, closing, navigating, dialogOpen, committedNotSaved, statusReceived, windowHidden;
    private int pendingApply, committedApply, settingsEpoch, libraryEpoch, devicesEpoch;
    // Set when a running machine was applied with edits that never reached its configuration file; the editor is
    // closed then, so reopening it (F12) must still report those edits as unsaved. Holds the machine's file ("" if untitled).
    private string? resumedUnsavedPath;
    private NativeClient Core => client ?? throw new InvalidOperationException("The emulator is not connected.");
    private bool Emulating => status.S("state") is "running" or "paused";
    private readonly Button stopButton;
    // Reopens the running machine's editor (and its Resume button) after the Library was chosen from F12.
    private readonly Button machineButton;
    // The current page's subtitle, so the editor can update the machine identity in place.
    private TextBlock? pageSubtitle;
    private readonly Button startButton, revertButton;
    private readonly SplitButton saveButton;

    // The default size is larger than a small display (a 1024x768 monitor hid the footer and most of the
    // window off screen): clamp it to the work area and keep the default position inside it.
    private void FitToWorkArea(int width, int height)
    {
        var area = DisplayArea.GetFromWindowId(AppWindow.Id, DisplayAreaFallback.Nearest).WorkArea;
        int fitted = Math.Min(width, area.Width), fittedHeight = Math.Min(height, area.Height);
        AppWindow.Resize(new global::Windows.Graphics.SizeInt32(fitted, fittedHeight));
        var position = AppWindow.Position;
        int x = Math.Clamp(position.X, area.X, area.X + area.Width - fitted), y = Math.Clamp(position.Y, area.Y, area.Y + area.Height - fittedHeight);
        if (x != position.X || y != position.Y) AppWindow.Move(new global::Windows.Graphics.PointInt32(x, y));
    }

    public MainWindow(string? pipe)
    {
        pipeName = pipe;
        Title = "Amiberry";
        SystemBackdrop = new MicaBackdrop();
        FitToWorkArea(1220, 880);
        AppWindow.Closing += OnClosing;
        ExtendsContentIntoTitleBar = true;
        SetTitleBar(appTitleBar);
        appTitleBar.BackRequested += async (_, _) => await RunAsync(() => NavigateAsync("machines"));
        appTitleBar.PaneToggleRequested += (_, _) => navigation.IsPaneOpen = !navigation.IsPaneOpen;
        var root = new Grid();
        root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
        root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.Children.Add(appTitleBar);
        root.ActualThemeChanged += (_, _) => UpdateCaptionTheme();
        void UpdateCaptionTheme() => AppWindow.TitleBar.PreferredTheme = root.ActualTheme == ElementTheme.Dark ? TitleBarTheme.Dark : TitleBarTheme.Light;
        runtimeStatus.MaxWidth = 280;
        runtimeStatus.TextWrapping = TextWrapping.NoWrap;
        runtimeStatus.TextTrimming = TextTrimming.CharacterEllipsis;
        runtimePanel.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        runtimePanel.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        runtimePanel.Children.Add(runtimeStatus);
        Grid.SetColumn(runtimeBar, 1); runtimePanel.Children.Add(runtimeBar);
        Grid.SetRow(runtimePanel, 1); root.Children.Add(runtimePanel);
        var feedback = new StackPanel(); feedback.Children.Add(progress); feedback.Children.Add(notice);
        Grid.SetRow(feedback, 2); root.Children.Add(feedback);
        Grid.SetRow(navigation, 3); root.Children.Add(navigation);
        editorBar.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        editorBar.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        editorBar.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        editorBar.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        editorBar.Children.Add(editStatus);
        Grid.SetRow(editorBar, 4); root.Children.Add(editorBar);
        pageScroll.Content = page; navigation.Content = pageScroll;
        AutomationProperties.SetAutomationId(navigation, "LibraryNavigation");
        AutomationProperties.SetAutomationId(appTitleBar, "AppTitleBar");
        navigation.SelectionChanged += (sender, e) =>
        {
            if (!navigating && e.SelectedItem is NavigationViewItem item)
                _ = RunAsync(() => NavigateAsync((string)item.Tag));
        };
        machineButton = Runtime("Current Machine", Symbol.Setting, () => OpenEditorAsync(new { }), "RuntimeMachineButton");
        Runtime("Drives & States", Symbol.OpenFile, async () => { if (await FlushDraftsAsync()) RenderRuntime(); }, "RuntimeDrivesButton");
        stopButton = Runtime("Stop", Symbol.Stop, () => ConfirmActionAsync("Stop emulation?", "Unsaved work in the Amiga will be lost.", () => Core.ActionAsync("stop"), "Stop"), "RuntimeStopButton");
        RuntimeMenu("Full Screen", Symbol.FullScreen, () => Core.ActionAsync("fullscreen"));
        RuntimeMenu("Screenshot", Symbol.Camera, async () =>
        {
            string folder = Folder(6);
            if (folder.Length == 0) throw new InvalidOperationException("Choose a Screenshots folder in App Settings first.");
            await Task.Run(() => Directory.CreateDirectory(folder));
            await Core.ActionAsync("screenshot", Path.Combine(folder, "Amiberry-" + DateTime.Now.ToString("yyyyMMdd-HHmmss-fff") + ".png"));
        });
        RuntimeMenu("Keyboard", Symbol.Keyboard, async () => await Core.CallAsync("keyboard_toggle"));
        RuntimeMenu("Reset Amiga", Symbol.Refresh, () => ConfirmActionAsync("Reset the running machine?", "Unsaved work in the Amiga will be lost.", () => Core.ActionAsync("reset"), "Reset"));
        RuntimeMenu("Hard Reset", Symbol.Refresh, () => ConfirmActionAsync("Hard reset the machine?", "Unsaved work in the Amiga will be lost.", () => Core.ActionAsync("hard_reset"), "Hard Reset"));
        var more = new DropDownButton { Content = new SymbolIcon(Symbol.More), Flyout = runtimeMenu, VerticalAlignment = VerticalAlignment.Center };
        AutomationProperties.SetName(more, "More emulation commands");
        AutomationProperties.SetAutomationId(more, "RuntimeMoreButton");
        ToolTipService.SetToolTip(more, "More emulation commands");
        runtimeBar.Children.Add(more);
        revertButton = Button("Revert…", RevertAsync, "MachineRevertButton");
        saveButton = new SplitButton { Content = "Save", VerticalAlignment = VerticalAlignment.Center };
        AutomationProperties.SetAutomationId(saveButton, "MachineSaveButton");
        AutomationProperties.SetName(saveButton, "Save machine");
        saveButton.Click += async (_, _) => await RunAsync(() => SaveAsync());
        var saveMenu = new MenuFlyout();
        var saveAs = new MenuFlyoutItem { Text = "Save As…", Icon = new SymbolIcon(Symbol.Save) };
        saveAs.Click += async (_, _) => await RunAsync(() => SaveAsync(true));
        saveMenu.Items.Add(saveAs); saveButton.Flyout = saveMenu;
        startButton = PrimaryButton("Start Amiga", () => StartAsync(), "MachineStartButton");
        startButton.MinWidth = 144;
        var actions = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right };
        actions.Children.Add(revertButton); actions.Children.Add(saveButton); actions.Children.Add(startButton);
        Grid.SetColumn(actions, 1); editorBar.Children.Add(actions);
        editorBar.SizeChanged += (_, e) =>
        {
            bool narrow = e.NewSize.Width < 800;
            Grid.SetRow(actions, narrow ? 1 : 0); Grid.SetColumn(actions, narrow ? 0 : 1);
            Grid.SetColumnSpan(actions, narrow ? 2 : 1);
            Grid.SetColumnSpan(editStatus, narrow ? 2 : 1);
        };
        AutomationProperties.SetAutomationId(searchBox, "SettingsSearchBox");
        searchBox.TextChanged += (_, e) =>
        {
            if (e.Reason != AutoSuggestionBoxTextChangeReason.UserInput) return;
            searchTimer.Stop(); searchTimer.Start();
        };
        searchBox.QuerySubmitted += async (_, _) => { searchTimer.Stop(); await RunAsync(SearchAsync); };
        searchTimer.Tick += async (_, _) => { searchTimer.Stop(); await RunAsync(SearchAsync); };
        AddShortcut(root, VirtualKey.F, VirtualKeyModifiers.Control, () => { searchBox.Focus(FocusState.Programmatic); return Task.CompletedTask; });
        AddShortcut(root, VirtualKey.S, VirtualKeyModifiers.Control, async () => { if (session) await SaveAsync(); });
        ToolTipService.SetToolTip(saveButton, "Save machine (Ctrl+S)");
        Content = root;
        root.Loaded += (_, _) => UpdateCaptionTheme();
        root.Loaded += async (_, _) => await InitializeAsync();
        root.AllowDrop = true;
        root.DragOver += (_, e) => e.AcceptedOperation = e.DataView.Contains(StandardDataFormats.StorageItems) ? DataPackageOperation.Copy : DataPackageOperation.None;
        root.Drop += async (_, e) =>
        {
            if (e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                var items = await e.DataView.GetStorageItemsAsync();
                if (items.FirstOrDefault() is { } item) await RunAsync(() => PlayFileAsync(item.Path));
            }
        };
    }

    private async Task InitializeAsync()
    {
        await RunAsync(async () =>
        {
            if (pipeName is null) throw new InvalidOperationException("Start Amiberry.Windows through Amiberry.exe. A private --pipe connection is required.");
            client = new NativeClient(pipeName);
            client.Disconnected += error => DispatcherQueue.TryEnqueue(() => _ = ConnectionLostAsync(error));
            await client.ConnectAsync();
            catalog = await Core.CallAsync("catalog");
            foreach (var descriptor in catalog.Items("options")) descriptors[descriptor.S("id")] = descriptor;
            foreach (var descriptor in catalog.Items("hostOptions")) hostDescriptors[descriptor.S("id")] = descriptor;
            library = await Core.CallAsync("library");
            SetNavigation(false);
            await RenderLibraryAsync();
            _ = PollStatusAsync();
        });
    }

    private async Task RunAsync(Func<Task> action)
    {
        await uiOperations.WaitAsync();
        try { progress.Visibility = Visibility.Visible; await action(); }
        catch (OperationCanceledException) when (lifetime.IsCancellationRequested) { }
        catch (Exception error) { ShowError(error.Message); }
        finally { progress.Visibility = Visibility.Collapsed; uiOperations.Release(); }
    }

    private void ShowError(string message) { notice.Title = "Amiberry"; notice.Message = message; notice.IsOpen = true; }
    private Button Runtime(string label, Symbol symbol, Func<Task> action, string? id = null)
    {
        var button = Button(label, action, id);
        SetRuntimeButton(button, label, symbol);
        runtimeBar.Children.Add(button);
        return button;
    }
    private static void SetRuntimeButton(Button button, string label, Symbol symbol)
    {
        if (AutomationProperties.GetName(button) == label) return;
        var content = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8 };
        content.Children.Add(new SymbolIcon(symbol));
        content.Children.Add(new TextBlock { Text = label });
        button.Content = content;
        AutomationProperties.SetName(button, label);
    }
    private void RuntimeMenu(string label, Symbol symbol, Func<Task> action)
    {
        var item = new MenuFlyoutItem { Text = label, Icon = new SymbolIcon(symbol) };
        item.Click += async (_, _) => await RunAsync(action);
        runtimeMenu.Items.Add(item);
    }

    private void SetNavigation(bool editor)
    {
        navigating = true;
        navigation.MenuItems.Clear(); navigation.FooterMenuItems.Clear();
        foreach (var entry in editor ? EditorPages : LibraryPages)
        {
            var item = new NavigationViewItem { Content = entry.Title, Tag = entry.Id, Icon = new SymbolIcon(entry.Icon) };
            AutomationProperties.SetAutomationId(item, "Page_" + entry.Id);
            if (entry.Id == "app") navigation.FooterMenuItems.Add(item); else navigation.MenuItems.Add(item);
        }
        if (editor) navigation.FooterMenuItems.Add(new NavigationViewItem { Content = "Library", Tag = "machines", Icon = new SymbolIcon(Symbol.AllApps) });
        navigating = false;
        SelectNavigation(currentPage); UpdateFooter();
    }
    private void SelectNavigation(string id)
    {
        navigating = true;
        navigation.SelectedItem = navigation.MenuItems.Concat(navigation.FooterMenuItems).OfType<NavigationViewItem>().FirstOrDefault(item => (string)item.Tag == id);
        appTitleBar.IsBackButtonEnabled = session || id != "machines";
        navigating = false;
    }
    private async Task NavigateAsync(string id)
    {
        searchTimer.Stop();
        if (id == currentPage) { SelectNavigation(id); return; }
        if (!await FlushDraftsAsync()) { SelectNavigation(currentPage); return; }
        if (session && !EditorPages.Any(item => item.Id == id))
        {
            if (!await LeaveSessionAsync()) { SelectNavigation(currentPage); return; }
            SetNavigation(false);
        }
        currentPage = id; query = "";
        await RenderCurrentPageAsync();
        pageScroll.ChangeView(null, 0, null, true);
        SelectNavigation(currentPage);
    }
    private Task RenderCurrentPageAsync()
    {
        if (currentPage == "runtime") { RenderRuntime(); return Task.CompletedTask; }
        return session ? RenderEditorAsync() : RenderLibraryAsync();
    }
    private async Task SearchAsync()
    {
        string next = searchBox.Text.Trim();
        if (next == query || closing || !await FlushDraftsAsync()) return;
        query = next;
        await RenderCurrentPageAsync();
        pageScroll.ChangeView(null, 0, null, true);
        searchBox.Focus(FocusState.Programmatic);
    }

    private async Task PollStatusAsync()
    {
        using var timer = new PeriodicTimer(TimeSpan.FromMilliseconds(500));
        try
        {
            do
            {
                // A long legitimate request (creating a large hardfile) keeps the core's pipe busy past the queue
                // limit. That only skips this poll; it must not end the connection, which a quit would follow.
                try { await PollOnceAsync(); }
                catch (NativeQueueTimeoutException) { }
            } while (await timer.WaitForNextTickAsync(lifetime.Token));
        }
        catch (OperationCanceledException) when (lifetime.IsCancellationRequested) { }
        catch (Exception error) { if (!closing) await ConnectionLostAsync(error); }
    }

    private async Task PollOnceAsync()
    {
        bool wasRunning = Emulating;
        status = await Core.CallAsync("status", cancellation: lifetime.Token);
        if (wasRunning && !Emulating) { committedApply = 0; resumedUnsavedPath = null; }
        runtimePanel.Visibility = Emulating ? Visibility.Visible : Visibility.Collapsed;
        string machine = status.S("description");
        string activity = status.S("state") == "paused" ? "Paused" : $"Running · {status.D("fps"):0.0} fps";
        runtimeStatus.Text = $"{(machine == "UAE default configuration" ? "Amiga" : machine)} · {activity}";
        stopButton.IsEnabled = Emulating;
        int nextSettingsEpoch = status.I("settingsEpoch"), nextLibraryEpoch = status.I("libraryEpoch"), nextDevicesEpoch = status.I("devicesEpoch");
        bool refreshedSession = false;
        if (statusReceived && ((wasRunning && !Emulating) || nextLibraryEpoch != libraryEpoch))
            await RunAsync(async () =>
            {
                if (session && !await LeaveSessionAsync()) return;
                currentPage = "machines"; SetNavigation(false); await RenderLibraryAsync();
            });
        else if (statusReceived && nextSettingsEpoch != settingsEpoch && status.B("settingsOpen"))
            await RunAsync(async () =>
            {
                if (session)
                {
                    ApplySnapshot(await Core.CallAsync("snapshot"));
                    await RefreshCustomControlsAsync();
                    if (!EditorPages.Any(item => item.Id == currentPage)) await RenderEditorAsync();
                }
                else await OpenEditorAsync(new { });
                refreshedSession = session;
            });
        if (statusReceived && nextDevicesEpoch != devicesEpoch && session && status.B("settingsOpen") && !refreshedSession)
            await RunAsync(async () =>
            {
                if (!session) return;
                ApplySnapshot(await Core.CallAsync("snapshot"));
                await RefreshCustomControlsAsync();
            });
        settingsEpoch = nextSettingsEpoch; libraryEpoch = nextLibraryEpoch; devicesEpoch = nextDevicesEpoch; statusReceived = true;
        SyncWindowVisibility();
        UpdateFooter();
        UpdateRuntimeStatus();
        await UpdateKeyboardAsync();
        await RefreshDownloadAsync();
    }

    // The Library/editor is only on screen while the core is stopped or its F12
    // settings loop is open. While the emulation window owns input (running or
    // paused, settingsOpen false) the main window is hidden, never closed, so the
    // poller and pipe stay alive and F12 can bring the editor back.
    private void SyncWindowVisibility()
    {
        bool hide = Emulating && !status.B("settingsOpen") && !closing;
        if (hide == windowHidden) return;
        if (!hide) ShowMainWindow();
        else if (!dialogOpen) { AppWindow.Hide(); windowHidden = true; }
    }
    private void ShowMainWindow()
    {
        AppWindow.Show();
        if (AppWindow.Presenter is OverlappedPresenter { State: OverlappedPresenterState.Minimized } presenter) presenter.Restore();
        Activate();
        windowHidden = false;
    }

    private async Task EnsureSettingsOpenAsync()
    {
        var current = await Core.CallAsync("status");
        status = current;
        if (current.S("state") is not ("running" or "paused") || current.B("settingsOpen")) return;
        await Core.CallAsync("show");
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
        deadline.CancelAfter(TimeSpan.FromSeconds(60));
        while (!current.B("settingsOpen"))
        {
            await Task.Delay(30, deadline.Token);
            current = await Core.CallAsync("status", cancellation: deadline.Token);
        }
        status = current;
    }
    private async Task ConnectionLostAsync(Exception error)
    {
        if (closing) return;
        closing = true; lifetime.Cancel();
        if (windowHidden) ShowMainWindow();
        await DialogAsync("Emulator connection closed", error.Message, "Close");
        if (keyboard is not null) await keyboard.HideAsync(false);
        if (client is not null) await client.DisposeAsync();
        Close();
    }
    private async void OnClosing(AppWindow sender, AppWindowClosingEventArgs args)
    {
        if (closing) return;
        args.Cancel = true;
        searchTimer.Stop();
        await RunAsync(async () =>
        {
            if (!await FlushDraftsAsync() || (session && !await LeaveSessionAsync())) return;
            if (keyboard is not null) await keyboard.HideAsync(true);
            closing = true; lifetime.Cancel();
            if (client is not null) await client.DisposeAsync();
            Close();
        });
    }

    private async Task<ContentDialogResult> DialogAsync(string title, object content, string primary, string? secondary = null, string? cancel = null, bool destructive = false, Func<Task>? primaryAction = null)
    {
        if (dialogOpen) return ContentDialogResult.None;
        dialogOpen = true;
        if (windowHidden) ShowMainWindow();
        try
        {
            var error = new InfoBar { IsClosable = false, Severity = InfoBarSeverity.Error, Visibility = Visibility.Collapsed };
            AutomationProperties.SetAutomationId(error, "DialogError");
            AutomationProperties.SetLiveSetting(error, AutomationLiveSetting.Assertive);
            var busy = new ProgressBar { IsIndeterminate = true, Visibility = Visibility.Collapsed };
            var input = new ContentControl { Content = content is string message ? Text(message) : content, HorizontalContentAlignment = HorizontalAlignment.Stretch, IsTabStop = false };
            var dialog = new ContentDialog { XamlRoot = ((FrameworkElement)Content).XamlRoot, Title = title, Content = Stack(busy, error, input), PrimaryButtonText = primary, SecondaryButtonText = secondary ?? "", CloseButtonText = cancel ?? "", DefaultButton = destructive ? ContentDialogButton.Close : ContentDialogButton.Primary };
            if (primaryAction is not null)
                dialog.PrimaryButtonClick += async (_, args) =>
                {
                    var deferral = args.GetDeferral();
                    try
                    {
                        error.IsOpen = false; error.Visibility = Visibility.Collapsed;
                        busy.Visibility = Visibility.Visible; input.IsEnabled = false;
                        await primaryAction();
                    }
                    catch (Exception failure)
                    {
                        args.Cancel = true;
                        error.Title = "Couldn't " + primary.ToLowerInvariant();
                        error.Message = failure.Message;
                        error.Visibility = Visibility.Visible; error.IsOpen = true;
                        error.StartBringIntoView();
                    }
                    finally
                    {
                        busy.Visibility = Visibility.Collapsed; input.IsEnabled = true;
                        deferral.Complete();
                    }
                };
            return await dialog.ShowAsync();
        }
        finally { dialogOpen = false; SyncWindowVisibility(); }
    }
    private async Task ConfirmActionAsync(string title, string message, Func<Task> action, string primary = "Continue")
    { if (await DialogAsync(title, message, primary, cancel: "Cancel", destructive: true) == ContentDialogResult.Primary) await action(); }

    private async Task<string?> PickFileAsync(bool save = false, string extension = "*", string suggested = "Machine", string? initialFolder = null)
    {
        if (save)
        {
            string fileExtension = extension == "*" ? ".uae" : extension;
            var picker = new FileSavePicker(AppWindow.Id) { SuggestedFileName = suggested, DefaultFileExtension = fileExtension };
            if (!string.IsNullOrEmpty(initialFolder)) picker.SuggestedFolder = initialFolder;
            picker.FileTypeChoices.Add(fileExtension.TrimStart('.').ToUpperInvariant() + " file", new[] { fileExtension });
            return (await picker.PickSaveFileAsync())?.Path;
        }
        var open = new FileOpenPicker(AppWindow.Id); open.FileTypeFilter.Add(extension);
        return (await open.PickSingleFileAsync())?.Path;
    }
    private async Task<IReadOnlyList<string>> PickFilesAsync(string extension = "*")
    {
        var picker = new FileOpenPicker(AppWindow.Id); picker.FileTypeFilter.Add(extension);
        return (await picker.PickMultipleFilesAsync()).Select(file => file.Path).ToList();
    }
    private async Task<string?> PickFolderAsync() => (await new FolderPicker(AppWindow.Id).PickSingleFolderAsync())?.Path;

    private void BeginPage(string title, string subtitle, bool search = false, string? searchPlaceholder = null)
    {
        rows.Clear(); mappings.Clear(); mappingRevisions.Clear(); page.Children.Clear();
        SelectNavigation(currentPage);
        var heading = new TextBlock { Text = session && query.Length > 0 ? "Search results" : title, Style = (Style)Application.Current.Resources["TitleTextBlockStyle"], TextWrapping = TextWrapping.Wrap };
        AutomationProperties.SetHeadingLevel(heading, AutomationHeadingLevel.Level1);
        AutomationProperties.SetAutomationId(heading, "PageHeading");
        page.Children.Add(heading);
        pageSubtitle = string.IsNullOrEmpty(subtitle) ? null : new TextBlock { Text = subtitle, TextWrapping = TextWrapping.Wrap, Opacity = 0.75 };
        if (pageSubtitle is not null) { AutomationProperties.SetAutomationId(pageSubtitle, "PageSubtitle"); page.Children.Add(pageSubtitle); }
        if (search)
        {
            searchBox.PlaceholderText = searchPlaceholder ?? (session ? "Search machine settings (Ctrl+F)" : "Search this page (Ctrl+F)");
            searchBox.Text = query;
            page.Children.Add(searchBox);
        }
    }
    private void AddShortcut(UIElement root, VirtualKey key, VirtualKeyModifiers modifiers, Func<Task> action)
    {
        var shortcut = new KeyboardAccelerator { Key = key, Modifiers = modifiers };
        shortcut.Invoked += async (_, e) => { e.Handled = true; await RunAsync(action); };
        root.KeyboardAccelerators.Add(shortcut);
    }
    private static TextBlock Text(string text) => new() { Text = text, TextWrapping = TextWrapping.Wrap };
    private Button Button(string label, Func<Task> action, string? id = null)
    {
        var button = new Button { Content = label };
        if (id is not null) AutomationProperties.SetAutomationId(button, id);
        button.Click += async (_, _) => await RunAsync(action); return button;
    }
    private Button PrimaryButton(string label, Func<Task> action, string? id = null)
    {
        var button = Button(label, action, id);
        button.Style = (Style)Application.Current.Resources["AccentButtonStyle"];
        return button;
    }
    private static StackPanel Stack(params UIElement[] children)
    { var stack = new StackPanel { Spacing = 10 }; foreach (var child in children) stack.Children.Add(child); return stack; }
    private static Panel Horizontal(params UIElement[] children)
    {
        var panel = new ActionWrapPanel();
        foreach (var child in children) panel.Children.Add(child);
        return panel;
    }
    private sealed class ActionWrapPanel : Panel
    {
        protected override Size MeasureOverride(Size availableSize) => Layout(availableSize.Width, false);
        protected override Size ArrangeOverride(Size finalSize) { Layout(finalSize.Width, true); return finalSize; }
        private Size Layout(double width, bool arrange)
        {
            const double spacing = 8;
            double x = 0, y = 0, rowHeight = 0, usedWidth = 0;
            foreach (var child in Children)
            {
                if (!arrange) child.Measure(new Size(width, double.PositiveInfinity));
                var size = child.DesiredSize;
                if (x > 0 && x + size.Width > width)
                {
                    x = 0; y += rowHeight + spacing; rowHeight = 0;
                }
                if (arrange) child.Arrange(new Rect(x, y, size.Width, size.Height));
                usedWidth = Math.Max(usedWidth, x + size.Width);
                x += size.Width + spacing;
                rowHeight = Math.Max(rowHeight, size.Height);
            }
            return new Size(usedWidth, y + rowHeight);
        }
    }
    private static Border Card(UIElement content) => new() { Child = content, Style = (Style)Application.Current.Resources["NativeCardStyle"] };
    private static readonly string[] SearchFields = ["id", "label", "name", "description", "category", "path", "help", "unit", "constraint", "group"];
    private static bool Matches(JsonNode item, string query)
    {
        foreach (string term in query.Split(' ', StringSplitOptions.RemoveEmptyEntries))
        {
            bool found = false;
            if (item is JsonObject entry)
            {
                foreach (string field in SearchFields)
                    if (entry[field] is JsonValue fieldValue && fieldValue.TryGetValue<string>(out var fieldText) && fieldText.Contains(term, StringComparison.CurrentCultureIgnoreCase))
                    { found = true; break; }
                if (!found && entry["choices"] is JsonArray choices)
                    found = choices.Any(choice => choice is not null && (choice.S("label").Contains(term, StringComparison.CurrentCultureIgnoreCase) || choice.S("value").Contains(term, StringComparison.CurrentCultureIgnoreCase)));
            }
            else if (item is JsonValue scalarValue && scalarValue.TryGetValue<string>(out var scalarText))
                found = scalarText.Contains(term, StringComparison.CurrentCultureIgnoreCase);
            if (!found) return false;
        }
        return true;
    }
    private static readonly (string Id, string Title, Symbol Icon)[] LibraryPages = [("machines", "Machines", Symbol.AllApps), ("recent", "Recent", Symbol.Clock), ("floppies", "Floppy Disks", Symbol.OpenFile), ("cds", "CDs", Symbol.Library), ("whdload", "WHDLoad", Symbol.Download), ("roms", "Kickstart ROMs", Symbol.Library), ("app", "App Settings", Symbol.Setting)];
    private static readonly (string Id, string Title, Symbol Icon)[] EditorPages = [("overview", "Overview", Symbol.Home), ("hardware", "Hardware", Symbol.Repair), ("display", "Display & Sound", Symbol.Volume), ("input", "Input", Symbol.Keyboard), ("storage", "Storage", Symbol.OpenFile), ("ports", "Ports & Misc", Symbol.Switch), ("all", "All Settings", Symbol.List)];
}
