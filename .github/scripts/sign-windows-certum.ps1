# Signs the Windows release artifacts with the Certum SimplySign cloud
# certificate: Amiberry.exe inside each portable ZIP, and each installer EXE.
#
# Inputs (environment):
#   CERTUM_OTPAUTH_URI      required; the otpauth://totp/Certum:<login>?secret=...
#                           URI from the SimplySign activation QR code
#   CERTUM_USERNAME         optional; SimplySign login, if it differs from the
#                           login in the URI
#   CERTUM_CERT_THUMBPRINT  optional; SHA-1 thumbprint the certificate must have
#
# The login uses SimplySign Desktop's own "/autologin <login> <otp>" switch, so
# no GUI automation is involved. The secret, login and generated codes are
# masked before anything can print them.
param(
	[Parameter(Mandatory = $true)]
	[string[]]$ArtifactDirectory
)

$ErrorActionPreference = "Stop"

$simplySignVersion = "9.4.5.95"
$simplySignSha256 = "6309E8B46F1F4E786288E777AEF515A2C30D885165B71A21191E9941BADEBD6B"
$simplySignUrl = "https://files.certum.eu/software/SimplySignDesktop/Windows/$simplySignVersion/SimplySignDesktop-$simplySignVersion-64-bit-en.msi"
$simplySignExe = Join-Path $env:ProgramFiles "Certum\SimplySign Desktop\SimplySignDesktop.exe"
$timestampUrl = "http://time.certum.pl"
$codeSigningOid = "1.3.6.1.5.5.7.3.3"
$workDir = Join-Path ([System.IO.Path]::GetTempPath()) "amiberry-certum-signing"

function Add-Mask([string]$Value) {
	if (-not [string]::IsNullOrEmpty($Value)) {
		Write-Host "::add-mask::$Value"
	}
}

function ConvertFrom-Base32([string]$Text) {
	$alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
	$clean = $Text.ToUpperInvariant().TrimEnd("=").Replace(" ", "")
	$bytes = New-Object System.Collections.Generic.List[byte]
	$buffer = 0
	$bits = 0
	foreach ($char in $clean.ToCharArray()) {
		$value = $alphabet.IndexOf($char)
		if ($value -lt 0) {
			throw "The TOTP secret is not valid Base32."
		}
		$buffer = (($buffer -shl 5) -bor $value) -band 0xFFFF
		$bits += 5
		if ($bits -ge 8) {
			$bits -= 8
			$bytes.Add([byte](($buffer -shr $bits) -band 0xFF))
		}
	}
	return ,$bytes.ToArray()
}

# Same rules as SimplySign Desktop's own QR parser (QRData): the path is
# "/Certum:<login>", SHA1/6 digits/30 seconds unless the query says otherwise.
function ConvertFrom-OtpAuthUri([string]$Text) {
	$uri = [System.Uri]$Text.Trim()
	if ($uri.Scheme -ne "otpauth" -or $uri.Host -ne "totp") {
		throw "CERTUM_OTPAUTH_URI must be the otpauth://totp/... URI from the SimplySign QR code."
	}
	$query = @{}
	foreach ($pair in $uri.Query.TrimStart("?").Split("&", [System.StringSplitOptions]::RemoveEmptyEntries)) {
		$name, $value = $pair.Split("=", 2)
		$query[$name.ToLowerInvariant()] = [System.Uri]::UnescapeDataString("$value")
	}
	if (-not $query["secret"]) {
		throw "CERTUM_OTPAUTH_URI has no secret parameter."
	}
	$path = [System.Uri]::UnescapeDataString($uri.AbsolutePath)
	$login = if ($path.StartsWith("/Certum:")) { $path.Substring("/Certum:".Length) } else { "" }
	return [pscustomobject]@{
		Login = $login
		Secret = $query["secret"]
		Algorithm = if ($query["algorithm"]) { $query["algorithm"].ToUpperInvariant() } else { "SHA1" }
		Digits = if ($query["digits"]) { [int]$query["digits"] } else { 6 }
		Period = if ($query["period"]) { [int]$query["period"] } else { 30 }
	}
}

