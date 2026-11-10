package com.blitterstudio.amiberry.data.model

import org.json.JSONArray
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

/**
 * The custom control lines and the accelerator jumper codec: what a
 * configuration spells, and the pack/unpack of the one cpuboard_settings
 * line the launcher writes itself.
 */
class CustomControlsAndJumpersTest {

	private lateinit var schema: SettingsSchema

	@Before
	fun install() {
		val emptyTable = JSONObject().put("inputs", JSONArray()).put("free", JSONArray()).put("rows", JSONArray())
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
				.put("board_types", JSONArray())
				.put("custom_controls", JSONArray())
				.put("input_events", JSONArray())
				.put(
					"accelerator_boards", JSONArray()
						.put(
							JSONObject()
								.put("id", "Blizzard1230IV").put("label", "1230 IV (Blizzard)")
								.put("memory_max", 256)
								.put(
									"settings", JSONArray()
										.put(jumper("maprom", "switch"))
										.put(jumper("jscache", "selector", listOf("off", "on")))
										.put(jumper("clock", "switch", invert = true))
								)
						)
						.put(
							JSONObject()
								.put("id", "BlizzardPPC").put("label", "PPC (Blizzard)")
								.put("memory_max", 128)
								.put("requires", JSONArray().put("ppc"))
								.put("settings", JSONArray())
						)
				)
		)
		SettingsSchema.install(schema)
	}

	@After
	fun remove() {
		SettingsSchema.install(null)
	}

	private fun jumper(name: String, type: String, positions: List<String> = emptyList(), invert: Boolean = false) =
		JSONObject()
			.put("name", name).put("label", name).put("type", type).put("invert", invert)
			.put("positions", JSONArray(positions))
			.put("position_labels", JSONArray(positions))

	@Test
	fun `a custom control's line key spells its port, mode and control`() {
		val key = CustomControls.lineKey("joyport1_amiberry_custom_hotkey_south")!!
		assertEquals(1, key.port)
		assertEquals(true, key.hotkey)
		assertEquals(false, key.axis)
		assertEquals("south", key.control)
		assertEquals("joyport1_amiberry_custom_hotkey_south", key.id)

		val axis = CustomControls.lineKey("joyport0_amiberry_custom_axis_none_leftx")!!
		assertEquals(true, axis.axis)
		assertEquals(false, axis.hotkey)
		assertEquals("leftx", axis.control)
	}

	@Test
	fun `anything else is no custom control line`() {
		assertEquals(null, CustomControls.lineKey("joyport1"))
		assertEquals(null, CustomControls.lineKey("joyport1_amiberry_custom_maybe_south"))
		assertEquals(null, CustomControls.lineKey("cpu_model"))
	}

	@Test
	fun `the default maps nothing and writes no line`() {
		val controls = CustomControls()
		assertTrue(!controls.anyMapped)
		val key = CustomControls.Key(1, hotkey = false, axis = false, control = "south")
		assertEquals("", controls.event(key))
		// Set and cleared again leaves no mapping.
		assertEquals(CustomControls(), controls.with(key, "Joy2Fire").with(key, ""))
	}

	@Test
	fun `jumpers pack as the configuration's line spells them`() {
		val board = schema.acceleratorBoard("Blizzard1230IV")!!
		val values = mapOf("maprom" to "true", "jscache" to "on")
		assertEquals("maprom,on", AcceleratorJumpers.pack(board, values))
	}

	@Test
	fun `an inverted switch packs by its name, the selector at its first position`() {
		val board = schema.acceleratorBoard("Blizzard1230IV")!!
		// The native writer spells every selector once any jumper is set.
		assertEquals("off,clock", AcceleratorJumpers.pack(board, mapOf("clock" to "true")))
	}

	@Test
	fun `defaults pack to no line`() {
		val board = schema.acceleratorBoard("Blizzard1230IV")!!
		assertEquals("", AcceleratorJumpers.pack(board, mapOf("maprom" to "false", "jscache" to "off")))
		assertEquals("", AcceleratorJumpers.pack(board, emptyMap()))
	}

	@Test
	fun `a packed line unpacks to the jumpers it sets and the defaults it leaves`() {
		val board = schema.acceleratorBoard("Blizzard1230IV")!!
		val values = AcceleratorJumpers.unpack(board, "maprom,on")
		assertEquals("true", values["maprom"])
		assertEquals("on", values["jscache"])
		assertEquals("false", values["clock"])
		// An empty line holds no mapping: the jumpers read their defaults
		// by their absence, as the editor does.
		assertEquals(emptyMap<String, String>(), AcceleratorJumpers.unpack(board, ""))
	}

	@Test
	fun `round trip through the line keeps the values`() {
		val board = schema.acceleratorBoard("Blizzard1230IV")!!
		val values = mapOf("maprom" to "true", "jscache" to "on", "clock" to "true")
		assertEquals(values, AcceleratorJumpers.unpack(board, AcceleratorJumpers.pack(board, values)))
	}

	@Test
	fun `a board a build without its feature needs is marked`() {
		assertEquals(listOf("ppc"), schema.acceleratorBoard("BlizzardPPC")!!.requires)
		assertTrue(schema.acceleratorBoard("Blizzard1230IV")!!.requires.isEmpty())
	}
}
