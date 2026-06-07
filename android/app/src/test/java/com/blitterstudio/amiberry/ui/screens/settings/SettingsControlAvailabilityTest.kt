package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SettingsControlAvailabilityTest {

	@Test
	fun `24-bit addressing is fixed for 68000 and editable for 68020 plus`() {
		assertFalse(SettingsControlAvailability.isAddress24BitEditable(cpuModel = 68000))
		assertEquals(
			R.string.settings_cpu_24bit_forced_68000,
			SettingsControlAvailability.address24BitReasonRes(cpuModel = 68000)
		)

		assertTrue(SettingsControlAvailability.isAddress24BitEditable(cpuModel = 68020))
		assertEquals(null, SettingsControlAvailability.address24BitReasonRes(cpuModel = 68020))
	}

	@Test
	fun `fastest speed is disabled by cycle exact mode`() {
		val settings = EmulatorSettings(cycleExact = true)

		assertFalse(SettingsControlAvailability.isFastestSpeedEditable(settings))
		assertEquals(
			R.string.settings_cpu_fastest_disabled_cycle_exact,
			SettingsControlAvailability.fastestSpeedReasonRes(settings)
		)
	}

	@Test
	fun `jit disabled reason explains cycle exact before 24-bit addressing`() {
		val settings = EmulatorSettings(cpuModel = 68030, cycleExact = true, address24Bit = true)

		assertFalse(SettingsControlAvailability.isJitAllowed(settings))
		assertEquals(
			R.string.settings_cpu_jit_disabled_cycle_exact,
			SettingsControlAvailability.jitDisabledReasonRes(settings)
		)
	}

	@Test
	fun `jit disabled reason explains 24-bit addressing`() {
		val settings = EmulatorSettings(cpuModel = 68030, address24Bit = true)

		assertFalse(SettingsControlAvailability.isJitAllowed(settings))
		assertEquals(
			R.string.settings_cpu_jit_disabled_24bit,
			SettingsControlAvailability.jitDisabledReasonRes(settings)
		)
	}

	@Test
	fun `compatible mode is disabled for 68040 plus JIT`() {
		val settings = EmulatorSettings(cpuModel = 68040, address24Bit = false, jitCacheSize = 8192)

		assertFalse(SettingsControlAvailability.isCpuCompatibleEditable(settings))
		assertEquals(
			R.string.settings_cpu_compatible_disabled_jit,
			SettingsControlAvailability.cpuCompatibleReasonRes(settings)
		)
	}

	@Test
	fun `immediate blits are disabled by cycle exact mode`() {
		val settings = EmulatorSettings(cycleExact = true)

		assertFalse(SettingsControlAvailability.isImmediateBlitsEditable(settings))
		assertEquals(
			R.string.settings_chipset_immediate_blitter_disabled_cycle_exact,
			SettingsControlAvailability.immediateBlitsReasonRes(settings)
		)
	}

	@Test
	fun `z3 memory is hidden with 24-bit addressing`() {
		val settings = EmulatorSettings(address24Bit = true)

		assertFalse(SettingsControlAvailability.isZ3MemoryVisible(settings))
		assertEquals(
			R.string.settings_memory_z3_disabled_24bit,
			SettingsControlAvailability.z3HiddenReasonRes(settings)
		)
	}

	@Test
	fun `fpu option values match CPUs that can keep the selected FPU`() {
		assertEquals(listOf(0), SettingsControlAvailability.fpuOptionValues(68000))
		assertEquals(listOf(0, 68881, 68882), SettingsControlAvailability.fpuOptionValues(68020))
		assertEquals(listOf(0, 68881, 68882), SettingsControlAvailability.fpuOptionValues(68030))
		assertEquals(listOf(0, 68040), SettingsControlAvailability.fpuOptionValues(68040))
		assertEquals(listOf(0, 68060), SettingsControlAvailability.fpuOptionValues(68060))
	}

	@Test
	fun `cpu tab builds FPU dropdown from shared availability helper`() {
		val cpuTab = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/CpuTab.kt").readText()

		assertTrue(
			"CpuTab should not maintain a separate FPU option list that can diverge from constraints.",
			cpuTab.contains("SettingsControlAvailability.fpuOptionValues(settings.cpuModel)")
		)
	}
}
