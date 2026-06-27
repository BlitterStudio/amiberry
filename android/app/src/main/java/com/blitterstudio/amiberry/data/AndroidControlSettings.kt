package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings

object AndroidControlSettings {

	fun withFallback(
		settings: EmulatorSettings,
		explicitKeys: Set<String>,
		fallback: EmulatorSettings
	): EmulatorSettings {
		val keys = explicitKeys.map { it.lowercase() }.toSet()
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
			onScreenKeyboard = if ("amiberry.vkbd_enabled" in keys || "input.default_osk" in keys) {
				settings.onScreenKeyboard
			} else {
				fallback.onScreenKeyboard
			}
		)
	}
}
