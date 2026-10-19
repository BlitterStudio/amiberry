#pragma once

#include <string>
#include <vector>

enum class linux_release_arch
{
	x86_64,
	aarch64,
	armhf
};

enum class linux_installation_kind
{
	native_package,
	appimage
};

linux_installation_kind get_linux_installation_kind();

const std::string& select_linux_notification_url(const std::string& release_url,
	const std::string& download_url, linux_installation_kind installation_kind);


std::string select_linux_release_asset(const std::vector<std::string>& asset_names,
	linux_release_arch architecture, linux_installation_kind installation_kind);

std::string find_release_asset_sha256(const std::string& sums, const std::string& asset_name);