function Get-TotpCode([byte[]]$Key, [string]$Algorithm, [int]$Digits, [long]$Counter) {
	$hmac = switch ($Algorithm) {
		"SHA1" { [System.Security.Cryptography.HMACSHA1]::new($Key) }
		"SHA256" { [System.Security.Cryptography.HMACSHA256]::new($Key) }
		"SHA384" { [System.Security.Cryptography.HMACSHA384]::new($Key) }
		"SHA512" { [System.Security.Cryptography.HMACSHA512]::new($Key) }
		default { throw "Unsupported TOTP algorithm '$Algorithm'." }
	}
	try {
		$message = [System.BitConverter]::GetBytes($Counter)
		if ([System.BitConverter]::IsLittleEndian) {
			[System.Array]::Reverse($message)
		}
		$hash = $hmac.ComputeHash($message)
	} finally {
		$hmac.Dispose()
	}
	$offset = $hash[$hash.Length - 1] -band 0x0F
	$binary = (([int]$hash[$offset] -band 0x7F) -shl 24) -bor
		(([int]$hash[$offset + 1]) -shl 16) -bor
		(([int]$hash[$offset + 2]) -shl 8) -bor
		([int]$hash[$offset + 3])
	return ($binary % [int][System.Math]::Pow(10, $Digits)).ToString().PadLeft($Digits, "0")
}

