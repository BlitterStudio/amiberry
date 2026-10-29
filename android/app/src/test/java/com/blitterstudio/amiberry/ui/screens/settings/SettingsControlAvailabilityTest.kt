package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsControlAvailabilityTest {

	@Test
	fun `a 68030 keeps 24-bit addressing editable without the JIT`() {
		assertTrue(SettingsControlAvailability.isAvailable(EmulatorSettings(cpuModel = 68030), "cpu_24bit_addressing"))
		assertFalse(
			SettingsControlAvailability.isAvailable(
				EmulatorSettings(cpuModel = 68030, address24Bit = false, jitCacheSize = 2048),
				"cpu_24bit_addressing"
			)
		)
	}

	@Test
	fun `the DMA-only cycle-exact mode needs a 68020`() {
		assertEquals(
			listOf("false", "true"),
			SettingsControlAvailability.choiceValues(EmulatorSettings(cpuModel = 68000), "cycle_exact")
		)
		assertEquals(
			listOf("false", "memory", "true"),
			SettingsControlAvailability.choiceValues(EmulatorSettings(cpuModel = 68020), "cycle_exact")
		)
	}

	@Test
	fun `sound and drive sound tables answer for their own options`() {
		val noSound = EmulatorSettings(soundOutput = "none", floppy2Sound = 1)
		assertFalse(SettingsControlAvailability.isAvailable(noSound, "sound_frequency"))
		assertFalse(SettingsControlAvailability.isAvailable(noSound, "floppy2soundvolume_disk"))
		val driveSound = EmulatorSettings(soundOutput = "normal", floppy2Sound = 1)
		assertTrue(SettingsControlAvailability.isAvailable(driveSound, "floppy2soundvolume_empty"))
		assertFalse(SettingsControlAvailability.isAvailable(driveSound, "floppy1soundvolume_empty"))
	}

	@Test
	fun `a state the tables do not list leaves the control available`() {
		val handEdited = EmulatorSettings(cpuModel = 68050)
		assertTrue(SettingsControlAvailability.isAvailable(handEdited, "cpu_compatible"))
		assertNull(SettingsControlAvailability.choiceValues(handEdited, "fpu_model"))
		assertTrue(SettingsControlAvailability.isAvailable(EmulatorSettings(), "cpu_compatible", schema = null))
	}
}
