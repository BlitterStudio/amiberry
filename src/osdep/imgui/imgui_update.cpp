#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "amiberry_update.h"
#include "target.h"

#include <string>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>

static bool s_update_popup_open = false;
static std::atomic<bool> s_download_in_progress{false};
static std::atomic<bool> s_download_complete{false};
static std::atomic<bool> s_download_failed{false};
static std::mutex s_download_error_mutex;
static std::string s_download_error;
static std::atomic<int64_t> s_download_progress_bytes{0};
static std::atomic<int64_t> s_download_total_bytes{0};
static UpdateInfo s_cached_update_info;
static bool s_check_triggered_manually = false;
static bool s_show_no_update_msg = false;
static bool s_has_cached_result = false;

static std::atomic<bool> s_download_cancel_requested{false};

static std::string format_bytes(int64_t bytes)
{
	if (bytes < 1024) return std::to_string(bytes) + " B";
	if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
	char buf[32];
	snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
	return buf;
}

static const char* get_update_method_label(const UpdateMethod method)
{
	switch (method)
	{
		case UpdateMethod::SELF_UPDATE:
			return "Self-update";
		case UpdateMethod::NOTIFY_ONLY:
			return "Notify only";
		case UpdateMethod::DISABLED:
		default:
			return "Disabled";
	}
}

static void skip_cached_version()
{
	const size_t max_len = sizeof(amiberry_options.skipped_version) / sizeof(amiberry_options.skipped_version[0]);
	_tcsncpy(amiberry_options.skipped_version, s_cached_update_info.latest_version.c_str(), max_len - 1);
	amiberry_options.skipped_version[max_len - 1] = '\0';
	save_amiberry_settings();
	s_has_cached_result = false;
	s_show_no_update_msg = false;
}

static void start_background_download()
{
	if (s_download_in_progress || s_cached_update_info.download_url.empty())
		return;

	s_download_in_progress.store(true);
	s_download_complete.store(false);
	s_download_failed.store(false);
	{
		std::lock_guard<std::mutex> lock(s_download_error_mutex);
		s_download_error.clear();
	}
	s_download_progress_bytes.store(0);
	s_download_total_bytes.store(s_cached_update_info.asset_size);
	s_download_cancel_requested.store(false);

	const UpdateInfo info = s_cached_update_info;
	std::thread([info]() {
		auto progress_cb = [](const int64_t downloaded, const int64_t total) {
			s_download_progress_bytes.store(downloaded);
			s_download_total_bytes.store(total);
			return s_download_cancel_requested.load();
		};

		const std::string downloaded_file = download_update(info, progress_cb);
		if (downloaded_file.empty())
		{
			{
				std::lock_guard<std::mutex> lock(s_download_error_mutex);
				s_download_error = s_download_cancel_requested.load() ? "Download cancelled." : "Failed to download update package.";
			}
			s_download_failed.store(true);
			s_download_in_progress.store(false);
			return;
		}

		if (!verify_update_checksum(downloaded_file, info.sha256_expected))
		{
			{
				std::lock_guard<std::mutex> lock(s_download_error_mutex);
				s_download_error = "Checksum verification failed.";
			}
			s_download_failed.store(true);
			s_download_in_progress.store(false);
			return;
		}

		if (!apply_update(downloaded_file, info))
		{
			{
				std::lock_guard<std::mutex> lock(s_download_error_mutex);
				s_download_error = "Failed to apply update.";
			}
			s_download_failed.store(true);
			s_download_in_progress.store(false);
			return;
		}

		s_download_complete.store(true);
		s_download_in_progress.store(false);
	}).detach();
}

