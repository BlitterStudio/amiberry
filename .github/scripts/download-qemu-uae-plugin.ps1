param(
	[Parameter(Mandatory = $true)]
	[string]$Asset,
	[string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
	if ([string]::IsNullOrWhiteSpace($env:GITHUB_WORKSPACE)) {
		$OutputDirectory = Join-Path (Get-Location) ".qemu-uae-plugin"
	} else {
		$OutputDirectory = Join-Path $env:GITHUB_WORKSPACE ".qemu-uae-plugin"
	}
}

$repo = if ([string]::IsNullOrWhiteSpace($env:QEMU_UAE_RELEASE_REPO)) { "BlitterStudio/amiberry-qemu-uae" } else { $env:QEMU_UAE_RELEASE_REPO }
$tag = if ([string]::IsNullOrWhiteSpace($env:QEMU_UAE_RELEASE_TAG)) { "v11.0.1-amiberry.7" } else { $env:QEMU_UAE_RELEASE_TAG }
$downloadDir = Join-Path $OutputDirectory "download"
$extractDir = Join-Path $OutputDirectory "extract"

function Get-RelativePathCompat {
	param(
		[Parameter(Mandatory = $true)]
		[string]$BasePath,
		[Parameter(Mandatory = $true)]
		[string]$TargetPath
	)

	$baseFull = [System.IO.Path]::GetFullPath($BasePath)
	if (-not $baseFull.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and -not $baseFull.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
		$baseFull += [System.IO.Path]::DirectorySeparatorChar
	}

	$targetFull = [System.IO.Path]::GetFullPath($TargetPath)
	$relativeUri = ([System.Uri]$baseFull).MakeRelativeUri([System.Uri]$targetFull)
	return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", [System.IO.Path]::DirectorySeparatorChar)
}

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
	throw "gh is required to download QEMU-UAE release assets."
}

Remove-Item -Recurse -Force $OutputDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $downloadDir, $extractDir | Out-Null

Write-Host "Downloading $Asset from $repo@$tag"
& gh release download $tag `
	--repo $repo `
	--pattern $Asset `
	--pattern SHA256SUMS `
	--dir $downloadDir

$archivePath = Join-Path $downloadDir $Asset
$sumsPath = Join-Path $downloadDir "SHA256SUMS"
$sumLine = Get-Content $sumsPath | Where-Object {
	$parts = $_.Trim() -split '\s+', 2
	if ($parts.Count -ne 2) { return $false }
	$name = $parts[1].TrimStart("*")
	return $name -eq $Asset -or [System.IO.Path]::GetFileName($name) -eq $Asset
} | Select-Object -First 1

if (-not $sumLine) {
	throw "No SHA256SUMS entry found for $Asset."
}

$expected = (($sumLine.Trim() -split '\s+', 2)[0]).ToLowerInvariant()
$actual = (Get-FileHash -Algorithm SHA256 -Path $archivePath).Hash.ToLowerInvariant()
if ($actual -ne $expected) {
	throw "Checksum mismatch for ${Asset}: expected $expected, got $actual."
}
Write-Host "Verified SHA256 for ${Asset}: $actual"

if ($Asset.EndsWith(".zip", [System.StringComparison]::OrdinalIgnoreCase)) {
	Expand-Archive -Path $archivePath -DestinationPath $extractDir
} elseif ($Asset.EndsWith(".tar.xz", [System.StringComparison]::OrdinalIgnoreCase)) {
	& tar -xJf $archivePath -C $extractDir
} else {
	throw "Unsupported QEMU-UAE asset type: $Asset."
}

$plugin = Get-ChildItem -Path $extractDir -Recurse -File |
	Where-Object { $_.Name -in @("qemu-uae.so", "qemu-uae.dylib", "qemu-uae.dll") } |
	Select-Object -First 1

if (-not $plugin) {
	throw "Could not find qemu-uae plugin in $Asset."
}

$absPlugin = (Resolve-Path $plugin.FullName).Path
$pluginDir = (Resolve-Path (Split-Path -Parent $absPlugin)).Path
$cmakePlugin = $absPlugin -replace '\\', '/'
$cmakePluginDir = $pluginDir -replace '\\', '/'

if ([string]::IsNullOrWhiteSpace($env:GITHUB_WORKSPACE)) {
	$workspace = (Get-Location).Path
} else {
	$workspace = (Resolve-Path $env:GITHUB_WORKSPACE).Path
}
$relativePlugin = (Get-RelativePathCompat -BasePath $workspace -TargetPath $absPlugin) -replace '\\', '/'

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_ENV)) {
	"QEMU_UAE_PLUGIN=$cmakePlugin" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
	"QEMU_UAE_PLUGIN_DIR=$cmakePluginDir" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
	"QEMU_UAE_PLUGIN_RELATIVE=$relativePlugin" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
}

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_OUTPUT)) {
	"qemu_uae_plugin=$cmakePlugin" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
	"qemu_uae_plugin_dir=$cmakePluginDir" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
	"qemu_uae_plugin_relative=$relativePlugin" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
}

Write-Host "QEMU-UAE plugin ready: $cmakePlugin"
