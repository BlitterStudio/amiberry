package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class AndroidControlSettingsTest {

	@Test
	fun `fallback controls are used when a config has no explicit Android control keys`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(),
			explicitKeys = emptySet(),
			fallback = EmulatorSettings(
				joyport0 = "joy0",
				joyport1 = "joy1",
				onScreenJoystick = false,
				onScreenKeyboard = false
			)
		)

		assertEquals("joy0", settings.joyport0)
		assertEquals("joy1", settings.joyport1)
		assertFalse(settings.onScreenJoystick)
		assertFalse(settings.onScreenKeyboard)
	}

	@Test
	fun `explicit Android control keys from a config override fallback controls`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(
				joyport0 = "mouse",
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboard = true
			),
			explicitKeys = setOf(
				"joyport0",
				"amiberry.android_joyport1",
				"amiberry.onscreen_joystick",
				"amiberry.vkbd_enabled"
			),
			fallback = EmulatorSettings(
				joyport0 = "joy0",
				joyport1 = "joy1",
				onScreenJoystick = false,
				onScreenKeyboard = false
			)
		)

		assertEquals("mouse", settings.joyport0)
		assertEquals("onscreen_joy", settings.joyport1)
		assertTrue(settings.onScreenJoystick)
		assertTrue(settings.onScreenKeyboard)
	}
}
