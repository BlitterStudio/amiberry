package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
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
		assertEquals(-1, s.floppy0Type)
		assertEquals(-1, s.floppy1Type)
	}

	@Test
	fun `CDTV defaults have CD support and no floppy`() {
		val s = EmulatorSettings.fromModel(AmigaModel.CDTV)
		assertFalse(AmigaModel.CDTV.hasFloppy)
		assertTrue(AmigaModel.CDTV.hasCd)
		assertEquals("ocs", s.chipset)
		assertEquals(-1, s.floppy0Type)
		assertEquals(-1, s.floppy1Type)
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
				if (s.cpuModel >= 68040) {
					assertFalse("${model.name}: 68040+ JIT should disable compatible mode", s.cpuCompatible)
				}
			}
			if (s.chipset == "aga") {
				assertTrue("${model.name}: AGA requires 68020+", s.cpuModel >= 68020)
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

	// --- Settings constraints ---

	@Test
	fun `constraints move stale on-screen joystick selection from port zero to port one`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "onscreen_joy",
				joyport1 = "joy1",
				onScreenJoystick = true
			),
			hasTouchScreen = true
		)

		assertEquals("mouse", constrained.joyport0)
		assertEquals("onscreen_joy", constrained.joyport1)
		assertTrue(constrained.onScreenJoystick)
	}

	@Test
	fun `constraints disable on-screen joystick without touchscreen`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport1 = "onscreen_joy",
				onScreenJoystick = true
			),
			hasTouchScreen = false
		)

		assertEquals("joy1", constrained.joyport1)
		assertFalse(constrained.onScreenJoystick)
	}

	@Test
	fun `constraints replace invalid input device ids with defaults`() {
		val invalidPort0 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "garbled",
				joyport1 = "none",
				onScreenJoystick = false
			),
			hasTouchScreen = true
		)
		val invalidPort1 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "none",
				joyport1 = "missing",
				onScreenJoystick = false
			),
			hasTouchScreen = true
		)

		assertEquals("mouse", invalidPort0.joyport0)
		assertEquals("none", invalidPort0.joyport1)
		assertEquals("none", invalidPort1.joyport0)
		assertEquals("joy1", invalidPort1.joyport1)
		assertFalse(invalidPort1.onScreenJoystick)
	}

	@Test
	fun `constraints clear FPU when CPU cannot use one`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68000, fpuModel = 68882),
			hasTouchScreen = true
		)

		assertEquals(0, constrained.fpuModel)
	}

	@Test
	fun `constraints map stale internal FPU to external FPU on 68020 and 68030`() {
		val cpu020 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68020, fpuModel = 68040),
			hasTouchScreen = true
		)
		val cpu030 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68030, fpuModel = 68060),
			hasTouchScreen = true
		)

		assertEquals(68881, cpu020.fpuModel)
		assertEquals(68881, cpu030.fpuModel)
	}

	@Test
	fun `constraints align internal FPU with 68040 and 68060 CPU`() {
		val cpu040 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68040, fpuModel = 68882),
			hasTouchScreen = true
		)
		val cpu060 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68060, fpuModel = 68040),
			hasTouchScreen = true
		)

		assertEquals(68040, cpu040.fpuModel)
		assertEquals(68060, cpu060.fpuModel)
	}

	@Test
	fun `constraints disable compatible mode for 68040 plus JIT`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				cpuModel = 68040,
				address24Bit = false,
				cpuCompatible = true,
				jitCacheSize = 8192
			),
			hasTouchScreen = true
		)

		assertFalse(constrained.cpuCompatible)
		assertEquals("max", constrained.cpuSpeed)
		assertTrue(constrained.jitFpu)
	}

	@Test
	fun `constraints clear JIT FPU when 24-bit addressing disables JIT`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				cpuModel = 68040,
				address24Bit = true,
				cpuCompatible = true,
				jitCacheSize = 8192,
				jitFpu = true
			),
			hasTouchScreen = true
		)

		assertEquals(0, constrained.jitCacheSize)
		assertFalse(constrained.jitFpu)
		assertTrue(constrained.cpuCompatible)
	}

	@Test
	fun `constraints disable immediate blits in cycle exact mode`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				cycleExact = true,
				immediateBlits = true
			),
			hasTouchScreen = true
		)

		assertFalse(constrained.immediateBlits)
		assertEquals("real", constrained.cpuSpeed)
	}

	@Test
	fun `constraints upgrade low-end CPU when AGA chipset is selected`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				chipset = "aga",
				cpuModel = 68000,
				fpuModel = 68882
			),
			hasTouchScreen = true
		)

		assertEquals(68020, constrained.cpuModel)
		assertEquals(68882, constrained.fpuModel)
	}

	@Test
	fun `constraints replace invalid display resolution with defaults`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				gfxWidth = 0,
				gfxHeight = -1
			),
			hasTouchScreen = true
		)

		assertEquals(720, constrained.gfxWidth)
		assertEquals(568, constrained.gfxHeight)
	}

	@Test
	fun `constraints normalize legacy sound output aliases`() {
		val best = EmulatorSettingsConstraints.apply(
			EmulatorSettings(soundOutput = "best"),
			hasTouchScreen = true
		)
		val good = EmulatorSettingsConstraints.apply(
			EmulatorSettings(soundOutput = "good"),
			hasTouchScreen = true
		)

		assertEquals("exact", best.soundOutput)
		assertEquals("normal", good.soundOutput)
	}

	@Test
	fun `constraints replace invalid sound settings with defaults`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				soundOutput = "garbled",
				soundFreq = 0,
				soundChannels = "surround"
			),
			hasTouchScreen = true
		)

		assertEquals("exact", constrained.soundOutput)
		assertEquals(44100, constrained.soundFreq)
		assertEquals("stereo", constrained.soundChannels)
	}

	// --- Integer scaling defaults ---

	@Test
	fun `default settings have integer scaling off`() {
		val s = EmulatorSettings()
		assertEquals(-1, s.scalingMethod)
		assertEquals(0, s.gfxAutoresolution)
	}

	@Test
	fun `constraints normalize floppy drive types and hidden disk paths`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				floppy0 = "/disks/disabled.adf",
				floppy0Type = -1,
				floppy1 = "/disks/imported.adf",
				floppy1Type = 42,
				floppy2 = "",
				floppy2Type = 99,
				floppy3 = "/disks/hd.adf",
				floppy3Type = 1
			),
			hasTouchScreen = true
		)

		assertEquals("", constrained.floppy0)
		assertEquals(-1, constrained.floppy0Type)
		assertEquals("/disks/imported.adf", constrained.floppy1)
		assertEquals(0, constrained.floppy1Type)
		assertEquals("", constrained.floppy2)
		assertEquals(-1, constrained.floppy2Type)
		assertEquals("/disks/hd.adf", constrained.floppy3)
		assertEquals(1, constrained.floppy3Type)
	}
}
