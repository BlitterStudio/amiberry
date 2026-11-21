# Leaves the interactive desktop of a GitHub-hosted Windows runner clear for tests that drive real UI and input.
#
# The windows-11-arm image can start a job while the first sign-in is still showing its full-screen
# "Choose privacy settings for your device" experience and the open Start menu. They hold the foreground over the whole
# desktop, so synthetic input and window activation never reach the application under test
# (actions/runner-images#14069; the image fix, #14673, only stops it for newly created accounts).
#
# This runs only on an ephemeral GitHub-hosted runner and refuses to run anywhere else: it never touches a developer's
# desktop. It does not answer the privacy experience. It sets Microsoft's documented policy that stops the experience
# from launching (Policy CSP Privacy/DisablePrivacyExperience, Windows Components > OOBE); ends the one process that
# owns the window showing the experience, verified by the heading the window displays or by its image name (never a
# shell host); closes an open Start menu with Escape only while the Start menu host owns the foreground; and then
# requires the desktop to stay clear. It reports the foreground window explicitly (handle, process, image, session),
# because a UWP window can be the foreground without being found by name. If an immersive surface it cannot identify
# stays in the foreground, or the experience's host is one it may not end, the step fails with that identity instead of
# reporting success.
#
# Only immersive or first-run shell surfaces count as obstructions. Observed on windows-11-arm: a full-screen
# Windows.UI.Core.CoreWindow owned by WWAHost.exe, fronted by the explorer's Shell_OOBEProxy window, then the Search or
# Start host once it is gone. An ordinary console or terminal (the hosted runner's own) never blocks activation and is
# left alone whatever its size.
#
# -CheckOnly only reports what obstructs the desktop and changes nothing (it also runs outside CI).
param(
    [switch]$CheckOnly,
    [int]$TimeoutSeconds = 120,
    [int]$StableSeconds = 10
)

$ErrorActionPreference = 'Stop'

if (-not $CheckOnly -and ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_ENVIRONMENT -ne 'github-hosted')) {
    throw 'Refusing to change the desktop: this only runs on an ephemeral GitHub-hosted runner (GITHUB_ACTIONS=true, RUNNER_ENVIRONMENT=github-hosted).'
}

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class AmiberryCiDesktop {
    private delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] private static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetClassName(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] private static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")] private static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll")] private static extern uint SendInput(uint count, Input[] inputs, int size);
    [StructLayout(LayoutKind.Sequential)] private struct Rect { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] private struct KeyboardInput { public ushort Key, Scan; public uint Flags, Time; public UIntPtr Extra; }
    [StructLayout(LayoutKind.Sequential)] private struct MouseInput { public int X, Y; public uint Data, Flags, Time; public UIntPtr Extra; }
    [StructLayout(LayoutKind.Explicit)] private struct InputData { [FieldOffset(0)] public MouseInput Mouse; [FieldOffset(0)] public KeyboardInput Keyboard; }
    [StructLayout(LayoutKind.Sequential)] private struct Input { public uint Type; public InputData Data; }

    public sealed class Window {
        public long Handle; public uint ProcessId; public string Class, Title; public int Left, Top, Right, Bottom;
        public bool Visible, Foreground; public double ScreenCoverage;
    }
    private static Window Describe(IntPtr handle, IntPtr foreground, double screen) {
        Rect rect; GetWindowRect(handle, out rect);
        uint pid; GetWindowThreadProcessId(handle, out pid);
        var cls = new StringBuilder(256); GetClassName(handle, cls, cls.Capacity);
        var title = new StringBuilder(256); GetWindowText(handle, title, title.Capacity);
        double area = (double)Math.Max(0, rect.Right - rect.Left) * Math.Max(0, rect.Bottom - rect.Top);
        return new Window { Handle = handle.ToInt64(), ProcessId = pid, Class = cls.ToString(), Title = title.ToString(),
            Left = rect.Left, Top = rect.Top, Right = rect.Right, Bottom = rect.Bottom, Visible = IsWindowVisible(handle),
            Foreground = handle == foreground, ScreenCoverage = screen > 0 ? area / screen : 0 };
    }
    // The visible top-level windows, plus the foreground window whether or not EnumWindows lists it.
    public static List<Window> Candidates() {
        var result = new List<Window>();
        IntPtr foreground = GetForegroundWindow();
        double screen = (double)GetSystemMetrics(0) * GetSystemMetrics(1);
        bool sawForeground = false;
        EnumWindows(delegate(IntPtr handle, IntPtr parameter) {
            if (handle == foreground) sawForeground = true;
            if (IsWindowVisible(handle) || handle == foreground) result.Add(Describe(handle, foreground, screen));
            return true;
        }, IntPtr.Zero);
        if (!sawForeground && foreground != IntPtr.Zero) result.Add(Describe(foreground, foreground, screen));
        return result;
    }
    public static bool EnumeratesForeground() {
        IntPtr foreground = GetForegroundWindow();
        bool seen = false;
        EnumWindows(delegate(IntPtr handle, IntPtr parameter) { if (handle == foreground) seen = true; return true; }, IntPtr.Zero);
        return seen;
    }
    public static void PressEscape() {
        var inputs = new[] {
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x1b } } },
            new Input { Type = 1, Data = new InputData { Keyboard = new KeyboardInput { Key = 0x1b, Flags = 2 } } }
        };
        if (SendInput(2, inputs, Marshal.SizeOf(typeof(Input))) != 2) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
}
'@

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$privacyHost = 'WWAHost'
$startHosts = @('StartMenuExperienceHost', 'SearchHost')
# Never end these, even if one owns the window that obstructs the desktop: they are the shell itself.
$protectedHosts = @('explorer', 'ApplicationFrameHost', 'dwm', 'winlogon', 'csrss', 'svchost', 'sihost', 'ShellExperienceHost', 'StartMenuExperienceHost', 'SearchHost', 'TextInputHost', 'LogonUI', 'wininit', 'services', 'lsass')
$shellClasses = @('Shell_TrayWnd', 'Shell_SecondaryTrayWnd', 'Progman', 'WorkerW')
$immersiveClasses = @('Windows.UI.Core.CoreWindow', 'ApplicationFrameWindow', 'Shell_OOBEProxy')

