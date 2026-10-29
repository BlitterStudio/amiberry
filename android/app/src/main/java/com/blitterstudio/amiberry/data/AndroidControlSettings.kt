package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings

object AndroidControlSettings {

	/**
	 * The controls a new installation starts with. Touch devices get the
	 * on-screen joystick in port 2 and the keyboard's touch button; a TV
	 * (leanback: its emulator and some boxes report a touchscreen they do
	 * not have) draws no touch overlays over the picture, and port 2 is the
	 * controller a touchless device gets (the remote drives it at run time
	 * while no gamepad is connected, see overlay.RemoteJoystick).
	 */
	data class Defaults(
		val joyport0: String,
		val joyport1: String,
		val onScreenJoystick: Boolean,
		val onScreenKeyboardButton: Boolean
	)

	fun defaults(isTv: Boolean): Defaults =
		if (isTv) {
			Defaults(joyport0 = "mouse", joyport1 = "joy1", onScreenJoystick = false, onScreenKeyboardButton = false)
		} else {
			Defaults(joyport0 = "mouse", joyport1 = "onscreen_joy", onScreenJoystick = true, onScreenKeyboardButton = true)
		}

	/** Android control settings that a configuration sets, or lacks, as one unit. */
	enum class Group {
		JOYPORT0,
		JOYPORT1,
		ON_SCREEN_JOYSTICK,
		KEYBOARD_BUTTON,
		KEYBOARD_NUMPAD,
		KEYBOARD_TOGGLE,
		JOYPORT0_MOUSE_MAP,
		JOYPORT1_MOUSE_MAP,
		ANALOG_MOUSE_SPEED
	}

	/**
	 * The control groups [explicitKeys] does not set, i.e. the ones remembered
	 * controls fill. [settings] is the parse of the same configuration.
	 */
	fun missingGroups(settings: EmulatorSettings, explicitKeys: Set<String>): Set<Group> {
		val keys = explicitKeys.map { it.lowercase() }.toSet()
		val keyboardSwitchExplicit = "amiberry.vkbd_enabled" in keys || "input.default_osk" in keys
		// An explicitly disabled keyboard also fixes the toggle (parsed as disabled).
		val keyboardExplicitlyOff = keyboardSwitchExplicit && !settings.onScreenKeyboardAvailable
		val explicit = buildSet {
			if ("joyport0" in keys) add(Group.JOYPORT0)
			if ("joyport1" in keys || "amiberry.android_joyport1" in keys) add(Group.JOYPORT1)
			if ("amiberry.onscreen_joystick" in keys) add(Group.ON_SCREEN_JOYSTICK)
			if (keyboardSwitchExplicit || "amiberry.vkbd_touch_button" in keys) add(Group.KEYBOARD_BUTTON)
			if ("amiberry.vkbd_numpad" in keys) add(Group.KEYBOARD_NUMPAD)
			if ("amiberry.vkbd_toggle" in keys || keyboardExplicitlyOff) add(Group.KEYBOARD_TOGGLE)
			if ("joyport0mousemap" in keys) add(Group.JOYPORT0_MOUSE_MAP)
			if ("joyport1mousemap" in keys) add(Group.JOYPORT1_MOUSE_MAP)
			if ("input.joymouse_speed_analog" in keys) add(Group.ANALOG_MOUSE_SPEED)
		}
		return Group.entries.toSet() - explicit
	}

	fun withFallback(
		settings: EmulatorSettings,
		explicitKeys: Set<String>,
		fallback: EmulatorSettings
	): EmulatorSettings {
		val missing = missingGroups(settings, explicitKeys)
		fun <T> pick(group: Group, own: T, remembered: T): T = if (group in missing) remembered else own
		return settings.copy(
			joyport0 = pick(Group.JOYPORT0, settings.joyport0, fallback.joyport0),
			joyport1 = pick(Group.JOYPORT1, settings.joyport1, fallback.joyport1),
			onScreenJoystick = pick(Group.ON_SCREEN_JOYSTICK, settings.onScreenJoystick, fallback.onScreenJoystick),
			onScreenKeyboardButton = pick(
				Group.KEYBOARD_BUTTON, settings.onScreenKeyboardButton, fallback.onScreenKeyboardButton
			),
			onScreenKeyboardNumpad = pick(
				Group.KEYBOARD_NUMPAD, settings.onScreenKeyboardNumpad, fallback.onScreenKeyboardNumpad
			),
			onScreenKeyboardToggle = pick(
				Group.KEYBOARD_TOGGLE, settings.onScreenKeyboardToggle, fallback.onScreenKeyboardToggle
			),
			joyport0MouseMap = pick(Group.JOYPORT0_MOUSE_MAP, settings.joyport0MouseMap, fallback.joyport0MouseMap),
			joyport1MouseMap = pick(Group.JOYPORT1_MOUSE_MAP, settings.joyport1MouseMap, fallback.joyport1MouseMap),
			analogMouseSpeed = pick(Group.ANALOG_MOUSE_SPEED, settings.analogMouseSpeed, fallback.analogMouseSpeed)
		)
	}
}
