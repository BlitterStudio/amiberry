package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsIntentPreset
import com.blitterstudio.amiberry.data.model.SettingsIntentPresets
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsIntentPresetsTest {

	@Test
	fun `preset match remains true until one of its settings changes`() {
		val settings = SettingsIntentPresets.apply(
			EmulatorSettings(),
			SettingsIntentPreset.PixelPerfect
		)

		assertTrue(SettingsIntentPresets.matches(settings, SettingsIntentPreset.PixelPerfect))
		assertFalse(
			SettingsIntentPresets.matches(
				settings.copy(scalingMethod = 1),
				SettingsIntentPreset.PixelPerfect
			)
		)
	}

	@Test
	fun `pixel perfect preset enables integer scaling pair`() {
		val settings = SettingsIntentPresets.apply(
			EmulatorSettings(scalingMethod = -1, gfxAutoresolution = 0),
			SettingsIntentPreset.PixelPerfect
		)

		assertEquals(2, settings.scalingMethod)
		assertEquals(1, settings.gfxAutoresolution)
		assertTrue(settings.correctAspect)
	}

	@Test
	fun `touch preset enables both on-screen controls`() {
		val settings = SettingsIntentPresets.apply(
			EmulatorSettings(onScreenJoystick = false, onScreenKeyboardButton = false, joyport1 = "joy1"),
			SettingsIntentPreset.TouchControls
		)

		assertTrue(settings.onScreenJoystick)
		assertTrue(settings.onScreenKeyboardButton)
		assertEquals("onscreen_joy", settings.joyport1)
	}
}
