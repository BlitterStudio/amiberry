// Display filter options: the GUI's Filter panel. How the picture is cropped
// (automatically, or to a manual window and its offsets), the shaders of
// the native and RTG screens, and the bezel around the picture.
//
// The shaders and the custom bezel are picked from what the shaders and
// bezels folders hold (display_catalog.h); a configuration can name one the
// folder no longer has, which reads back as it is.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "amiga_constants.h"
#include "display_catalog.h"

#include "settings_definitions.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

void set_text(TCHAR* field, size_t size, const value& v)
{
	std::snprintf(field, size, "%.*s", static_cast<int>(v.text.size()), v.text.data());
}

// ------------------------------------------------------------------ crop

void get_auto_crop(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_auto_crop);
}

// Centering would fight the crop: target_fixup_options() clears it.
void set_auto_crop(uae_prefs& p, const value& v)
{
	p.gfx_auto_crop = v.number != 0;
	if (p.gfx_auto_crop)
		p.gfx_xcenter = p.gfx_ycenter = 0;
}

// One cropping at a time: automatic or a manual window.
bool auto_crop_available(const uae_prefs& p, const context&)
{
	return !p.gfx_manual_crop;
}

void get_manual_crop(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.gfx_manual_crop);
}

void set_manual_crop(uae_prefs& p, const value& v)
{
	p.gfx_manual_crop = v.number != 0;
}

bool manual_crop_available(const uae_prefs& p, const context&)
{
	return !p.gfx_auto_crop;
}

// The window and its offsets are only used while cropping manually.
bool window_available(const uae_prefs& p, const context&)
{
	return p.gfx_manual_crop && !p.gfx_auto_crop;
}

// The whole frame at the render resolution: the window can take all of it,
// and the offsets move it by up to as much either way.
int frame_width(const uae_prefs& p)
{
	return AMIGA_WIDTH_MAX << p.gfx_resolution;
}

int frame_height(const uae_prefs& p)
{
	return AMIGA_HEIGHT_MAX << p.gfx_vresolution;
}

} // namespace

// The window within the frame at the render resolution, as the old panel
// kept it while it was shown; the resolution options call this after a
// change.
void fit_manual_crop(uae_prefs& p)
{
	const int width = frame_width(p);
	const int height = frame_height(p);
	p.gfx_manual_crop_width = std::clamp(p.gfx_manual_crop_width, 0, width);
	p.gfx_manual_crop_height = std::clamp(p.gfx_manual_crop_height, 0, height);
	p.gfx_horizontal_offset = std::clamp(p.gfx_horizontal_offset, -width, width);
	p.gfx_vertical_offset = std::clamp(p.gfx_vertical_offset, -height, height);
}

namespace {

// The built-in CRT emulations (crtemu) and "none" read as words; shader
// files from the shaders folder are listed by their file names.
const char* shader_label(std::string_view name)
{
	if (name == "none")
		return "None";
	if (name == "tv")
		return "TV (built-in)";
	if (name == "pc")
		return "PC monitor (built-in)";
	if (name == "lite")
		return "Lite (built-in)";
	if (name == "1084")
		return "Commodore 1084 (built-in)";
	return nullptr;
}

void width_range(const uae_prefs& p, double& min, double& max)
{
	min = 0;
	max = frame_width(p);
}

void height_range(const uae_prefs& p, double& min, double& max)
{
	min = 0;
	max = frame_height(p);
}

void horizontal_offset_range(const uae_prefs& p, double& min, double& max)
{
	min = -frame_width(p);
	max = frame_width(p);
}

void vertical_offset_range(const uae_prefs& p, double& min, double& max)
{
	min = -frame_height(p);
	max = frame_height(p);
}

void get_crop_width(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_manual_crop_width);
}

void set_crop_width(uae_prefs& p, const value& v)
{
	p.gfx_manual_crop_width = static_cast<int>(v.number);
}

void get_crop_height(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_manual_crop_height);
}

void set_crop_height(uae_prefs& p, const value& v)
{
	p.gfx_manual_crop_height = static_cast<int>(v.number);
}

void get_horizontal_offset(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_horizontal_offset);
}

void set_horizontal_offset(uae_prefs& p, const value& v)
{
	p.gfx_horizontal_offset = static_cast<int>(v.number);
}

void get_vertical_offset(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_vertical_offset);
}

void set_vertical_offset(uae_prefs& p, const value& v)
{
	p.gfx_vertical_offset = static_cast<int>(v.number);
}

