package com.blitterstudio.amiberry.data.model

import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigParser
import org.json.JSONArray
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

/**
 * The boards, custom controls, accelerator jumpers and disk swapper
 * entries surviving a generate-parse round trip: what the editor writes is
 * what the emulator's own writer spells, and what a file has the editor
 * reads back.
 */
class BoardRoundTripTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private lateinit var schema: SettingsSchema

	@Before
	fun install() {
		val emptyTable = JSONObject().put("inputs", JSONArray()).put("free", JSONArray()).put("rows", JSONArray())
		val option = { id: String, group: String, type: String, default: String ->
			JSONObject().put("id", id).put("group", group).put("type", type).put("apply", "reset")
				.put("default", default)
		}
		schema = SettingsSchema.parse(
			JSONObject()
				.put(
					"options", JSONArray(
						listOf(
							option("cpuboard_type", "accelerator", "choice", "none"),
							option("cpuboard_settings", "accelerator", "string", ""),
							option("cpuboardmem1_size", "accelerator", "choice", "0"),
							option("cpuboard_rom_file", "accelerator", "path", ""),
							option("diskimage0", "diskswapper", "path", ""),
							option("diskimage1", "diskswapper", "path", ""),
							option("diskimage2", "diskswapper", "path", "")
						)
					)
				)
				.put(
					"presets", JSONArray(
						listOf(
							JSONObject()
								.put("id", "a500").put("label", "Amiga 500").put("compatibility_max", 0)
								.put(
									"configs", JSONArray(
										listOf(
											JSONObject().put("label", "A500")
												.put("rom_sets", JSONArray())
												.put("levels", JSONArray(listOf(JSONObject())))
										)
									)
								)
						)
					)
				)
				.put("compatibility_levels", JSONArray())
				.put("roms", JSONArray())
				.put("resolutions", emptyTable)
				.put("memory_resolutions", emptyTable)
				.put(
					"board_types", JSONArray(
						listOf(
							JSONObject()
								.put("id", "a2091").put("label", "A590/A2091").put("manufacturer", "Commodore")
								.put("categories", JSONArray(listOf("scsi"))).put("units", 4)
								.put(
									"fields", JSONArray(
										listOf(
											JSONObject().put("id", "enabled").put("label", "Enabled")
												.put("type", "bool").put("apply", "restart").put("default", "false"),
											JSONObject().put("id", "subtype").put("label", "Variant")
												.put("type", "choice").put("apply", "restart").put("default", "a590"),
											JSONObject().put("id", "rom_file").put("label", "ROM image")
												.put("type", "path").put("apply", "restart").put("default", ""),
											JSONObject().put("id", "scsi_id").put("label", "SCSI ID")
												.put("type", "choice").put("apply", "restart").put("default", "7"),
											JSONObject().put("id", "autoboot_disabled").put("label", "Disable autoboot")
												.put("type", "bool").put("apply", "restart").put("default", "false"),
											JSONObject().put("id", "setting:model").put("label", "Model")
												.put("type", "choice").put("apply", "restart").put("default", "mk1")
												.put("choices", JSONArray(
													listOf(JSONObject().put("value", "mk1").put("label", "MK I"),
														JSONObject().put("value", "mk2").put("label", "MK II"))
												))
										)
									)
								)
						)
					)
				)
				.put(
					"custom_controls", JSONArray(
						listOf(
							JSONObject().put("id", "south").put("label", "South").put("axis", false),
							JSONObject().put("id", "east").put("label", "East").put("axis", false),
							JSONObject().put("id", "righttrigger").put("label", "Right trigger").put("axis", true)
						)
					)
				)
				.put("input_events", JSONArray())
				.put(
					"accelerator_boards", JSONArray(
						listOf(
							JSONObject()
								.put("id", "Blizzard1230IV").put("label", "1230 IV (Blizzard)")
								.put("memory_max", 256)
								.put(
									"settings", JSONArray(
										listOf(
											JSONObject().put("name", "maprom").put("label", "Maprom")
												.put("type", "switch").put("invert", false)
										)
									)
								)
						)
					)
				)
		)
		SettingsSchema.install(schema)
	}

	@After
	fun remove() {
		SettingsSchema.install(null)
	}

	private fun roundTrip(settings: EmulatorSettings): ConfigParser.ParsedConfig {
		val file = tempDir.newFile("round.uae")
		file.writeText(ConfigGenerator.generate(settings))
		return ConfigParser.parse(file)
	}

	@Test
	fun `a board survives a save and a load`() {
		var settings = EmulatorSettings()
		settings = BoardEditor.add(settings, schema, "a2091")
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "subtype", "a2091")
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "scsi_id", "6")
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "autoboot_disabled", "true")
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "rom_file", "/roms/a2091.rom")

		val text = ConfigGenerator.generate(settings)
		assertTrue("a2091_rom_file=/roms/a2091.rom" in text.lines())
		assertTrue("a2091_rom_options=subtype=a2091,id=6,autoboot_disabled=true" in text.lines())

		val loaded = roundTrip(settings)
		assertEquals(settings.boards, loaded.settings.boards)
		assertTrue(loaded.unknownLines.isEmpty())
	}

	@Test
	fun `custom controls write their lines and read back`() {
		val south = CustomControls.Key(1, hotkey = false, axis = false, control = "south")
		val trigger = CustomControls.Key(1, hotkey = true, axis = true, control = "righttrigger")
		var settings = EmulatorSettings()
		settings = settings.copy(customControls = settings.customControls
			.with(south, "Joy2Fire")
			.with(trigger, "none"))

		val text = ConfigGenerator.generate(settings)
		assertTrue("joyport1_amiberry_custom_none_south=Joy2Fire" in text.lines())
		assertTrue("joyport1_amiberry_custom_axis_hotkey_righttrigger=none" in text.lines())

		val loaded = roundTrip(settings)
		assertEquals(settings.customControls, loaded.settings.customControls)
	}

	@Test
	fun `the jumpers pack into their line and unpack again`() {
		var settings = EmulatorSettings().withOption("cpuboard_type", "Blizzard1230IV")
		settings = settings.copy(acceleratorJumpers = mapOf("maprom" to "true"))
		val text = ConfigGenerator.generate(settings)
		assertTrue("cpuboard_settings=maprom" in text.lines())

		val loaded = roundTrip(settings)
		assertEquals(mapOf("maprom" to "true"), loaded.settings.acceleratorJumpers)
	}

	@Test
	fun `a board the schema does not know keeps its lines`() {
		val file = tempDir.newFile("unknown.uae")
		file.writeText(
			"""
			; test
			mysteryboard_rom_file=/roms/mystery.rom
			mysteryboard_rom_options=id=3
			""".trimIndent()
		)
		val parsed = ConfigParser.parse(file)
		assertEquals(emptyList<BoardUnit>(), parsed.settings.boards)
		assertEquals(
			listOf("mysteryboard_rom_file=/roms/mystery.rom", "mysteryboard_rom_options=id=3"),
			parsed.unknownLines
		)
	}

	@Test
	fun `the board identity lines are dropped for a unit the codec owns`() {
		val file = tempDir.newFile("identity.uae")
		file.writeText(
			"""
			a2091_rom_file=:NOROM
			a2091_rom=A590 ROM
			a2091_rom_file_id=12345678,A590 ROM
			""".trimIndent()
		)
		val parsed = ConfigParser.parse(file)
		assertEquals(1, parsed.settings.boards.size)
		assertTrue(parsed.unknownLines.isEmpty())
	}

	@Test
	fun `the board's selector setting survives as its position token`() {
		var settings = EmulatorSettings()
		settings = BoardEditor.add(settings, schema, "a2091")
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "setting:model", "mk2")
		val text = ConfigGenerator.generate(settings)
		// The reader looks for the position's token, not the field's name
		// (cfgfile_read_rom_settings, cfgfile.cpp:1722-1725).
		assertTrue(text, "a2091_rom_options=mk2" in text.lines())
		val loaded = roundTrip(settings)
		assertEquals(mapOf("setting:model" to "mk2"), loaded.settings.boards[0].fields)
	}

	@Test
	fun `the accelerator's jumpers line follows the board that owns them`() {
		var settings = EmulatorSettings().withOption("cpuboard_type", "Blizzard1230IV")
		settings = settings.copy(acceleratorJumpers = mapOf("maprom" to "true"))
		val lines = ConfigGenerator.generate(settings).lines()
		// cfgfile.cpp:7077-7082 reads the packed line against the selected
		// board, so the type must come first.
		val type = lines.indexOfFirst { it.startsWith("cpuboard_type=") }
		val jumpers = lines.indexOfFirst { it.startsWith("cpuboard_settings=") }
		assertTrue(type in lines.indices)
		assertEquals(type + 1, jumpers)
		// And the type is written exactly once.
		assertEquals(1, lines.count { it.startsWith("cpuboard_type=") })
	}

	@Test
	fun `a control set back to Default resets what the port had`() {
		val south = CustomControls.Key(1, hotkey = false, axis = false, control = "south")
		val east = CustomControls.Key(1, hotkey = false, axis = false, control = "east")
		var settings = EmulatorSettings()
		settings = settings.copy(customControls = settings.customControls.with(east, "Joy2Fire"))

		val lines = ConfigGenerator.generate(settings).lines()
		// The writer spells every control of a mapped port and mode, an
		// empty value included (cfgfile.cpp:2847-2880): the reader applies
		// an empty line as the slot's default (custom_event_of,
		// amiberry_input.cpp:1790), so a binding an earlier file set on the
		// port does not survive a Default.
		assertTrue("joyport1_amiberry_custom_none_south=" in lines)
		assertTrue("joyport1_amiberry_custom_none_east=Joy2Fire" in lines)
		// The hotkey mode the user has not mapped writes nothing.
		assertTrue(lines.none { it.startsWith("joyport1_amiberry_custom_hotkey_") })

		val loaded = roundTrip(settings)
		// The empty reset line is no mapping.
		assertEquals(settings.customControls, loaded.settings.customControls)
	}

	@Test
	fun `a board identified only by its ROM checksum keeps its identity line`() {
		val file = tempDir.newFile("crc.uae")
		file.writeText(
			"""
			a2091_rom_file_id=12345678,A590 ROM
			a2091_rom=A590 ROM
			""".trimIndent()
		)
		val parsed = ConfigParser.parse(file)
		// The codec owns no unit (no entry, no options), so the identity
		// lines survive for the reader to resolve (cfgfile_rom,
		// cfgfile.cpp:6393-6399).
		assertEquals(emptyList<BoardUnit>(), parsed.settings.boards)
		assertEquals(
			listOf("a2091_rom_file_id=12345678,A590 ROM", "a2091_rom=A590 ROM"),
			parsed.unknownLines
		)
	}

	@Test
	fun `the disk swapper entries round trip through the options`() {
		val settings = EmulatorSettings()
			.withOption("diskimage0", "/disks/game1.adf")
			.withOption("diskimage2", "/disks/game2.adf")
		val loaded = roundTrip(settings)
		assertEquals("/disks/game1.adf", loaded.settings.optionValue("diskimage0"))
		assertEquals("/disks/game2.adf", loaded.settings.optionValue("diskimage2"))
		assertEquals(null, loaded.settings.optionValue("diskimage1"))
	}
}
