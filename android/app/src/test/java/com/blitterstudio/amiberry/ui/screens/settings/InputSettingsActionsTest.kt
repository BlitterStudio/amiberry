package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class InputSettingsActionsTest {

	@Test
	fun `selecting on-screen joystick on port 1 enables the overlay switch`() {
		val settings = InputSettingsActions.selectPort1Device(
			settings = EmulatorSettings(onScreenJoystick = false, joyport1 = "joy1"),
			deviceId = "onscreen_joy"
		)

		assertEquals("onscreen_joy", settings.joyport1)
		assertTrue(settings.onScreenJoystick)
	}

	@Test
	fun `selecting a physical port 1 device disables the overlay switch`() {
		val settings = InputSettingsActions.selectPort1Device(
			settings = EmulatorSettings(onScreenJoystick = true, joyport1 = "onscreen_joy"),
			deviceId = "joy1"
		)

		assertEquals("joy1", settings.joyport1)
		assertFalse(settings.onScreenJoystick)
	}

	@Test
	fun `turning off on-screen joystick falls back to joystick 1`() {
		val settings = InputSettingsActions.setOnScreenJoystick(
			settings = EmulatorSettings(onScreenJoystick = true, joyport1 = "onscreen_joy"),
			isEnabled = false
		)

		assertEquals("joy1", settings.joyport1)
		assertFalse(settings.onScreenJoystick)
	}

	@Test
	fun `enabling the mouse map targets the selected port only`() {
		val settings = InputSettingsActions.setPortMouseMap(
			settings = EmulatorSettings(joyport0 = "mouse", joyport1 = "joy0"),
			port = 1,
			isEnabled = true
		)

		assertFalse(settings.joyport0MouseMap)
		assertTrue(settings.joyport1MouseMap)
	}

	@Test
	fun `disabling the mouse map keeps the port device untouched`() {
		val settings = InputSettingsActions.setPortMouseMap(
			settings = EmulatorSettings(joyport0 = "joy0", joyport1 = "joy1", joyport0MouseMap = true),
			port = 0,
			isEnabled = false
		)

		assertFalse(settings.joyport0MouseMap)
		assertEquals("joy0", settings.joyport0)
		assertEquals("joy1", settings.joyport1)
	}
}
