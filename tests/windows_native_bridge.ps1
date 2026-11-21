param(
    [Parameter(Mandatory = $true)][string]$AmiberryPath,
    [ValidateSet('win-x64', 'win-arm64')][string]$RuntimeIdentifier
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/lib/windows_native_layout.ps1"

if ($env:OS -ne 'Windows_NT') { throw 'This test requires Windows with a visible desktop session.' }
if (-not $RuntimeIdentifier) {
    switch ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()) {
        'X64' { $RuntimeIdentifier = 'win-x64' }
        'Arm64' { $RuntimeIdentifier = 'win-arm64' }
        default { throw 'Only Windows x64 and arm64 hosts are supported.' }
    }
}
Get-Command dotnet -ErrorAction Stop | Out-Null

$scratch = $null
$core = $null
try {
    # Do not copy the production native-ui: the real core launches this test client instead.
    $scratch = New-NativeTestLayout -AmiberryPath $AmiberryPath
    $exe = Join-Path $scratch 'Amiberry.exe'
    $reportPath = Join-Path $scratch 'native-bridge-report.txt'
    $ui = Join-Path $scratch 'native-ui'
    $project = Join-Path $PSScriptRoot 'windows-native-bridge/Amiberry.Windows.csproj'
    & dotnet publish $project --configuration Release --runtime $RuntimeIdentifier --self-contained true `
        --output $ui "-p:BaseIntermediateOutputPath=$scratch/obj/" `
        "-p:MSBuildProjectExtensionsPath=$scratch/obj/" "-p:BaseOutputPath=$scratch/bin/"
    if ($LASTEXITCODE -ne 0) { throw "Native bridge client publish failed with code $LASTEXITCODE." }

    $childEnvironment = @{
        AMIBERRY_NATIVE_TEST_REPORT = $reportPath
        AMIBERRY_HOME_DIR = $scratch
        AMIBERRY_CONFIG_DIR = (Join-Path $scratch 'Configurations')
        AMIBERRY_DATA_DIR = (Join-Path $scratch 'data')
        AMIBERRY_PLUGINS_DIR = (Join-Path $scratch 'plugins')
        USERPROFILE = (Join-Path $scratch 'profile')
        HOME = (Join-Path $scratch 'profile')
        APPDATA = (Join-Path $scratch 'profile/AppData/Roaming')
        LOCALAPPDATA = (Join-Path $scratch 'profile/AppData/Local')
        AMIBERRY_NATIVE_TEST_STARTUP_WHD = (Join-Path $scratch ('startup-' + [char]0x6A4B + '-' + [char]0xE4))
    }
    foreach ($name in @('Configurations', 'Settings', 'profile', 'profile/AppData/Roaming', 'profile/AppData/Local')) {
        New-Item -ItemType Directory -Path (Join-Path $scratch $name) -Force | Out-Null
    }
    # An earlier session saved a WHDBoot folder whose name is outside the ANSI code page; the core loads it at startup.
    New-Item -ItemType Directory -Path $childEnvironment.AMIBERRY_NATIVE_TEST_STARTUP_WHD -Force | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $scratch 'Settings/amiberry.conf'),
        (New-Object Text.UTF8Encoding($false)).GetBytes("whdboot_path=$($childEnvironment.AMIBERRY_NATIVE_TEST_STARTUP_WHD)`n"))

    # Start-Process inherits this environment. Restore the caller immediately, even if launch fails.
    $previousEnvironment = @{}
    foreach ($name in $childEnvironment.Keys) {
        $previousEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
    }
    try {
        foreach ($name in $childEnvironment.Keys) {
            [Environment]::SetEnvironmentVariable($name, $childEnvironment[$name], 'Process')
        }
        # No dummy SDL driver, hidden window, or dump-only path: exercise real Windows initialization.
        $core = Start-Process -FilePath $exe -WorkingDirectory $scratch -PassThru
    } finally {
        foreach ($name in $previousEnvironment.Keys) {
            [Environment]::SetEnvironmentVariable($name, $previousEnvironment[$name], 'Process')
        }
    }

    if (-not $core.WaitForExit(300000)) { throw 'Native bridge core did not exit within 300 seconds after client disconnect.' }
    if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
        throw "Native core exited with code $($core.ExitCode) without a bridge report. Confirm this is an AMIBERRY_NATIVE_WINDOWS_UI build."
    }
    $report = [IO.File]::ReadAllText($reportPath)
    Write-Output $report
    if (-not $report.StartsWith('PASS: ')) { throw 'Native bridge regression failed; see the client assertion report above.' }
    if ($core.ExitCode -ne 0) { throw "Native core exited with code $($core.ExitCode)." }
} finally {
    # This process handle and directory were created by this invocation; never stop by executable name.
    try {
        if ($core -and -not $core.HasExited) {
            $core.Kill()
            if (-not $core.WaitForExit(10000)) { throw "Cannot stop owned native core PID $($core.Id)." }
        }
    } finally {
        if ($core) { $core.Dispose() }
        if ($scratch) { Remove-Item -LiteralPath $scratch -Recurse -Force }
    }
}
