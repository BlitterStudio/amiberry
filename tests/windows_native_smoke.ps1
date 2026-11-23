param(
    [Parameter(Mandatory = $true)][string]$AmiberryPath,
    [string]$ScreenshotPath
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
if (-not ('AmiberrySmokeDesktop' -as [type])) {
    Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
public static class AmiberrySmokeDesktop {
    [DllImport("dwmapi.dll")] public static extern int DwmFlush();
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(System.IntPtr window);
    [DllImport("user32.dll")] public static extern System.IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(System.IntPtr window);
    [DllImport("user32.dll")] public static extern System.IntPtr GetWindow(System.IntPtr window, uint command);
    [DllImport("user32.dll")] public static extern int GetSystemMetrics(int index);
    [StructLayout(LayoutKind.Sequential)]
    private struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("dwmapi.dll")]
    private static extern int DwmGetWindowAttribute(System.IntPtr window, int attribute, out Rect value, int size);
    // DWMWA_EXTENDED_FRAME_BOUNDS: the visible frame, without the invisible resize margins.
    public static int[] GetVisibleFrame(System.IntPtr window) {
        Rect frame;
        if (DwmGetWindowAttribute(window, 9, out frame, Marshal.SizeOf(typeof(Rect))) != 0) return null;
        return new int[] { frame.Left, frame.Top, frame.Right, frame.Bottom };
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct MonitorInfo { public int Size; public Rect Monitor, Work; public uint Flags; }
    [DllImport("user32.dll")] private static extern System.IntPtr MonitorFromWindow(System.IntPtr window, uint flags);
    [DllImport("user32.dll")] private static extern bool GetMonitorInfo(System.IntPtr monitor, ref MonitorInfo info);
    // The work area (monitor minus taskbar) of the monitor nearest to the window.
    public static int[] WorkArea(System.IntPtr window) {
        var info = new MonitorInfo { Size = Marshal.SizeOf(typeof(MonitorInfo)) };
        if (!GetMonitorInfo(MonitorFromWindow(window, 2), ref info)) return null;
        return new int[] { info.Work.Left, info.Work.Top, info.Work.Right, info.Work.Bottom };
    }
    private delegate bool EnumProc(System.IntPtr window, System.IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback, System.IntPtr parameter);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(System.IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetClassName(System.IntPtr window, System.Text.StringBuilder text, int count);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowText(System.IntPtr window, System.Text.StringBuilder text, int count);
    public static System.IntPtr[] TopLevelWindows() {
        var windows = new System.Collections.Generic.List<System.IntPtr>();
        EnumWindows(delegate(System.IntPtr window, System.IntPtr parameter) { windows.Add(window); return true; }, System.IntPtr.Zero);
        return windows.ToArray();
    }
    public static uint ProcessId(System.IntPtr window) { uint id; GetWindowThreadProcessId(window, out id); return id; }
    public static string ClassName(System.IntPtr window) {
        var text = new System.Text.StringBuilder(256); GetClassName(window, text, text.Capacity); return text.ToString();
    }
    public static string Title(System.IntPtr window) {
        var text = new System.Text.StringBuilder(256); GetWindowText(window, text, text.Capacity); return text.ToString();
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct MouseInput {
        public int X, Y;
        public uint Data, Flags, Time;
        public System.UIntPtr Extra;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct KeyboardInput {
        public ushort Key, Scan;
        public uint Flags, Time;
        public System.UIntPtr Extra;
    }
    [StructLayout(LayoutKind.Explicit)]
    private struct InputData {
        [FieldOffset(0)] public MouseInput Mouse;
        [FieldOffset(0)] public KeyboardInput Keyboard;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct Input { public uint Type; public InputData Data; }
    [DllImport("user32.dll", SetLastError = true)]
    private static extern uint SendInput(uint count, Input[] inputs, int size);
    public static void PressF12() {
        var inputs = new[] {
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x7b } } },
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x7b, Flags = 2 } } }
        };
        if (SendInput(2, inputs, Marshal.SizeOf(typeof(Input))) != 2)
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
    // Real keyboard input for the focused control: Ctrl+A, then each character as a Unicode key event.
    public static void SelectAll() {
        var inputs = new[] {
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x11 } } },
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x41 } } },
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x41, Flags = 2 } } },
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x11, Flags = 2 } } }
        };
        if (SendInput((uint)inputs.Length, inputs, Marshal.SizeOf(typeof(Input))) != inputs.Length)
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
    public static void TypeText(string text) {
        var inputs = new Input[text.Length * 2];
        for (int i = 0; i < text.Length; i++) {
            inputs[2 * i] = new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Scan = text[i], Flags = 4 } } };
            inputs[2 * i + 1] = new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Scan = text[i], Flags = 6 } } };
        }
        if (SendInput((uint)inputs.Length, inputs, Marshal.SizeOf(typeof(Input))) != inputs.Length)
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
}
'@
}
. "$PSScriptRoot/lib/windows_native_layout.ps1"

