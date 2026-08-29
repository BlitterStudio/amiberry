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
				onScreenKeyboard = false,
				onScreenKeyboardNumpad = true
			)
		)

		assertEquals("joy0", settings.joyport0)
		assertEquals("joy1", settings.joyport1)
		assertFalse(settings.onScreenJoystick)
		assertFalse(settings.onScreenKeyboard)
		assertTrue(settings.onScreenKeyboardNumpad)
	}

	@Test
	fun `explicit Android control keys from a config override fallback controls`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(
				joyport0 = "mouse",
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboard = true,
				onScreenKeyboardNumpad = true
			),
			explicitKeys = setOf(
				"joyport0",
				"amiberry.android_joyport1",
				"amiberry.onscreen_joystick",
				"amiberry.vkbd_enabled",
				"amiberry.vkbd_numpad"
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
		assertTrue(settings.onScreenKeyboardNumpad)
	}

	@Test
	fun `mouse map falls back when a config has no explicit mouse map keys`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(),
			explicitKeys = setOf("joyport0", "joyport1"),
			fallback = EmulatorSettings(joyport1MouseMap = true)
		)

		assertTrue(settings.joyport1MouseMap)
	}

	@Test
	fun `explicit mouse map keys from a config override the fallback`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(joyport1MouseMap = false),
			explicitKeys = setOf("joyport1mousemap"),
			fallback = EmulatorSettings(joyport1MouseMap = true)
		)

		assertFalse(settings.joyport1MouseMap)
	}
}
