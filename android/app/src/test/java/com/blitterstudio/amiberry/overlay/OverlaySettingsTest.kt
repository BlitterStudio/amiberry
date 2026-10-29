package com.blitterstudio.amiberry.overlay

import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class OverlaySettingsTest {

	private val bundled = requireNotNull(SettingsSchema.current) { "the bundled settings schema did not load" }

	private fun schemaOf(vararg options: Pair<String, String>): SettingsSchema {
		val emptyTable = JSONObject().put("inputs", JSONArray()).put("free", JSONArray()).put("rows", JSONArray())
		return SettingsSchema.parse(
			JSONObject()
				.put("options", JSONArray(options.map { (id, apply) ->
					JSONObject().put("id", id).put("type", "bool").put("apply", apply).put("default", "false")
				}))
				.put("presets", JSONArray())
				.put("compatibility_levels", JSONArray())
				.put("roms", JSONArray())
				.put("resolutions", emptyTable)
				.put("memory_resolutions", emptyTable)
		)
	}

	@Test
	fun `only options the layer applies live are offered`() {
		val schema = schemaOf("a" to "live", "b" to "reset", "c" to "restart", "d" to "live")
		assertEquals(listOf("d", "a"), OverlaySettings.offered(schema, listOf("d", "b", "c", "a", "missing")).map { it.id })
	}

	@Test
	fun `nothing is offered without a schema`() {
		assertTrue(OverlaySettings.offered(null, OverlaySettings.SETTINGS).isEmpty())
	}

	@Test
	fun `the bundled schema offers every control and setting of the menu`() {
		assertEquals(OverlaySettings.CONTROLS, OverlaySettings.offered(bundled, OverlaySettings.CONTROLS).map { it.id })
		assertEquals(OverlaySettings.SETTINGS, OverlaySettings.offered(bundled, OverlaySettings.SETTINGS).map { it.id })
	}

	@Test
	fun `a bundled option needing a reset is not offered`() {
		val notLive = bundled.options.values.first { it.apply != "live" }
		assertTrue(OverlaySettings.offered(bundled, listOf(notLive.id)).isEmpty())
	}

	@Test
	fun `read settings pair each id with its value and availability`() {
		val ids = listOf("sound_volume", "cpu_speed")
		assertEquals(
			mapOf("sound_volume" to SettingState("20", true), "cpu_speed" to SettingState("real", false)),
			SettingState.parse(ids, arrayOf("20", "1", "real", "0"))
		)
		assertTrue(SettingState.parse(ids, arrayOf("20", "1")).isEmpty())
		assertTrue(SettingState.parse(ids, null).isEmpty())
	}
}