# Identity of a process by id, from WMI so that it also works for a process Get-Process cannot describe.
function Get-ProcessInfo([uint32]$ProcessId) {
    $wmi = Get-CimInstance Win32_Process -Filter "ProcessId=$ProcessId" -ErrorAction SilentlyContinue
    if ($wmi) {
        return [pscustomobject]@{ Id = [int]$ProcessId; ProcessName = [IO.Path]::GetFileNameWithoutExtension($wmi.Name); Path = $wmi.ExecutablePath; SessionId = $wmi.SessionId }
    }
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if ($process) { return [pscustomobject]@{ Id = $process.Id; ProcessName = $process.ProcessName; Path = $null; SessionId = $process.SessionId } }
    return $null
}

# The first-run privacy experience, identified by the heading it shows (UI Automation, English image).
function Test-PrivacyExperienceWindow($window) {
    try {
        $element = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$window.Handle)
        if ($element.Current.Name -match 'privacy settings') { return $true }
        $heading = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, 'Choose privacy settings for your device')
        return $null -ne $element.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $heading)
    } catch { return $false }
}

function Format-Window($window, $info) {
    $who = if ($info) { 'process={0} image={1} session={2}' -f $info.ProcessName, $info.Path, $info.SessionId } else { 'process=unknown' }
    '  hwnd=0x{0:X} pid={1} {2} class={3} title="{4}" rect={5},{6},{7},{8} coverage={9:P0} visible={10} foreground={11}' -f
        $window.Handle, $window.ProcessId, $who, $window.Class, $window.Title, $window.Left, $window.Top, $window.Right, $window.Bottom, $window.ScreenCoverage, $window.Visible, $window.Foreground
}