function Wait-For([scriptblock]$Condition, [string]$Description, [int]$Seconds = 45, [scriptblock]$Detail = $null) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    do {
        try { $value = & $Condition }
        catch [System.Windows.Automation.ElementNotAvailableException] { $value = $null }
        if ($value) { return $value }
        Start-Sleep -Milliseconds 150
    } while ([DateTime]::UtcNow -lt $deadline)
    $diagnostic = @()
    if ($script:Window) {
        try {
            $elements = $script:Window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition)
            $diagnostic = foreach ($element in $elements) {
                if ($element.Current.AutomationId -or $element.Current.ControlType -eq [System.Windows.Automation.ControlType]::Text) {
                    "$($element.Current.ControlType.ProgrammaticName): $($element.Current.AutomationId) · $($element.Current.Name)"
                }
            }
        } catch { $diagnostic += $_.Exception.Message }
        try { $diagnostic += Get-OwnedWindowDiagnostic } catch { $diagnostic += $_.Exception.Message }
        try {
            if ($ScreenshotPath) {
                $failureImage = Join-Path (Split-Path ([IO.Path]::GetFullPath($ScreenshotPath))) ([IO.Path]::GetFileNameWithoutExtension($ScreenshotPath) + '-failure.png')
                Save-WindowImage $failureImage
            }
        } catch { $diagnostic += $_.Exception.Message }
        if ($Detail) { try { $diagnostic += & $Detail } catch { $diagnostic += "detail failed: $($_.Exception.Message)" } }
    }
    # PowerShell wraps and truncates a long exception message in the CI log, so the
    # evidence goes out as plain lines before the throw.
    Write-Host "--- diagnostics: timed out waiting for $Description ---"
    foreach ($line in $diagnostic) { Write-Host $line }
    throw ("Timed out waiting for $Description.`n" + (Get-PSCallStack | Out-String))
}

function Find-Control([string]$Id) {
    if (-not $script:Window) { return $null }
    try {
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::AutomationIdProperty, $Id)
        return $script:Window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
    } catch [System.Windows.Automation.ElementNotAvailableException] { return $null }
}

function Invoke-Control([string]$Id) {
    $control = Wait-For { Find-Control $Id } "control $Id"
    if (-not $control.Current.IsEnabled) { throw "Control $Id is disabled." }
    $pattern = $control.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $pattern.Invoke()

}

function Select-Control([string]$Id) {
    $control = Wait-For { Find-Control $Id } "navigation item $Id"
    $control.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
}

function Set-SearchText([string]$Text) {
    $box = Wait-For { Find-Control 'SettingsSearchBox' } 'settings search'
    $edit = $box.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::Edit))
    if (-not $edit) { throw 'Settings search has no editable text field.' }
    $edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($Text)
}

function Save-WindowImage([string]$Path) {
    if (-not $Path) { return }
    # A native picker owned by the editor disables it; failure diagnostics
    # still need an image in that state.
    if ($script:Window.Current.IsEnabled) { $script:Window.SetFocus() }
    # The window's UIA provider can briefly lack WindowPattern while it starts (ARM CI); the sleep and
    # DwmFlush below still fence the frame.
    $windowPattern = $null
    if ($script:Window.TryGetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern, [ref]$windowPattern)) {
        $windowPattern.WaitForInputIdle(5000) | Out-Null
    }
    # Input idle precedes WinUI entrance/layout animations; DwmFlush alone
    # fences only the current frame, which can still be mid-transition.
    Start-Sleep -Milliseconds 750
    [AmiberrySmokeDesktop]::DwmFlush() | Out-Null
    # Capture the visible frame, not UIA bounds that include invisible resize margins.
    $frame = [AmiberrySmokeDesktop]::GetVisibleFrame([IntPtr]$script:Window.Current.NativeWindowHandle)
    if ($frame) {
        $x = $frame[0]; $y = $frame[1]; $width = $frame[2] - $frame[0]; $height = $frame[3] - $frame[1]
    } else {
        $rect = $script:Window.Current.BoundingRectangle
        $x = [int]$rect.X; $y = [int]$rect.Y; $width = [int]$rect.Width; $height = [int]$rect.Height
    }
    if ($width -le 0 -or $height -le 0) { throw 'Native window has no visible bounds.' }
    $bitmap = [System.Drawing.Bitmap]::new($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($x, $y, 0, 0, $bitmap.Size)
        $bitmap.Save([IO.Path]::GetFullPath($Path), [System.Drawing.Imaging.ImageFormat]::Png)
    } finally { $graphics.Dispose(); $bitmap.Dispose() }
}

function Find-EmulationWindow {
    $condition = [System.Windows.Automation.PropertyCondition]::new(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $core.Id)
    $windows = [System.Windows.Automation.AutomationElement]::RootElement.FindAll(
        [System.Windows.Automation.TreeScope]::Children, $condition)
    foreach ($candidate in $windows) {
        if ($candidate.Current.NativeWindowHandle -and -not $candidate.Current.IsOffscreen -and
            [AmiberrySmokeDesktop]::IsWindowVisible([IntPtr]$candidate.Current.NativeWindowHandle)) {
            return $candidate
        }
    }
}

# A modal the core itself owns (a classic #32770 message box in the emulator process).
function Find-CoreDialog {
    foreach ($handle in [AmiberrySmokeDesktop]::TopLevelWindows()) {
        if ([AmiberrySmokeDesktop]::ProcessId($handle) -ne $core.Id -or -not [AmiberrySmokeDesktop]::IsWindowVisible($handle)) { continue }
        if ([AmiberrySmokeDesktop]::ClassName($handle) -eq '#32770') { return [System.Windows.Automation.AutomationElement]::FromHandle($handle) }
    }
    return $null
}

