using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Automation;

namespace Amiberry.Windows;

public sealed partial class MainWindow
{
    private readonly Dictionary<int, TextBlock> driveStatus = [];
    private readonly Dictionary<int, (Button Insert, Button Eject)> driveActions = [];
    private Button? ejectCd;
    private TextBlock? cdStatus;
    private AmigaKeyboardWindow? keyboard;

    private void RenderRuntime()
    {
        currentPage = "runtime"; SelectNavigation(currentPage);
        BeginPage("Drives & Save States", "These actions affect the current Amiga. Use the sidebar to return to machine settings.");
        driveStatus.Clear(); driveActions.Clear();
        var disabledDrives = new StackPanel { Spacing = 10 };
        foreach (var drive in status.Items("floppy"))
        {
            int number = drive.I("drive");
            var label = Text(""); label.IsTextSelectionEnabled = true; driveStatus[number] = label;
            var insert = Button("Insert Disk…", async () =>
            {
                if (await PickFileAsync() is { } path) await Core.ActionAsync("insert_floppy", path, drive: number);
            });
            var eject = Button("Eject", () => Core.ActionAsync("eject_floppy", drive: number));
            AutomationProperties.SetName(insert, "Insert disk in DF" + number);
            AutomationProperties.SetName(eject, "Eject disk from DF" + number);
            driveActions[number] = (insert, eject);
            var heading = new TextBlock { Text = "Floppy drive DF" + number, Style = (Style)Application.Current.Resources["SubtitleTextBlockStyle"] };
            var card = Card(Stack(heading, label, Horizontal(insert, eject)));
            if (drive.B("disabled")) disabledDrives.Children.Add(card);
            else page.Children.Add(card);
        }
        cdStatus = Text(""); cdStatus.IsTextSelectionEnabled = true;
        ejectCd = Button("Eject", () => Core.ActionAsync("eject_cd"));
        AutomationProperties.SetName(ejectCd, "Eject CD");
        page.Children.Add(Card(Stack(new TextBlock { Text = "CD drive", Style = (Style)Application.Current.Resources["SubtitleTextBlockStyle"] }, cdStatus,
            Horizontal(Button("Insert CD…", async () => { if (await PickFileAsync() is { } path) await InsertCdAsync(path); }), ejectCd))));
        var slots = new ComboBox { Header = "Save-state slot", Width = 240, HorizontalAlignment = HorizontalAlignment.Left };
        AutomationProperties.SetAutomationId(slots, "StateSlotPicker");
        for (int i = 0; i < 10; i++) slots.Items.Add("Slot " + i);
        slots.SelectedIndex = 0;
        page.Children.Add(Card(Stack(new TextBlock { Text = "Save states", Style = (Style)Application.Current.Resources["SubtitleTextBlockStyle"] },
            Text("Save the current session to a slot or file. Loading a state replaces the current Amiga session."), slots,
            Horizontal(Button("Save Slot", () => Core.ActionAsync("save_slot", index: slots.SelectedIndex)),
                Button("Load Slot…", () => ConfirmActionAsync("Load save-state slot " + slots.SelectedIndex + "?", "Unsaved work in the Amiga will be lost.", () => Core.ActionAsync("load_slot", index: slots.SelectedIndex), "Load State"))),
            Horizontal(Button("Save State File…", async () => { if (await PickFileAsync(true, ".uss", "Amiga State") is { } path) await Core.ActionAsync("save_state", path); }),
                Button("Load State File…", async () =>
                {
                    if (await PickFileAsync(false, ".uss") is { } path)
                        await ConfirmActionAsync("Load " + Path.GetFileName(path) + "?", "Unsaved work in the Amiga will be lost.", () => Core.ActionAsync("load_state", path), "Load State");
                })))));
        if (disabledDrives.Children.Count > 0)
            page.Children.Add(Disclosure("runtime:disabled-drives", $"Disabled floppy drives ({disabledDrives.Children.Count})", disabledDrives, false));
        var states = library.Items("states").ToList();
        if (states.Count > 0)
        {
            page.Children.Add(new TextBlock { Text = "Saved state files", Style = (Style)Application.Current.Resources["SubtitleTextBlockStyle"] });
            foreach (var state in states)
                page.Children.Add(Button("Load " + state.S("name") + "…", () => ConfirmActionAsync("Load saved state?", "Unsaved work in the Amiga will be lost.", () => Core.ActionAsync("load_state", state.S("path")), "Load State")));
        }
        page.Children.Add(Button("Show Amiga Keyboard", async () => { await Core.CallAsync("keyboard_toggle"); }));
        UpdateRuntimeStatus();
    }
    // Start is finished once the emulation runs, or once F12 has already paused it behind its settings loop: the
    // core clears settingsOpen before it acknowledges a start, so settingsOpen on a paused machine is a new F12.
    // Waiting only for "running" held the UI lock until the 60 s limit while the editor stayed hidden.
    private async Task WaitForRunningAsync()
    {
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
        deadline.CancelAfter(TimeSpan.FromSeconds(60));
        while (true)
        {
            var next = await Core.CallAsync("status", cancellation: deadline.Token);
            string state = next.S("state");
            if (state == "running" || (state == "paused" && next.B("settingsOpen"))) { status = next; SyncWindowVisibility(); return; }
            await Task.Delay(100, deadline.Token);
        }
    }
    private async Task InsertCdAsync(string path)
    {
        await Core.ActionAsync("insert_cd", path);
        await RememberCdAsync(path);
    }
    private async Task RememberCdAsync(string path)
    {
        if (await Task.Run(() => File.Exists(path)))
            library = await Core.CallAsync("recent_add", new { list = 1, path });
    }
    private void UpdateRuntimeStatus()
    {
        foreach (var drive in status.Items("floppy"))
        {
            int number = drive.I("drive");
            bool disabled = drive.B("disabled");
            string path = drive.S("path");
            if (driveStatus.TryGetValue(number, out var label))
                label.Text = disabled ? "Not enabled. Enable this drive in Storage to insert a disk." : (path.Length == 0 ? "No disk inserted" : path) + (drive.B("motor") ? " · Motor on" : "") + (drive.B("writing") ? " · Writing" : "");
            if (driveActions.TryGetValue(number, out var actions))
            {
                actions.Insert.IsEnabled = !disabled;
                actions.Eject.IsEnabled = !disabled && path.Length > 0;
            }
        }
        if (cdStatus is not null) cdStatus.Text = status.S("cdPath").Length == 0 ? "No CD inserted" : status.S("cdPath");
        if (ejectCd is not null) ejectCd.IsEnabled = status.S("cdPath").Length > 0;
    }
    private async Task UpdateKeyboardAsync()
    {
        var options = status["keyboardOptions"];
        if (status.B("keyboardVisible") && options is not null && options.B("enabled"))
        {
            if (keyboard is null)
            {
                keyboard = new AmigaKeyboardWindow(Core, ShowError);
                var created = keyboard;
                created.Closed += (_, _) => { if (ReferenceEquals(keyboard, created)) keyboard = null; };
                await created.ApplyOptionsAsync(options, status.B("capsLock"));
                created.ShowWithoutActivation();
            }
            else await keyboard.ApplyOptionsAsync(options, status.B("capsLock"));
        }
        else if (keyboard is not null)
        {
            var existing = keyboard; keyboard = null;
            await existing.HideAsync(false);
        }
    }
}
