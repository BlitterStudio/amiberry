package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.HardDrive
import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.*
import org.junit.Test

class EmulatorSettingsTest {

	// --- Model defaults ---

	@Test
	fun `A500 defaults have OCS chipset and 68000`() {
		val s = EmulatorSettings.fromModel(AmigaModel.A500)
		assertEquals(68000, s.cpuModel)
		assertEquals("ocs", s.chipset)
		assertEquals("true", s.cycleExact)
	}

	@Test
	fun `A1200 defaults have AGA chipset and 68020`() {
		val s = EmulatorSettings.fromModel(AmigaModel.A1200)
		assertEquals(68020, s.cpuModel)
		assertEquals("aga", s.chipset)
	}

	@Test
	fun `A4000 defaults are the emulator's Quick Start A4000`() {
		val s = EmulatorSettings.fromModel(AmigaModel.A4000)
		assertEquals(68030, s.cpuModel)
		assertEquals("aga", s.chipset)
		assertEquals(68882, s.fpuModel)
		assertEquals(16384, s.jitCacheSize)
		assertFalse(s.address24Bit)
		assertEquals("max", s.cpuSpeed)
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
		assertEquals("ecs_agnus", s.chipset)
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
				assertEquals("${model.cmdArg}: 24-bit should have no Z3 RAM", 0, s.z3Ram)
			}
			// JIT requires 68020+
			if (s.jitCacheSize > 0) {
				assertTrue("${model.cmdArg}: JIT requires 68020+", s.cpuModel >= 68020)
				if (s.cpuModel >= 68040) {
					assertFalse("${model.cmdArg}: 68040+ JIT should disable compatible mode", s.cpuCompatible)
				}
			}
			if (s.chipset == "aga") {
				assertTrue("${model.cmdArg}: AGA requires 68020+", s.cpuModel >= 68020)
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
	fun `z3RamOptions start with none`() {
		assertEquals(0, EmulatorSettings.z3RamOptions.first().first)
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
			assertTrue("${model.cmdArg} has blank displayName", model.displayName.isNotBlank())
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
	fun `constraints keep port 1 mouse map when the mouse stays on port 0`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "mouse",
				joyport1 = "joy0",
				onScreenJoystick = false,
				joyport1MouseMap = true
			),
			hasTouchScreen = true
		)

