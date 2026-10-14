package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings

object AndroidControlSettings {

	fun withFallback(
		settings: EmulatorSettings,
		explicitKeys: Set<String>,
		fallback: EmulatorSettings
	): EmulatorSettings {
		val keys = explicitKeys.map { it.lowercase() }.toSet()
		val keyboardSwitchExplicit = "amiberry.vkbd_enabled" in keys || "input.default_osk" in keys
		// An explicitly disabled keyboard also fixes the toggle (parsed as disabled).
		val keyboardExplicitlyOff = keyboardSwitchExplicit && !settings.onScreenKeyboardAvailable
		return settings.copy(
			joyport0 = if ("joyport0" in keys) settings.joyport0 else fallback.joyport0,
			joyport1 = if ("joyport1" in keys || "amiberry.android_joyport1" in keys) {
				settings.joyport1
			} else {
				fallback.joyport1
			},
			onScreenJoystick = if ("amiberry.onscreen_joystick" in keys) {
				settings.onScreenJoystick
			} else {
				fallback.onScreenJoystick
			},
			onScreenKeyboardButton = if (keyboardSwitchExplicit || "amiberry.vkbd_touch_button" in keys) {
				settings.onScreenKeyboardButton
			} else {
				fallback.onScreenKeyboardButton
			},
			onScreenKeyboardNumpad = if ("amiberry.vkbd_numpad" in keys) {
				settings.onScreenKeyboardNumpad
			} else {
				fallback.onScreenKeyboardNumpad
			},
			onScreenKeyboardToggle = if ("amiberry.vkbd_toggle" in keys || keyboardExplicitlyOff) {
				settings.onScreenKeyboardToggle
			} else {
				fallback.onScreenKeyboardToggle
			},
			joyport0MouseMap = if ("joyport0mousemap" in keys) settings.joyport0MouseMap else fallback.joyport0MouseMap,
			joyport1MouseMap = if ("joyport1mousemap" in keys) settings.joyport1MouseMap else fallback.joyport1MouseMap,
			analogMouseSpeed = if ("input.joymouse_speed_analog" in keys) {
				settings.analogMouseSpeed
			} else {
				fallback.analogMouseSpeed
			}
		)
	}
}
