// Custom controls: the Amiga input event each gamepad button and axis of the
// joystick on a port triggers, alone or with the hotkey button held. The
// GUI's Custom Controls panel. A configuration keeps them per port
// (joyportN_amiberry_custom_<none|hotkey>_<control>), in the joystick
// state the input code holds rather than in uae_prefs, so a session stages
// its edits per port (only the controls it set, so a commit keeps what
// changed the port's other controls meanwhile) and applies them when it
// commits.
//
// A control's event is a configuration name from the remapping list
// (inputdevice event names); "none" is no event (the control does
// nothing, even where the port's mode would give it one), and "" leaves
// the control unset: it gets what the port's mode gives it (the input code
// maps the joystick's directions and fire buttons, the CD32 pad's, and a
// few keys by mode), so the layer reports that as "" rather than naming
// an event.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include "settings_definitions.h"

#include <array>
#include <cstdio>
#include <map>
#include <cstring>
#include <string>
#include <vector>

struct amiberry_settings_custom_controls_stage {
	// The controls a session set on a port, by slot (see slot_key), for the
	// device the port had (its name and configuration name, as the input
	// code keys the mappings).
	struct port {
		std::string name;
		std::string configname;
		std::map<int, int> slots;
	};
	std::array<port, MAX_JPORTS> ports{};
};

namespace amiberry_settings {

namespace {

struct control {
	std::string id;    // as the configuration names it ("a", "leftx")
	const char* label;
	bool axis;
	int index;         // SDL_GamepadButton or SDL_GamepadAxis
};

const char* const button_labels[] = {
	"South", "East", "West", "North", "Select", "Guide", "Start", "Left stick", "Right stick",
	"Left shoulder", "Right shoulder", "D-pad up", "D-pad down", "D-pad left", "D-pad right",
	"Misc 1", "Paddle 1", "Paddle 2", "Paddle 3", "Paddle 4", "Touchpad",
	"Misc 2", "Misc 3", "Misc 4", "Misc 5", "Misc 6",
};
const char* const axis_labels[] = {
	"Left stick X", "Left stick Y", "Right stick X", "Right stick Y", "Left trigger", "Right trigger",
};
static_assert(sizeof button_labels / sizeof button_labels[0] == SDL_GAMEPAD_BUTTON_COUNT, "a label per button");
static_assert(sizeof axis_labels / sizeof axis_labels[0] == SDL_GAMEPAD_AXIS_COUNT, "a label per axis");

const std::vector<control>& controls()
{
	static const std::vector<control> list = [] {
		std::vector<control> out;
		for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++) {
			const char* name = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(i));
			out.push_back({name ? name : std::to_string(i), button_labels[i], false, i});
		}
		for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++) {
			const char* name = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(i));
			out.push_back({name ? name : std::to_string(i), axis_labels[i], true, i});
		}
		return out;
	}();
	return list;
}

std::vector<amiberry_settings_custom_control>& control_items()
{
	static std::vector<amiberry_settings_custom_control> items = [] {
		std::vector<amiberry_settings_custom_control> out;
		for (const control& c : controls())
			out.push_back({c.id.c_str(), c.label, c.axis});
		return out;
	}();
	return items;
}

const control* find_control(std::string_view id)
{
	for (const control& c : controls()) {
		if (id == c.id)
			return &c;
	}
	return nullptr;
}

std::vector<amiberry_setting_choice>& event_items()
{
	static std::vector<amiberry_setting_choice> items = [] {
		std::vector<amiberry_setting_choice> out;
		for (int i = 0; i < remap_event_list_size; i++) {
			const inputevent* ie = inputdevice_get_eventinfo(remap_event_list[i]);
			if (ie && ie->name)
				out.push_back({ie->name, ie->name});
		}
		return out;
	}();
	return items;
}