// --------------------------------------------------------------- shaders

// The catalog's names, kept for the choice list (the catalog rescans when
// the folder changes; the list points into this copy until the next call).
std::vector<std::string> shader_values;
std::vector<amiberry_setting_choice> shader_items;

void shader_choices(const uae_prefs&, choice_list& out)
{
	shader_values = get_available_shader_names();
	shader_items.clear();
	for (const std::string& name : shader_values) {
		const char* label = shader_label(name);
		shader_items.push_back({name.c_str(), label ? label : name.c_str()});
	}
	out.items = shader_items.data();
	out.count = shader_items.size();
}

// The parser stores an empty name as "none".
void set_shader_name(TCHAR* field, size_t size, const value& v)
{
	if (v.text.empty())
		std::snprintf(field, size, "%s", "none");
	else
		set_text(field, size, v);
}

void get_shader(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.shader);
}

void set_shader(uae_prefs& p, const value& v)
{
	set_shader_name(p.shader, sizeof p.shader, v);
}

void get_shader_rtg(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.shader_rtg);
}

void set_shader_rtg(uae_prefs& p, const value& v)
{
	set_shader_name(p.shader_rtg, sizeof p.shader_rtg, v);
}

// ---------------------------------------------------------------- bezels

// The built-in CRT bezel or a custom image, never both: each turns the other
// off, as the configuration reader does.
void get_use_bezel(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.use_bezel);
}

void set_use_bezel(uae_prefs& p, const value& v)
{
	p.use_bezel = v.number != 0;
	if (p.use_bezel)
		p.use_custom_bezel = false;
}

bool use_bezel_available(const uae_prefs& p, const context&)
{
	return !p.use_custom_bezel;
}

void get_use_custom_bezel(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.use_custom_bezel);
}

void set_use_custom_bezel(uae_prefs& p, const value& v)
{
	p.use_custom_bezel = v.number != 0;
	if (p.use_custom_bezel)
		p.use_bezel = false;
}

// Only with an image to show.
bool use_custom_bezel_available(const uae_prefs& p, const context&)
{
	return _tcsicmp(p.custom_bezel, "none") != 0;
}

std::vector<std::string> bezel_values;
std::vector<amiberry_setting_choice> bezel_items;

void bezel_choices(const uae_prefs&, choice_list& out)
{
	bezel_values = get_available_bezel_names();
	bezel_items.clear();
	for (const std::string& name : bezel_values)
		bezel_items.push_back({name.c_str(), name == "none" ? "None" : name.c_str()});
	out.items = bezel_items.data();
	out.count = bezel_items.size();
}

void get_custom_bezel(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.custom_bezel);
}

// Picking an image shows it (instead of the built-in bezel); "none" shows
// none, as the panel did.
void set_custom_bezel(uae_prefs& p, const value& v)
{
	if (v.text.empty())
		std::snprintf(p.custom_bezel, sizeof p.custom_bezel, "%s", "none");
	else
		set_text(p.custom_bezel, sizeof p.custom_bezel, v);
	p.use_custom_bezel = _tcsicmp(p.custom_bezel, "none") != 0;
	if (p.use_custom_bezel)
		p.use_bezel = false;
}

