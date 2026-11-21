using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using WinRT.Interop;

namespace Amiberry.Windows;

internal sealed class AmigaKeyboardWindow : Window
{
    private readonly NativeClient core;
    private readonly Action<string> error;
    private readonly SemaphoreSlim events = new(1, 1);
    private readonly HashSet<int> down = [], latched = [], touching = [];
    private readonly Dictionary<int, Button> buttons = [];
    private readonly StackPanel root = new() { Spacing = 6, Margin = new Thickness(12) };
    private readonly List<Grid> keypadRows = [];
    private readonly Dictionary<int, bool> latchBeforePress = [];
    private bool capsLatches, keypad;
    private string language = "";
    private bool closing;
    public AmigaKeyboardWindow(NativeClient core, Action<string> error)
    {
        this.core = core; this.error = error;
        Title = "Amiberry — Amiga Keyboard";
        SystemBackdrop = new MicaBackdrop();
        root.Children.Add(new TextBlock { Text = "Amiga Keyboard · Tap a modifier to hold it for the next key", TextWrapping = TextWrapping.Wrap });
        for (int rowIndex = 0; rowIndex < Keys.Length; rowIndex++)
        {
            var row = Keys[rowIndex];
            var line = new Grid { ColumnSpacing = 4 };
            for (int i = 0; i < row.Length; i++)
            {
                var key = row[i];
                line.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(key.Width, GridUnitType.Star) });
                var button = new KeyButton(
                    () => DispatchAsync(() => { touching.Add(key.Code); return PressAsync(key.Code); }),
                    () => DispatchAsync(() => { touching.Remove(key.Code); return ReleaseAsync(key.Code); }),
                    () => DispatchAsync(() => { touching.Remove(key.Code); return CancelAsync(key.Code); }))
                { Content = key.Label, HorizontalAlignment = HorizontalAlignment.Stretch, MinWidth = 24, MinHeight = 36, Padding = new Thickness(4), IsTabStop = true };
                AutomationProperties.SetName(button, key.Label.Length == 0 ? "Space" : key.Label);
                AutomationProperties.SetAutomationId(button, "AmigaKey_" + key.Code.ToString("X2"));
                buttons[key.Code] = button;
                // Pointer/key handling suppresses ButtonBase's Click; this path is the native UIA Invoke action.
                button.Click += async (_, _) => await DispatchAsync(async () => { await PressAsync(key.Code); await ReleaseAsync(key.Code); });
                Grid.SetColumn(button, i); line.Children.Add(button);
            }
            if (rowIndex >= 6) { keypadRows.Add(line); line.Visibility = Visibility.Collapsed; }
            root.Children.Add(line);
        }
        var release = new Button { Content = "Release All Keys" };
        release.Click += async (_, _) => await DispatchAsync(ReleaseAllAsync); root.Children.Add(release);
        Content = new ScrollViewer { Content = root, HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled };
        AppWindow.Resize(new global::Windows.Graphics.SizeInt32(1100, 480));
        nint handle = WindowNative.GetWindowHandle(this);
        SetWindowLongPtr(handle, -20, GetWindowLongPtr(handle, -20) | 0x08000000 | 0x00000080);
        AppWindow.Closing += async (_, args) => { if (!closing) { args.Cancel = true; await HideAsync(true); } };
    }
    public void ShowWithoutActivation() => ShowWindow(WindowNative.GetWindowHandle(this), 4);
    [DllImport("user32.dll")] private static extern bool ShowWindow(nint window, int command);
    private async Task DispatchAsync(Func<Task> action)
    {
        await events.WaitAsync();
        try { if (!closing) await action(); }
        catch (Exception exception) { error(exception.Message); }
        finally { events.Release(); }
    }
    public async Task ApplyOptionsAsync(JsonNode options, bool capsOn)
    {
        await events.WaitAsync();
        try
        {
            if (closing) return;
            bool nextCapsLatches = options.B("capsLatches"), nextKeypad = options.B("numpad");
            if (capsLatches != nextCapsLatches)
            {
                await ReleaseAllAsync();
                down.Remove(0x62); latched.Remove(0x62);
                capsLatches = nextCapsLatches;
            }
            if (keypad && !nextKeypad)
                foreach (var row in Keys.Skip(6)) foreach (var key in row) await SendAsync(key.Code, false);
            keypad = nextKeypad;
            foreach (var row in keypadRows) row.Visibility = keypad ? Visibility.Visible : Visibility.Collapsed;
            root.Opacity = Math.Clamp(options.D("opacity", 0.85), 0, 1);
            string nextLanguage = options.S("language", "US").ToUpperInvariant();
            if (language != nextLanguage)
            {
                language = nextLanguage;
                foreach (var row in Keys) foreach (var key in row)
                {
                    string legend = Legends.TryGetValue(language, out var overrides) && overrides.TryGetValue(key.Code, out string? translated) ? translated : key.Label;
                    buttons[key.Code].Content = legend;
                    AutomationProperties.SetName(buttons[key.Code], legend.Length == 0 ? "Space" : legend);
                }
            }
            // Caps is the MCU keyboard's guest-driven LED state, not an injected press.
            if (capsLatches && !touching.Contains(0x62))
            {
                if (capsOn) { latched.Add(0x62); down.Add(0x62); }
                else { latched.Remove(0x62); down.Remove(0x62); }
                buttons[0x62].Opacity = capsOn ? 0.55 : 1;
            }
        }
        catch (Exception exception) { error(exception.Message); }
        finally { events.Release(); }
    }
    private async Task SendAsync(int code, bool pressed)
    {
        if (pressed ? !down.Add(code) : !down.Remove(code)) return;
        await core.ActionAsync(pressed ? "key_down" : "key_up", index: code);
        if (buttons.TryGetValue(code, out var button)) button.Opacity = pressed ? 0.55 : 1;
    }
    private static bool Modifier(int code) => code is 0x60 or 0x61 or 0x63 or 0x64 or 0x65 or 0x66 or 0x67;
    private bool Latches(int code) => Modifier(code) || (code == 0x62 && capsLatches);
    private async Task PressAsync(int code)
    {
        if (Latches(code))
        {
            bool previous = latched.Contains(code); latchBeforePress[code] = previous;
            if (previous) latched.Remove(code); else latched.Add(code);
            await SendAsync(code, !previous);
        }
        else await SendAsync(code, true);
    }
    private async Task ReleaseAsync(int code)
    {
        latchBeforePress.Remove(code);
        if (Latches(code)) return;
        await SendAsync(code, false);
        foreach (int modifier in latched.Where(value => value != 0x62).ToArray()) { latched.Remove(modifier); await SendAsync(modifier, false); }
    }
    private async Task CancelAsync(int code)
    {
        if (Latches(code) && latchBeforePress.Remove(code, out bool previous))
        {
            if (previous) latched.Add(code); else latched.Remove(code);
            await SendAsync(code, previous);
        }
        else await SendAsync(code, false);
    }
    private async Task ReleaseAllAsync()
    {
        foreach (int code in down.ToArray())
            if (!(capsLatches && code == 0x62)) await SendAsync(code, false);
        latched.RemoveWhere(code => !(capsLatches && code == 0x62));
        latchBeforePress.Clear(); touching.Clear();
    }
    public async Task HideAsync(bool notifyCore)
    {
        await events.WaitAsync();
        try
        {
            if (closing) return;
            closing = true;
            await ReleaseAllAsync();
            if (notifyCore) await core.CallAsync("keyboard_hide");
        }
        catch (Exception exception) { error(exception.Message); }
        finally { events.Release(); Close(); }
    }
    [DllImport("user32.dll", EntryPoint = "GetWindowLongPtrW")] private static extern nint GetWindowLongPtr(nint window, int index);
    [DllImport("user32.dll", EntryPoint = "SetWindowLongPtrW")] private static extern nint SetWindowLongPtr(nint window, int index, nint value);
    private sealed class KeyButton(Func<Task> press, Func<Task> release, Func<Task> cancel) : Button
    {
        private bool pointerHeld, keyHeld;
        private uint pointerId;
        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            e.Handled = true;
            if (pointerHeld || (e.Pointer.PointerDeviceType == Microsoft.UI.Input.PointerDeviceType.Mouse && !e.GetCurrentPoint(this).Properties.IsLeftButtonPressed)) return;
            pointerHeld = true; pointerId = e.Pointer.PointerId; CapturePointer(e.Pointer); _ = press();
        }
        protected override async void OnPointerReleased(PointerRoutedEventArgs e)
        {
            e.Handled = true;
            if (!pointerHeld || pointerId != e.Pointer.PointerId) return;
            pointerHeld = false; await release(); ReleasePointerCapture(e.Pointer);
        }
        protected override void OnPointerCanceled(PointerRoutedEventArgs e)
        {
            e.Handled = true;
            if (!pointerHeld || pointerId != e.Pointer.PointerId) return;
            pointerHeld = false; _ = cancel();
        }
        protected override void OnPointerCaptureLost(PointerRoutedEventArgs e)
        {
            if (!pointerHeld) return;
            pointerHeld = false; _ = cancel();
        }
        protected override void OnKeyDown(KeyRoutedEventArgs e)
        {
            if (e.Key is not (global::Windows.System.VirtualKey.Space or global::Windows.System.VirtualKey.Enter)) { base.OnKeyDown(e); return; }
            e.Handled = true;
            if (keyHeld) return;
            keyHeld = true; _ = press();
        }
        protected override void OnKeyUp(KeyRoutedEventArgs e)
        {
            if (e.Key is not (global::Windows.System.VirtualKey.Space or global::Windows.System.VirtualKey.Enter)) { base.OnKeyUp(e); return; }
            e.Handled = true;
            if (!keyHeld) return;
            keyHeld = false; _ = release();
        }
        protected override void OnLostFocus(RoutedEventArgs e)
        {
            if (keyHeld) { keyHeld = false; _ = cancel(); }
            base.OnLostFocus(e);
        }
    }
    private static readonly Dictionary<string, Dictionary<int, string>> Legends = new()
    {
        ["UK"] = new() { [0x0D] = "# ~", [0x2B] = "\\", [0x02] = "2 \"", [0x03] = "3 £", [0x2A] = "' @", [0x30] = "\\" },
        ["DE"] = new() { [0x15] = "Z", [0x31] = "Y", [0x1A] = "Ü", [0x1B] = "+ *", [0x29] = "Ö", [0x2A] = "Ä", [0x00] = "^ °", [0x0B] = "ß ?", [0x0C] = "´ `", [0x3A] = "- _", [0x38] = ", ;", [0x39] = ". :", [0x0D] = "# '", [0x30] = "< >", [0x02] = "2 \"", [0x03] = "3 §", [0x06] = "6 &", [0x07] = "7 /", [0x08] = "8 (", [0x09] = "9 )", [0x0A] = "0 =" },
        ["FR"] = new() { [0x10] = "A", [0x11] = "Z", [0x20] = "Q", [0x31] = "W", [0x29] = "M", [0x37] = ", ?", [0x38] = "; .", [0x39] = ": /", [0x3A] = "!", [0x01] = "& 1", [0x02] = "é 2", [0x03] = "\" 3", [0x04] = "' 4", [0x05] = "( 5", [0x06] = "- 6", [0x07] = "è 7", [0x08] = "_ 8", [0x09] = "ç 9", [0x0A] = "à 0", [0x0B] = ")", [0x0C] = "= +", [0x00] = "@ #", [0x2A] = "ù", [0x30] = "< >" }
    };
    private sealed record Key(int Code, string Label, double Width = 1);
    private static Key K(int code, string label, double width = 1) => new(code, label, width);
    private static readonly Key[][] Keys =
    [
        [K(0x45,"Esc",1.5),K(0x50,"F1"),K(0x51,"F2"),K(0x52,"F3"),K(0x53,"F4"),K(0x54,"F5"),K(0x55,"F6"),K(0x56,"F7"),K(0x57,"F8"),K(0x58,"F9"),K(0x59,"F10"),K(0x46,"Del",1.5),K(0x5F,"Help",2)],
        [K(0x00,"` ~"),K(0x01,"1 !"),K(0x02,"2 @"),K(0x03,"3 #"),K(0x04,"4 $"),K(0x05,"5 %"),K(0x06,"6 ^"),K(0x07,"7 &"),K(0x08,"8 *"),K(0x09,"9 ("),K(0x0A,"0 )"),K(0x0B,"- _"),K(0x0C,"= +"),K(0x0D,"\\ |"),K(0x41,"Backspace",2)],
        [K(0x42,"Tab",1.5),K(0x10,"Q"),K(0x11,"W"),K(0x12,"E"),K(0x13,"R"),K(0x14,"T"),K(0x15,"Y"),K(0x16,"U"),K(0x17,"I"),K(0x18,"O"),K(0x19,"P"),K(0x1A,"[ {"),K(0x1B,"] }"),K(0x44,"Return",2)],
        [K(0x63,"Ctrl",1.5),K(0x62,"Caps",1.5),K(0x20,"A"),K(0x21,"S"),K(0x22,"D"),K(0x23,"F"),K(0x24,"G"),K(0x25,"H"),K(0x26,"J"),K(0x27,"K"),K(0x28,"L"),K(0x29,"; :"),K(0x2A,"' \""),K(0x2B,"#")],
        [K(0x60,"L Shift",1.5),K(0x30,"< >"),K(0x31,"Z"),K(0x32,"X"),K(0x33,"C"),K(0x34,"V"),K(0x35,"B"),K(0x36,"N"),K(0x37,"M"),K(0x38,", <"),K(0x39,". >"),K(0x3A,"/ ?"),K(0x61,"R Shift",2),K(0x4C,"↑")],
        [K(0x64,"L Alt",1.5),K(0x66,"L Amiga",1.5),K(0x40,"Space",7),K(0x67,"R Amiga",1.5),K(0x65,"R Alt",1.5),K(0x4F,"←"),K(0x4D,"↓"),K(0x4E,"→")],
        [K(0x5A,"Num ("),K(0x5B,"Num )"),K(0x5C,"Num /"),K(0x5D,"Num *"),K(0x3D,"Num 7"),K(0x3E,"Num 8"),K(0x3F,"Num 9"),K(0x4A,"Num -"),K(0x2D,"Num 4"),K(0x2E,"Num 5"),K(0x2F,"Num 6")],
        [K(0x5E,"Num +"),K(0x1D,"Num 1"),K(0x1E,"Num 2"),K(0x1F,"Num 3"),K(0x0F,"Num 0",2),K(0x3C,"Num ."),K(0x43,"Num Enter",2)]
    ];
}