// The event a control stores for a value (0 unset, -1 none); false for an
// unknown event.
bool event_named(std::string_view name, int& event)
{
	event = name.empty() ? 0 : -1;
	if (name.empty() || name == "none")
		return true;
	for (int i = 0; i < remap_event_list_size; i++) {
		const inputevent* ie = inputdevice_get_eventinfo(remap_event_list[i]);
		if (ie && ie->name && name == ie->name) {
			event = remap_event_list[i];
			return true;
		}
	}
	return false;
}

// The joystick a port has in this configuration; -1 without one, or when
// it names a joystick that is not connected.
int port_joystick(const uae_prefs& p, int port)
{
	if (port < 0 || port >= MAX_JPORTS)
		return -1;
	const int index = p.jports[port].id - JSEM_JOYS;
	return index >= 0 && index < MAX_INPUT_DEVICES && index < inputdevice_get_device_total(IDTYPE_JOYSTICK)
		? index : -1;
}

int& slot(controller_mapping& m, const control& c, bool hotkey)
{
	if (c.axis)
		return hotkey ? m.amiberry_custom_axis_hotkey[c.index] : m.amiberry_custom_axis_none[c.index];
	return hotkey ? m.amiberry_custom_hotkey[c.index] : m.amiberry_custom_none[c.index];
}

// A slot of a port's mapping: a control, alone or with the hotkey.
int slot_key(const control& c, bool hotkey)
{
	return static_cast<int>((&c - controls().data()) * 2 + (hotkey ? 1 : 0));
}

// Whether the session's edits for a port are for the device it has in p.
bool staged_for(const amiberry_settings_custom_controls_stage::port& staged, const jport& jp)
{
	return !staged.slots.empty() && staged.name == jp.idc.name && staged.configname == jp.idc.configname;
}

// The mapping a port has now: the one the input code holds for the port's
// device, with the session's edits for that device on top.
controller_mapping port_mapping(const amiberry_settings_custom_controls_stage* stage, const uae_prefs& p, int port)
{
	const jport& jp = p.jports[port];
	controller_mapping mapping{};
	if (!amiberry_get_port_joystick_custom_mapping(port, jp.idc.name, jp.idc.configname, mapping)) {
		const int joy = port_joystick(p, port);
		if (joy >= 0)
			mapping = di_joystick[joy].mapping;
	}
	if (stage && staged_for(stage->ports[port], jp)) {
		for (const auto& [key, event] : stage->ports[port].slots)
			slot(mapping, controls()[key / 2], key % 2 != 0) = event;
	}
	return mapping;
}

// Whether the device has the control (a D-pad on a hat counts).
bool device_has(const didata& did, const control& c)
{
	if (c.axis)
		return did.mapping.axis[c.index] > -1;
	if (c.index >= SDL_GAMEPAD_BUTTON_DPAD_UP && c.index <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT
		&& did.mapping.number_of_hats > 0)
		return true;
	return did.mapping.button[c.index] > -1;
}

} // namespace

const amiberry_settings_custom_control* custom_control_at(size_t index)
{
	std::vector<amiberry_settings_custom_control>& items = control_items();
	return index < items.size() ? &items[index] : nullptr;
}

size_t custom_control_count()
{
	return control_items().size();
}

size_t input_event_count()
{
	return event_items().size();
}

const amiberry_setting_choice* input_event_at(size_t index)
{
	std::vector<amiberry_setting_choice>& items = event_items();
	return index < items.size() ? &items[index] : nullptr;
}

amiberry_settings_custom_controls_stage* custom_controls_stage_new()
{
	return new amiberry_settings_custom_controls_stage;
}

void custom_controls_stage_free(amiberry_settings_custom_controls_stage* stage)
{
	delete stage;
}

void custom_controls_stage_clear(amiberry_settings_custom_controls_stage* stage)
{
	if (stage) {
		for (auto& port : stage->ports)
			port = {};
	}
}