function Get-Obstruction {
    $found = @()
    foreach ($window in [AmiberryCiDesktop]::Candidates()) {
        if ($window.Class -in $shellClasses) { continue }
        $info = Get-ProcessInfo $window.ProcessId
        $name = if ($info) { $info.ProcessName } else { $null }
        # Only an immersive or first-run shell surface can hold the foreground against a desktop app. Ordinary
        # desktop windows (a console, a terminal, another app) never block activation, whatever their size.
        $immersive = $window.Class -in $immersiveClasses
        if ($info -and ($name -eq $privacyHost -or ($immersive -and $window.ScreenCoverage -ge 0.5 -and (Test-PrivacyExperienceWindow $window)))) {
            $found += [pscustomobject]@{ Kind = 'privacy'; Info = $info; Window = $window }
        } elseif ($window.Foreground -and $startHosts -contains $name) {
            $found += [pscustomobject]@{ Kind = 'start'; Info = $info; Window = $window }
        } elseif ($window.Class -eq 'Shell_OOBEProxy' -and $window.ScreenCoverage -ge 0.5) {
            # The shell's own full-screen first-run surface (it goes away with the experience it fronts).
            $found += [pscustomobject]@{ Kind = 'oobe'; Info = $info; Window = $window }
        } elseif ($window.Foreground -and $immersive -and $window.ScreenCoverage -ge 0.5) {
            # An immersive surface this step cannot identify holds the foreground: it only fails the step, with its identity.
            $found += [pscustomobject]@{ Kind = 'unidentified'; Info = $info; Window = $window }
        }
    }
    return $found
}

function Write-Desktop {
    Write-Host ('Foreground window listed by EnumWindows: {0}' -f [AmiberryCiDesktop]::EnumeratesForeground())
    Write-Host 'Foreground and visible top-level windows:'
    foreach ($window in [AmiberryCiDesktop]::Candidates()) {
        if ($window.Class -in $shellClasses) { continue }
        Write-Host (Format-Window $window (Get-ProcessInfo $window.ProcessId))
    }
}

if ($CheckOnly) {
    Write-Desktop
    $obstructions = @(Get-Obstruction)
    if ($obstructions.Count -eq 0) { Write-Host 'The interactive desktop is clear.'; return }
    foreach ($item in $obstructions) { Write-Host ("{0}: {1}" -f $item.Kind, (Format-Window $item.Window $item.Info)) }
    exit 1
}

Write-Desktop

# Documented policy: do not launch the privacy settings experience at sign-in. It stops new launches; it does not
# close one that is already showing, which is handled below.
$policy = 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\OOBE'
New-Item -Path $policy -Force | Out-Null
New-ItemProperty -Path $policy -Name DisablePrivacyExperience -Value 1 -PropertyType DWord -Force | Out-Null

$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
$clearSince = $null
$lastEscape = [DateTime]::MinValue
while ([DateTime]::UtcNow -lt $deadline) {
    $obstructions = @(Get-Obstruction)
    if ($obstructions.Count -eq 0) {
        if (-not $clearSince) { $clearSince = [DateTime]::UtcNow }
        if (([DateTime]::UtcNow - $clearSince).TotalSeconds -ge $StableSeconds) {
            Write-Host "The interactive desktop has been clear for $StableSeconds s."
            Write-Desktop
            return
        }
        Start-Sleep -Milliseconds 500
        continue
    }
    $clearSince = $null
    foreach ($item in $obstructions) {
        Write-Host ("{0}: {1}" -f $item.Kind, (Format-Window $item.Window $item.Info))
        $name = if ($item.Info) { $item.Info.ProcessName } else { $null }
        if ($item.Kind -eq 'privacy' -and $protectedHosts -notcontains $name) {
            # Exactly this process: it owns the window that shows the first-run privacy experience (by its heading or
            # its image name), and it is not one of the shell's own hosts.
            Write-Host "Ending the first-run privacy experience host $name, pid $($item.Info.Id)."
            Stop-Process -Id $item.Info.Id -Force
            Wait-Process -Id $item.Info.Id -Timeout 10 -ErrorAction SilentlyContinue
        } elseif ($item.Kind -in @('privacy', 'unidentified', 'oobe')) {
            Write-Host "Not ending pid $($item.Window.ProcessId) ($name): this step may not end it."
        } elseif (([DateTime]::UtcNow - $lastEscape).TotalSeconds -ge 2) {
            Write-Host "Closing the open $name window with Escape."
            [AmiberryCiDesktop]::PressEscape()
            $lastEscape = [DateTime]::UtcNow
        }
    }
    Start-Sleep -Milliseconds 500
}

Write-Host 'The interactive desktop is still obstructed:'
foreach ($item in @(Get-Obstruction)) { Write-Host ("{0}: {1}" -f $item.Kind, (Format-Window $item.Window $item.Info)) }
Write-Desktop
throw "The interactive desktop was not clear within $TimeoutSeconds s; UI tests would not receive input."