bool custom_bezel_available(const uae_prefs& p, const context&)
{
	return !p.use_bezel;
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
// A size or offset in pixels at the render resolution.
#define PIXEL_OPTION(id, group, label, help, apply, min, max, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_INT, apply, min, max, 1, def, nullptr, 0, constraint, false, "px"}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, nullptr, 0, constraint}

// The widest ranges (superhires, interlaced); the current ones follow the
// render resolution.
constexpr double widest_width = AMIGA_WIDTH_MAX << 2;
constexpr double widest_height = AMIGA_HEIGHT_MAX << 1;

const definition definitions[] = {
	{BOOL_OPTION("amiberry.gfx_auto_crop", "filter", "Auto crop",
			"Crop the borders by scanning what the picture shows.",
			AMIBERRY_APPLY_LIVE, "false", "Not together with manual crop."),
		get_auto_crop, set_auto_crop, auto_crop_available, nullptr, nullptr},
	{BOOL_OPTION("amiberry.gfx_manual_crop", "filter", "Manual crop",
			"Show a window of the picture you set (size and position).",
			AMIBERRY_APPLY_LIVE, "false", "Not together with auto crop."),
		get_manual_crop, set_manual_crop, manual_crop_available, nullptr, nullptr},
	{PIXEL_OPTION("amiberry.gfx_manual_crop_width", "filter", "Crop width",
			"The width of the window, in pixels at the render resolution.",
			AMIBERRY_APPLY_LIVE, 0, widest_width, "754", "Only with manual crop; at most the whole frame."),
		get_crop_width, set_crop_width, window_available, nullptr, width_range},
	{PIXEL_OPTION("amiberry.gfx_manual_crop_height", "filter", "Crop height",
			"The height of the window, in pixels at the render resolution.",
			AMIBERRY_APPLY_LIVE, 0, widest_height, "576", "Only with manual crop; at most the whole frame."),
		get_crop_height, set_crop_height, window_available, nullptr, height_range},
	{PIXEL_OPTION("amiberry.gfx_horizontal_offset", "filter", "Horizontal offset",
			"Moves the window left or right.",
			AMIBERRY_APPLY_LIVE, -widest_width, widest_width, "0", "Only with manual crop."),
		get_horizontal_offset, set_horizontal_offset, window_available, nullptr, horizontal_offset_range},
	{PIXEL_OPTION("amiberry.gfx_vertical_offset", "filter", "Vertical offset",
			"Moves the window up or down.",
			AMIBERRY_APPLY_LIVE, -widest_height, widest_height, "0", "Only with manual crop."),
		get_vertical_offset, set_vertical_offset, window_available, nullptr, vertical_offset_range},
	{DYNAMIC_CHOICE_OPTION("amiberry.shader", "filter", "Native screen shader",
			"The shader the native Amiga screen modes are shown through: a built-in CRT shader or a preset from the shaders folder.",
			AMIBERRY_APPLY_LIVE, "none", nullptr),
		get_shader, set_shader, nullptr, nullptr, nullptr, nullptr, shader_choices},
	{DYNAMIC_CHOICE_OPTION("amiberry.shader_rtg", "filter", "RTG screen shader",
			"The shader the RTG screen is shown through.",
			AMIBERRY_APPLY_LIVE, "none", nullptr),
		get_shader_rtg, set_shader_rtg, nullptr, nullptr, nullptr, nullptr, shader_choices},
	{BOOL_OPTION("amiberry.use_bezel", "filter", "Built-in CRT bezel",
			"A television frame around the picture (with the built-in CRT shaders).",
			AMIBERRY_APPLY_LIVE, "false", "Not together with a custom bezel."),
		get_use_bezel, set_use_bezel, use_bezel_available, nullptr, nullptr},
	{BOOL_OPTION("amiberry.use_custom_bezel", "filter", "Show the custom bezel",
			"Show the custom bezel image around the picture.",
			AMIBERRY_APPLY_LIVE, "false", "Only with a custom bezel image picked."),
		get_use_custom_bezel, set_use_custom_bezel, use_custom_bezel_available, nullptr, nullptr},
	{DYNAMIC_CHOICE_OPTION("amiberry.custom_bezel", "filter", "Custom bezel",
			"A bezel image (PNG with transparency) from the bezels folder, shown around the picture with any shader.",
			AMIBERRY_APPLY_LIVE, "none", "Not together with the built-in CRT bezel."),
		get_custom_bezel, set_custom_bezel, custom_bezel_available, nullptr, nullptr, nullptr, bezel_choices},
};

bool commit_filter(const uae_prefs& base, const uae_prefs& from, uae_prefs& to, const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(gfx_auto_crop);
	FIELD(gfx_manual_crop);
	FIELD(gfx_manual_crop_width);
	FIELD(gfx_manual_crop_height);
	FIELD(gfx_horizontal_offset);
	FIELD(gfx_vertical_offset);
	FIELD(shader);
	FIELD(shader_rtg);
	FIELD(use_bezel);
	FIELD(use_custom_bezel);
	FIELD(custom_bezel);
#undef FIELD
	return wrote;
}

// The host's shaders and bezels, scanned again.
void refresh_filter_choices()
{
	invalidate_shader_catalog();
	invalidate_bezel_catalog();
}

#undef BOOL_OPTION
#undef PIXEL_OPTION
#undef DYNAMIC_CHOICE_OPTION
#undef CHOICES

const domain filter = {
	definitions, sizeof definitions / sizeof definitions[0], commit_filter, refresh_filter_choices,
};

} // namespace

const domain& filter_domain()
{
	return filter;
}

} // namespace amiberry_settings
