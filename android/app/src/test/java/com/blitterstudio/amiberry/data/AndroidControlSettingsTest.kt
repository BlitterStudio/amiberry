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
				onScreenKeyboardButton = false,
				onScreenKeyboardNumpad = true
			)
		)

		assertEquals("joy0", settings.joyport0)
		assertEquals("joy1", settings.joyport1)
		assertFalse(settings.onScreenJoystick)
		assertFalse(settings.onScreenKeyboardButton)
		assertTrue(settings.onScreenKeyboardNumpad)
	}

	@Test
	fun `explicit Android control keys from a config override fallback controls`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(
				joyport0 = "mouse",
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboardButton = true,
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
				onScreenKeyboardButton = false
			)
		)

		assertEquals("mouse", settings.joyport0)
		assertEquals("onscreen_joy", settings.joyport1)
		assertTrue(settings.onScreenJoystick)
		assertTrue(settings.onScreenKeyboardButton)
		assertTrue(settings.onScreenKeyboardNumpad)
	}

	@Test
	fun `mouse map falls back when a config has no explicit mouse map keys`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(),
			explicitKeys = setOf("joyport0", "joyport1"),
			fallback = EmulatorSettings(joyport1MouseMap = true, analogMouseSpeed = 75)
		)

		assertTrue(settings.joyport1MouseMap)
		assertEquals(75, settings.analogMouseSpeed)
	}

	@Test
	fun `explicit mouse map keys from a config override the fallback`() {
		val settings = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(joyport1MouseMap = false, analogMouseSpeed = 40),
			explicitKeys = setOf("joyport1mousemap", "input.joymouse_speed_analog"),
			fallback = EmulatorSettings(joyport1MouseMap = true, analogMouseSpeed = 75)
		)

		assertFalse(settings.joyport1MouseMap)
		assertEquals(40, settings.analogMouseSpeed)
	}

	@Test
	fun `explicit vkbd toggle overrides fallback and absent key uses fallback`() {
		val explicit = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(onScreenKeyboardToggle = "rightstick"),
			explicitKeys = setOf("amiberry.vkbd_toggle"),
			fallback = EmulatorSettings(onScreenKeyboardToggle = "leftstick")
		)
		assertEquals("rightstick", explicit.onScreenKeyboardToggle)

		val fallback = AndroidControlSettings.withFallback(
			settings = EmulatorSettings(onScreenKeyboardToggle = null),
			explicitKeys = setOf("amiberry.vkbd_enabled"),
			fallback = EmulatorSettings(onScreenKeyboardToggle = "leftstick")
		)
		assertEquals("leftstick", fallback.onScreenKeyboardToggle)
	}
}