function Get-EditorStatus {
    $texts = $script:Window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Text))
    foreach ($text in $texts) {
        $name = $text.Current.Name
        if ($name -match '^(Unsaved changes|All changes saved|Not saved to Library)') { return $name }
    }
}

function Wait-EditorStatus([string]$Prefix, [string]$Description) {
    Wait-For { $status = Get-EditorStatus; if ($status -and $status.StartsWith($Prefix)) { $status } } $Description
}

# Resume from the editor; a running machine with pending changes asks to apply them first.
function Resume-Emulation {
    Invoke-Control 'MachineStartButton'
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while ([DateTime]::UtcNow -lt $deadline -and [AmiberrySmokeDesktop]::IsWindowVisible($uiHandle)) {
        if (Find-Control 'PrimaryButton') { Invoke-Control 'PrimaryButton'; break }
        Start-Sleep -Milliseconds 150
    }
    Wait-MainWindowHidden
}

function Open-EmulatorSettings {
    $emulator = Wait-For { Find-EmulationWindow } 'visible SDL emulation window'
    $emulator.SetFocus()
    $handle = [IntPtr]$emulator.Current.NativeWindowHandle
    if ([AmiberrySmokeDesktop]::GetForegroundWindow() -ne $handle) {
        [AmiberrySmokeDesktop]::SetForegroundWindow($handle) | Out-Null
    }
    Wait-For { [AmiberrySmokeDesktop]::GetForegroundWindow() -eq $handle } 'owned SDL window receiving F12' | Out-Null
    [AmiberrySmokeDesktop]::PressF12()
    Wait-For { [AmiberrySmokeDesktop]::IsWindowVisible($uiHandle) } 'F12 restoring the native editor' | Out-Null
    $script:Window = [System.Windows.Automation.AutomationElement]::FromHandle($uiHandle)
    Wait-For { Find-Control 'MachineStartButton' } 'F12 editor session' | Out-Null
}

# The SDK's native FileSavePicker is a classic #32770 common dialog created in
# the WinUI process and owned by the main window. UIA nests owned windows under
# their owner, so it never appears among the desktop's direct children: find it
# with Win32 ownership and process id, never by its localized title.
function Find-OwnedDialog {
    if (-not $uiHandle -or -not $uiProcess) { return $null }
    foreach ($handle in [AmiberrySmokeDesktop]::TopLevelWindows()) {
        if ($handle -eq $uiHandle -or [AmiberrySmokeDesktop]::ProcessId($handle) -ne $uiProcess.Id) { continue }
        if (-not [AmiberrySmokeDesktop]::IsWindowVisible($handle)) { continue }
        if ([AmiberrySmokeDesktop]::GetWindow($handle, 4) -ne $uiHandle) { continue }
        if ([AmiberrySmokeDesktop]::ClassName($handle) -ne '#32770') { continue }
        return [System.Windows.Automation.AutomationElement]::FromHandle($handle)
    }
    return $null
}

# Only windows of this smoke's own processes or owned by its editor.
function Get-OwnedWindowDiagnostic {
    if (-not $uiHandle) { return @() }
    $ours = @($uiProcess.Id)
    if ($core) { $ours += $core.Id }
    $bounds = $script:Window.Current.BoundingRectangle
    $lines = @("main editor enabled: $($script:Window.Current.IsEnabled)",
        "main editor bounds: $($bounds.X),$($bounds.Y) $($bounds.Width)x$($bounds.Height); primary display $([AmiberrySmokeDesktop]::GetSystemMetrics(0))x$([AmiberrySmokeDesktop]::GetSystemMetrics(1))")
    $foreground = [AmiberrySmokeDesktop]::GetForegroundWindow()
    $lines += ('foreground: hwnd=0x{0:X} pid={1} class={2}' -f [int64]$foreground, [AmiberrySmokeDesktop]::ProcessId($foreground), [AmiberrySmokeDesktop]::ClassName($foreground))
    foreach ($handle in [AmiberrySmokeDesktop]::TopLevelWindows()) {
        $owner = [AmiberrySmokeDesktop]::GetWindow($handle, 4)
        $visible = [AmiberrySmokeDesktop]::IsWindowVisible($handle)
        $class = [AmiberrySmokeDesktop]::ClassName($handle)
        $processId = [AmiberrySmokeDesktop]::ProcessId($handle)
        if (($ours -notcontains $processId -and $owner -ne $uiHandle) -or -not ($visible -or $class -eq '#32770')) { continue }
        $lines += ('window: hwnd=0x{0:X} pid={1} owner=0x{2:X} visible={3} class={4} title={5}' -f [int64]$handle, $processId, [int64]$owner, $visible, $class, [AmiberrySmokeDesktop]::Title($handle))
    }
    return $lines
}

function Get-PickerControl($dialog, [string]$Id) {
    $dialog.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::AutomationIdProperty, $Id))
}

