package com.blitterstudio.amiberry.data.model

import com.blitterstudio.amiberry.data.ConfigGenerator
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsSchemaTest {

	private val schema = requireNotNull(SettingsSchema.current) { "the bundled settings schema did not load" }

	/** Keys the launcher writes that are not options of the emulator's settings layer. */
	private val launcherOnlyKeys = setOf(
		"amiberry.android_joyport1",
		"amiberry.android_preset",
		"amiberry.gfx_auto_crop",
		"amiberry.shader",
		"gfx_width",
		"gfx_height",
		"hardfile2",
		"input.default_osk",
		"use_gui"
	)

	@Test
	fun `every model has a configuration that names its CPU and chipset`() {
		assertTrue(AmigaModel.entries.isNotEmpty())
		for (model in AmigaModel.entries) {
			assertTrue(model.cmdArg, model.defaultCpu.isNotEmpty() && model.chipset.isNotEmpty())
			assertTrue(model.cmdArg, model.configs.all { it.romSets.isNotEmpty() })
		}
	}

	@Test
	fun `stored models from before presets still resolve`() {
		assertEquals("a500plus", AmigaModel.find("A500P")?.cmdArg)
		assertEquals("a500", AmigaModel.find("A2000")?.cmdArg)
		assertEquals("cd32", AmigaModel.find("CD32")?.cmdArg)
		assertEquals("a1200", AmigaModel.find("a1200")?.cmdArg)
		assertEquals(null, AmigaModel.find("A9000"))
	}

	@Test
	fun `generated configurations use the emulator's keys and values`() {
		val samples = AmigaModel.entries.map { EmulatorSettingsConstraints.apply(EmulatorSettings.fromModel(it), hasTouchScreen = true) } +
			EmulatorSettings()
		for (settings in samples) {
			for (line in ConfigGenerator.generate(settings).lines()) {
				if (line.isBlank() || line.startsWith(";")) continue
				val key = line.substringBefore('=')
				val value = line.substringAfter('=')
				if (key in launcherOnlyKeys) continue
				val option = schema.options[key]
				assertNotNull("${settings.baseModel}: '$key' is not an option of the settings layer", option)
				option!!
				when (option.type) {
					"bool" -> assertTrue("$key=$value", value == "true" || value == "false")
					"choice" -> if (option.choices.isNotEmpty())
						assertTrue("${settings.baseModel}: $key=$value is not a choice", option.choices.any { it.value == value })
					"int", "real" -> {
						val number = value.toDoubleOrNull()
						assertNotNull("$key=$value", number)
						assertTrue("$key=$value is out of range", number!! >= option.min!! && number <= option.max!!)
					}
				}
			}
		}
	}

	@Test
	fun `the arcade boards are not offered`() {
		assertEquals(null, AmigaModel.find("alg"))
		assertEquals(null, AmigaModel.find("arcadia"))
	}

	@Test
	fun `the preset configuration survives a save and reload`() {
		val file = java.io.File.createTempFile("preset", ".uae")
		try {
			file.writeText(ConfigGenerator.generate(EmulatorSettings.fromModel(AmigaModel.A3000, 2)))
			val parsed = com.blitterstudio.amiberry.data.ConfigParser.parse(file).settings
			assertEquals(AmigaModel.A3000, parsed.baseModel)
			assertEquals(2, parsed.baseConfig)
		} finally {
			file.delete()
		}
	}

	@Test
	fun `ROM requests name ROMs the schema describes`() {
		for (model in AmigaModel.entries) for (config in model.configs) for (set in config.romSets) for (request in set) {
			assertTrue("${model.cmdArg}: ${request.ids}", request.ids.any { it in schema.roms })
		}
	}
}
