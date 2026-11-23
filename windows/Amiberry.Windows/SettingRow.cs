using System.Globalization;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using global::Windows.System;

namespace Amiberry.Windows;

internal sealed class SettingRow
{
    // Windows 11 Settings-style row: label/context on the left, a bounded control on the right.
    // The control drops below the label only when the row is too narrow for both.
    private const double MinLabelWidth = 200, ColumnGap = 16, HelpButtonWidth = 28, InlineHelpLimit = 60;

    private readonly JsonNode descriptor;
    private JsonNode state;
    private readonly Func<string, Task> setter;
    private readonly Func<string, Func<Task>?, Task> draft;
    private readonly Func<Func<Task>, Task> run;
    private readonly Func<Task<string?>> browse;
    private readonly Func<uint, uint, Task<string>> formatHotkey;
    private readonly TextBox? text;
    private readonly ToggleSwitch? toggle;
    private readonly ComboBox? choice;
    private readonly NumberBox? number;
    private readonly Slider? slider;
    private readonly TextBlock titleText;
    private readonly TextBlock caption = Themed("TextFillColorSecondaryBrush");
    private readonly TextBlock note = Themed("TextFillColorPrimaryBrush");
    private readonly TextBlock error = Themed("SystemFillColorCriticalBrush");
    private readonly HyperlinkButton discard = new() { Content = "Discard this change", Padding = new Thickness(0, 2, 0, 2), HorizontalAlignment = HorizontalAlignment.Left, Visibility = Visibility.Collapsed };
    private readonly StackPanel feedbackStack = new() { Margin = new Thickness(0, 4, 0, 0), Visibility = Visibility.Collapsed };
    private readonly Button helpButton;
    private readonly TextBlock helpBody = new() { TextWrapping = TextWrapping.Wrap, MaxWidth = 340, IsTextSelectionEnabled = true };
    private readonly Grid controlCell = new() { VerticalAlignment = VerticalAlignment.Center };
    private readonly ContentControl controlHost = new() { HorizontalContentAlignment = HorizontalAlignment.Stretch, VerticalAlignment = VerticalAlignment.Center, IsTabStop = false };
    private readonly FrameworkElement control;
    private readonly ContentControl rowContainer = new() { HorizontalContentAlignment = HorizontalAlignment.Stretch, IsTabStop = false };
    private readonly string help, unit, shortHelp, displayLabel;
    private readonly double wideWidth;
    private readonly bool canStack, stretchWhenStacked;
    private Button? recordButton;
    private MenuFlyoutItem? clearItem;
    private string rangeText = "";
    private bool? stacked;
    private bool updating, changed, recording;
    private string stored = "";
    private uint recordedModifiers;
    private long editRevision;
    private bool recordedKeyPressed;
    public string Id => descriptor.S("id");
    public FrameworkElement Element { get; }
    private double Scale => descriptor.D("scale", 1) is var value && value != 0 ? value : 1;
    private double Minimum => state.D("min", descriptor.D("min"));
    private double Maximum => state.D("max", descriptor.D("max"));
    private bool Available => state.B("available", true);
    private string Reason => descriptor.S("constraint", "Not available with the current configuration.");
    private double Shown(double value) => (descriptor.B("reversed") ? descriptor.D("min") + descriptor.D("max") - value : value) / Scale;
    private double Stored(double value)
    {
        double result = value * Scale;
        if (descriptor.B("reversed")) result = descriptor.D("min") + descriptor.D("max") - result;
        double step = descriptor.D("step");
        if (step > 0) { double origin = descriptor.D("min"); result = origin + Math.Round((result - origin) / step, MidpointRounding.AwayFromZero) * step; }
        return descriptor.S("type") == "int" ? Math.Round(result) : result;
    }

    // Text whose brush is a theme resource resolved by the framework, so it follows live theme changes.
    private static TextBlock Themed(string brush)
    {
        var block = (TextBlock)XamlReader.Load("<TextBlock xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Foreground=\"{ThemeResource " + brush + "}\" TextWrapping=\"Wrap\"/>");
        block.FontSize = 12;
        block.Visibility = Visibility.Collapsed;
        AutomationProperties.SetLiveSetting(block, AutomationLiveSetting.Polite);
        return block;
    }

