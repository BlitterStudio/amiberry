#!/bin/bash
# Packages a built Amiberry.app for App Store Connect.
#
#   store-package.sh macos <Amiberry.app> <profile.provisionprofile> <out.pkg> [build number]
#   store-package.sh ios   <Amiberry.app> <profile.mobileprovision>  <out.ipa> [build number]
#
# The app is copied, never changed in place. The copy gets the build number
# as CFBundleVersion, the provisioning profile, and a signature by the Apple
# Distribution identity with the repository's entitlements plus the
# application and team identifiers from the profile (TestFlight and the
# store refuse an app without them). The Mac copy becomes an installer
# package signed by the Mac Installer Distribution identity; the iOS copy
# becomes an .ipa.
#
# The version (CFBundleShortVersionString) stays the project version. The
# build number defaults to the Android versionCode formula, so every upload
# of the same version gets a higher one: major * 100000000 + minor * 1000000
# + patch * 10000 + minutes since 2024-01-01 (UTC).
#
# Identities are looked up in the keychain by name; override them with
# AMIBERRY_DIST_IDENTITY and AMIBERRY_INSTALLER_IDENTITY.
#
# Upload with:
#   xcrun altool --upload-app -t macos|ios -f <file> --apiKey <id> --apiIssuer <issuer>

set -euo pipefail

if [ $# -lt 4 ] || [ $# -gt 5 ]; then
	sed -n '2,5p' "$0" >&2
	exit 2
fi

platform=$1
app=${2%/}
profile=$3
out=$4
build_number=${5:-}
repo=$(cd "$(dirname "$0")/../.." && pwd)

case "$platform" in
	macos)
		entitlements_src="$repo/packaging/macos/Amiberry-AppStore.entitlements"
		app_id_key=com.apple.application-identifier
		;;
	ios)
		entitlements_src="$repo/packaging/ios/Amiberry.entitlements"
		app_id_key=application-identifier
		;;
	*)
		echo "unknown platform: $platform (macos or ios)" >&2
		exit 2
		;;
esac

[ -d "$app" ] || { echo "no app bundle at $app" >&2; exit 1; }
[ -f "$profile" ] || { echo "no provisioning profile at $profile" >&2; exit 1; }
if [ -n "$build_number" ] && ! [[ "$build_number" =~ ^[0-9]+(\.[0-9]+){0,2}$ ]]; then
	echo "build number must be one to three dot-separated integers: $build_number" >&2
	exit 2
fi

dist_identity=${AMIBERRY_DIST_IDENTITY:-Apple Distribution}
installer_identity=${AMIBERRY_INSTALLER_IDENTITY:-3rd Party Mac Developer Installer}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

security cms -D -i "$profile" > "$work/profile.plist"
pb() { /usr/libexec/PlistBuddy -c "$1" "$2"; }
profile_app_id=$(pb "Print :Entitlements:$app_id_key" "$work/profile.plist")
team_id=$(pb "Print :Entitlements:com.apple.developer.team-identifier" "$work/profile.plist")

staged="$work/$(basename "$app")"
ditto "$app" "$staged"

if [ "$platform" = macos ]; then
	contents="$staged/Contents"
	cp "$profile" "$contents/embedded.provisionprofile"
else
	contents="$staged"
	cp "$profile" "$staged/embedded.mobileprovision"
fi

bundle_id=$(pb "Print :CFBundleIdentifier" "$contents/Info.plist")
if [ "$profile_app_id" != "$team_id.$bundle_id" ]; then
	echo "the profile is for $profile_app_id, the app is $team_id.$bundle_id" >&2
	exit 1
fi
version=$(pb "Print :CFBundleShortVersionString" "$contents/Info.plist")
if [ -z "$build_number" ]; then
	IFS=. read -r v_major v_minor v_patch <<< "$version"
	minutes=$(( ($(date -u +%s) - $(date -u -j -f '%Y-%m-%d %H:%M:%S' '2024-01-01 00:00:00' +%s)) / 60 ))
	build_number=$(( v_major * 100000000 + v_minor * 1000000 + ${v_patch:-0} * 10000 + minutes ))
fi
pb "Set :CFBundleVersion $build_number" "$contents/Info.plist"