function Wait-NativePicker([string]$Description) {
    $dialog = Wait-For { Find-OwnedDialog } "native $Description picker"
    Wait-For { (Get-PickerControl $dialog '1001') -and (Get-PickerControl $dialog '1') -and (Get-PickerControl $dialog '2') } "native $Description picker controls (file name 1001, Save 1, Cancel 2)" | Out-Null
    # ARM CI saved "Amiga 500.uae" (the suggestion) instead of the typed path: wait until the dialog has filled in
    # its suggested file name, so that typed text is not raced by the dialog's own initialisation.
    Wait-For {
        $name = Get-PickerControl $dialog '1001'
        $name -and $name.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value.Length -gt 0
    } "native $Description picker suggested file name" | Out-Null
    return $dialog
}

function Wait-NativePickerClosed([string]$Description) {
    Wait-For { -not (Find-OwnedDialog) -and $script:Window.Current.IsEnabled } "native $Description picker closing and the editor re-enabling" | Out-Null
}

function Set-ControlText([string]$Id, [string]$Text) {
    $control = Wait-For { Find-Control $Id } "editable $Id"
    $control.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($Text)
}

function Invoke-NamedMenuItem([string]$Name) {
    $menuCondition = [System.Windows.Automation.AndCondition]::new(
        [System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem),
        [System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::NameProperty, $Name))
    $item = Wait-For { $script:Window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $menuCondition) } "menu item $Name"
    $item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
}

function Wait-MainWindowHidden {
    Wait-For { -not [AmiberrySmokeDesktop]::IsWindowVisible($uiHandle) } 'main UI hidden while emulation owns the screen' | Out-Null
}

function Save-TestMachine([string]$Path) {
    Invoke-Control 'MachineSaveButton'
    $dialog = Wait-NativePicker 'Save'
    $edit = Get-PickerControl $dialog '1001'
    $value = $edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
    # ARM CI showed the file name UIA reports after SetValue is not what the dialog saves (it returned its own
    # suggestion), so type the name as a user would: only into this dialog's focused file name field.
    $dialogHandle = [IntPtr]$dialog.Current.NativeWindowHandle
    Wait-For {
        if ([AmiberrySmokeDesktop]::GetForegroundWindow() -ne $dialogHandle) { [AmiberrySmokeDesktop]::SetForegroundWindow($dialogHandle) | Out-Null }
        $edit.SetFocus()
        [AmiberrySmokeDesktop]::GetForegroundWindow() -eq $dialogHandle -and
            [System.Windows.Automation.AutomationElement]::FocusedElement.Current.AutomationId -eq '1001'
    } 'native Save picker file name field focused' | Out-Null
    [AmiberrySmokeDesktop]::SelectAll()
    [AmiberrySmokeDesktop]::TypeText($Path)
    Wait-For { Start-Sleep -Milliseconds 400; $value.Current.Value -eq $Path } 'picker file name holding the typed path' | Out-Null
    $typed = $value.Current.Value
    (Get-PickerControl $dialog '1').GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    # Say what the picker saw and where any saved file landed: the UI reports a
    # successful save, so a timeout means the file is elsewhere or its content differs.
    $saveDetail = {
        $lines = @("expected: $Path", "picker file name after SetValue: $typed", "expected exists: $(Test-Path -LiteralPath $Path)")
        if (Test-Path -LiteralPath $Path) {
            $text = [IO.File]::ReadAllText($Path)
            $lines += "expected length: $($text.Length); cpu lines: " + (($text -split "`n" | Where-Object { $_ -match 'cpu' } | Select-Object -First 6) -join ' | ')
        }
        foreach ($file in @(Get-ChildItem -LiteralPath $scratch -Recurse -Depth 4 -Filter '*.uae' -File -ErrorAction SilentlyContinue | Select-Object -First 20)) {
            $lines += "found .uae: $($file.FullName) ($($file.Length) bytes, $($file.LastWriteTime.ToString('HH:mm:ss')))"
        }
        foreach ($log in @(Get-ChildItem -LiteralPath $scratch -Filter '*.log' -File -ErrorAction SilentlyContinue)) {
            $lines += "log $($log.Name):"
            $lines += @(Get-Content -LiteralPath $log.FullName -Tail 15 -ErrorAction SilentlyContinue)
        }
        $lines
    }
    Wait-For { (Test-Path -LiteralPath $Path) -and [IO.File]::ReadAllText($Path).Contains('cpu_model=68000') } 'machine actually saved through the native picker' -Detail $saveDetail | Out-Null
    Wait-NativePickerClosed 'Save'
    Wait-For { Find-Control 'MachineStartButton' } 'saved editor after native Save' | Out-Null
    # The header shows the saved machine's identity in place, not the stale unsaved one.
    $savedName = [IO.Path]::GetFileNameWithoutExtension($Path)
    Wait-For { (Find-Control 'PageSubtitle').Current.Name -eq $savedName -and (Find-Control 'MachineIdentityName').Current.Name -eq $savedName } 'saved machine identity in the editor header after Save' | Out-Null
}