static void render_download_progress_dialog()
{
	if (!s_download_in_progress && !s_download_complete && !s_download_failed)
		return;

	ImGui::OpenPopup("Download Update");
	if (!ImGui::BeginPopupModal("Download Update", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::Text("Downloading Amiberry %s...", s_cached_update_info.latest_version.c_str());

	const int64_t downloaded = s_download_progress_bytes.load();
	const int64_t total = s_download_total_bytes.load();
	float progress = 0.0f;
	if (total > 0)
		progress = static_cast<float>(downloaded) / static_cast<float>(total);
	if (progress < 0.0f)
		progress = 0.0f;
	if (progress > 1.0f)
		progress = 1.0f;

	ImGui::ProgressBar(progress, ImVec2(420.0f, 0.0f));
	ImGui::Text("%d%% (%s / %s)", static_cast<int>(progress * 100.0f), format_bytes(downloaded).c_str(), format_bytes(total).c_str());

	if (s_download_in_progress)
	{
		if (AmigaButton("Cancel", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
			s_download_cancel_requested.store(true);
	}
	else if (s_download_complete)
	{
		ImGui::TextUnformatted("Update ready! Restart to apply.");
		if (AmigaButton("Restart Now", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
			restart_after_update();
		ImGui::SameLine();
		if (AmigaButton("Close", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			s_download_complete.store(false);
			ImGui::CloseCurrentPopup();
		}
	}
	else if (s_download_failed.load())
	{
		{
			std::lock_guard<std::mutex> lock(s_download_error_mutex);
			ImGui::TextWrapped("%s", s_download_error.c_str());
		}
		if (AmigaButton("OK", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			s_download_failed.store(false);
			{
				std::lock_guard<std::mutex> lock(s_download_error_mutex);
				s_download_error.clear();
			}
			ImGui::CloseCurrentPopup();
		}
	}

	ImGui::EndPopup();
}

void render_panel_update()
{
	ImGui::Indent(4.0f);

	if (s_check_triggered_manually && !is_update_check_running())
	{
		s_cached_update_info = get_async_update_result();
		s_has_cached_result = true;
		s_check_triggered_manually = false;
		s_show_no_update_msg = !s_cached_update_info.update_available && s_cached_update_info.error_message.empty();
	}

	bool check_updates = amiberry_options.update_check != 0;
	if (AmigaCheckbox("Check for updates on startup", &check_updates))
	{
		amiberry_options.update_check = check_updates ? 1 : 0;
		save_amiberry_settings();
	}

	ImGui::AlignTextToFramePadding();
	const char* current_channel = amiberry_options.update_channel == 1 ? "Preview" : "Stable";
	ImGui::TextUnformatted("Update channel:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.3f);
	if (ImGui::BeginCombo("##UpdateChannel", current_channel))
	{
		const bool stable_selected = amiberry_options.update_channel == 0;
		if (ImGui::Selectable("Stable", stable_selected))
		{
			amiberry_options.update_channel = 0;
			save_amiberry_settings();
		}
		if (stable_selected)
			ImGui::SetItemDefaultFocus();

		const bool preview_selected = amiberry_options.update_channel == 1;
		if (ImGui::Selectable("Preview", preview_selected))
		{
			amiberry_options.update_channel = 1;
			save_amiberry_settings();
		}
		if (preview_selected)
			ImGui::SetItemDefaultFocus();

		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

	ImGui::Spacing();
	ImGui::Text("Current version: %s", get_current_semver().c_str());
	ImGui::Spacing();
	ImGui::Text("Update method: %s", get_update_method_label(get_update_method()));
	ImGui::Spacing();

	if (is_update_check_running())
	{
		ImGui::TextUnformatted("Checking...");
	}
	else
	{
		if (AmigaButton("Check Now", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			const auto channel = amiberry_options.update_channel == 1 ? UpdateChannel::Preview : UpdateChannel::Stable;
			start_async_update_check(channel);
			s_check_triggered_manually = true;
			s_has_cached_result = false;
			s_show_no_update_msg = false;
		}
	}

	ImGui::Spacing();

	if (s_has_cached_result)
	{
		if (!s_cached_update_info.error_message.empty())
		{
			ImGui::TextWrapped("Update check failed: %s", s_cached_update_info.error_message.c_str());
			if (!is_update_check_running() && AmigaButton("Try Again", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
			{
				const auto channel = amiberry_options.update_channel == 1 ? UpdateChannel::Preview : UpdateChannel::Stable;
				start_async_update_check(channel);
				s_check_triggered_manually = true;
				s_has_cached_result = false;
				s_show_no_update_msg = false;
			}
		}
		else if (s_cached_update_info.update_available)
		{
			ImGui::Text("Update available: %s", s_cached_update_info.latest_version.c_str());
			if (AmigaButton("View Release Notes", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
				SDL_OpenURL(s_cached_update_info.release_url.c_str());

			if (get_update_method() == UpdateMethod::SELF_UPDATE)
			{
				ImGui::SameLine();
				if (AmigaButton("Download & Update", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
					start_background_download();
			}
			else if (get_update_method() == UpdateMethod::NOTIFY_ONLY)
			{
				ImGui::SameLine();
				if (AmigaButton("Open Download Page", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
					SDL_OpenURL(s_cached_update_info.release_url.c_str());
			}

			if (AmigaButton("Skip This Version", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
				skip_cached_version();
			ImGui::SameLine();
			if (AmigaButton("Remind Me Later", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
			{
				s_has_cached_result = false;
				s_show_no_update_msg = false;
			}
		}
		else
		{
			ImGui::TextUnformatted("You are running the latest version.");
			s_show_no_update_msg = true;
		}
	}
	else if (s_show_no_update_msg)
	{
		ImGui::TextUnformatted("You are running the latest version.");
	}

	render_download_progress_dialog();
}

void update_check_show_notification()
{
	static bool s_notification_shown = false;

	if (!s_update_popup_open)
	{
		if (s_notification_shown || is_update_check_running() || get_update_method() == UpdateMethod::DISABLED)
		{
			render_download_progress_dialog();
			return;
		}

		const auto result = get_async_update_result();
		const bool has_result = result.update_available || !result.error_message.empty() || !result.latest_version.empty();
		if (!has_result)
		{
			render_download_progress_dialog();
			return;
		}

		if (result.update_available && std::strcmp(amiberry_options.skipped_version, result.latest_version.c_str()) != 0)
		{
			s_cached_update_info = result;
			s_has_cached_result = true;
			s_update_popup_open = true;
			ImGui::OpenPopup("Update Available");
		}
		s_notification_shown = true;
	}

	if (s_update_popup_open)
		ImGui::OpenPopup("Update Available");

	if (ImGui::BeginPopupModal("Update Available", &s_update_popup_open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Amiberry %s is available.", s_cached_update_info.latest_version.c_str());

		if (AmigaButton("Update", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			if (get_update_method() == UpdateMethod::SELF_UPDATE)
				start_background_download();
			else if (get_update_method() == UpdateMethod::NOTIFY_ONLY)
				SDL_OpenURL(s_cached_update_info.release_url.c_str());
			s_update_popup_open = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (AmigaButton("Skip", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			skip_cached_version();
			s_update_popup_open = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (AmigaButton("Later", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			s_update_popup_open = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	render_download_progress_dialog();
}