    public SettingRow(JsonNode descriptor, JsonNode state, Func<string, Task> setter,
        Func<string, Func<Task>?, Task> draft, Func<Func<Task>, Task> run,
        Func<Task<string?>> browse, Func<uint, uint, Task<string>> formatHotkey)
    {
        this.descriptor = descriptor; this.state = state; this.setter = setter; this.draft = draft; this.run = run; this.browse = browse; this.formatHotkey = formatHotkey;
        string label = descriptor.S("label", Id);
        displayLabel = label;
        string type = descriptor.S("type");
        help = descriptor.S("help"); unit = descriptor.S("unit");
        shortHelp = help.Length is > 0 and <= (int)InlineHelpLimit && !help.Contains('\n') ? help : "";
        titleText = new TextBlock { Text = label, TextWrapping = TextWrapping.Wrap, VerticalAlignment = VerticalAlignment.Center };
        discard.Click += async (_, _) => await DiscardAsync();
        AutomationProperties.SetAutomationId(discard, "SettingDiscard_" + Id);
        AutomationProperties.SetName(discard, "Discard the unapplied change to " + label);

        controlCell.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        controlCell.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        if (type == "bool")
        {
            toggle = new ToggleSwitch { OnContent = "On", OffContent = "Off", MinWidth = 0, HorizontalAlignment = HorizontalAlignment.Right };
            toggle.Toggled += async (_, _) => { if (!updating) { await MarkChangedAsync(); await run(CommitAsync); } };
            Grid.SetColumnSpan(toggle, 2);
            controlCell.Children.Add(toggle);
            control = toggle; wideWidth = 0; canStack = false; stretchWhenStacked = false;
        }
        else if (type == "choice")
        {
            choice = new ComboBox { HorizontalAlignment = HorizontalAlignment.Stretch };
            choice.SelectionChanged += async (_, _) =>
            {
                if (!updating && choice.SelectedItem is ComboBoxItem) { await MarkChangedAsync(); await run(CommitAsync); }
            };
            Grid.SetColumnSpan(choice, 2);
            controlCell.Children.Add(choice);
            control = choice; wideWidth = 320; canStack = true; stretchWhenStacked = true;
        }
        else if (type is "int" or "real")
        {
            number = new NumberBox { SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Compact, ValidationMode = NumberBoxValidationMode.Disabled, HorizontalAlignment = HorizontalAlignment.Stretch, PlaceholderText = "Enter a number" };
            number.ValueChanged += async (_, _) => { if (!updating) await MarkChangedAsync(); };
            number.LostFocus += async (_, _) => { if (changed) await run(CommitAsync); };
            number.KeyDown += async (_, e) => { if (e.Key == VirtualKey.Enter && changed) { e.Handled = true; await run(CommitAsync); } };
            if (descriptor.B("level"))
            {
                slider = new Slider { MinWidth = 100, VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 12, 0) };
                slider.ValueChanged += async (_, _) =>
                {
                    if (!updating)
                    {
                        updating = true; number.Value = slider.Value; updating = false;
                        await MarkChangedAsync();
                    }
                };
                slider.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(async (_, _) => { if (changed) await run(CommitAsync); }), true);
                slider.KeyUp += async (_, _) => { if (changed) await run(CommitAsync); };
                AutomationProperties.SetName(slider, label + " slider");
                AutomationProperties.SetAutomationId(slider, "SettingSlider_" + Id);
                AutomationProperties.SetLabeledBy(slider, titleText);
                controlCell.Children.Add(slider);
                number.Width = 120;
                Grid.SetColumn(number, 1);
                controlCell.Children.Add(number);
                wideWidth = 340; stretchWhenStacked = true;
            }
            else
            {
                Grid.SetColumnSpan(number, 2);
                controlCell.Children.Add(number);
                wideWidth = 140; stretchWhenStacked = false;
            }
            control = number; canStack = true;
        }
        else
        {
            text = new TextBox { TextWrapping = TextWrapping.NoWrap, HorizontalAlignment = HorizontalAlignment.Stretch, VerticalAlignment = VerticalAlignment.Center };
            // Mark the edit from TextChanging, which runs synchronously with the change. TextChanged is raised
            // asynchronously: an action invoked right after an edit (UI Automation SetValue, then Invoke) could
            // flush drafts before the edit registered, and when Browse or Clear set the text and commit at once,
            // the late TextChanged marked the committed value as a new draft.
            text.TextChanging += async (_, _) => { if (!updating) await MarkChangedAsync(); };
            text.TextChanged += (_, _) => { if (clearItem is not null) clearItem.IsEnabled = text.Text.Length > 0; };
            text.LostFocus += async (_, _) =>
            {
                if (recording)
                {
                    // Moving to the Record button toggles recording off through its own click.
                    if (text.XamlRoot is { } root && ReferenceEquals(FocusManager.GetFocusedElement(root), recordButton)) return;
                    CancelRecording(false);
                }
                if (changed) await run(CommitAsync);
            };
            text.KeyDown += async (_, e) =>
            {
                if (recording)
                {
                    e.Handled = true;
                    uint usage = WindowsKeyboard.Usage(e.KeyStatus.ScanCode, e.KeyStatus.IsExtendedKey, e.Key == VirtualKey.Pause);
                    uint modifier = WindowsKeyboard.Modifier(usage);
                    if (modifier != 0) { recordedModifiers |= modifier; return; }
                    uint modifiers = Modifiers();
                    if (usage == 41 && modifiers == 0) { CancelRecording(true); return; }
                    if (usage == 0) { ShowFeedback("This physical key cannot be recorded as a shortcut. Press another key, or Escape to cancel.", true); return; }
                    recordedKeyPressed = true; SetRecording(false);
                    await CaptureHotkeyAsync(usage, descriptor.B("takesModifiers", true) ? modifiers : 0);
                }
                else if (e.Key == VirtualKey.Enter && changed) { e.Handled = true; await run(CommitAsync); }
            };
            text.KeyUp += async (_, e) =>
            {
                if (!recording || WindowsKeyboard.Modifier(WindowsKeyboard.Usage(e.KeyStatus.ScanCode, e.KeyStatus.IsExtendedKey, e.Key == VirtualKey.Pause)) == 0) return;
                e.Handled = true;
                if (!recordedKeyPressed && recordedModifiers != 0 && (recordedModifiers & (recordedModifiers - 1)) == 0)
                {
                    uint modifier = recordedModifiers; recordedModifiers = 0; SetRecording(false);
                    await CaptureHotkeyAsync(0, modifier);
                }
                else if (Modifiers() == 0) { recordedModifiers = 0; recordedKeyPressed = false; }
            };
            AutomationProperties.SetLabeledBy(text, titleText);
            clearItem = new MenuFlyoutItem { Text = "Clear", IsEnabled = false };
            clearItem.Click += async (_, _) => await run(async () => { SetRecording(false); text.Text = ""; await CommitAsync(); });
            AutomationProperties.SetAutomationId(clearItem, "SettingClear_" + Id);
            if (type is "path" or "file")
            {
                // Entry and Browse share one line; Clear lives in the split button's flyout.
                text.PlaceholderText = "Not set";
                var menu = new MenuFlyout();
                menu.Items.Add(clearItem);
                var choose = new SplitButton { Content = "Browse…", Flyout = menu, Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center };
                choose.Click += async (_, _) => await run(async () => { if (await browse() is { } path) { text.Text = path; await CommitAsync(); } });
                AutomationProperties.SetName(choose, "Browse for " + label);
                AutomationProperties.SetAutomationId(choose, "SettingBrowse_" + Id);
                ToolTipService.SetToolTip(choose, "Browse for " + label + ". Use the arrow to clear it.");
                Grid.SetColumn(choose, 1);
                controlCell.Children.Add(text);
                controlCell.Children.Add(choose);
                wideWidth = 400;
            }
            else if (type == "hotkey")
            {
                text.PlaceholderText = "No shortcut";
                controlCell.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
                controlCell.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
                Grid.SetColumnSpan(text, 2);
                controlCell.Children.Add(text);
                var buttons = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right, Margin = new Thickness(0, 6, 0, 0) };
                Grid.SetRow(buttons, 1); Grid.SetColumnSpan(buttons, 2);
                recordButton = new Button { Content = "Record shortcut" };
                recordButton.Click += (_, _) =>
                {
                    if (recording) { CancelRecording(false); return; }
                    SetRecording(true);
                    ShowFeedback("Press the key combination now. Press Escape to cancel.");
                    text.Focus(FocusState.Programmatic);
                };
                AutomationProperties.SetName(recordButton, "Record shortcut for " + label);
                AutomationProperties.SetAutomationId(recordButton, "SettingRecord_" + Id);
                ToolTipService.SetToolTip(recordButton, "Press this, then the key combination to use. Escape cancels.");
                buttons.Children.Add(recordButton);
                var menu = new MenuFlyout();
                var escape = new MenuFlyoutItem { Text = "Escape Key" };
                escape.Click += async (_, _) => await run(async () => { SetRecording(false); text.Text = await formatHotkey(41, 0); await CommitAsync(); });
                menu.Items.Add(escape);
                foreach (var controller in descriptor.Items("buttons"))
                {
                    var item = new MenuFlyoutItem { Text = controller.S("label") };
                    item.Click += async (_, _) => await run(async () => { SetRecording(false); text.Text = controller.S("value"); await CommitAsync(); });
                    menu.Items.Add(item);
                }
                menu.Items.Add(new MenuFlyoutSeparator());
                menu.Items.Add(clearItem);
                var pick = new DropDownButton { Content = "Choose Key / Button", Flyout = menu };
                AutomationProperties.SetName(pick, "Choose key or button for " + label);
                AutomationProperties.SetAutomationId(pick, "SettingChoose_" + Id);
                ToolTipService.SetToolTip(pick, "Pick a key or controller button from a list, or clear the shortcut");
                buttons.Children.Add(pick);
                controlCell.Children.Add(buttons);
                wideWidth = 400;
            }
            else
            {
                // Plain text: the TextBox's own delete button clears it; focus loss or Enter commits.
                Grid.SetColumnSpan(text, 2);
                controlCell.Children.Add(text);
                wideWidth = 400;
            }
            control = text; canStack = true; stretchWhenStacked = true;
        }

        helpButton = new Button
        {
            Content = new FontIcon { Glyph = "\uE946", FontSize = 14, Opacity = 0.8 },
            Width = HelpButtonWidth, Height = HelpButtonWidth, Padding = new Thickness(0),
            Margin = new Thickness(2, -4, 0, -4), VerticalAlignment = VerticalAlignment.Center,
            Background = new SolidColorBrush(Microsoft.UI.Colors.Transparent), BorderThickness = new Thickness(0),
            Flyout = new Flyout { Content = helpBody },
            Visibility = help.Length > 0 && shortHelp.Length == 0 ? Visibility.Visible : Visibility.Collapsed,
        };
        AutomationProperties.SetName(helpButton, "Help for " + label);
        AutomationProperties.SetAutomationId(helpButton, "SettingHelp_" + Id);
        ToolTipService.SetToolTip(helpButton, help);

        var title = new StackPanel { Orientation = Orientation.Horizontal };
        title.Children.Add(titleText);
        title.Children.Add(helpButton);
        var labelStack = new StackPanel { VerticalAlignment = VerticalAlignment.Center };
        labelStack.Children.Add(title);
        labelStack.Children.Add(caption);
        feedbackStack.Children.Add(note);
        feedbackStack.Children.Add(error);
        feedbackStack.Children.Add(discard);

        var root = new Grid { MinHeight = 44, Padding = new Thickness(0, 6, 0, 6), ColumnSpacing = ColumnGap };
        root.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        root.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        for (int i = 0; i < 3; i++) root.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        root.Children.Add(labelStack);
        controlHost.Content = controlCell;
        root.Children.Add(controlHost);
        Grid.SetRow(feedbackStack, 2); Grid.SetColumnSpan(feedbackStack, 2);
        root.Children.Add(feedbackStack);

        AutomationProperties.SetName(control, label);
        AutomationProperties.SetAutomationId(control, "Setting_" + Id);
        AutomationProperties.SetLabeledBy(control, titleText);
        rowContainer.Content = root;
        rowContainer.SizeChanged += (_, e) => ApplyLayout(e.NewSize.Width);
        Element = rowContainer;
        Place(false);
        Update(state);
    }

    private void ApplyLayout(double width)
    {
        if (width <= 0) return;
        bool stack = canStack && width < wideWidth + MinLabelWidth + ColumnGap;
        double reserve = helpButton.Visibility == Visibility.Visible ? HelpButtonWidth + 2 : 0;
        titleText.MaxWidth = Math.Max(80, (stack ? width : width - Math.Max(wideWidth, 100) - ColumnGap) - reserve);
        if (stacked == stack) return;
        Place(stack);
    }

    private void Place(bool stack)
    {
        stacked = stack;
        if (stack)
        {
            Grid.SetRow(controlHost, 1); Grid.SetColumn(controlHost, 0); Grid.SetColumnSpan(controlHost, 2);
            controlHost.Width = stretchWhenStacked ? double.NaN : wideWidth;
            controlHost.HorizontalAlignment = stretchWhenStacked ? HorizontalAlignment.Stretch : HorizontalAlignment.Left;
            controlHost.Margin = new Thickness(0, 6, 0, 0);
        }
        else
        {
            Grid.SetRow(controlHost, 0); Grid.SetColumn(controlHost, 1); Grid.SetColumnSpan(controlHost, 1);
            controlHost.Width = wideWidth > 0 ? wideWidth : double.NaN;
            controlHost.HorizontalAlignment = HorizontalAlignment.Right;
            controlHost.Margin = new Thickness(0);
        }
    }

    // Notes and errors collapse to zero height when empty. A rejected edit that is still pending
    // offers a way back to the value the machine actually holds.
    private void ShowFeedback(string message, bool isError = false)
    {
        note.Text = isError ? "" : message; error.Text = isError ? message : "";
        note.Visibility = note.Text.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        error.Visibility = error.Text.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        discard.Visibility = isError && changed ? Visibility.Visible : Visibility.Collapsed;
        feedbackStack.Visibility = note.Text.Length > 0 || error.Text.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
    }

    // Short help, range and unit stay visible under the label; long help lives behind the help button.
    private void RefreshContext()
    {
        string caption = string.Join(" · ", new[] { shortHelp, rangeText, unit }.Where(part => part.Length > 0));
        this.caption.Text = caption;
        this.caption.Visibility = caption.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        string full = Available || help.Length == 0 ? help : help + "\n\n" + Reason;
        if (!Available && help.Length == 0) full = Reason;
        helpBody.Text = full;
        AutomationProperties.SetHelpText(control, full);
    }

    public void Update(JsonNode value)
    {
        state = value; stored = value.S("value");
        controlHost.IsEnabled = Available;
        RefreshContext();
        if (changed || recording) return;
        updating = true;
        try
        {
            if (toggle is not null) toggle.IsOn = stored is "true" or "1";
            if (text is not null)
            {
                text.Text = stored;
                if (clearItem is not null) clearItem.IsEnabled = stored.Length > 0;
            }
            if (choice is not null)
            {
                choice.Items.Clear();
                var choices = (value["choices"] is JsonArray ? value.Items("choices") : descriptor.Items("choices")).ToList();
                if (!choices.Any(item => item.S("value") == stored)) choices.Insert(0, new JsonObject { ["value"] = stored, ["label"] = stored.Length == 0 ? "None" : stored });
                foreach (var item in choices)
                {
                    bool choiceAvailable = item.B("available", true);
                    var entry = new ComboBoxItem { Content = item.S("label") + (choiceAvailable ? "" : " (unavailable)"), Tag = item.S("value"), IsEnabled = choiceAvailable };
                    if (!choiceAvailable) ToolTipService.SetToolTip(entry, Reason);
                    choice.Items.Add(entry);
                }
                choice.SelectedItem = choice.Items.OfType<ComboBoxItem>().FirstOrDefault(item => (string)item.Tag == stored);
            }
            if (number is not null)
            {
                double low = Math.Min(Shown(Minimum), Shown(Maximum)), high = Math.Max(Shown(Minimum), Shown(Maximum));
                number.Minimum = low; number.Maximum = high;
                number.SmallChange = descriptor.D("step", 1) / Scale;
                if (number.SmallChange <= 0) number.SmallChange = 1;
                number.LargeChange = number.SmallChange * 10;
                number.Value = double.TryParse(stored, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsed) ? Shown(parsed) : double.NaN;
                if (slider is not null && high > low)
                {
                    slider.Maximum = Math.Max(slider.Maximum, high); slider.Minimum = low; slider.Maximum = high;
                    slider.StepFrequency = number.SmallChange; slider.Value = double.IsFinite(number.Value) ? number.Value : low;
                }
                rangeText = slider is null && high > low ? low.ToString("0.###", CultureInfo.CurrentCulture) + "–" + high.ToString("0.###", CultureInfo.CurrentCulture) : "";
                RefreshContext();
            }
            ShowFeedback(Available ? "" : Reason);
        }
        finally { updating = false; }
    }

    private Task MarkChangedAsync()
    {
        editRevision++;
        changed = true;
        return draft(Id, CommitAsync);
    }

    // Abandons an unapplied edit and shows the value the machine holds again.
    private async Task DiscardAsync()
    {
        recording = false; changed = false; editRevision++;
        await draft(Id, null);
        Update(state);
    }

    private void SetRecording(bool on)
    {
        recording = on;
        if (on) { recordedModifiers = 0; recordedKeyPressed = false; }
        if (recordButton is null) return;
        recordButton.Content = on ? "Cancel recording" : "Record shortcut";
        AutomationProperties.SetName(recordButton, (on ? "Cancel recording for " : "Record shortcut for ") + displayLabel);
    }

    private void CancelRecording(bool returnFocus)
    {
        if (!recording) return;
        SetRecording(false);
        ShowFeedback("");
        Update(state);
        if (returnFocus) recordButton?.Focus(FocusState.Programmatic);
    }

    // The key is already captured; a rejected combination is reported on the row and the
    // Record button regains focus so the keyboard user can try again.
    private async Task CaptureHotkeyAsync(uint usage, uint modifiers)
    {
        await run(async () =>
        {
            try { text!.Text = await formatHotkey(usage, modifiers); }
            catch (Exception failure) { ShowFeedback(failure.Message + " Use Record shortcut to try another combination.", true); throw; }
            await CommitAsync();
        });
        recordButton?.Focus(FocusState.Programmatic);
    }

    // Backend failures name settings by id; the row already carries the label, so only the reason is shown.
    private string Describe(Exception failure)
    {
        string message = failure.Message, prefix = "Setting " + Id + ": ";
        if (message.StartsWith(prefix, StringComparison.Ordinal)) message = message[prefix.Length..];
        if (message.Length > 0 && !message.EndsWith('.')) message += ".";
        return message + " Correct the value or discard the change.";
    }

    public async Task CommitAsync()
    {
        if (!changed) return;
        try
        {
            string value;
            if (toggle is not null) value = toggle.IsOn ? "true" : "false";
            else if (choice?.SelectedItem is ComboBoxItem selected) value = (string)selected.Tag;
            else if (number is not null)
            {
                if (!double.IsFinite(number.Value)) throw new InvalidOperationException(descriptor.S("label") + ": enter a valid number.");
                double raw = Stored(number.Value);
                if (raw < Minimum || raw > Maximum || Math.Abs(Shown(raw) - number.Value) > 1e-8 * Math.Max(1, Math.Abs(number.Value)))
                    throw new InvalidOperationException(descriptor.S("label") + ": enter a number in the indicated range and step.");
                value = raw.ToString("G17", CultureInfo.InvariantCulture);
            }
            else value = text?.Text ?? stored;
            await SubmitAsync(value);
        }
        catch (Exception failure) { ShowFeedback(Describe(failure), true); throw; }
    }
    private async Task SubmitAsync(string value)
    {
        long submittedRevision = editRevision;
        await setter(value);
        if (editRevision != submittedRevision) return;
        changed = false;
        await draft(Id, null);
        Update(state);
    }

    [DllImport("user32.dll")] private static extern short GetKeyState(int key);
    private static uint Modifiers()
    {
        uint bits = 0;
        int[] keys = [0xA2, 0xA3, 0xA0, 0xA1, 0xA4, 0xA5, 0x5B, 0x5C];
        for (int i = 0; i < keys.Length; i++) if ((GetKeyState(keys[i]) & 0x8000) != 0) bits |= 1u << i;
        return bits;
    }
}
