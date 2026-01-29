#include "sysdeps.h"
#include "imgui.h"
#include "amiberry_gfx.h"
#include "imgui_panels.h"
#include "fsdb_host.h"
#include <SDL_image.h>

// About panel logo texture (loaded on demand)
SDL_Texture* about_logo_texture = nullptr;
static int about_logo_tex_w = 0, about_logo_tex_h = 0;

static void ensure_about_logo_texture()
{
	if (about_logo_texture)
		return;
	AmigaMonitor* mon = &AMonitors[0];
	const auto path = prefix_with_data_path("amiberry-logo.png");
	SDL_Surface* surf = IMG_Load(path.c_str());
	if (!surf) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "About panel: failed to load %s: %s", path.c_str(), SDL_GetError());
		return;
	}
	about_logo_tex_w = surf->w;
	about_logo_tex_h = surf->h;
	about_logo_texture = SDL_CreateTextureFromSurface(mon->gui_renderer, surf);
	SDL_FreeSurface(surf);
	if (!about_logo_texture) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "About panel: failed to create texture: %s", SDL_GetError());
	}
}

void render_panel_about()
{
	// Ensure logo texture is available
	ensure_about_logo_texture();

	// Draw centered logo banner, scaled to content width
	if (about_logo_texture)
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
		// center horizontally
		const float pos_x = (ImGui::GetContentRegionAvail().x - draw_w) * 0.5f;
		if (pos_x > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pos_x);
		ImGui::Image(about_logo_texture, ImVec2(draw_w, draw_h));
	}

	ImGui::Spacing();

	ImGui::Indent(10.0f);
	// Version and environment info
	ImGui::TextUnformatted(get_version_string().c_str());
	ImGui::TextUnformatted(get_copyright_notice().c_str());
	ImGui::TextUnformatted(get_sdl2_version_string().c_str());
	ImGui::Text("SDL2 video driver: %s", sdl_video_driver ? sdl_video_driver : "unknown");

	ImGui::Spacing();

	// License and credits text inside a bordered, scrollable region
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
		"TomB - Original ARM port of UAE, JIT ARM updates\n"
		"Rob Smith, Drawbridge floppy controller\n"
		"Gareth Fare - Original Serial port implementation\n"
		"Dom Cresswell (Horace & The Spider) - Controller and WHDBooter updates\n"
		"Christer Solskogen - Makefile and testing\n"
		"Gunnar Kristjansson - Amibian and inspiration\n"
		"Thomas Navarro Garcia - Original Amiberry logo\n"
		"Chips - Original RPI port\n"
		"Vasiliki Soufi - Amiberry name\n\n"
		"Dedicated to HeZoR - R.I.P. little brother (1978-2017)\n";

	// Use a child region with border to mimic a textbox look
	ImGui::BeginChild("AboutScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Indent(10.0f);
	ImGui::Dummy(ImVec2(0, 10.0f));
	ImGui::TextUnformatted(about_long_text);
	ImGui::Dummy(ImVec2(0, 10.0f));
	ImGui::EndChild();
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
}
