#!/usr/bin/env bash
set -euo pipefail

require_text() {
	local file="$1"
	local text="$2"
	if ! grep -Fq "$text" "$file"; then
		printf 'Missing RP9 association in %s: %s\n' "$file" "$text" >&2
		exit 1
	fi
}

mime_type="application/vnd.cloanto.rp9"

require_text packaging/linux/mime/amiberry.xml "<mime-type type=\"$mime_type\">"
require_text packaging/linux/mime/amiberry.xml '<glob pattern="*.rp9"'
require_text packaging/linux/Amiberry.desktop "$mime_type;"
require_text packaging/linux/Amiberry.metainfo.xml "<mediatype>$mime_type</mediatype>"

for plist in packaging/MacOSXBundleInfo.plist.in packaging/ios/iOSBundleInfo.plist.in; do
	require_text "$plist" '<string>rp9</string>'
	require_text "$plist" "<string>$mime_type</string>"
done

require_text packaging/windows/fileassoc.iss 'Name: "fileassoc\rp9"'
require_text packaging/windows/fileassoc.iss 'ValueName: "Amiberry.rp9"'
require_text packaging/windows/fileassoc.iss "ValueData: \"$mime_type\""

require_text android/app/src/main/AndroidManifest.xml 'android:pathPattern=".*\\.rp9"'
require_text android/app/src/main/AndroidManifest.xml "android:mimeType=\"$mime_type\""

if command -v xmllint >/dev/null 2>&1; then
	xmllint --noout packaging/linux/mime/amiberry.xml
	xmllint --noout packaging/linux/Amiberry.metainfo.xml
	xmllint --noout android/app/src/main/AndroidManifest.xml
fi

if command -v plutil >/dev/null 2>&1; then
	plutil -lint packaging/MacOSXBundleInfo.plist.in >/dev/null
	plutil -lint packaging/ios/iOSBundleInfo.plist.in >/dev/null
fi
