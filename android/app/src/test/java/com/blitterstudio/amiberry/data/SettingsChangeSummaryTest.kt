package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsChangeSummary
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsChangeSummaryTest {

	@Test
	fun `settings change summary reports relevant changed fields`() {
		val before = EmulatorSettings.fromModel(AmigaModel.A500).copy(
			floppy0 = "/old/Workbench.adf",
			onScreenJoystick = true
		)
		val after = before.copy(
			cpuModel = 68030,
			address24Bit = false,
			floppy0 = "/new/Lotus.adf",
			onScreenJoystick = false
		)

		val changes = SettingsChangeSummary.diff(before, after)

		assertEquals(
			listOf("CPU", "Addressing", "DF0", "On-screen joystick"),
			changes.map { it.label }
		)
		assertEquals("68000", changes[0].before)
		assertEquals("68030", changes[0].after)
	}

	@Test
	fun `settings change summary returns no rows for identical settings`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A1200)

		assertTrue(SettingsChangeSummary.diff(settings, settings).isEmpty())
	}
}
