package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings

object InputSettingsActions {

	fun selectPort1Device(settings: EmulatorSettings, deviceId: String): EmulatorSettings =
		if (deviceId == "onscreen_joy") {
			settings.copy(joyport1 = deviceId, onScreenJoystick = true)
		} else {
			settings.copy(joyport1 = deviceId, onScreenJoystick = false)
		}

	fun setOnScreenJoystick(settings: EmulatorSettings, isEnabled: Boolean): EmulatorSettings =
		if (isEnabled) {
			settings.copy(onScreenJoystick = true, joyport1 = "onscreen_joy")
		} else if (settings.joyport1 == "onscreen_joy") {
			settings.copy(onScreenJoystick = false, joyport1 = "joy1")
		} else {
			settings.copy(onScreenJoystick = false)
		}
}
