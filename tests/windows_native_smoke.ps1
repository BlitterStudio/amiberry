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
}
'@
}
. "$PSScriptRoot/lib/windows_native_layout.ps1"

function Wait-For([scriptblock]$Condition, [string]$Description, [int]$Seconds = 45) {
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
    }
    throw ("Timed out waiting for $Description.`n" + (Get-PSCallStack | Out-String) + ($diagnostic -join "`n"))
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
    $script:Window.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).WaitForInputIdle(5000) | Out-Null
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

function Open-EmulatorSettings {
    $emulator = Wait-For {
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
    } 'visible SDL emulation window'
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
    $lines = @("main editor enabled: $($script:Window.Current.IsEnabled)")
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
    (Get-PickerControl $dialog '1001').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($Path)
    (Get-PickerControl $dialog '1').GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Wait-For { (Test-Path -LiteralPath $Path) -and [IO.File]::ReadAllText($Path).Contains('cpu_model=68000') } 'machine actually saved through the native picker' | Out-Null
    Wait-NativePickerClosed 'Save'
    Wait-For { Find-Control 'MachineStartButton' } 'saved editor after native Save' | Out-Null
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
    Wait-For {
        $hardware = Find-Control 'Page_hardware'
        $overview = Find-Control 'Page_overview'
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
    $transform.Resize(700, 880)
    Wait-For {
        $bounds = $script:Window.Current.BoundingRectangle
        $cpu = (Find-Control 'Setting_cpu_model').Current.BoundingRectangle
        $start = (Find-Control 'MachineStartButton').Current.BoundingRectangle
        $bounds.Width -le 710 -and $cpu.Width -gt 0 -and $cpu.Right -le $bounds.Right -and
            $start.Width -gt 0 -and $start.Right -le $bounds.Right
    } 'narrow window keeps settings and primary action usable' | Out-Null
    if ($ScreenshotPath) { Save-WindowImage (Join-Path $imageDirectory "$imageStem-narrow.png") }
    $transform.Resize(1220, 880)
    Wait-For { $script:Window.Current.BoundingRectangle.Width -ge 1210 -and -not (Find-Control 'Setting_cpu_model').Current.IsOffscreen } 'restored desktop layout' | Out-Null

    # Exercise a real storage dialog error: the input must survive backend
    # rejection, and cancelling must leave the machine's mounts unchanged.
    Select-Control 'Page_storage'
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
    Invoke-Control 'MachineStartButton'
    Wait-MainWindowHidden
    Open-EmulatorSettings
    Invoke-Control 'RuntimeStopButton'
    Invoke-Control 'PrimaryButton'
    Wait-For { [AmiberrySmokeDesktop]::IsWindowVisible($uiHandle) -and (Find-Control 'NewMachineButton') } 'Library after saved-machine Play and stop' | Out-Null
    Write-Output 'PASS: native pages -> settings/search/narrow layout -> retained dialog errors -> native Save -> hidden emulation -> physical F12 -> resume hides -> stop restores Library -> clean Save As -> safe rename -> saved-machine Play/F12/resume/stop'
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
