# Leaves the interactive desktop of a GitHub-hosted Windows runner clear for tests that drive real UI and input.
#
# The windows-11-arm image can start a job while the first sign-in is still showing its full-screen
# "Choose privacy settings for your device" experience (WWAHost.exe) and the open Start menu. They hold the
# foreground over the whole desktop, so synthetic input and window activation never reach the application under
# test (actions/runner-images#14069; the image fix, #14673, only stops it for newly created accounts).
#
# This runs only on an ephemeral GitHub-hosted runner and refuses to run anywhere else: it never touches a developer's
# desktop. It does not answer the privacy experience: it sets Microsoft's documented policy that stops the experience
# from launching (Policy CSP Privacy/DisablePrivacyExperience, Windows Components > OOBE), ends the exact WWAHost.exe
# process that owns a visible window, closes an open Start menu with Escape only while the Start menu host owns the
# foreground, and then requires the desktop to stay clear. If it cannot, the job fails instead of reporting success.
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
        public long Handle; public uint ProcessId; public string Class, Title; public int Left, Top, Right, Bottom; public bool Foreground;
        public double ScreenCoverage;
    }
    public static List<Window> Visible() {
        var result = new List<Window>();
        IntPtr foreground = GetForegroundWindow();
        double screen = (double)GetSystemMetrics(0) * GetSystemMetrics(1);
        EnumWindows(delegate(IntPtr handle, IntPtr parameter) {
            if (!IsWindowVisible(handle)) return true;
            Rect rect; if (!GetWindowRect(handle, out rect)) return true;
            uint pid; GetWindowThreadProcessId(handle, out pid);
            var cls = new StringBuilder(256); GetClassName(handle, cls, cls.Capacity);
            var title = new StringBuilder(256); GetWindowText(handle, title, title.Capacity);
            double area = (double)Math.Max(0, rect.Right - rect.Left) * Math.Max(0, rect.Bottom - rect.Top);
            result.Add(new Window { Handle = handle.ToInt64(), ProcessId = pid, Class = cls.ToString(), Title = title.ToString(),
                Left = rect.Left, Top = rect.Top, Right = rect.Right, Bottom = rect.Bottom, Foreground = handle == foreground,
                ScreenCoverage = screen > 0 ? area / screen : 0 });
            return true;
        }, IntPtr.Zero);
        return result;
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

$session = [Diagnostics.Process]::GetCurrentProcess().SessionId
# Windows that belong to the interactive session's own shell hosts, by exact image name.
$privacyHost = 'WWAHost'
$startHosts = @('StartMenuExperienceHost', 'SearchHost')

function Get-Obstruction {
    $found = @()
    foreach ($window in [AmiberryCiDesktop]::Visible()) {
        $process = Get-Process -Id $window.ProcessId -ErrorAction SilentlyContinue
        if (-not $process -or $process.SessionId -ne $session) { continue }
        if ($process.ProcessName -eq $privacyHost) {
            $found += [pscustomobject]@{ Kind = 'privacy'; Process = $process; Window = $window }
        } elseif ($window.Foreground -and $startHosts -contains $process.ProcessName) {
            $found += [pscustomobject]@{ Kind = 'start'; Process = $process; Window = $window }
        }
    }
    return $found
}

function Format-Window($window, $process) {
    '  hwnd=0x{0:X} pid={1} process={2} class={3} title="{4}" rect={5},{6},{7},{8} coverage={9:P0} foreground={10}' -f
        $window.Handle, $window.ProcessId, $process, $window.Class, $window.Title, $window.Left, $window.Top, $window.Right, $window.Bottom, $window.ScreenCoverage, $window.Foreground
}

function Write-Desktop {
    Write-Host 'Visible top-level windows in this session:'
    foreach ($window in [AmiberryCiDesktop]::Visible()) {
        if ($window.Class -in @('Shell_TrayWnd', 'Shell_SecondaryTrayWnd', 'Progman', 'WorkerW')) { continue }
        $process = Get-Process -Id $window.ProcessId -ErrorAction SilentlyContinue
        Write-Host (Format-Window $window $(if ($process) { $process.ProcessName } else { '?' }))
    }
}

if ($CheckOnly) {
    $obstructions = @(Get-Obstruction)
    if ($obstructions.Count -eq 0) { Write-Host 'The interactive desktop is clear.'; return }
    foreach ($item in $obstructions) { Write-Host ("{0}: {1}" -f $item.Kind, (Format-Window $item.Window $item.Process.ProcessName)) }
    exit 1
}

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
            return
        }
        Start-Sleep -Milliseconds 500
        continue
    }
    $clearSince = $null
    foreach ($item in $obstructions) {
        Write-Host ("{0}: {1}" -f $item.Kind, (Format-Window $item.Window $item.Process.ProcessName))
        if ($item.Kind -eq 'privacy') {
            # Exactly this WWAHost.exe, identified by image name, session and its own visible window; nothing else.
            Write-Host "Ending the first-run privacy experience host, pid $($item.Process.Id)."
            Stop-Process -Id $item.Process.Id -Force
            $null = $item.Process.WaitForExit(10000)
        } elseif (([DateTime]::UtcNow - $lastEscape).TotalSeconds -ge 2) {
            Write-Host "Closing the open $($item.Process.ProcessName) window with Escape."
            [AmiberryCiDesktop]::PressEscape()
            $lastEscape = [DateTime]::UtcNow
        }
    }
    Start-Sleep -Milliseconds 500
}

Write-Host 'The interactive desktop is still obstructed:'
foreach ($item in @(Get-Obstruction)) { Write-Host ("{0}: {1}" -f $item.Kind, (Format-Window $item.Window $item.Process.ProcessName)) }
Write-Desktop
throw "The interactive desktop was not clear within $TimeoutSeconds s; UI tests would not receive input."
