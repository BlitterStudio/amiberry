// The WHDBoot tools API (settings/amiberry_whdboot.h): which of the
// on-demand WHDBooter files are installed, and downloading them. WHDLoad
// (Bert Jahn, whdload.de), the kickstart RTB files (Aminet skick346), JST
// and the optional Amiga libraries (Aminet) are not bundled with Amiberry,
// so the native front ends offer the download the Paths panel's button
// performs.

#include "sysconfig.h"
#include "sysdeps.h"
#include "target.h"
#include "host_fs.h"
#include "gui/gui_handling.h"
#include "settings/amiberry_whdboot.h"

#include <atomic>
#include <filesystem>
#include <string>
#include <system_error>

namespace
{

std::filesystem::path whdboot_root()
{
	const std::string path = get_whdbootpath();
	return path.empty() ? std::filesystem::path{} : host_path(path);
}

bool whdboot_file_exists(const std::filesystem::path& root, const std::filesystem::path& relative)
{
	std::error_code error;
	return !root.empty() && std::filesystem::exists(root / relative, error) && !error;
}

void get_status(const std::filesystem::path& root, amiberry_whdboot_status* status)
{
	status->downloads_available = whdboot_can_download();
	status->emulation_running = whdboot_emulation_blocks_downloads();
	status->whdload_installed = whdboot_file_exists(root, "WHDLoad");
	status->jst_installed = whdboot_file_exists(root, "JST");
	status->host_quit_installed = whdboot_file_exists(root, "host-quit");
	status->boot_data_installed = whdboot_file_exists(root, "boot-data.zip")
		|| whdboot_file_exists(root, "boot-data");
	status->database_installed = whdboot_file_exists(root, "game-data/whdload_db.json")
		|| whdboot_file_exists(root, "game-data/whdload_db.xml");

	// kick31034.A1000.RTB is not in skick346 and stays bundled.
	// (kick33192.A500.RTB is skick346's kick33180.A500.RTB byte for
	// byte; the download creates it as a copy, so it is not required.)
	bool rtb_files_installed = true;
	for (const auto& filename : whdboot_skick_rtb_files())
	{
		if (!whdboot_file_exists(root, std::filesystem::path("save-data") / "Kickstarts" / filename))
			rtb_files_installed = false;
	}
	if (!whdboot_file_exists(root, std::filesystem::path("save-data") / "Kickstarts" / "kick31034.A1000.RTB"))
		rtb_files_installed = false;
	status->rtb_files_installed = rtb_files_installed;
	status->libraries_installed = whdboot_optional_libraries_installed(host_path_utf8(root));
}

}

extern "C" void amiberry_whdboot_status_get(amiberry_whdboot_status* status)
{
	if (status == nullptr)
		return;
	get_status(whdboot_root(), status);
}

extern "C" void amiberry_whdboot_status_get_at_path(const char* path, amiberry_whdboot_status* status)
{
	if (status == nullptr)
		return;
	get_status(path ? host_path(path) : std::filesystem::path{}, status);
}

extern "C" amiberry_whdboot_download_result amiberry_whdboot_download(
	amiberry_whdboot_progress_fn progress_fn, void* user)
{
	if (!whdboot_can_download())
		return AMIBERRY_WHDBOOT_DOWNLOAD_UNAVAILABLE;

	if (whdboot_emulation_blocks_downloads())
		return AMIBERRY_WHDBOOT_DOWNLOAD_BUSY;

	std::atomic<bool> cancelled{ false };
	const auto result = download_whdboot_assets(
		[&](const whdboot_download_progress& progress)
		{
			if (!progress_fn)
				return false;
			amiberry_whdboot_progress reported{};
			reported.step = progress.step;
			reported.total_steps = progress.total_steps;
			reported.label = progress.label.c_str();
			reported.bytes_downloaded = progress.bytes_downloaded;
			reported.bytes_total = progress.bytes_total;
			const int cancel = progress_fn(&reported, user);
			if (cancel != 0)
				cancelled.store(true);
			return cancel != 0;
		},
		&cancelled);

	switch (result.outcome)
	{
	case whdboot_download_outcome::success:
		return AMIBERRY_WHDBOOT_DOWNLOAD_OK;
	case whdboot_download_outcome::partial_failure:
		return AMIBERRY_WHDBOOT_DOWNLOAD_PARTIAL;
	case whdboot_download_outcome::cancelled:
		return AMIBERRY_WHDBOOT_DOWNLOAD_CANCELLED;
	case whdboot_download_outcome::busy:
		return AMIBERRY_WHDBOOT_DOWNLOAD_BUSY;
	case whdboot_download_outcome::failed:
	default:
		return AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
	}
}
