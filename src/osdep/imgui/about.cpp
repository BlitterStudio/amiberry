#include "sysdeps.h"
#include "options.h"
#include "imgui.h"
#include "amiberry_gfx.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "fsdb_host.h"
#include "amiberry_update.h"
#include "target.h"
#include <SDL3_image/SDL_image.h>
#include "gui/amiberry_logo_png_data.h"

#include <string>
#include <thread>
#include <atomic>
#include <mutex>

ImTextureID about_logo_texture = ImTextureID_Invalid;
static int about_logo_tex_w = 0, about_logo_tex_h = 0;

static std::atomic<bool> s_download_in_progress{false};
static std::atomic<bool> s_download_complete{false};
static std::atomic<bool> s_download_failed{false};
static std::mutex s_download_error_mutex;
static std::string s_download_error;
static std::atomic<int64_t> s_download_progress_bytes{0};
static std::atomic<int64_t> s_download_total_bytes{0};
static UpdateInfo s_cached_update_info;
static bool s_check_triggered = false;
static bool s_has_cached_result = false;
static bool s_show_no_update_msg = false;
static std::atomic<bool> s_download_cancel_requested{false};

static void ensure_about_logo_texture()
{
	if (about_logo_texture != ImTextureID_Invalid)
		return;
	SDL_IOStream* io = SDL_IOFromConstMem(amiberry_logo_png_data, amiberry_logo_png_size);
	SDL_Surface* surf = io ? IMG_Load_IO(io, true) : nullptr;
	if (!surf) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "About panel: failed to load embedded logo: %s", SDL_GetError());
		return;
	}
	about_logo_texture = gui_create_texture(surf, &about_logo_tex_w, &about_logo_tex_h);
	SDL_DestroySurface(surf);
	if (about_logo_texture == ImTextureID_Invalid) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "About panel: failed to create texture: %s", SDL_GetError());
	}
}

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

static void render_update_section()
{
	if (s_check_triggered && !is_update_check_running())
	{
		s_cached_update_info = get_async_update_result();
		s_has_cached_result = true;
		s_check_triggered = false;
		s_show_no_update_msg = !s_cached_update_info.update_available && s_cached_update_info.error_message.empty();
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

	if (is_update_check_running())
	{
		ImGui::TextUnformatted("Checking for updates...");
	}
	else
	{
		if (AmigaButton("Check for Updates", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
		{
			const auto channel = amiberry_options.update_channel == 1 ? UpdateChannel::Preview : UpdateChannel::Stable;
			start_async_update_check(channel);
			s_check_triggered = true;
			s_has_cached_result = false;
			s_show_no_update_msg = false;
		}
	}

	if (s_has_cached_result)
	{
		ImGui::Spacing();
		if (!s_cached_update_info.error_message.empty())
		{
			ImGui::TextWrapped("Update check failed: %s", s_cached_update_info.error_message.c_str());
		}
		else if (s_cached_update_info.update_available)
		{
			ImGui::Text("Update available: %s", s_cached_update_info.latest_version.c_str());
			if (AmigaButton("View Release Notes", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
				SDL_OpenURL(s_cached_update_info.release_url.c_str());

			if (!s_download_in_progress && !s_download_complete && !s_download_failed)
			{
				if (get_update_method() == UpdateMethod::SELF_UPDATE)
				{
					ImGui::SameLine();
					if (AmigaButton(ICON_FA_DOWNLOAD " Download & Update", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
						start_background_download();
				}
				else if (get_update_method() == UpdateMethod::NOTIFY_ONLY)
				{
					ImGui::SameLine();
					if (AmigaButton("Open Download Page", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
						SDL_OpenURL(s_cached_update_info.release_url.c_str());
				}
			}
		}
		else
		{
			ImGui::TextUnformatted("You are running the latest version.");
		}
	}
	else if (s_show_no_update_msg)
	{
		ImGui::Spacing();
		ImGui::TextUnformatted("You are running the latest version.");
	}

	if (s_download_in_progress || s_download_complete || s_download_failed)
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (s_download_in_progress)
		{
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
			ImGui::Text("%d%% (%s / %s)", static_cast<int>(progress * 100.0f),
				format_bytes(downloaded).c_str(), format_bytes(total).c_str());

			if (AmigaButton(ICON_FA_XMARK " Cancel", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
				s_download_cancel_requested.store(true);
		}
		else if (s_download_complete)
		{
			ImGui::TextUnformatted("Update applied successfully! Restart to use the new version.");
			if (AmigaButton("Restart Now", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
				restart_after_update();
			ImGui::SameLine();
			if (AmigaButton(ICON_FA_XMARK " Close", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
				s_download_complete.store(false);
		}
		else if (s_download_failed.load())
		{
			{
				std::lock_guard<std::mutex> lock(s_download_error_mutex);
				ImGui::TextWrapped("%s", s_download_error.c_str());
			}
			if (AmigaButton(ICON_FA_CHECK " OK", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
			{
				s_download_failed.store(false);
				{
					std::lock_guard<std::mutex> lock(s_download_error_mutex);
					s_download_error.clear();
				}
			}
		}
	}
}

void render_panel_about()
{
	ensure_about_logo_texture();

	if (about_logo_texture != ImTextureID_Invalid)
	{
		const float region_w = ImGui::GetContentRegionAvail().x;
		auto draw_w = static_cast<float>(about_logo_tex_w);
		auto draw_h = static_cast<float>(about_logo_tex_h);
		if (draw_w > region_w * 0.9f)
		{
			const float scale = (region_w * 0.9f) / draw_w;
			draw_w *= scale;
			draw_h *= scale;
		}
		const float pos_x = (ImGui::GetContentRegionAvail().x - draw_w) * 0.5f;
		if (pos_x > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pos_x);
		ImGui::Image(about_logo_texture, ImVec2(draw_w, draw_h));
	}

	ImGui::Spacing();

	ImGui::Indent(10.0f);

	ImGui::TextUnformatted(get_version_string().c_str());
	ImGui::TextUnformatted(get_copyright_notice().c_str());
	ImGui::TextUnformatted(get_sdl_version_string().c_str());
	ImGui::Text("SDL video driver: %s", sdl_video_driver ? sdl_video_driver : "unknown");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	render_update_section();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	static auto about_long_text =
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"any later version.\n\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"GNU General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program. If not, see https://www.gnu.org/licenses\n\n"
		"Credits:\n"
		"Dimitris Panokostas (MiDWaN) - Amiberry author\n"
		"Toni Wilen - WinUAE author\n"
		"Romain Tisserand - Libretro implementation\n"
		"Darryl Lyle - FreeBSD and Haiku ports\n"
		"TomB - Original ARM port of UAE, JIT ARM updates\n"
		"Rob Smith - Drawbridge floppy controller\n"
		"Gareth Fare - Original Serial port implementation\n"
		"Dom Cresswell (Horace & The Spider) - Controller and WHDBooter updates\n"
		"Christer Solskogen - Makefile and testing\n"
		"Gunnar Kristjansson - Amibian and inspiration\n"
		"Thomas Navarro Garcia - Original Amiberry logo\n"
		"Chips - Original RPI port\n"
		"Vasiliki Soufi - Amiberry name\n\n"
		"Dedicated to HeZoR - R.I.P. little brother (1978-2017)\n";

	ImGui::BeginChild("AboutScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Indent(10.0f);
	ImGui::Spacing();
	ImGui::TextUnformatted(about_long_text);
	ImGui::Spacing();
	ImGui::EndChild();
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
}
