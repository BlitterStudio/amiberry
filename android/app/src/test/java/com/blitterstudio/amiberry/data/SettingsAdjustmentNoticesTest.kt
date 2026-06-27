package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotice
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotices
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsAdjustmentNoticesTest {

	@Test
	fun `notices include addressing changes for 68030 and newer CPUs`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(cpuModel = 68030, address24Bit = true),
			applied = EmulatorSettings(cpuModel = 68030, address24Bit = false)
		)

		assertEquals(listOf(SettingsAdjustmentNotice.CpuRequires32BitAddressing), notices)
	}

	@Test
	fun `notices include integer scaling synchronization`() {
		val notices = SettingsAdjustmentNotices.fromAdjustment(
			requested = EmulatorSettings(scalingMethod = 2, gfxAutoresolution = 0),
			applied = EmulatorSettings(scalingMethod = 2, gfxAutoresolution = 1)
		)

		assertEquals(listOf(SettingsAdjustmentNotice.IntegerScalingSynced), notices)
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
