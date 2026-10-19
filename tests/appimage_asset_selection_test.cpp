#include "amiberry_update_asset_policy.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
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

	const auto root = std::filesystem::temp_directory_path() / "amiberry-appimage-payload-test";
	std::error_code error;
	std::filesystem::remove_all(root, error);
	std::filesystem::create_directories(root / "mount" / "usr" / "bin");
	std::filesystem::create_directories(root / "native" / "bin");
	std::ofstream(root / "mount" / "usr" / "bin" / "amiberry") << "payload\n";
	std::ofstream(root / "native" / "bin" / "amiberry") << "native\n";
	const auto appimage = (root / "Amiberry.AppImage").string();
	const auto appdir = (root / "mount").string();
	const auto payload = (root / "mount" / "usr" / "bin" / "amiberry").string();
	const auto native = (root / "native" / "bin" / "amiberry").string();
	const auto expect_payload = [](const bool actual, const bool expected, const char* scenario)
	{
		expect_equal(actual ? "appimage" : "native", expected ? "appimage" : "native", scenario);
	};
	expect_payload(is_appimage_payload(appimage.c_str(), appdir.c_str(), payload), true,
		"executable inside APPDIR");
	expect_payload(is_appimage_payload(appimage.c_str(), (appdir + "/").c_str(), payload), true,
		"APPDIR with trailing separator");
	expect_payload(is_appimage_payload(appimage.c_str(), appdir.c_str(), native), false,
		"inherited AppImage environment outside APPDIR");
	expect_payload(is_appimage_payload(appimage.c_str(), (root / "mount-other").string().c_str(), payload), false,
		"sibling directory sharing the APPDIR prefix");
	expect_payload(is_appimage_payload(appimage.c_str(), nullptr, payload), false,
		"APPIMAGE without APPDIR");
	expect_payload(is_appimage_payload(nullptr, appdir.c_str(), payload), false,
		"APPDIR without APPIMAGE");
	std::filesystem::remove_all(root, error);

	std::cout << "AppImage asset selection policy test passed\n";
	return 0;
}
