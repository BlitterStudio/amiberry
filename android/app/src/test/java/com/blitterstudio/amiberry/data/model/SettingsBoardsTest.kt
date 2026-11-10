package com.blitterstudio.amiberry.data.model

import org.json.JSONArray
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

/**
 * The expansion board codec (SettingsBoards) and the editor logic on top
 * (BoardEditor): what the configuration's own lines spell, parsed back and
 * written again exactly as the emulator's writer does.
 */
class SettingsBoardsTest {

	private lateinit var schema: SettingsSchema

	@Before
	fun install() {
		val emptyTable = JSONObject().put("inputs", JSONArray()).put("free", JSONArray()).put("rows", JSONArray())
		val a2091 = JSONObject()
			.put("id", "a2091").put("label", "A590/A2091").put("manufacturer", "Commodore")
			.put("categories", JSONArray().put("scsi")).put("units", 4)
			.put(
				"fields", JSONArray()
					.put(field("enabled", "bool", "false"))
					.put(
						field("subtype", "choice", "a590")
							.put("choices", JSONArray().put(choice("a590", "A590")).put(choice("a2091", "A2091")))
					)
					.put(field("rom_file", "path", ""))
					.put(
						field("scsi_id", "choice", "7")
							.put("choices", JSONArray().put(choice("0", "0")).put(choice("7", "7")))
					)
					.put(field("autoboot_disabled", "bool", "false"))
					.put(field("dma24bit", "bool", "false"))
					.put(field("setting:dma", "bool", "false", "dma"))
			)
		val pcmcia = JSONObject()
			.put("id", "pcmcia_mb").put("label", "PCMCIA").put("manufacturer", null)
			.put("categories", JSONArray().put("internal")).put("units", 1).put("option", "pcmcia")
			.put("fields", JSONArray().put(field("enabled", "bool", "false")).put(field("rom_file", "path", "")))
		val xsurf = JSONObject()
			.put("id", "xsurf").put("label", "X-Surf").put("manufacturer", "Individual Computers")
			.put("categories", JSONArray().put("network")).put("units", 1)
			.put(
				"fields", JSONArray()
					.put(field("enabled", "bool", "false"))
					.put(field("rom_file", "path", ""))
					.put(field("setting:netmode", "choice", "slirp", "netmode"))
			)
		schema = SettingsSchema.parse(
			JSONObject()
				.put("options", JSONArray())
				.put(
					"presets", JSONArray(
						listOf(
							JSONObject()
								.put("id", "a500").put("label", "Amiga 500").put("compatibility_max", 0)
								.put(
									"configs", JSONArray(
										listOf(
											JSONObject()
												.put("label", "A500")
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
				.put("board_types", JSONArray().put(a2091).put(pcmcia).put(xsurf))
				.put("custom_controls", JSONArray())
				.put("input_events", JSONArray())
				.put("accelerator_boards", JSONArray())
		)
		SettingsSchema.install(schema)
	}

	@After
	fun remove() {
		SettingsSchema.install(null)
	}

	private fun field(id: String, type: String, default: String, token: String = id) = JSONObject()
		.put("id", id).put("label", id).put("help", "").put("type", type).put("apply", "restart")
		.put("default", default)

	private fun choice(value: String, label: String) = JSONObject().put("value", value).put("label", label)

	private fun parse(vararg lines: String) =
		SettingsBoards.parse(schema, lines.associate { it.substringBefore("=") to it.substringAfter("=") })

	@Test
	fun `a unit parses from its entry and options lines`() {
		val units = parse(
			"a2091_rom_file=:NOROM",
			"a2091_rom_options=subtype=a2091,id=7,autoboot_disabled=true"
		)
		assertEquals(1, units.size)
		assertEquals(BoardUnit("a2091", 0, "", true, mapOf(
			"subtype" to "a2091", "scsi_id" to "7", "autoboot_disabled" to "true"
		)), units[0])
	}

	@Test
	fun `a second unit's lines carry its number`() {
		val units = parse("a2091-2_rom_file=:NOROM")
		assertEquals(1, units.size)
		assertEquals("a2091", units[0].typeId)
		assertEquals(1, units[0].unit)
	}

	@Test
	fun `an options-only unit is a built-in controller without a ROM`() {
		val units = parse("pcmcia_mb_rom_options=inserted=true")
		assertEquals(1, units.size)
		assertEquals(false, units[0].entry)
		assertEquals("", units[0].romFile)
	}

	@Test
	fun `a bare setting token switches on and its absence is the default`() {
		val units = parse("a2091_rom_file=:NOROM", "a2091_rom_options=dma")
		assertEquals("true", units[0].fields["setting:dma"])
		val off = parse("a2091_rom_file=:NOROM")
		assertNull(off[0].fields["setting:dma"])
	}

	@Test
	fun `tokens no field of the type takes ride along`() {
		val units = parse("xsurf_rom_file=:NOROM", "xsurf_rom_options=netmode=slirp")
		assertEquals(listOf("netmode=slirp"), units[0].extras)
		// ... and stay when the unit is written back.
		assertEquals(
			listOf("xsurf_rom_file=:NOROM", "xsurf_rom_options=netmode=slirp"),
			SettingsBoards.render(schema, units)
		)
	}

	@Test
	fun `units render to the writer's lines`() {
		val units = parse(
			"a2091_rom_file=:NOROM",
			"a2091_rom_options=subtype=a2091,id=7,autoboot_disabled=true"
		)
		assertEquals(
			listOf("a2091_rom_file=:NOROM", "a2091_rom_options=subtype=a2091,id=7,autoboot_disabled=true"),
			SettingsBoards.render(schema, units)
		)
	}

	@Test
	fun `defaults leave no options line`() {
		val units = parse("a2091_rom_file=:NOROM")
		assertEquals(listOf("a2091_rom_file=:NOROM"), SettingsBoards.render(schema, units))
	}

	@Test
	fun `a unit a removed type had writes nothing`() {
		assertTrue(SettingsBoards.render(schema, listOf(BoardUnit("gone", 0, "", true))).isEmpty())
	}

	@Test
	fun `adding a board writes its entry with the marker of a ROM board`() {
		var settings = EmulatorSettings()
		settings = BoardEditor.add(settings, schema, "a2091")
		assertEquals(1, settings.boards.size)
		assertEquals(listOf("a2091_rom_file=:NOROM"), SettingsBoards.render(schema, settings.boards))
	}

	@Test
	fun `a board of a full type goes to the first free unit`() {
		var settings = EmulatorSettings()
		settings = BoardEditor.add(settings, schema, "a2091")
		settings = BoardEditor.add(settings, schema, "a2091")
		assertEquals(
			listOf("a2091_rom_file=:NOROM", "a2091-2_rom_file=:NOROM"),
			SettingsBoards.render(schema, settings.boards)
		)
	}

	@Test
	fun `a field set and cleared returns to the default`() {
		var settings = BoardEditor.add(EmulatorSettings(), schema, "a2091")
		val unit = settings.boards[0]
		settings = BoardEditor.setField(settings, schema, unit, "autoboot_disabled", "true")
		assertEquals("true", settings.boards[0].fields["autoboot_disabled"])
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "autoboot_disabled", "false")
		assertNull(settings.boards[0].fields["autoboot_disabled"])
	}

	@Test
	fun `the built-in boards list while their option is on and go through it`() {
		var settings = EmulatorSettings().withOption("pcmcia", "true")
		assertEquals(1, BoardEditor.units(schema, settings).size)
		// Removing it switches the option off, with the layer's rules.
		settings = BoardEditor.remove(settings, schema, BoardEditor.units(schema, settings)[0])
		assertTrue(BoardEditor.units(schema, settings).isEmpty())
	}

	@Test
	fun `an untouched built-in board writes no lines`() {
		val settings = EmulatorSettings().withOption("pcmcia", "true")
		assertTrue(SettingsBoards.render(schema, settings.boards).isEmpty())
	}

	@Test
	fun `a built-in board edited writes its options line but no entry`() {
		var settings = EmulatorSettings().withOption("pcmcia", "true")
		val unit = BoardEditor.units(schema, settings)[0]
		settings = BoardEditor.setField(settings, schema, unit, "rom_file", "/roms/pcmcia.rom")
		assertEquals(
			listOf("pcmcia_mb_rom_file=/roms/pcmcia.rom"),
			SettingsBoards.render(schema, settings.boards)
		)
	}

	@Test
	fun `a ROM picked replaces the marker`() {
		var settings = BoardEditor.add(EmulatorSettings(), schema, "a2091")
		settings = BoardEditor.setField(settings, schema, settings.boards[0], "rom_file", "/roms/a2091.rom")
		assertEquals(listOf("a2091_rom_file=/roms/a2091.rom"), SettingsBoards.render(schema, settings.boards))
	}

	@Test
	fun `the identity lines of known boards are recognised for the parser to drop`() {
		assertEquals(SettingsBoards.Line.Kind.Drop, SettingsBoards.line("a2091_rom")?.kind)
		assertEquals(SettingsBoards.Line.Kind.Drop, SettingsBoards.line("a2091_rom_file_id")?.kind)
		assertEquals(SettingsBoards.Line.Kind.Drop, SettingsBoards.line("scsi_a2091")?.kind)
		assertEquals(SettingsBoards.Line.Kind.Entry, SettingsBoards.line("a2091-3_rom_file")?.kind)
		assertNull(SettingsBoards.line("unknown_rom_file"))
	}

	@Test
	fun `text settings escape what the line needs`() {
		assertEquals("plain", SettingsBoards.escapeMin("plain"))
		assertEquals("\"a,b\"", SettingsBoards.escapeMin("a,b"))
		assertEquals("\" lead\"", SettingsBoards.escapeMin(" lead"))
		assertEquals("\"e\\\"\"", SettingsBoards.escapeMin("e\""))
	}
}
