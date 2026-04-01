package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.*
import org.junit.Test

class EmulatorSettingsTest {

	// --- Model defaults ---

	@Test
	fun `A500 defaults have OCS chipset and 68000`() {
		val s = EmulatorSettings.fromModel(AmigaModel.A500)
		assertEquals(68000, s.cpuModel)
		assertEquals("ocs", s.chipset)
		assertTrue(s.cycleExact)
	}

	@Test
	fun `A1200 defaults have AGA chipset and 68020`() {
		val s = EmulatorSettings.fromModel(AmigaModel.A1200)
		assertEquals(68020, s.cpuModel)
		assertEquals("aga", s.chipset)
	}

	@Test
	fun `A4000 defaults have JIT enabled`() {
		val s = EmulatorSettings.fromModel(AmigaModel.A4000)
		assertEquals(68040, s.cpuModel)
		assertTrue(s.jitCacheSize > 0)
		assertTrue(s.jitFpu)
		assertEquals(68040, s.fpuModel)
	}

	@Test
	fun `CD32 defaults have CD support and no floppy`() {
		val s = EmulatorSettings.fromModel(AmigaModel.CD32)
		assertFalse(AmigaModel.CD32.hasFloppy)
		assertTrue(AmigaModel.CD32.hasCd)
		assertEquals("aga", s.chipset)
	}

	// --- All models produce valid settings ---

	@Test
	fun `all model presets are internally consistent`() {
		for (model in AmigaModel.entries) {
			val s = EmulatorSettings.fromModel(model)
			// 24-bit addressing models should not have Z3 RAM
			if (s.address24Bit) {
				assertEquals("${model.name}: 24-bit should have no Z3 RAM", 0, s.z3Ram)
			}
			// JIT requires 68020+
			if (s.jitCacheSize > 0) {
				assertTrue("${model.name}: JIT requires 68020+", s.cpuModel >= 68020)
			}
		}
	}

	// --- Option lists ---

	@Test
	fun `chipRamOptions are sorted ascending`() {
		val values = EmulatorSettings.chipRamOptions.map { it.first }
		assertEquals(values, values.sorted())
	}

	@Test
	fun `cpuModels are sorted ascending`() {
		assertEquals(EmulatorSettings.cpuModels, EmulatorSettings.cpuModels.sorted())
	}

	@Test
	fun `z3RamOptions start with None`() {
		assertEquals(0, EmulatorSettings.z3RamOptions.first().first)
		assertEquals("None", EmulatorSettings.z3RamOptions.first().second)
	}

	// --- Config key coverage ---

	@Test
	fun `all AmigaModel entries have unique cmdArgs`() {
		val cmdArgs = AmigaModel.entries.map { it.cmdArg }
		assertEquals(cmdArgs.size, cmdArgs.toSet().size)
	}

	@Test
	fun `AmigaModel displayNames are not blank`() {
		for (model in AmigaModel.entries) {
			assertTrue("${model.name} has blank displayName", model.displayName.isNotBlank())
		}
	}
}
