// Keyboard shortcuts (AMIBERRY_SETTING_HOTKEY): the values a front end
// records from a key press or offers as controller buttons, spelled as
// get_hotkey_from_config() (amiberry.cpp) reads them. The key name is SDL's
// name for the scancode, which that parser hands to
// SDL_GetScancodeFromName(); a button is SDL's gamepad button name, spelled
// as SDL spells it ("a"; the letter key is "A"), which it hands to
// gamepad_button_from_config().

#include "settings_definitions.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {

// The modifier tokens in the order the GUI's capture writes them, left
// before right.
struct modifier_pair {
	unsigned int left;
	unsigned int right;
	const char* left_name;
	const char* right_name;
};

constexpr modifier_pair modifier_pairs[] = {
	{AMIBERRY_HOTKEY_LCTRL, AMIBERRY_HOTKEY_RCTRL, "LCtrl", "RCtrl"},
	{AMIBERRY_HOTKEY_LSHIFT, AMIBERRY_HOTKEY_RSHIFT, "LShift", "RShift"},
	{AMIBERRY_HOTKEY_LALT, AMIBERRY_HOTKEY_RALT, "LAlt", "RAlt"},
	{AMIBERRY_HOTKEY_LGUI, AMIBERRY_HOTKEY_RGUI, "LGUI", "RGUI"},
};

constexpr unsigned int all_modifiers = 0xffu;

// The token of a modifier key pressed alone: exactly one bit.
const char* single_modifier(unsigned int modifiers)
{
	for (const modifier_pair& m : modifier_pairs) {
		if (modifiers == m.left)
			return m.left_name;
		if (modifiers == m.right)
			return m.right_name;
	}
	return nullptr;
}

bool format_hotkey(unsigned int usage, unsigned int modifiers, std::string& out)
{
	if (modifiers & ~all_modifiers)
		return false;
	if (usage == 0) {
		const char* name = single_modifier(modifiers);
		if (!name)
			return false;
		out = name;
		return true;
	}
	// Keyboard usages run from A (4) to the modifiers (Left Ctrl, 0xe0);
	// SDL's scancodes past the modifiers are its own, not HID usages.
	if (usage < SDL_SCANCODE_A || usage >= SDL_SCANCODE_LCTRL)
		return false;
	const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(usage));
	// The parser splits the value at '+', so "Keypad +" cannot be spelled.
	if (!name || !*name || std::strchr(name, '+'))
		return false;
	out.clear();
	for (const modifier_pair& m : modifier_pairs) {
		if (modifiers & m.left)
			out.append(m.left_name).push_back('+');
		else if (modifiers & m.right)
			out.append(m.right_name).push_back('+');
	}
	out += name;
	return true;
}

// The gamepad buttons, named and labelled as the custom controls name
// them (the buttons come first in that list, then the axes).
const std::vector<amiberry_setting_choice>& button_choices()
{
	static const std::vector<amiberry_setting_choice> list = [] {
		std::vector<amiberry_setting_choice> out;
		for (size_t i = 0; i < amiberry_settings::custom_control_count(); i++) {
			const amiberry_settings_custom_control* c = amiberry_settings::custom_control_at(i);
			if (c && !c->axis)
				out.push_back({c->id, c->label});
		}
		return out;
	}();
	return list;
}

// The Amiga key options map a host key onto an Amiga key; the emulator
// only compares their scancode, never a controller button.
bool key_only(std::string_view id)
{
	return id == "amiberry.left_amiga" || id == "amiberry.right_amiga";
}

} // namespace

extern "C" {

bool amiberry_settings_hotkey_format(unsigned int usage, unsigned int modifiers, char* buffer, size_t size)
{
	return amiberry_settings::no_throw(false, [=] {
		std::string text;
		if (!buffer || !format_hotkey(usage, modifiers, text) || text.size() >= size)
			return false;
		std::memcpy(buffer, text.c_str(), text.size() + 1);
		return true;
	});
}

size_t amiberry_settings_hotkey_buttons(const amiberry_setting_choice** choices)
{
	return amiberry_settings::no_throw(size_t{0}, [=] {
		const std::vector<amiberry_setting_choice>& list = button_choices();
		if (choices)
			*choices = list.data();
		return list.size();
	});
}

bool amiberry_settings_hotkey_takes_buttons(const char* id)
{
	return amiberry_settings::no_throw(false, [=] {
		if (!id || key_only(id))
			return false;
		const amiberry_setting_descriptor* d = amiberry_settings_find(id);
		if (!d)
			d = amiberry_host_settings_find(id);
		return d && d->type == AMIBERRY_SETTING_HOTKEY;
	});
}

} // extern "C"
