// On-screen control options: the touch joystick and the on-screen
// keyboard overlay, including its toggle key.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

// Guide is intercepted as the menu trigger on Android and the libretro
// joypad action space has no guide button, so both front ends default
// the toggle to the left stick.
#if defined(__ANDROID__) || defined(LIBRETRO)
constexpr const char* vkbd_toggle_default = "leftstick";
#else
constexpr const char* vkbd_toggle_default = "guide";
#endif

bool vkbd_enabled(const uae_prefs& p)
{
	return p.vkbd_enabled;
}

void get_onscreen_joystick(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.onscreen_joystick);
}

void set_onscreen_joystick(uae_prefs& p, const value& v)
{
	p.onscreen_joystick = v.number != 0;
}

void get_vkbd_enabled(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.vkbd_enabled);
}

void set_vkbd_enabled(uae_prefs& p, const value& v)
{
	p.vkbd_enabled = v.number != 0;
}

#define VKBD_BOOL(field, id) \
	void get_##id(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_bool(out, size, p.field); \
	} \
	void set_##id(uae_prefs& p, const value& v) \
	{ \
		p.field = v.number != 0; \
	}

VKBD_BOOL(vkbd_numpad, vkbd_numpad)
VKBD_BOOL(vkbd_touch_button, vkbd_touch_button)
VKBD_BOOL(use_retroarch_vkbd, use_retroarch_vkbd)

bool vkbd_option_available(const uae_prefs& p, const context&)
{
	return vkbd_enabled(p);
}

void get_vkbd_transparency(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.vkbd_transparency);
}

void set_vkbd_transparency(uae_prefs& p, const value& v)
{
	p.vkbd_transparency = static_cast<int>(v.number);
}

const amiberry_setting_choice language_choices[] = {
	{"US", "US"},
	{"UK", "UK"},
	{"DE", "German"},
	{"FR", "French"},
};

void get_vkbd_language(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.vkbd_language);
}

void set_vkbd_language(uae_prefs& p, const value& v)
{
	std::snprintf(p.vkbd_language, sizeof p.vkbd_language, "%.*s",
		static_cast<int>(sizeof p.vkbd_language - 1), v.text.data());
}

void get_vkbd_toggle(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.vkbd_toggle);
}

// The value names a key (an SDL scancode name, or the ImGui name the GUI's
// capture popup records) or an SDL gamepad button; empty unbinds the toggle.
void set_vkbd_toggle(uae_prefs& p, const value& v)
{
	std::snprintf(p.vkbd_toggle, sizeof p.vkbd_toggle, "%.*s",
		static_cast<int>(v.text.size()), v.text.data());
}

// A binding that does not fit the field is refused rather than truncated
// into a different one.
bool vkbd_toggle_fits(const uae_prefs& p, const value& v)
{
	return v.text.size() < sizeof p.vkbd_toggle;
}

bool vkbd_toggle_available(const uae_prefs& p, const context&)
{
	return vkbd_enabled(p) && !p.use_retroarch_vkbd;
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
// A number shown with its unit (amiberry_settings.h).
#define MEASURED_OPTION(id, group, label, help, apply, min, max, step, def, constraint, unit) \
	{id, group, label, help, AMIBERRY_SETTING_INT, apply, min, max, step, def, nullptr, 0, constraint, \
		false, unit}
#define HOTKEY_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_HOTKEY, apply, 0, 0, 0, def, nullptr, 0, constraint}

const definition definitions[] = {
	{BOOL_OPTION("amiberry.onscreen_joystick", "onscreen", "On-screen joystick",
			"Show the virtual D-pad and fire buttons for touchscreen control.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_onscreen_joystick, set_onscreen_joystick, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.vkbd_enabled", "onscreen", "On-screen keyboard",
			"Show the Amiga-style on-screen keyboard overlay during emulation.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_vkbd_enabled, set_vkbd_enabled, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.vkbd_numpad", "onscreen", "Include numeric keypad",
			"Include the Amiga numeric keypad; keys will be smaller to fit the full keyboard.",
			AMIBERRY_APPLY_LIVE, "false", "Needs the on-screen keyboard enabled."),
		get_vkbd_numpad, set_vkbd_numpad, vkbd_option_available, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.vkbd_touch_button", "onscreen", "Keyboard button on the on-screen joystick",
			"Show the KB touch button next to the fire buttons.",
			AMIBERRY_APPLY_LIVE, "true", "Needs the on-screen keyboard enabled."),
		get_vkbd_touch_button, set_vkbd_touch_button, vkbd_option_available, nullptr, nullptr,
		nullptr},
	// 0 is not the transparent end of a scale but "the default", so this
	// stays a number field rather than a level.
	{MEASURED_OPTION("amiberry.vkbd_transparency", "onscreen", "On-screen keyboard opacity",
			"Opacity of the on-screen keyboard (100% is opaque); 0 uses the default of 85%.",
			AMIBERRY_APPLY_LIVE, 0, 100, 1, "0", "Needs the on-screen keyboard enabled.", "%"),
		get_vkbd_transparency, set_vkbd_transparency, vkbd_option_available, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("amiberry.vkbd_language", "onscreen", "Layout",
			"Key layout of the on-screen keyboard.",
			AMIBERRY_APPLY_LIVE, "US", language_choices,
			"Needs the on-screen keyboard enabled."),
		get_vkbd_language, set_vkbd_language, vkbd_option_available, nullptr, nullptr, nullptr},
	{HOTKEY_OPTION("amiberry.vkbd_toggle", "onscreen", "Toggle button",
			"Key or controller button that shows and hides the on-screen keyboard; empty "
			"unbinds it.",
			AMIBERRY_APPLY_LIVE, vkbd_toggle_default,
			"Needs the on-screen keyboard enabled and the RetroArch on-screen keyboard button off."),
		get_vkbd_toggle, set_vkbd_toggle, vkbd_toggle_available, nullptr, nullptr, vkbd_toggle_fits},
	{BOOL_OPTION("amiberry.use_retroarch_vkbd", "onscreen", "Use the RetroArch on-screen keyboard button",
			"Use the front end's RetroArch keyboard button instead of a custom toggle.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_use_retroarch_vkbd, set_use_retroarch_vkbd, nullptr, nullptr, nullptr, nullptr},
};

bool commit_onscreen(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(onscreen_joystick);
	FIELD(vkbd_enabled);
	FIELD(vkbd_numpad);
	FIELD(vkbd_touch_button);
	FIELD(vkbd_transparency);
	FIELD(vkbd_language);
	FIELD(vkbd_toggle);
	FIELD(use_retroarch_vkbd);
#undef FIELD
	return wrote;
}

const domain onscreen = {
	definitions, sizeof definitions / sizeof definitions[0], commit_onscreen, nullptr,
};

} // namespace

const domain& onscreen_domain()
{
	return onscreen;
}

} // namespace amiberry_settings