$scratch = New-NativeTestLayout -AmiberryPath $AmiberryPath -IncludeUI
$exe = Join-Path $scratch 'Amiberry.exe'
$core = $null
$ui = $null
$uiProcess = $null
try {
    $configurationDirectory = Join-Path $scratch 'Configurations'
    New-Item -ItemType Directory -Path $configurationDirectory -Force | Out-Null
    $previousConfigDirectory = [Environment]::GetEnvironmentVariable('AMIBERRY_CONFIG_DIR', 'Process')
    try {
        [Environment]::SetEnvironmentVariable('AMIBERRY_CONFIG_DIR', $configurationDirectory, 'Process')
        $core = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -PassThru
    } finally {
        [Environment]::SetEnvironmentVariable('AMIBERRY_CONFIG_DIR', $previousConfigDirectory, 'Process')
    }
    $ui = Wait-For {
        if ($core.HasExited) { throw "Amiberry exited before opening the native UI (code $($core.ExitCode))." }
        Get-CimInstance Win32_Process -Filter "ParentProcessId=$($core.Id)" |
            Where-Object { $_.Name -eq 'Amiberry.Windows.exe' } | Select-Object -First 1
    } 'native WinUI child process'
    $uiProcess = [Diagnostics.Process]::GetProcessById([int]$ui.ProcessId)
    $script:Window = Wait-For {
        if ($uiProcess.HasExited) { throw "Native WinUI exited before opening the Library (code $($uiProcess.ExitCode)); inspect the Windows Application error log." }
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty, [int]$ui.ProcessId)
        [System.Windows.Automation.AutomationElement]::RootElement.FindFirst(
            [System.Windows.Automation.TreeScope]::Children, $condition)
    } 'native Library window'
    $uiHandle = [IntPtr]$script:Window.Current.NativeWindowHandle
    Wait-For { Find-Control 'NewMachineButton' } 'Library actions' | Out-Null
    # The default window must fit the work area of the monitor it opens on (a 1024x768 monitor once pushed the footer
    # off screen). UIA bounds include the invisible resize borders, hence the tolerance.
    $initialBounds = $script:Window.Current.BoundingRectangle
    $work = [AmiberrySmokeDesktop]::WorkArea($uiHandle)
    if (-not $work) { throw 'Could not read the work area of the monitor holding the editor window.' }
    if ($initialBounds.X -lt $work[0] - 16 -or $initialBounds.Y -lt $work[1] - 16 -or $initialBounds.Right -gt $work[2] + 16 -or $initialBounds.Bottom -gt $work[3] + 16) {
        throw "The initial editor window $($initialBounds.X),$($initialBounds.Y) $($initialBounds.Width)x$($initialBounds.Height) does not fit its monitor's work area $($work -join ',')."
    }
    if ($ScreenshotPath) {
        $imageDirectory = Split-Path ([IO.Path]::GetFullPath($ScreenshotPath))
        $imageStem = [IO.Path]::GetFileNameWithoutExtension($ScreenshotPath)
        Save-WindowImage (Join-Path $imageDirectory "$imageStem-library.png")
    }
    if ($ScreenshotPath) {
        foreach ($section in @(
            @('recent', 'Recent'), @('floppies', 'Floppy Disks'), @('cds', 'CDs'),
            @('whdload', 'WHDLoad'), @('roms', 'Kickstart ROMs'), @('app', 'App Settings'))) {
            Select-Control ('Page_' + $section[0])
            Wait-For { (Find-Control 'PageHeading').Current.Name -eq $section[1] } ($section[1] + ' page') | Out-Null
            Save-WindowImage (Join-Path $imageDirectory "$imageStem-$($section[0]).png")
        }
        Select-Control 'Page_machines'
        Wait-For { Find-Control 'NewMachineButton' } 'Library after app settings' | Out-Null
    }
    # App Settings has the same rule: choosing a Library folder rebuilds the page, so a rejected draft must stop it
    # before the folder picker opens or anything changes. The over-long path is refused by the backend.
    Select-Control 'Page_app'
    Wait-For { (Find-Control 'PageHeading').Current.Name -eq 'App Settings' } 'App Settings page' | Out-Null
    $logfileRow = Wait-For { Find-Control 'Setting_logfile_path' } 'logfile path setting'
    $logfileRow.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue('A:\' + ('x' * 5000) + '.log')
    Invoke-Control 'FolderChoose_4'
    Wait-For { Find-Control 'SettingDiscard_logfile_path' } 'rejected logfile path reported on its row by the folder action' | Out-Null
    Start-Sleep -Seconds 1
    if (Find-OwnedDialog) { throw 'A folder dialog opened while a rejected app setting draft was pending.' }
    Invoke-Control 'SettingDiscard_logfile_path'
    Wait-For { -not (Find-Control 'SettingDiscard_logfile_path') } 'discarded logfile path draft' | Out-Null
    Select-Control 'Page_machines'
    Invoke-Control 'NewMachineButton'
    Invoke-Control 'Preset_a500'
    Invoke-Control 'CloseButton'
    Invoke-Control 'NewMachineBackButton'
    Wait-For { Find-Control 'NewMachineButton' } 'model chooser cancellation returns to Library' | Out-Null
    Invoke-Control 'NewMachineButton'
    Invoke-Control 'Preset_a500'
    Invoke-Control 'PrimaryButton'
    Wait-For { Find-Control 'MachineStartButton' } 'machine editor' | Out-Null
    if ($ScreenshotPath) {
        foreach ($section in @(
            @('overview', 'Overview'), @('display', 'Display & Sound'), @('input', 'Input'),
            @('storage', 'Storage'), @('ports', 'Ports & Misc'), @('all', 'All Settings'))) {
            Select-Control ('Page_' + $section[0])
            Wait-For { (Find-Control 'PageHeading').Current.Name -eq $section[1] } ($section[1] + ' editor page') | Out-Null
            Save-WindowImage (Join-Path $imageDirectory "$imageStem-$($section[0]).png")
        }
    }
    Select-Control 'Page_hardware'
    Wait-For {
        $cpu = Find-Control 'Setting_cpu_model'
        if ($cpu -and $cpu.Current.IsEnabled -and -not $cpu.Current.IsOffscreen -and $cpu.Current.BoundingRectangle.Width -gt 0) { return $cpu }
    } 'visible editable CPU model' | Out-Null
    # Navigation items can be missing from the UIA tree while the page rebuilds: not selected yet.
    Wait-For {
        $hardware = Find-Control 'Page_hardware'
        $overview = Find-Control 'Page_overview'
        $hardware -and $overview -and
            $hardware.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Current.IsSelected -and
            -not $overview.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Current.IsSelected
    } 'single matching navigation selection' | Out-Null
    $ntsc = Wait-For { Find-Control 'Setting_ntsc' } 'NTSC setting'
    $original = $ntsc.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState
    $ntsc.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Toggle()
    # Navigate after the edit completes, then read the rebuilt row. This
    # exercises the real settings session and reply, not just the toggle's
    # immediate local visual state.
    Select-Control 'Page_overview'
    Select-Control 'Page_hardware'
    Wait-For {
        $row = Find-Control 'Setting_ntsc'
        if ($row -and $row.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState -ne $original) { return $row }
    } 'settings roundtrip' | Out-Null
    (Find-Control 'Setting_ntsc').GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Toggle()
    Select-Control 'Page_overview'
    Select-Control 'Page_hardware'
    Wait-For {
        $row = Find-Control 'Setting_ntsc'
        if ($row -and $row.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState -eq $original) { return $row }
    } 'restored NTSC setting' | Out-Null

    Set-SearchText 'cpu_model'
    Wait-For {
        (Find-Control 'PageHeading').Current.Name -eq 'Search results' -and
            (Find-Control 'Setting_cpu_model') -and -not (Find-Control 'Setting_ntsc')
    } 'live settings search' | Out-Null
    Set-SearchText 'no-such-amiberry-option-ux-smoke'
    Wait-For { -not (Find-Control 'Setting_cpu_model') -and (Find-Control 'PageHeading').Current.Name -eq 'Search results' } 'empty search results' | Out-Null
    Set-SearchText ''
    Wait-For { (Find-Control 'PageHeading').Current.Name -eq 'Hardware' -and (Find-Control 'Setting_cpu_model') } 'cleared search restores Hardware' | Out-Null
    Save-WindowImage $ScreenshotPath

    $transform = $script:Window.GetCurrentPattern([System.Windows.Automation.TransformPattern]::Pattern)
    $desktopBounds = $script:Window.Current.BoundingRectangle
    $transform.Resize(700, 880)
    Wait-For {
        $bounds = $script:Window.Current.BoundingRectangle
        $cpu = (Find-Control 'Setting_cpu_model').Current.BoundingRectangle
        $start = (Find-Control 'MachineStartButton').Current.BoundingRectangle
        $bounds.Width -le 710 -and $cpu.Width -gt 0 -and $cpu.Right -le $bounds.Right -and
            $start.Width -gt 0 -and $start.Right -le $bounds.Right
    } 'narrow window keeps settings and primary action usable' | Out-Null
    if ($ScreenshotPath) { Save-WindowImage (Join-Path $imageDirectory "$imageStem-narrow.png") }
    # Return to the size the window had before narrowing. A fixed target is unreachable on a
    # hosted runner whose display is smaller than the app's default window.
    $transform.Resize($desktopBounds.Width, $desktopBounds.Height)
    Wait-For {
        $cpu = Find-Control 'Setting_cpu_model'
        $script:Window.Current.BoundingRectangle.Width -ge $desktopBounds.Width - 10 -and $cpu -and -not $cpu.Current.IsOffscreen
    } 'restored desktop layout' | Out-Null

    # Exercise a real storage dialog error: the input must survive backend
    # rejection, and cancelling must leave the machine's mounts unchanged.
    Select-Control 'Page_storage'
    # A rejected draft must stop a mount dialog: its completion rebuilds the page, which would hide the row that
    # reports the error while Save/Start kept retrying the invisible draft. A path longer than the core's limit is
    # refused by the backend; set it through UIA, so no key is sent to whatever window has the foreground.
    $imageRow = Wait-For { Find-Control 'Setting_floppy0' } 'DF0 image setting'
    $imageRow.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue('A:\' + ('x' * 5000) + '.adf')
    Invoke-Control 'StorageAddDrive'
    Invoke-Control 'StorageAddCd'
    Wait-For { Find-Control 'SettingDiscard_floppy0' } 'rejected DF0 image reported on its row by the mount action' | Out-Null
    Start-Sleep -Seconds 1
    if (Find-Control 'Mount_path') { throw 'A mount dialog opened while a rejected setting draft was pending.' }
    Invoke-Control 'SettingDiscard_floppy0'
    Wait-For { -not (Find-Control 'SettingDiscard_floppy0') } 'discarded DF0 image draft' | Out-Null
    Invoke-Control 'StorageAddDrive'
    Invoke-Control 'StorageAddCd'
    Set-ControlText 'Mount_path' 'a"b'
    Invoke-Control 'PrimaryButton'
    Wait-For {
        $error = Find-Control 'DialogError'
        $path = Find-Control 'Mount_path'
        $error -and -not $error.Current.IsOffscreen -and $path.Current.IsEnabled -and
            $path.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value -eq 'a"b'
    } 'mount validation retains editable input' | Out-Null
    Invoke-Control 'CloseButton'
    Select-Control 'Page_overview'
    $savedMachine = Join-Path $configurationDirectory 'UX smoke.uae'
    Save-TestMachine $savedMachine
    Invoke-Control 'MachineStartButton'
    Wait-MainWindowHidden
    Open-EmulatorSettings
    Wait-For { (Find-Control 'MachineStartButton').Current.IsEnabled } 'resume action after physical F12' | Out-Null
    Invoke-Control 'RuntimeDrivesButton'
    Wait-For {
        $slots = Find-Control 'StateSlotPicker'
        $slots -and -not $slots.Current.IsOffscreen
    } 'save-state controls visible without scrolling past disabled drives' | Out-Null
    if ($ScreenshotPath) { Save-WindowImage (Join-Path $imageDirectory "$imageStem-runtime.png") }
    Select-Control 'Page_overview'
    Invoke-Control 'MachineStartButton'
    Wait-MainWindowHidden
    Open-EmulatorSettings
    Invoke-Control 'RuntimeStopButton'
    Invoke-Control 'PrimaryButton'
    Wait-For { [AmiberrySmokeDesktop]::IsWindowVisible($uiHandle) -and (Find-Control 'NewMachineButton') } 'visible Library after stop' | Out-Null

    # A clean saved machine must still offer Save As. Invalid rename must keep
    # the user's input and leave the original file untouched.
    Invoke-Control 'MachineEdit_UX smoke'
    Wait-For { (Find-Control 'MachineSaveButton').Current.IsEnabled } 'Save As available for a clean saved machine' | Out-Null
    $beforeSaveAs = [IO.File]::ReadAllText($savedMachine)
    $filesBeforeSaveAs = @(Get-ChildItem -LiteralPath $configurationDirectory -Filter '*.uae' | ForEach-Object Name | Sort-Object)
    (Find-Control 'MachineSaveButton').GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    Invoke-NamedMenuItem ('Save As' + [char]0x2026)
    $dialog = Wait-NativePicker 'clean Save As'
    $suggested = (Get-PickerControl $dialog '1001').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value
    if (-not $suggested.StartsWith('UX smoke')) { throw "Clean Save As suggested '$suggested' instead of the saved machine name." }
    (Get-PickerControl $dialog '2').GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Wait-NativePickerClosed 'clean Save As cancel'
    Wait-For { (Find-Control 'MachineStartButton') -and (Find-Control 'MachineSaveButton').Current.IsEnabled } 'saved editor after cancelling Save As' | Out-Null
    if ([IO.File]::ReadAllText($savedMachine) -ne $beforeSaveAs) { throw 'Cancelled Save As changed the saved machine.' }
    $filesAfterSaveAs = @(Get-ChildItem -LiteralPath $configurationDirectory -Filter '*.uae' | ForEach-Object Name | Sort-Object)
    if (($filesAfterSaveAs -join '|') -ne ($filesBeforeSaveAs -join '|')) { throw 'Cancelled Save As created or removed a machine file.' }
    Invoke-Control 'PART_BackButton'
    Wait-For { Find-Control 'MachineMore_UX smoke' } 'saved Library after editor Back' | Out-Null
    $beforeRename = [IO.File]::ReadAllText($savedMachine)
    Invoke-Control 'MachineMore_UX smoke'
    Invoke-NamedMenuItem ('Rename' + [char]0x2026)
    Set-ControlText 'RenameMachineName' 'bad/name'
    Invoke-Control 'PrimaryButton'
    Wait-For {
        $error = Find-Control 'DialogError'
        $name = Find-Control 'RenameMachineName'
        $error -and -not $error.Current.IsOffscreen -and $name.Current.IsEnabled -and
            $name.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value -eq 'bad/name'
    } 'rename validation retains editable input' | Out-Null
    Invoke-Control 'CloseButton'
    if ([IO.File]::ReadAllText($savedMachine) -ne $beforeRename) { throw 'Rejected rename changed the saved machine.' }
    Invoke-Control 'MachinePlay_UX smoke'
    Wait-MainWindowHidden
    Open-EmulatorSettings
    Wait-EditorStatus 'All changes saved' 'clean saved machine opened with F12' | Out-Null

    # Choosing Library from F12 closes the editor but the core stays paused; the runtime bar must still offer a
    # way back to this machine's editor and its Resume action.
    Invoke-Control 'PART_BackButton'
    Wait-For { (Find-Control 'MachineMore_UX smoke') -and -not (Find-Control 'MachineStartButton') } 'Library chosen from F12' | Out-Null
    Invoke-Control 'RuntimeMachineButton'
    Wait-For { (Find-Control 'MachineStartButton').Current.IsEnabled } 'Current Machine reopens the paused machine editor' | Out-Null
    Wait-EditorStatus 'All changes saved' 'reopened current machine is clean' | Out-Null

    # Edit the running saved machine and Resume without saving. Reopening with F12
    # must still report the edit as unsaved on disk, keep the save prompt, and
    # persist it once saved.
    Select-Control 'Page_hardware'
    (Wait-For { Find-Control 'Setting_ntsc' } 'NTSC setting on the running machine').GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Toggle()
    Wait-EditorStatus 'Unsaved changes' 'edit reported unsaved' | Out-Null
    $fileBeforeResume = [IO.File]::ReadAllText($savedMachine)
    Resume-Emulation
    Open-EmulatorSettings
    Wait-EditorStatus 'Unsaved changes' 'F12 reopen still reports edits that never reached the configuration file' | Out-Null
    if ([IO.File]::ReadAllText($savedMachine) -ne $fileBeforeResume) { throw 'Resuming without saving changed the saved machine file.' }
    Invoke-Control 'PART_BackButton'
    Wait-For { Find-Control 'SecondaryButton' } 'Save machine changes prompt on leaving the reopened editor' | Out-Null
    Invoke-Control 'CloseButton'
    Wait-EditorStatus 'Unsaved changes' 'cancelled leave keeps the unsaved state' | Out-Null
    Invoke-Control 'MachineSaveButton'
    Wait-For { [IO.File]::ReadAllText($savedMachine) -ne $fileBeforeResume } 'edit persisted to the saved machine file' | Out-Null
    Wait-EditorStatus 'All changes saved' 'saving clears the unsaved state' | Out-Null
    Resume-Emulation
    Open-EmulatorSettings
    Wait-EditorStatus 'All changes saved' 'F12 after saving reports a clean machine' | Out-Null
    Resume-Emulation

    # A core-owned modal blocks the thread that services the UI pipe. Closing the
    # emulation window raises the core's real Stop confirmation; leave it open longer
    # than the UI's former 60 s request limit, dismiss it, and require the same
    # connection to keep working.
    $emulationWindow = Wait-For { Find-EmulationWindow } 'visible SDL emulation window for the modal scenario'
    $emulationWindow.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).Close()
    Wait-For { Find-CoreDialog } 'core Stop confirmation' | Out-Null
    $modalDeadline = [DateTime]::UtcNow.AddSeconds(70)
    while ([DateTime]::UtcNow -lt $modalDeadline) {
        if ($core.HasExited -or $uiProcess.HasExited) { throw 'The emulator or the native UI ended while the core dialog was open.' }
        Start-Sleep -Milliseconds 500
    }
    $dialog = Find-CoreDialog
    if (-not $dialog) { throw 'The core Stop confirmation closed by itself.' }
    (Get-PickerControl $dialog '7').GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Wait-For { -not (Find-CoreDialog) } 'core Stop confirmation dismissed' | Out-Null
    Start-Sleep -Seconds 3
    if ($core.HasExited -or $uiProcess.HasExited) { throw 'Dismissing a core dialog left open past 60 s ended the emulator or the native UI.' }
    Open-EmulatorSettings
    Invoke-Control 'RuntimeStopButton'
    Invoke-Control 'PrimaryButton'
    Wait-For { [AmiberrySmokeDesktop]::IsWindowVisible($uiHandle) -and (Find-Control 'NewMachineButton') } 'Library after saved-machine Play and stop' | Out-Null
    Write-Output 'PASS: native pages -> settings/search/narrow layout -> rejected draft blocks mount dialog -> retained dialog errors -> native Save -> hidden emulation -> physical F12 -> resume hides -> stop restores Library -> clean Save As -> safe rename -> saved-machine Play/F12 -> Library from F12 and Current Machine back to the editor -> resume/stop -> unsaved edit survives Resume/F12 and Save -> core modal held past 60 s keeps the connection'
} catch {
    # An error that is not a Wait-For timeout (a null control, a failed pattern call) otherwise reaches the CI log
    # without its position. Report it with the failing statement, the stack and the state of the screen, then rethrow.
    Write-Host '--- unexpected smoke error ---'
    Write-Host ($_.Exception.GetType().FullName + ': ' + $_.Exception.Message)
    Write-Host $_.InvocationInfo.PositionMessage
    Write-Host $_.ScriptStackTrace
    try { if ($script:Window) { Get-OwnedWindowDiagnostic | ForEach-Object { Write-Host $_ } } } catch { Write-Host "window diagnostics failed: $($_.Exception.Message)" }
    try {
        if ($ScreenshotPath -and $script:Window) {
            Save-WindowImage (Join-Path (Split-Path ([IO.Path]::GetFullPath($ScreenshotPath))) ([IO.Path]::GetFileNameWithoutExtension($ScreenshotPath) + '-failure.png'))
        }
    } catch { Write-Host "failure image failed: $($_.Exception.Message)" }
    throw
} finally {
    # These PIDs belong to this smoke only; leave other Amiberry instances alone.
    if ($uiProcess) {
        if (-not $uiProcess.HasExited) { $uiProcess.Kill(); $uiProcess.WaitForExit() }
        $uiProcess.Dispose()
    }
    if ($core -and -not $core.HasExited) {
        if (-not $core.WaitForExit(5000)) { $core.Kill(); $core.WaitForExit() }
    }
    Remove-Item -LiteralPath $scratch -Recurse -Force
}