# The Mac app is built by Ninja, not Xcode, so its Info.plist lacks the
# platform and toolchain keys Xcode writes and App Store Connect reads.
# LSMinimumSystemVersion is the newest minimum of any binary in the bundle:
# the bundled Homebrew libraries are built for the build machine's macOS,
# whatever the app's own deployment target.
if [ "$platform" = macos ]; then
	min_os=0
	while IFS= read -r -d '' file; do
		if file -b "$file" | grep -q 'Mach-O'; then
			v=$(vtool -show-build "$file" 2>/dev/null | awk '/minos/ {print $2}' | sort -V | tail -1)
			if [ -n "$v" ] && [ "$(printf '%s\n%s\n' "$min_os" "$v" | sort -V | tail -1)" = "$v" ]; then
				min_os=$v
			fi
		fi
	done < <(find "$staged" -type f -print0)
	sdk_version=$(xcrun --sdk macosx --show-sdk-version)
	sdk_build=$(xcrun --sdk macosx --show-sdk-build-version)
	xcode_version=$(xcodebuild -version | awk '/^Xcode/ {print $2}')
	xcode_build=$(xcodebuild -version | awk '/^Build version/ {print $3}')
	IFS=. read -r xc_major xc_minor xc_patch <<< "$xcode_version"
	dt_xcode=$(printf '%d%d%d' "$xc_major" "${xc_minor:-0}" "${xc_patch:-0}")
	plist="$contents/Info.plist"
	pb "Delete :LSMinimumSystemVersion" "$plist" 2>/dev/null || true
	pb "Add :LSMinimumSystemVersion string $min_os" "$plist"
	pb "Delete :CFBundleSupportedPlatforms" "$plist" 2>/dev/null || true
	pb "Add :CFBundleSupportedPlatforms array" "$plist"
	pb "Add :CFBundleSupportedPlatforms:0 string MacOSX" "$plist"
	for kv in "DTPlatformName macosx" "DTPlatformVersion $sdk_version" "DTPlatformBuild $sdk_build" \
		"DTSDKName macosx$sdk_version" "DTSDKBuild $sdk_build" "DTXcode $(printf '%04d' "$dt_xcode")" \
		"DTXcodeBuild $xcode_build" "DTCompiler com.apple.compilers.llvm.clang.1_0" \
		"BuildMachineOSBuild $(sw_vers -buildVersion)"; do
		pb "Delete :${kv%% *}" "$plist" 2>/dev/null || true
		pb "Add :${kv%% *} string ${kv#* }" "$plist"
	done
	echo "LSMinimumSystemVersion $min_os (the newest minimum in the bundle)"
fi

cp "$entitlements_src" "$work/entitlements.plist"
pb "Add :$app_id_key string $profile_app_id" "$work/entitlements.plist"
pb "Add :com.apple.developer.team-identifier string $team_id" "$work/entitlements.plist"
if [ "$platform" = ios ]; then
	pb "Add :get-task-allow bool false" "$work/entitlements.plist"
	pb "Add :beta-reports-active bool true" "$work/entitlements.plist"
fi

# Re-signing replaces the build's signatures; only failures are reported.
sign() {
	local msg
	if ! msg=$(codesign --force --sign "$dist_identity" "$@" 2>&1); then
		echo "$msg" >&2
		exit 1
	fi
}

# Nested code first, the app last. Only Mach-O files are signed: the bundle
# also carries executable Amiga programs (whdboot/host-quit).
while IFS= read -r -d '' file; do
	if file -b "$file" | grep -q 'Mach-O'; then
		sign "$file"
	fi
done < <(find "$staged" -type f ! -path "$contents/MacOS/*" ! -path "$staged/$(pb "Print :CFBundleExecutable" "$contents/Info.plist")" -print0)

sign --entitlements "$work/entitlements.plist" "$staged"
codesign --verify --deep --strict "$staged"

mkdir -p "$(dirname "$out")"
out="$(cd "$(dirname "$out")" && pwd)/$(basename "$out")"
rm -f "$out"
if [ "$platform" = macos ]; then
	productbuild --component "$staged" /Applications --sign "$installer_identity" "$out"
else
	mkdir "$work/Payload"
	mv "$staged" "$work/Payload/"
	(cd "$work" && zip -qry "$out" Payload)
fi
echo "$out: $bundle_id $version ($build_number), $platform"