// What a hotkey function holds the button for, as the panel showed it:
// the hotkey button itself, the quit, menu and reset buttons (with the
// hotkey, when those functions are on), the on-screen keyboard button.
const char* custom_control_use(const uae_prefs& p, int port, bool hotkey, std::string_view id)
{
	const int joy = port_joystick(p, port);
	const control* c = find_control(id);
	if (joy < 0 || !c || c->axis)
		return nullptr;
	const didata& did = di_joystick[joy];
	if (!device_has(did, *c))
		return nullptr;
	// A D-pad direction on a hat is no button a function can hold (an
	// unset function's button is no button either).
	const int button = did.mapping.button[c->index];
	if (button < 0)
		return nullptr;
	if (button == did.mapping.hotkey_button)
		return "hotkey";
	if (hotkey && button == did.mapping.quit_button && p.use_retroarch_quit)
		return "quit";
	if (hotkey && button == did.mapping.menu_button && p.use_retroarch_menu)
		return "menu";
	if (hotkey && button == did.mapping.reset_button && p.use_retroarch_reset)
		return "reset";
	if (button == did.mapping.vkbd_button
		&& ((hotkey && p.use_retroarch_vkbd) || (!hotkey && !did.mapping.is_retroarch)))
		return "vkbd";
	return nullptr;
}

bool custom_control_available(const uae_prefs& p, int port, bool hotkey, std::string_view id)
{
	const int joy = port_joystick(p, port);
	const control* c = find_control(id);
	return joy >= 0 && c && device_has(di_joystick[joy], *c) && !custom_control_use(p, port, hotkey, id);
}

amiberry_settings_status custom_control_get(const amiberry_settings_custom_controls_stage* stage,
	const uae_prefs& p, int port, bool hotkey, std::string_view id, char* out, size_t size)
{
	const control* c = find_control(id);
	if (!c || port < 0 || port >= MAX_JPORTS)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	const int joy = port_joystick(p, port);
	if (joy < 0)
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	controller_mapping mapping = port_mapping(stage, p, port);
	// As the configuration spells it, so a value read sets back.
	const char* name = custom_event_name(slot(mapping, *c, hotkey));
	if (std::strlen(name) >= size)
		return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
	std::snprintf(out, size, "%s", name);
	return AMIBERRY_SETTINGS_OK;
}

amiberry_settings_status custom_control_set(amiberry_settings_custom_controls_stage* stage,
	const uae_prefs& p, int port, bool hotkey, std::string_view id, std::string_view event_name)
{
	const control* c = find_control(id);
	if (!c || port < 0 || port >= MAX_JPORTS)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	int event = 0;
	if (!event_named(event_name, event))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (!custom_control_available(p, port, hotkey, id))
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	auto& staged = stage->ports[port];
	const jport& jp = p.jports[port];
	if (!staged_for(staged, jp))
		staged = {jp.idc.name, jp.idc.configname, {}};
	staged.slots[slot_key(*c, hotkey)] = event;
	return AMIBERRY_SETTINGS_OK;
}

// Applies the staged ports to the input code, for the devices the ports
// have in p (a port that has another device now drops its edits), as the
// panel applied each change. Returns whether anything changed.
bool custom_controls_commit(amiberry_settings_custom_controls_stage* stage, uae_prefs& p)
{
	bool changed = false;
	for (int port = 0; stage && port < MAX_JPORTS; port++) {
		const jport& jp = p.jports[port];
		if (!staged_for(stage->ports[port], jp))
			continue;
		const controller_mapping current = port_mapping(nullptr, p, port);
		const controller_mapping staged = port_mapping(stage, p, port);
		if (current.amiberry_custom_none != staged.amiberry_custom_none
			|| current.amiberry_custom_hotkey != staged.amiberry_custom_hotkey
			|| current.amiberry_custom_axis_none != staged.amiberry_custom_axis_none
			|| current.amiberry_custom_axis_hotkey != staged.amiberry_custom_axis_hotkey) {
			amiberry_set_port_joystick_custom_mapping(port, jp.idc.name, jp.idc.configname, staged);
			changed = true;
		}
	}
	if (changed)
		inputdevice_updateconfig(nullptr, &p);
	custom_controls_stage_clear(stage);
	return changed;
}

} // namespace amiberry_settings
