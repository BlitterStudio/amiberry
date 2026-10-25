// The WHDBoot tools API (settings/amiberry_whdboot.h): which of the
// on-demand WHDBooter files are installed, and downloading them. WHDLoad
// (Bert Jahn, whdload.de), the kickstart RTB files (Aminet skick346) and
// JST (Aminet) are not bundled with Amiberry, so the native front ends
// offer the download the Paths panel's button performs.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "target.h"
#include "gui/gui_handling.h"
#include "settings/amiberry_whdboot.h"

#include <atomic>
#include <filesystem>
#include <string>

namespace
{

std::filesystem::path whdboot_root()
{
	const std::string path = get_whdbootpath();
	return path.empty() ? std::filesystem::path{} : std::filesystem::path(path);
}

bool whdboot_file_exists(const std::filesystem::path& relative)
{
	const auto root = whdboot_root();
	return !root.empty() && std::filesystem::exists(root / relative);
}

}

extern "C" void amiberry_whdboot_status_get(amiberry_whdboot_status* status)
{
	if (status == nullptr)
		return;

	status->downloads_available = whdboot_can_download();
	status->emulation_running = whdboot_emulation_blocks_downloads();
	status->whdload_installed = whdboot_file_exists("WHDLoad");
	status->jst_installed = whdboot_file_exists("JST");
	status->amiquit_installed = whdboot_file_exists("AmiQuit");
	status->boot_data_installed = whdboot_file_exists("boot-data.zip")
		|| whdboot_file_exists("boot-data");
	status->database_installed = whdboot_file_exists("game-data/whdload_db.json")
		|| whdboot_file_exists("game-data/whdload_db.xml");

	bool rtb_files_installed = true;
	for (const auto& filename : whdboot_skick_rtb_files())
	{
		if (!whdboot_file_exists(std::filesystem::path("save-data") / "Kickstarts" / filename))
			rtb_files_installed = false;
	}
	// kick31034.A1000.RTB is not in skick346 and stays bundled.
	// (kick33192.A500.RTB is skick346's kick33180.A500.RTB byte for byte;
	// the download creates it as a copy, so it is not required here.)
	if (!whdboot_file_exists(std::filesystem::path("save-data") / "Kickstarts" / "kick31034.A1000.RTB"))
		rtb_files_installed = false;
	status->rtb_files_installed = rtb_files_installed;
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