function Install-SimplySign {
	$msi = Join-Path $workDir "SimplySignDesktop-$simplySignVersion.msi"
	Write-Host "Downloading SimplySign Desktop $simplySignVersion"
	Invoke-WebRequest -Uri $simplySignUrl -OutFile $msi -UseBasicParsing

	$actual = (Get-FileHash -Algorithm SHA256 -Path $msi).Hash
	if ($actual -ne $simplySignSha256) {
		throw "SimplySign Desktop installer hash $actual does not match the pinned $simplySignSha256."
	}
	$signature = Get-AuthenticodeSignature -FilePath $msi
	if ($signature.Status -ne "Valid" -or $signature.SignerCertificate.Subject -notmatch "O=Asseco Data Systems S\.A\.") {
		throw "SimplySign Desktop installer is not validly signed by Asseco Data Systems ($($signature.Status))."
	}

	$install = Start-Process -FilePath msiexec.exe -ArgumentList @("/i", "`"$msi`"", "/qn", "/norestart") -Wait -PassThru
	if ($install.ExitCode -notin @(0, 3010)) {
		throw "SimplySign Desktop installation failed with msiexec exit code $($install.ExitCode)."
	}
	if (-not (Test-Path $simplySignExe)) {
		throw "SimplySign Desktop is missing after installation: $simplySignExe"
	}
}

function Find-SigningCertificate([string]$Thumbprint) {
	$now = Get-Date
	$certificates = @(Get-ChildItem Cert:\CurrentUser\My | Where-Object {
		$_.HasPrivateKey -and
		$_.Issuer -match "Certum" -and
		$_.NotBefore -le $now -and $_.NotAfter -gt $now -and
		@($_.EnhancedKeyUsageList | Where-Object { $_.ObjectId -eq $codeSigningOid }).Count -gt 0
	})
	if ($Thumbprint) {
		return $certificates | Where-Object { $_.Thumbprint -eq $Thumbprint } | Select-Object -First 1
	}
	if ($certificates.Count -gt 1) {
		throw "Several Certum code signing certificates are available ($($certificates.Thumbprint -join ', ')); set CERTUM_CERT_THUMBPRINT."
	}
	return $certificates | Select-Object -First 1
}

function Write-SimplySignLog {
	$logDir = Join-Path ([Environment]::GetFolderPath("MyDocuments")) "SimplySignLog"
	foreach ($log in @(Get-ChildItem -Path $logDir -Filter "*log.txt" -File -ErrorAction SilentlyContinue)) {
		Write-Host "--- $($log.Name)"
		Get-Content -Path $log.FullName | Write-Host
	}
}

function Connect-SimplySign([pscustomobject]$Otp, [string]$Login, [string]$Thumbprint) {
	$key = ConvertFrom-Base32 $Otp.Secret
	$lastCounter = -1L
	for ($attempt = 1; $attempt -le 3; $attempt++) {
		# Certum refuses a code from a period whose code was already tried, so
		# every attempt waits for a new period with time left to log in.
		while ($true) {
			$now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
			$counter = [long][System.Math]::Floor($now / $Otp.Period)
			$left = $Otp.Period - ($now % $Otp.Period)
			if ($counter -gt $lastCounter -and $left -ge 15) {
				break
			}
			Start-Sleep -Seconds ($left + 1)
		}
		$lastCounter = $counter
		$code = Get-TotpCode -Key $key -Algorithm $Otp.Algorithm -Digits $Otp.Digits -Counter $counter
		Add-Mask $code

		Write-Host "SimplySign login attempt $attempt"
		$process = Start-Process -FilePath $simplySignExe -ArgumentList @("/autologin", $Login, $code) -PassThru
		# A refused login makes SimplySign Desktop exit; a successful one keeps
		# it running and registers the cloud certificate in CurrentUser\My.
		$deadline = (Get-Date).AddSeconds(60)
		while ((Get-Date) -lt $deadline) {
			$certificate = Find-SigningCertificate $Thumbprint
			if ($certificate) {
				return $certificate
			}
			if ($process.HasExited) {
				break
			}
			Start-Sleep -Seconds 2
		}
		if (-not $process.HasExited) {
			Stop-Process -Id $process.Id -Force
		}
	}
	Write-SimplySignLog
	throw "SimplySign login failed: no Certum code signing certificate appeared in CurrentUser\My."
}

function Disconnect-SimplySign {
	if (-not (Get-Process -Name SimplySignDesktop -ErrorAction SilentlyContinue)) {
		return
	}
	# "/close" asks the running instance to quit, which also logs the session
	# out of the Certum cloud. Do not wait on it: it blocks until the running
	# instance has handled the request.
	Start-Process -FilePath $simplySignExe -ArgumentList "/close" | Out-Null
	Wait-Process -Name SimplySignDesktop -Timeout 15 -ErrorAction SilentlyContinue
	Get-Process -Name SimplySignDesktop -ErrorAction SilentlyContinue | Stop-Process -Force
}

function Get-SignTool {
	# SimplySign's CSP/KSP is x64, so this needs the x64 signtool.
	$signtool = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.*\x64\signtool.exe" -ErrorAction SilentlyContinue |
		Sort-Object { [version]$_.Directory.Parent.Name } -Descending |
		Select-Object -First 1
	if (-not $signtool) {
		throw "x64 signtool.exe from the Windows SDK was not found."
	}
	return $signtool.FullName
}

function Assert-Signed([string]$Path, [string]$Thumbprint) {
	$signature = Get-AuthenticodeSignature -FilePath $Path
	if ($signature.Status -ne "Valid") {
		throw "$Path signature status is $($signature.Status): $($signature.StatusMessage)"
	}
	if ($signature.SignerCertificate.Thumbprint -ne $Thumbprint) {
		throw "$Path is signed by $($signature.SignerCertificate.Thumbprint), expected $Thumbprint."
	}
	if (-not $signature.TimeStamperCertificate) {
		throw "$Path has no timestamp."
	}
}

function Get-PortableExeEntry([System.IO.Compression.ZipArchive]$Archive, [string]$ZipPath) {
	$entries = @($Archive.Entries | Where-Object { $_.FullName -match "^[^/]+/Amiberry\.exe$" })
	if ($entries.Count -ne 1) {
		throw "$ZipPath must contain exactly one <root>/Amiberry.exe, found $($entries.Count)."
	}
	return $entries[0]
}

$otpUri = $env:CERTUM_OTPAUTH_URI
if ([string]::IsNullOrWhiteSpace($otpUri)) {
	throw "CERTUM_OTPAUTH_URI is empty. Store it as a secret of the windows-signing environment."
}
Add-Mask $otpUri
$otp = ConvertFrom-OtpAuthUri $otpUri
Add-Mask $otp.Secret
$login = if ([string]::IsNullOrWhiteSpace($env:CERTUM_USERNAME)) { $otp.Login } else { $env:CERTUM_USERNAME.Trim() }
if ([string]::IsNullOrWhiteSpace($login)) {
	throw "No SimplySign login: the otpauth URI has no /Certum:<login> path and CERTUM_USERNAME is empty."
}
Add-Mask $login
$expectedThumbprint = ("$env:CERTUM_CERT_THUMBPRINT" -replace "[^0-9A-Fa-f]", "").ToUpperInvariant()

# Collect the files first, so a malformed artifact fails before the login.
Remove-Item -Recurse -Force $workDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $workDir | Out-Null
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem

$portables = @()
$installers = @()
foreach ($directory in $ArtifactDirectory) {
	$zips = @(Get-ChildItem -Path $directory -Filter "amiberry-*-portable.zip" -File)
	$exes = @(Get-ChildItem -Path $directory -Filter "amiberry-*.exe" -File)
	if ($zips.Count -ne 1 -or $exes.Count -ne 1) {
		throw "$directory must contain exactly one portable ZIP and one installer EXE (found $($zips.Count) and $($exes.Count))."
	}

	$extractDir = Join-Path $workDir ([System.IO.Path]::GetFileNameWithoutExtension($zips[0].Name))
	New-Item -ItemType Directory -Force -Path $extractDir | Out-Null
	$extracted = Join-Path $extractDir "Amiberry.exe"
	$archive = [System.IO.Compression.ZipFile]::OpenRead($zips[0].FullName)
	try {
		$entry = Get-PortableExeEntry $archive $zips[0].FullName
		[System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $extracted, $true)
	} finally {
		$archive.Dispose()
	}

	$portables += [pscustomobject]@{ Zip = $zips[0].FullName; Exe = $extracted }
	$installers += $exes[0].FullName
}

$signtool = Get-SignTool
try {
	Install-SimplySign
	$certificate = Connect-SimplySign -Otp $otp -Login $login -Thumbprint $expectedThumbprint
	$thumbprint = $certificate.Thumbprint
	Write-Host "Signing with certificate $thumbprint, issued by $($certificate.Issuer), valid to $($certificate.NotAfter.ToString('yyyy-MM-dd'))"
	if ($certificate.NotAfter -lt (Get-Date).AddDays(30)) {
		Write-Host "::warning::The Certum code signing certificate expires on $($certificate.NotAfter.ToString('yyyy-MM-dd'))."
	}

	$files = @($portables.Exe) + $installers
	& $signtool sign /sha1 $thumbprint /fd sha256 /tr $timestampUrl /td sha256 /d Amiberry /du https://amiberry.com $files
	if ($LASTEXITCODE -ne 0) {
		throw "signtool sign failed with exit code $LASTEXITCODE."
	}
} finally {
	Disconnect-SimplySign
}

foreach ($file in $files) {
	Assert-Signed $file $thumbprint
}

# Put the signed Amiberry.exe back into each ZIP in place, keeping every other
# entry as it was, then check the copy that is now inside the ZIP.
foreach ($portable in $portables) {
	$archive = [System.IO.Compression.ZipFile]::Open($portable.Zip, [System.IO.Compression.ZipArchiveMode]::Update)
	try {
		$entry = Get-PortableExeEntry $archive $portable.Zip
		$target = $entry.Open()
		try {
			$target.SetLength(0)
			$source = [System.IO.File]::OpenRead($portable.Exe)
			try {
				$source.CopyTo($target)
			} finally {
				$source.Dispose()
			}
		} finally {
			$target.Dispose()
		}
	} finally {
		$archive.Dispose()
	}

	$check = Join-Path $workDir "zip-check-Amiberry.exe"
	$archive = [System.IO.Compression.ZipFile]::OpenRead($portable.Zip)
	try {
		$entry = Get-PortableExeEntry $archive $portable.Zip
		[System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $check, $true)
	} finally {
		$archive.Dispose()
	}
	Assert-Signed $check $thumbprint
	Write-Host "Signed Amiberry.exe in $([System.IO.Path]::GetFileName($portable.Zip))"
}
foreach ($installer in $installers) {
	Write-Host "Signed $([System.IO.Path]::GetFileName($installer))"
}