		assertTrue(constrained.joyport1MouseMap)
	}

	@Test
	fun `constraints clear mouse map outside the supported arrangement`() {
		// Port 0 can never host the map: the stick feeds the port-0 mouse,
		// which requires port 0 to hold the mouse device
		val port0Controller = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "joy0",
				joyport1 = "joy1",
				onScreenJoystick = false,
				joyport0MouseMap = true
			),
			hasTouchScreen = true
		)
		assertFalse(port0Controller.joyport0MouseMap)

		// Port 1 map needs both a controller on port 1 and the mouse on port 0
		val noMouseOnPort0 = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "joy1",
				joyport1 = "joy0",
				onScreenJoystick = false,
				joyport1MouseMap = true
			),
			hasTouchScreen = true
		)
		assertFalse(noMouseOnPort0.joyport1MouseMap)

		val onScreenJoystickPort = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				joyport0 = "mouse",
				joyport1 = "onscreen_joy",
				joyport1MouseMap = true,
				onScreenJoystick = true
			),
			hasTouchScreen = true
		)
		assertFalse(onScreenJoystickPort.joyport1MouseMap)
	}

	@Test
	fun `constraints keep off-slider analog mouse speeds and reset invalid ones`() {
		val hotkeySpeed = EmulatorSettingsConstraints.apply(
			EmulatorSettings(analogMouseSpeed = 2),
			hasTouchScreen = true
		)
		assertEquals(2, hotkeySpeed.analogMouseSpeed)

		val invalid = EmulatorSettingsConstraints.apply(
			EmulatorSettings(analogMouseSpeed = 0),
			hasTouchScreen = true
		)
		assertEquals(25, invalid.analogMouseSpeed)
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
	fun `constraints drop the FPU of a compatible 68000`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68000, fpuModel = 68881, cpuCompatible = true),
			hasTouchScreen = true
		)

		assertEquals(0, constrained.fpuModel)
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
		assertEquals(8192, constrained.jitCacheSize)
	}

	@Test
	fun `constraints turn the JIT off in cycle-exact mode`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				chipset = "aga",
				cpuModel = 68020,
				address24Bit = false,
				cycleExact = "true",
				cpuCompatible = false,
				jitCacheSize = 8192
			),
			hasTouchScreen = true
		)

		assertEquals(0, constrained.jitCacheSize)
		assertTrue(constrained.cpuCompatible)
	}

	@Test
	fun `constraints clear 24-bit addressing on a 68040`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				cpuModel = 68040,
				address24Bit = true
			),
			hasTouchScreen = true
		)

		assertFalse(constrained.address24Bit)
	}

	@Test
	fun `constraints keep 24-bit addressing on a 68030 (68EC030)`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(chipset = "ecs", cpuModel = 68030, address24Bit = true),
			hasTouchScreen = true
		)

		assertTrue(constrained.address24Bit)
	}

	@Test
	fun `constraints keep 24-bit addressing editable for 68020`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(
				cpuModel = 68020,
				address24Bit = true
			),
			hasTouchScreen = true
		)

		assertTrue(constrained.address24Bit)
	}

	@Test
	fun `constraints give the emulator's 2 MB for 1_5 MB chip RAM with the JIT`() {
		val a4000 = EmulatorSettings.fromModel(AmigaModel.A4000)
		assertTrue(a4000.jitCacheSize > 0)

		val withJit = EmulatorSettingsConstraints.apply(a4000.copy(chipRam = 3, fastRam = 0), hasTouchScreen = true)
		assertEquals(4, withJit.chipRam)

		val withoutJit = EmulatorSettingsConstraints.apply(a4000.copy(chipRam = 3, fastRam = 0, jitCacheSize = 0), hasTouchScreen = true)
		assertEquals(3, withoutJit.chipRam)
	}

	@Test
	fun `constraints remove Z3 RAM a 24-bit CPU cannot reach`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cpuModel = 68000, address24Bit = true, z3Ram = 64),
			hasTouchScreen = true
		)

		assertEquals(0, constrained.z3Ram)
	}

	@Test
	fun `constraints follow the emulator's resolution for every combination it lists`() {
		val schema = SettingsSchema.current!!
		val resolutions = schema.resolutions
		val chipsets = listOf("ocs", "ecs", "aga")
		for (chipset in chipsets) for (cpu in listOf(68000, 68010, 68020, 68030, 68040, 68060))
			for (fpu in listOf(0, 68881, 68882, 68040, 68060)) for (a24 in listOf(true, false))
				for (ce in listOf("false", "memory", "true")) for (jit in listOf(0, 8192)) {
					val requested = EmulatorSettings(
						chipset = chipset, cpuModel = cpu, fpuModel = fpu, address24Bit = a24,
						cycleExact = ce, jitCacheSize = jit
					)
					val expected = resolutions.resolve(
						mapOf(
							"chipset" to chipset, "cpu_model" to "$cpu", "fpu_model" to "$fpu",
							"cpu_24bit_addressing" to "$a24", "cycle_exact" to if (ce == "false") "false" else "true",
							"cachesize" to if (jit > 0) "8192" else "0", "z3mem_size" to "0",
							"cpu_speed" to requested.cpuSpeed, "cpu_compatible" to "${requested.cpuCompatible}",
							"compfpu" to "${requested.jitFpu}", "immediate_blits" to "${requested.immediateBlits}"
						)
					)!!
					val actual = EmulatorSettingsConstraints.apply(requested, hasTouchScreen = true)
					val label = "$chipset/$cpu/$fpu/$a24/$ce/$jit"
					assertEquals(label, expected.getValue("cpu_model").toInt(), actual.cpuModel)
					assertEquals(label, expected.getValue("fpu_model").toInt(), actual.fpuModel)
					assertEquals(label, expected.getValue("cpu_24bit_addressing") == "true", actual.address24Bit)
					assertEquals(label, expected.getValue("cpu_compatible") == "true", actual.cpuCompatible)
					assertEquals(label, expected.getValue("cachesize") != "0", actual.jitCacheSize > 0)
					val expectedCycleExact = if (expected.getValue("cycle_exact") == "false") "false" else ce
					assertEquals(label, expectedCycleExact, actual.cycleExact)
				}
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
		assertEquals(48000, constrained.soundFreq)
		assertEquals("stereo", constrained.soundChannels)
	}

	@Test
	fun `constraints keep the min and off-list sound buffers and reset invalid ones`() {
		val min = EmulatorSettingsConstraints.apply(
			EmulatorSettings(soundBuffer = 0),
			hasTouchScreen = true
		)
		assertEquals(0, min.soundBuffer)

		val offList = EmulatorSettingsConstraints.apply(
			EmulatorSettings(soundBuffer = 5000),
			hasTouchScreen = true
		)
		assertEquals(5000, offList.soundBuffer)

		val invalid = EmulatorSettingsConstraints.apply(
			EmulatorSettings(soundBuffer = -1),
			hasTouchScreen = true
		)
		assertEquals(8192, invalid.soundBuffer)
	}

	// --- Hard drives ---

	@Test
	fun `default settings have no hard drives`() {
		assertTrue(EmulatorSettings().hardDrives.isEmpty())
	}

	// --- Integer scaling defaults ---

	@Test
	fun `default settings have integer scaling off`() {
		val s = EmulatorSettings()
		assertEquals(-1, s.scalingMethod)
		assertEquals(0, s.gfxAutoresolution)
	}

	@Test
	fun `constraints keep integer scaling and autoresolution in sync`() {
		val methodOnly = EmulatorSettingsConstraints.apply(
			EmulatorSettings(scalingMethod = 2, gfxAutoresolution = 0),
			hasTouchScreen = true
		)
		val autoresolutionOnly = EmulatorSettingsConstraints.apply(
			EmulatorSettings(scalingMethod = -1, gfxAutoresolution = 1),
			hasTouchScreen = true
		)

		assertEquals(2, methodOnly.scalingMethod)
		assertEquals(1, methodOnly.gfxAutoresolution)
		assertEquals(2, autoresolutionOnly.scalingMethod)
		assertEquals(1, autoresolutionOnly.gfxAutoresolution)
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

	@Test
	fun `constraints keep every drive type the emulator has`() {
		for (type in listOf(0, 1, 2, 7, 3, 8)) {
			val constrained = EmulatorSettingsConstraints.apply(
				EmulatorSettings(floppy1 = "/disks/game.adf", floppy1Type = type),
				hasTouchScreen = true
			)
			assertEquals("$type", type, constrained.floppy1Type)
			assertEquals("$type", "/disks/game.adf", constrained.floppy1)
		}
	}

	@Test
	fun `constraints keep the DMA-only cycle-exact mode`() {
		val constrained = EmulatorSettingsConstraints.apply(
			EmulatorSettings(chipset = "aga", cpuModel = 68020, cycleExact = "memory"),
			hasTouchScreen = true
		)

		assertEquals("memory", constrained.cycleExact)
		assertTrue(constrained.cpuCompatible)
	}

	@Test
	fun `constraints raise disabled sound to emulated in cycle-exact modes`() {
		val cycleExact = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cycleExact = "true", soundOutput = "none"),
			hasTouchScreen = true
		)
		val notCycleExact = EmulatorSettingsConstraints.apply(
			EmulatorSettings(cycleExact = "false", soundOutput = "none"),
			hasTouchScreen = true
		)

		assertEquals("interrupts", cycleExact.soundOutput)
		assertEquals("none", notCycleExact.soundOutput)
	}

	@Test
	fun `constraints keep every sound frequency the emulator offers`() {
		val frequencies = SettingsSchema.current!!.option("sound_frequency")!!.choices.map { it.value.toInt() }
		assertEquals(listOf(11025, 22050, 32000, 44100, 48000), frequencies)
		for (frequency in frequencies) {
			val constrained = EmulatorSettingsConstraints.apply(EmulatorSettings(soundFreq = frequency), hasTouchScreen = true)
			assertEquals(frequency, constrained.soundFreq)
		}
	}

	// --- Changes that bring others with them, as in the emulator ---

	@Test
	fun `a 68030 starts 32-bit and keeps 24-bit addressing when chosen`() {
		val on68030 = EmulatorSettings(cpuModel = 68020, address24Bit = true).withCpuModel(68030)
		assertFalse(on68030.address24Bit)

		val constrained = EmulatorSettingsConstraints.apply(on68030.copy(address24Bit = true), hasTouchScreen = true)
		assertTrue(constrained.address24Bit)
	}

	@Test
	fun `a 68000 is 24-bit without JIT and FPU in compatible mode`() {
		val s = EmulatorSettings(cpuModel = 68030, address24Bit = false, jitCacheSize = 8192, fpuModel = 68882)
			.withCpuModel(68000)

		assertTrue(s.address24Bit)
		assertEquals(0, s.jitCacheSize)
		assertEquals(0, s.fpuModel)
	}

	@Test
	fun `choosing cycle-exact turns off immediate blits and the JIT`() {
		val s = EmulatorSettings(
			cpuModel = 68020, address24Bit = false, cpuSpeed = "max", cpuCompatible = false,
			immediateBlits = true, jitCacheSize = 8192
		).withCycleExact("memory")

		assertEquals("memory", s.cycleExact)
		assertEquals("real", s.cpuSpeed)
		assertTrue(s.cpuCompatible)
		assertFalse(s.immediateBlits)
		assertEquals(0, s.jitCacheSize)
		// A 68000 has no DMA-only mode.
		assertEquals("true", EmulatorSettings(cpuModel = 68000).withCycleExact("memory").cycleExact)
	}

	@Test
	fun `turning the JIT on leaves cycle-exact mode`() {
		val s = EmulatorSettings(cpuModel = 68020, address24Bit = false, cycleExact = "true", jitFpu = false)
			.withJitCacheSize(8192)

		assertEquals("false", s.cycleExact)
		assertTrue(s.jitFpu)
	}
}
