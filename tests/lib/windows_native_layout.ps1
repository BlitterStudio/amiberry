# Both native tests use a portable copy, never the developer's settings or media.
function New-NativeTestLayout {
    param(
        [Parameter(Mandatory = $true)][string]$AmiberryPath,
        [switch]$IncludeUI
    )
    $source = Split-Path (Resolve-Path $AmiberryPath).Path
    $scratch = Join-Path ([IO.Path]::GetTempPath()) ('amiberry-native-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $scratch | Out-Null
    try {
        Copy-Item -LiteralPath (Resolve-Path $AmiberryPath).Path -Destination (Join-Path $scratch 'Amiberry.exe')
        Get-ChildItem -LiteralPath $source -Filter '*.dll' -File | Copy-Item -Destination $scratch
        $directories = @('data', 'Controllers', 'ROMs', 'WHDBoot', 'plugins')
        if ($IncludeUI) { $directories += 'native-ui' }
        foreach ($name in $directories) {
            $path = Join-Path $source $name
            if (Test-Path -LiteralPath $path -PathType Container) {
                Copy-Item -LiteralPath $path -Destination (Join-Path $scratch $name) -Recurse
            } elseif ($name -eq 'native-ui') {
                throw "Native UI is missing beside $AmiberryPath. Build and install the complete native application."
            }
        }
        [IO.File]::WriteAllText((Join-Path $scratch 'amiberry.portable'), '')
        return $scratch
    } catch {
        Remove-Item -LiteralPath $scratch -Recurse -Force
        throw
    }
}
