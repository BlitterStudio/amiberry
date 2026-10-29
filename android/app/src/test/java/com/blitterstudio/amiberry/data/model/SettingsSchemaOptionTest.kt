package com.blitterstudio.amiberry.data.model

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsSchemaOptionTest {

	private fun schemaOf(vararg options: JSONObject): SettingsSchema {
		val emptyTable = JSONObject().put("inputs", JSONArray()).put("free", JSONArray()).put("rows", JSONArray())
		return SettingsSchema.parse(
			JSONObject()
				.put("options", JSONArray(options.toList()))
				.put("presets", JSONArray())
				.put("compatibility_levels", JSONArray())
				.put("roms", JSONArray())
				.put("resolutions", emptyTable)
				.put("memory_resolutions", emptyTable)
		)
	}

	private fun option(id: String, type: String, default: String = "0") = JSONObject()
		.put("id", id)
		.put("group", "test")
		.put("label", "Label of $id")
		.put("help", "Help for $id.")
		.put("type", type)
		.put("apply", "live")
		.put("default", default)

	private fun choices(vararg pairs: Pair<String, String>) =
		JSONArray(pairs.map { (value, label) -> JSONObject().put("value", value).put("label", label) })

	private val volume = option("sound_volume", "int")
		.put("min", 0).put("max", 100).put("step", 1)
		.put("reversed", true).put("unit", "%").put("level", true)
		.put("constraint", "Needs sound emulation to be on.")

	private val gamma = option("gamma", "int")
		.put("min", -200).put("max", 200).put("step", 5).put("scale", 10)

	@Test
	fun `parses the layer's label, help, unit and constraint`() {
		val schema = schemaOf(volume, option("ntsc", "bool", "false"))
		val parsed = schema.option("sound_volume")!!

		assertEquals("Label of sound_volume", schema.label("sound_volume"))
		assertEquals("Help for sound_volume.", parsed.help)
		assertEquals("%", parsed.unit)
		assertEquals("Needs sound emulation to be on.", schema.constraint("sound_volume"))
		assertEquals(1.0, parsed.step!!, 0.0)
		assertTrue(parsed.reversed)
		assertEquals(1.0, parsed.scale, 0.0)

		val bool = schema.option("ntsc")!!
		assertNull(schema.constraint("ntsc"))
		assertNull(bool.unit)
		assertFalse(bool.reversed)
		assertNull(schema.label("unknown"))
	}

	@Test
	fun `a reversed volume shows 100 percent at full and stores attenuation`() {
		val schema = schemaOf(volume)
		val option = schema.option("sound_volume")!!

		assertEquals(100.0, option.shown(0.0), 0.0)
		assertEquals(80.0, option.shown(20.0), 0.0)
		assertEquals(20.0, option.stored(80.0), 0.0)
		assertEquals(0.0, option.stored(100.0), 0.0)
		assertEquals(0.0..100.0, option.shownRange)
		assertEquals("80%", option.numberText("20"))
	}

	@Test
	fun `a scaled number shows divided and stores on its step`() {
		val option = schemaOf(gamma).option("gamma")!!

		assertEquals(12.5, option.shown(125.0), 1e-9)
		assertEquals(125.0, option.stored(12.5), 0.0)
		// 12.34 is between steps of 0.5 (5 stored): it stores the nearest one.
		assertEquals(125.0, option.stored(12.34), 0.0)
		assertEquals(0.5, option.shownStep, 1e-9)
		assertEquals("12.5", option.shownText(option.shown(125.0)))
		assertEquals("-20", option.shownText(option.shown(-200.0)))
	}

	@Test
	fun `units follow the number`() {
		val width = option("gfx_width", "int").put("min", 0).put("max", 4096).put("step", 1).put("unit", "px")
		val cache = option("cachesize", "int").put("min", 0).put("max", 16384).put("step", 1).put("unit", "KB")
		val schema = schemaOf(width, cache)

		assertEquals("720 px", schema.option("gfx_width")!!.numberText("720"))
		assertEquals("4096 KB", schema.option("cachesize")!!.valueText("4096", "on", "off"))
	}

	@Test
	fun `on-off options read as switches only while they hold 0 or 1`() {
		val aspect = option("aspect", "int").put("min", 0).put("max", 1).put("step", 1)
		val wide = option("wide", "int").put("min", 0).put("max", 2).put("step", 1)
		val mouseMap = option("mousemap", "choice").put("choices", choices("0" to "None", "1" to "Left stick"))
		val disabledFirst = option("disabled", "choice").put("choices", choices("0" to "Disabled", "1" to "On"))
		val offLast = option("offlast", "choice").put("choices", choices("1" to "Enabled", "0" to "Disabled"))
		val notOff = option("auto", "choice").put("choices", choices("0" to "Auto", "1" to "Manual"))
		val three = option("three", "choice").put("choices", choices("0" to "Off", "1" to "On", "2" to "Auto"))
		val autoresolution = option("autores", "choice")
			.put("choices", choices("0" to "Disabled", "1" to "Always on"))
		val ntsc = option("ntsc", "bool", "false")
		val schema = schemaOf(aspect, wide, mouseMap, disabledFirst, offLast, notOff, three, autoresolution, ntsc)

		fun showsAsSwitch(id: String, value: String) = schema.option(id)!!.showsAsSwitch(value)
		assertTrue(showsAsSwitch("aspect", "1"))
		assertTrue(showsAsSwitch("aspect", "0"))
		assertFalse(showsAsSwitch("wide", "1"))
		assertTrue(showsAsSwitch("mousemap", "1"))
		assertTrue(showsAsSwitch("disabled", "0"))
		assertFalse("off comes first", showsAsSwitch("offlast", "0"))
		assertFalse(showsAsSwitch("auto", "0"))
		assertFalse(showsAsSwitch("three", "1"))
		assertTrue(schema.option("autores")!!.isSwitch)
		// A threshold copied in by hand is no on/off value: the option keeps its own control.
		assertFalse(showsAsSwitch("autores", "33"))
		assertTrue(showsAsSwitch("ntsc", "true"))

		assertEquals("on", schema.option("mousemap")!!.valueText("1", "on", "off"))
		assertEquals("off", schema.option("ntsc")!!.valueText("false", "on", "off"))
		assertEquals("Auto", schema.option("three")!!.valueText("2", "on", "off"))
	}

	@Test
	fun `the bundled schema names choices as the layer does`() {
		val schema = requireNotNull(SettingsSchema.current)

		assertEquals("68020 (A1200)", schema.choiceLabel("cpu_model", "68020"))
		assertEquals("Cycle Exact (DMA/Memory)", schema.choiceLabel("cycle_exact", "memory"))
		assertEquals("KB", schema.option("cachesize")!!.unit)
		assertEquals("CPU model", schema.label("cpu_model"))
		assertTrue(schema.option("cpu_model")!!.help.isNotBlank())
		assertTrue(schema.constraint("cpu_24bit_addressing")!!.isNotBlank())
		assertTrue(schema.option("joyport1mousemap")!!.showsAsSwitch("1"))
	}
}
