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
	fun `performance preset enables fastest safe CPU settings for 32-bit capable CPUs`() {
		val settings = SettingsIntentPresets.apply(
			EmulatorSettings(cpuModel = 68040, address24Bit = false, cycleExact = true, cpuCompatible = true),
			SettingsIntentPreset.Performance
		)

		assertEquals("max", settings.cpuSpeed)
		assertEquals(16384, settings.jitCacheSize)
		assertFalse(settings.cycleExact)
		assertFalse(settings.cpuCompatible)
	}

	@Test
	fun `compatibility preset disables JIT and restores conservative timing`() {
		val settings = SettingsIntentPresets.apply(
			EmulatorSettings(cpuModel = 68030, address24Bit = false, cpuSpeed = "max", jitCacheSize = 16384),
			SettingsIntentPreset.Compatibility
		)

		assertEquals("real", settings.cpuSpeed)
		assertEquals(0, settings.jitCacheSize)
		assertTrue(settings.cpuCompatible)
	}

	@Test
	fun `touch preset enables both on-screen controls`() {
		val settings = SettingsIntentPresets.apply(
			EmulatorSettings(onScreenJoystick = false, onScreenKeyboard = false, joyport1 = "joy1"),
			SettingsIntentPreset.TouchControls
		)

		assertTrue(settings.onScreenJoystick)
		assertTrue(settings.onScreenKeyboard)
		assertEquals("onscreen_joy", settings.joyport1)
	}
}
