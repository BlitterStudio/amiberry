#include "amiberry_update_asset_policy.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void expect_equal(const std::string& actual, const std::string& expected, const char* scenario)
{
	if (actual == expected)
		return;
	std::cerr << scenario << ": expected '" << expected << "', got '" << actual << "'\n";
	std::exit(1);
}

}

int main()
{
	const std::vector<std::string> mixed_x86_assets = {
		"Amiberry-8.4.2-x86_64.AppImage.zsync",
		"amiberry-ubuntu-24.04-amd64.zip",
		"Amiberry-8.4.2-aarch64.AppImage",
		"Amiberry-8.4.2-x86_64.AppImage",
		"amiberry-fedora-x86_64.zip",
		"amiberry-windows-x86_64.zip",
		"SHA256SUMS"
	};
	expect_equal(
		select_linux_release_asset(mixed_x86_assets, linux_release_arch::x86_64,
			linux_installation_kind::appimage),
		"Amiberry-8.4.2-x86_64.AppImage",
		"x86_64 AppImage selection");
	expect_equal(
		select_linux_release_asset(mixed_x86_assets, linux_release_arch::aarch64,
			linux_installation_kind::appimage),
		"Amiberry-8.4.2-aarch64.AppImage",
		"aarch64 AppImage selection");
	expect_equal(
		select_linux_release_asset(mixed_x86_assets, linux_release_arch::x86_64,
			linux_installation_kind::native_package),
		"amiberry-ubuntu-24.04-amd64.zip",
		"native package selection");

	const std::vector<std::string> reordered_assets = {
		"amiberry-fedora-x86_64.zip",
		"Amiberry-8.4.2-x86_64.AppImage",
		"Amiberry-8.4.2-x86_64.AppImage.zsync"
	};
	expect_equal(
		select_linux_release_asset(reordered_assets, linux_release_arch::x86_64,
			linux_installation_kind::appimage),
		"Amiberry-8.4.2-x86_64.AppImage",
		"AppImage selection independent of asset order");

	const std::string release_url = "https://github.com/BlitterStudio/amiberry/releases/tag/v8.4.2";
	const std::string appimage_url = "https://github.com/BlitterStudio/amiberry/releases/download/v8.4.2/Amiberry-8.4.2-x86_64.AppImage";
	expect_equal(
		select_linux_notification_url(release_url, appimage_url,
			linux_installation_kind::appimage),
		appimage_url,
		"AppImage notification direct download");
	expect_equal(
		select_linux_notification_url(release_url, appimage_url,
			linux_installation_kind::native_package),
		release_url,
		"native package notification release page");
	expect_equal(
		select_linux_notification_url(release_url, "",
			linux_installation_kind::appimage),
		release_url,
		"AppImage notification without selected download");


	const std::string wanted = "Amiberry-8.4.2-x86_64.AppImage";
	const std::string sums =
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  " + wanted + ".zsync\n"
		"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb *" + wanted + "\n";
	expect_equal(
		find_release_asset_sha256(sums, wanted),
		"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
		"exact checksum filename matching");
	expect_equal(find_release_asset_sha256(sums, "missing.AppImage"), "",
		"missing checksum filename");

	std::cout << "AppImage asset selection policy test passed\n";
	return 0;
}
