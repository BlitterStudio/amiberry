package com.blitterstudio.amiberry.data.model

import com.blitterstudio.amiberry.data.AndroidLaunchConfig
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.ConfigSettingsResolver
import com.blitterstudio.amiberry.ui.screens.settings.SettingsControlAvailability
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

/**
 * The launcher writes the whole machine a preset builds: every compatibility
 * level's values, the schema options without a field kept in
 * EmulatorSettings.options through a configuration file and back.
 */
class PresetMachineTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private val schema = requireNotNull(SettingsSchema.current)

	/** The key=value lines of a generated configuration (the last line of a key wins, as in the emulator). */
	private fun written(settings: EmulatorSettings): Map<String, String> =
		ConfigGenerator.generate(settings).lines()
			.filter { '=' in it && !it.startsWith(";") }
			.associate { it.substringBefore('=') to it.substringAfter('=') }

	private fun roundTrip(settings: EmulatorSettings, extra: String = ""): ConfigParser.ParsedConfig {
		val file = tempDir.newFile()
		file.writeText(ConfigGenerator.generate(settings) + extra)
		return ConfigParser.parse(file)
	}

	@Test
	fun `every preset level is written as that level's machine`() {
		for (model in AmigaModel.entries) {
			for (config in model.configs.indices) {
				for (level in 0..model.compatibilityMax) {
					val values = model.levelValues(config, level)
					val settings = EmulatorSettings.fromModel(model, config, level)
					val file = written(settings)
					val name = "${model.cmdArg}:$config:$level"
					assertEquals(name, level, settings.baseLevel)
					assertEquals(name, "${model.cmdArg}:$config:$level", file["amiberry.android_preset"])
					// The CD image is media the launcher supplies.
					for ((id, value) in values - "cdimage0") {
						assertEquals("$name $id", value, file[id])
					}
					// What a level leaves out has the emulator's default.
					for (id in listOf("cpu_model", "chipset", "cycle_exact", "compfpu", "cachesize", "floppy1type")) {
						if (id !in values) assertEquals("$name $id", schema.option(id)?.defaultOnAndroid, file[id])
					}
					assertEquals(name, level, settings.compatibilityLevel)
				}
			}
		}
	}

	@Test
	fun `every preset level reads back as that level after the launcher's rules and a configuration file`() {
		for (model in AmigaModel.entries) {
			for (config in model.configs.indices) {
				for (level in 0..model.compatibilityMax) {
					val name = "${model.cmdArg}:$config:$level"
					for (touch in listOf(true, false)) {
						val settings = EmulatorSettingsConstraints.apply(
							EmulatorSettings.fromModel(model, config, level), hasTouchScreen = touch
						)
						assertEquals("$name touch=$touch", level, settings.compatibilityLevel)
						val reloaded = EmulatorSettingsConstraints.apply(roundTrip(settings).settings, hasTouchScreen = touch)
						assertEquals("$name touch=$touch reloaded", level, reloaded.compatibilityLevel)
					}
				}
			}
		}
	}

	@Test
	fun `settings without a configuration are the A500 at its default level`() {
		val defaults = ConfigSettingsResolver.defaults(null)
		assertEquals(AmigaModel.A500, defaults.baseModel)
		assertEquals(AmigaModel.A500.defaultLevel, defaults.compatibilityLevel)
	}

	@Test
	fun `a preset record without a level brings level 0's machine and the file's keys win`() {
		// Written before levels: the launch applied the preset, the file has only the fields.
		val file = tempDir.newFile("old.uae")
		file.writeText("amiberry.android_preset=cd32:0\ncpu_model=68020\nchipset=aga\ncd32nvram=false\n")
		val parsed = ConfigParser.parse(file)
		val generated = written(parsed.settings)
		assertEquals("true", generated["cd32cd"])
		assertEquals("true", generated["cd32c2p"])
		assertEquals("CD32", generated["chipset_compatible"])
		assertEquals("false", generated["cd32nvram"])
		// What the record filled in is not what the file set.
		assertFalse("cd32cd" in parsed.explicitKeys)
		assertTrue("cd32nvram" in parsed.explicitKeys)

		// A record with a level wrote its whole machine: nothing is added.
		file.writeText("amiberry.android_preset=cd32:0:0\ncpu_model=68020\n")
		assertNull(ConfigParser.parse(file).settings.options["cd32cd"])
	}

	@Test
	fun `a model change keeps the input and on-screen options the new preset does not set`() {
		val played = EmulatorSettings.fromModel(AmigaModel.A500)
			.withOption("amiberry.vkbd_transparency", "40")
			.withOption("input.joystick_deadzone", "20")
			.withOption("joyport1mode", "djoy")
		val cd32 = played.withModel(AmigaModel.CD32, 0)
		assertEquals("40", cd32.options["amiberry.vkbd_transparency"])
		assertEquals("20", cd32.options["input.joystick_deadzone"])
		// The CD32 preset sets its pad mode; the machine's own values replace the A500's.
		assertEquals("cd32joy", cd32.options["joyport1mode"])
		assertEquals("CD32", cd32.options["chipset_compatible"])
		assertEquals(AmigaModel.CD32.defaultLevel, cd32.compatibilityLevel)

		// Quick Start builds its launch the same way.
		val snapshot = AndroidLaunchConfig.buildQuickStartSettingsSnapshot(
			AmigaModel.CD32, emptyList(), played, "", "", ""
		)
		assertEquals("40", snapshot.options["amiberry.vkbd_transparency"])
		assertEquals("cd32joy", snapshot.options["joyport1mode"])
	}

	@Test
	fun `a CPU change resets the MMU and sets the multiplier as the emulator does`() {
		val a3000 = EmulatorSettings.fromModel(AmigaModel.A3000, 0, 0)
		assertEquals("68030", a3000.optionValue("mmu_model"))
		assertNull(a3000.withOption("cpu_model", "68040").options["mmu_model"])

		// More compatible: a 68020 runs at 4x the chipset clock.
		val a500 = EmulatorSettings.fromModel(AmigaModel.A500)
		assertTrue(a500.cpuCompatible)
		assertEquals("4", a500.withOption("cpu_model", "68020").optionValue("cpu_multiplier"))
	}

	@Test
	fun `picking a sub-1 MB fast RAM size clears the MB size`() {
		val small = EmulatorSettings(fastRam = 2).withOption("fastmem_size_k", "256")
		assertEquals(0, small.fastRam)
		assertEquals("256", small.options["fastmem_size_k"])
	}

	@Test
	fun `genlock sources that need video capture are not offered`() {
		assertEquals(listOf("none", "noise", "testcard", "image"), schema.option("genlockmode")!!.choices.map { it.value })
	}

	@Test
	fun `a CD32 keeps its NVRAM in the emulator's NVRAM directory`() {
		// Spelled relative to the content base, which the core resolves to its NVRAM directory.
		val cd32 = EmulatorSettings.fromModel(AmigaModel.CD32)
		assertEquals("./NVRAM/cd32.nvr", written(cd32)["flash_file"])
		assertEquals("./NVRAM/cd32.nvr", roundTrip(cd32).settings.options["flash_file"])
		assertEquals("./NVRAM/cdtv.nvr", written(EmulatorSettings.fromModel(AmigaModel.CDTV))["flash_file"])
	}

	@Test
	fun `a fast RAM board's MB and KB keys are one size`() {
		// A file with both keys: the later KB line wins, so the board is 256 KB.
		val both = roundTrip(EmulatorSettings(), "fastmem_size=4\nfastmem_size_k=256\n")
		assertEquals(256 * 1024, installedKb(both.settings, "fastmem_size", "fastmem_size_k"))

		// Picking 4 MB clears the KB key: the file the launcher writes has one line.
		val fourMb = both.settings.withOption("fastmem_size", "4")
		assertNull(fourMb.options["fastmem_size_k"])
		val file = written(fourMb)
		assertEquals("4", file["fastmem_size"])
		assertNull(file["fastmem_size_k"])
		assertEquals(4 * 1024 * 1024, installedKb(ConfigParser.parse(tempDir.newFile().also { it.writeText(ConfigGenerator.generate(fourMb)) }).settings, "fastmem_size", "fastmem_size_k"))

		// Picking a sub-1 MB size clears the MB field's board and stores the KB key alone.
		val small = fourMb.withOption("fastmem2_size_k", "512")
		assertEquals("512", small.options["fastmem2_size_k"])
		assertNull(small.options["fastmem2_size"])
		val smallFile = written(small)
		// cfgfile saves a sub-1 MB board under the KB key alone, and so does the launcher.
		assertNull(smallFile["fastmem2_size"])
		assertEquals("512", smallFile["fastmem2_size_k"])

		// Clearing the KB size leaves the board without either key.
		val cleared = small.withOption("fastmem2_size_k", "0")
		assertNull(cleared.options["fastmem2_size_k"])
		assertNull(written(cleared)["fastmem2_size_k"])
	}

	/** The KB size the configuration files give a fast RAM board. */
	private fun installedKb(settings: EmulatorSettings, mb: String, kb: String): Int {
		val lines = written(settings)
		return when {
			kb in lines -> lines[kb]!!.toInt() * 1024
			lines[mb]?.toIntOrNull()?.let { it > 0 } == true -> lines[mb]!!.toInt() * 1024 * 1024
			else -> 0
		}
	}

	@Test
	fun `the machine a level sets without a field is written after the fields in the layer's order`() {
		val lines = ConfigGenerator.generate(EmulatorSettings.fromModel(AmigaModel.A3000, 0, 0)).lines()
		fun line(key: String) = lines.indexOfFirst { it.startsWith("$key=") }
		assertTrue(line("chipset_compatible") > line("cpu_model"))
		assertTrue(line("ramsey") > line("chipset_compatible"))
		assertTrue(lines.contains("a3000mem_size=8"))
		assertTrue(lines.contains("mmu_model=68030"))
	}

	@Test
	fun `CD32 writes its Akiko hardware`() {
		val file = written(EmulatorSettings.fromModel(AmigaModel.CD32))
		assertEquals("CD32", file["chipset_compatible"])
		assertEquals("true", file["cd32cd"])
		assertEquals("true", file["cd32c2p"])
		assertEquals("true", file["cd32nvram"])
	}

	@Test
	fun `options survive a configuration file and the last session`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A3000, 0, 0)
			.withOption("genlock", "true")
		assertEquals("true", settings.options["genlock"])

		val parsed = roundTrip(settings)

		assertEquals(settings.options, parsed.settings.options)
		assertEquals(emptyList<String>(), parsed.unknownLines)
		assertTrue("ramsey" in parsed.explicitKeys)
		assertEquals(0, parsed.settings.baseLevel)
		// Generating the parsed settings again writes the same file.
		assertEquals(ConfigGenerator.generate(settings), ConfigGenerator.generate(parsed.settings))
	}

	@Test
	fun `a schema option in a configuration goes to the options, other keys stay preserved lines`() {
		val parsed = roundTrip(EmulatorSettings(), "chipset_compatible=A1200\nnot_a_layer_option=1\n")

		assertEquals("A1200", parsed.settings.options["chipset_compatible"])
		assertEquals("A1200", parsed.settings.optionValue("chipset_compatible"))
		assertEquals(listOf("not_a_layer_option=1"), parsed.unknownLines)
	}

	@Test
	fun `a preset record without a level reads as level 0, which those configurations were written from`() {
		val file = tempDir.newFile()
		file.writeText("amiberry.android_preset=a1200:0\ncpu_model=68020\n")

		assertEquals(0, ConfigParser.parse(file).settings.baseLevel)
	}

	@Test
	fun `a model change replaces the previous machine's options and keeps the controls`() {
		val a3000 = EmulatorSettings.fromModel(AmigaModel.A3000)
			.copy(joyport1 = "joy1", onScreenJoystick = false, shader = "crt")
			.withOption("genlock", "true")

		val a500 = a3000.withModel(AmigaModel.A500, 0)

		assertEquals(AmigaModel.A500.levelValues(0, AmigaModel.A500.defaultLevel).filterKeys { it !in EmulatorSettings.FIELD_IDS }, a500.options)
		assertNull(a500.options["a3000mem_size"])
		assertNull(a500.options["genlock"])
		assertEquals("A500", a500.optionValue("chipset_compatible"))
		assertEquals("joy1", a500.joyport1)
		assertFalse(a500.onScreenJoystick)
		assertEquals("crt", a500.shader)
		assertFalse(written(a500).containsKey("ramsey"))
	}

	@Test
	fun `a compatibility level is selected while its values hold`() {
		val a1200 = EmulatorSettings.fromModel(AmigaModel.A1200)
		assertEquals(1, a1200.compatibilityLevel)

		val jit = a1200.withCompatibilityLevel(4)
		assertEquals(4, jit.compatibilityLevel)
		assertEquals(4, jit.baseLevel)
		assertEquals(16384, jit.jitCacheSize)
		assertNull(jit.options["cpu_multiplier"])

		val exact = jit.withCompatibilityLevel(0)
		assertEquals(0, exact.compatibilityLevel)
		assertEquals("true", exact.cycleExact)

		// An option the levels do not tell apart keeps the level; one they do ends it.
		assertEquals(4, jit.copy(fastRam = 8).compatibilityLevel)
		assertNull(jit.copy(cpuSpeed = "real").compatibilityLevel)
	}

	@Test
	fun `memory sizes are narrowed for an A1200 with the JIT`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A1200, 0, 4)
		assertTrue(settings.jitCacheSize > 0)

		val chip = requireNotNull(SettingsControlAvailability.choiceValues(settings, "chipmem_size"))
		val slow = requireNotNull(SettingsControlAvailability.choiceValues(settings, "bogomem_size"))

		// 1.5 MB is not available with the JIT; Gayle rules out 1.8 MB slow RAM.
		assertFalse("3" in chip)
		assertTrue("4" in chip)
		assertFalse("6" in slow)
		assertFalse("7" in slow)
		assertTrue("4" in slow)
	}

	@Test
	fun `memory sizes are narrowed for an A3000`() {
		val settings = EmulatorSettings.fromModel(AmigaModel.A3000, 0, 0)
		assertEquals(0, settings.jitCacheSize)

		val chip = requireNotNull(SettingsControlAvailability.choiceValues(settings, "chipmem_size"))
		val slow = requireNotNull(SettingsControlAvailability.choiceValues(settings, "bogomem_size"))

		// Gary and Ramsey rule out 1.8 MB slow RAM; without the JIT 1.5 MB stays.
		assertTrue("3" in chip)
		assertTrue("6" in slow)
		assertFalse("7" in slow)
	}

	@Test
	fun `memory sizes are all offered without the chipset extra`() {
		val all = SettingsSchema.current!!.option("bogomem_size")!!.choices.map { it.value }
		assertEquals(all, SettingsControlAvailability.choiceValues(EmulatorSettings(), "bogomem_size"))
	}
}
