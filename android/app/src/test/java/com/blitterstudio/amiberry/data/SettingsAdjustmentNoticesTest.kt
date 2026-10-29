package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotice
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotices
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsAdjustmentNoticesTest {

	@Test
	fun `an addressing change is explained by the schema's 24-bit constraint`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(cpuModel = 68040, address24Bit = true),
			applied = EmulatorSettings(cpuModel = 68040, address24Bit = false)
		)

		assertEquals(listOf(SettingsAdjustmentNotice.AddressingChanged), notices)
		assertEquals("cpu_24bit_addressing", notices.single().optionId)
	}

	@Test
	fun `a JIT the emulator turns off is explained by the cachesize constraint`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(cpuModel = 68000, jitCacheSize = 8192),
			applied = EmulatorSettings(cpuModel = 68000, jitCacheSize = 0)
		)

		assertEquals(listOf(SettingsAdjustmentNotice.JitTurnedOff), notices)
		assertEquals("cachesize", notices.single().optionId)
	}

	@Test
	fun `disabled sound raised for cycle-exact mode is explained by the sound_output constraint`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(cycleExact = "memory", cpuModel = 68020, soundOutput = "none"),
			applied = EmulatorSettings(cycleExact = "memory", cpuModel = 68020, soundOutput = "interrupts")
		)

		assertEquals(listOf(SettingsAdjustmentNotice.SoundOutputRaised), notices)
		assertEquals("sound_output", notices.single().optionId)
	}

	@Test
	fun `notices include integer scaling synchronization`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(scalingMethod = 2, gfxAutoresolution = 0),
			applied = EmulatorSettings(scalingMethod = 2, gfxAutoresolution = 1)
		)

		assertEquals(listOf(SettingsAdjustmentNotice.IntegerScalingSynced), notices)
		assertEquals(null, notices.single().optionId)
	}

	@Test
	fun `notices include touch-control normalization`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(joyport0 = "onscreen_joy", joyport1 = "joy1", onScreenJoystick = true),
			applied = EmulatorSettings(joyport0 = "mouse", joyport1 = "onscreen_joy", onScreenJoystick = true)
		)

		assertEquals(listOf(SettingsAdjustmentNotice.OnScreenJoystickMovedToPort1), notices)
	}

	@Test
	fun `no notices are returned when constraints did not alter settings`() {
		val settings = EmulatorSettings(cpuModel = 68020, address24Bit = true)

		assertTrue(SettingsAdjustmentNotices.fromAdjustment(settings, settings).isEmpty())
	}
}
