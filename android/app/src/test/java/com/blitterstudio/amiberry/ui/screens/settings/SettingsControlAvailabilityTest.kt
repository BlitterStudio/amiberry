package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SettingsControlAvailabilityTest {

	@Test
	fun `24-bit addressing is offered up to a 68030 without the JIT`() {
		for (cpu in listOf(68000, 68010, 68020, 68030)) {
			assertTrue("$cpu", SettingsControlAvailability.isAddress24BitEditable(EmulatorSettings(cpuModel = cpu)))
		}
		assertFalse(SettingsControlAvailability.isAddress24BitEditable(EmulatorSettings(cpuModel = 68040)))
		assertFalse(SettingsControlAvailability.isAddress24BitEditable(EmulatorSettings(cpuModel = 68060)))
		assertFalse(
			SettingsControlAvailability.isAddress24BitEditable(
				EmulatorSettings(cpuModel = 68030, address24Bit = false, jitCacheSize = 8192)
			)
		)
		// Below a 68020 the core ignores a JIT cache size.
		assertTrue(
			SettingsControlAvailability.isAddress24BitEditable(EmulatorSettings(cpuModel = 68000, jitCacheSize = 8192))
		)
	}

	@Test
	fun `the JIT needs a 68020 or later with 32-bit addressing`() {
		assertFalse(SettingsControlAvailability.isJitAllowed(EmulatorSettings(cpuModel = 68000, address24Bit = false)))
		assertFalse(SettingsControlAvailability.isJitAllowed(EmulatorSettings(cpuModel = 68030, address24Bit = true)))
		assertTrue(SettingsControlAvailability.isJitAllowed(EmulatorSettings(cpuModel = 68030, address24Bit = false)))
		// Turning the JIT on leaves cycle-exact mode, so it stays offered there.
		assertTrue(
			SettingsControlAvailability.isJitAllowed(
				EmulatorSettings(cpuModel = 68020, address24Bit = false, cycleExact = "true")
			)
		)
	}

	@Test
	fun `compatible mode is fixed in cycle-exact modes and for a 68040 with the JIT`() {
		assertFalse(
			SettingsControlAvailability.isCpuCompatibleEditable(
				EmulatorSettings(cpuModel = 68040, address24Bit = false, jitCacheSize = 8192)
			)
		)
		assertFalse(SettingsControlAvailability.isCpuCompatibleEditable(EmulatorSettings(cycleExact = "memory")))
		assertTrue(SettingsControlAvailability.isCpuCompatibleEditable(EmulatorSettings(cpuModel = 68020)))
	}

	@Test
	fun `z3 memory needs 32-bit addressing`() {
		assertFalse(SettingsControlAvailability.isZ3MemoryAvailable(EmulatorSettings(address24Bit = true)))
		assertTrue(SettingsControlAvailability.isZ3MemoryAvailable(EmulatorSettings(address24Bit = false)))
	}

	@Test
	fun `fpu option values match CPUs that can keep the selected FPU`() {
		assertEquals(listOf(0), SettingsControlAvailability.fpuOptionValues(EmulatorSettings(cpuModel = 68000)))
		assertEquals(
			listOf(0, 68881, 68882),
			SettingsControlAvailability.fpuOptionValues(EmulatorSettings(cpuModel = 68000, cpuCompatible = false))
		)
		assertEquals(
			listOf(0),
			SettingsControlAvailability.fpuOptionValues(
				EmulatorSettings(cpuModel = 68010, cpuCompatible = false, cycleExact = "true")
			)
		)
		assertEquals(listOf(0, 68881, 68882), SettingsControlAvailability.fpuOptionValues(EmulatorSettings(cpuModel = 68020)))
		assertEquals(listOf(0, 68881, 68882), SettingsControlAvailability.fpuOptionValues(EmulatorSettings(cpuModel = 68030)))
		assertEquals(listOf(0, 68040), SettingsControlAvailability.fpuOptionValues(EmulatorSettings(cpuModel = 68040)))
		assertEquals(listOf(0, 68060), SettingsControlAvailability.fpuOptionValues(EmulatorSettings(cpuModel = 68060)))
	}

	@Test
	fun `the DMA-only cycle-exact mode is offered from a 68020`() {
		assertEquals(
			listOf("false", "true"),
			SettingsControlAvailability.cycleExactChoiceValues(EmulatorSettings(cpuModel = 68000))
		)
		assertEquals(
			listOf("false", "memory", "true"),
			SettingsControlAvailability.cycleExactChoiceValues(EmulatorSettings(cpuModel = 68020))
		)
		// A stored value stays listed.
		assertEquals(
			listOf("false", "memory", "true"),
			SettingsControlAvailability.cycleExactChoiceValues(EmulatorSettings(cpuModel = 68000, cycleExact = "memory"))
		)
	}

	@Test
	fun `disabled sound is not offered in cycle-exact modes`() {
		assertFalse("none" in SettingsControlAvailability.soundOutputChoiceValues(EmulatorSettings(cycleExact = "true")))
		assertTrue("none" in SettingsControlAvailability.soundOutputChoiceValues(EmulatorSettings(cycleExact = "false")))
	}

	@Test
	fun `drive sound volumes need sound emulation and the drive sound`() {
		assertFalse(SettingsControlAvailability.isFloppySoundVolumeAvailable(EmulatorSettings(floppy0Sound = 0), 0))
		assertTrue(SettingsControlAvailability.isFloppySoundVolumeAvailable(EmulatorSettings(floppy0Sound = 1), 0))
		assertFalse(
			SettingsControlAvailability.isFloppySoundVolumeAvailable(
				EmulatorSettings(floppy0Sound = 1, soundOutput = "none"),
				0
			)
		)
	}

	@Test
	fun `cpu tab builds FPU dropdown from shared availability helper`() {
		val cpuTab = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/CpuTab.kt").readText()

		assertTrue(
			"CpuTab should not maintain a separate FPU option list that can diverge from constraints.",
			cpuTab.contains("SettingsControlAvailability.fpuOptionValues(settings)")
		)
	}
}
