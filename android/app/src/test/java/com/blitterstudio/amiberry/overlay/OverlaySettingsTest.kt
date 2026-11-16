package com.blitterstudio.amiberry.overlay

import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.screens.settings.SettingsGroups
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
		assertEquals(OverlaySettings.POINTER, OverlaySettings.offered(bundled, OverlaySettings.POINTER).map { it.id })
		assertEquals(OverlaySettings.SETTINGS, OverlaySettings.offered(bundled, OverlaySettings.SETTINGS).map { it.id })
	}

	@Test
	fun `all settings includes generic rows but not dedicated pages or hidden rows`() {
		val ids = OverlaySettings.allSettings(bundled).map { it.id }.toSet()
		assertTrue("input.joystick_deadzone" in ids)
		assertTrue("floppy_speed" in ids)
		assertTrue("chipmem_size" in ids)
		assertTrue(OverlaySettings.PORTS.none { it in ids })
		assertTrue(OverlaySettings.CONTROLS.none { it in ids })
		assertTrue("floppy0" !in ids)
		assertTrue("floppy0type" !in ids)
		assertTrue(SettingsGroups.managed.none { it in ids })
		assertTrue("cart_file" !in ids)
		assertTrue("amiberry.open_gui" !in ids)
	}

	@Test
	fun `non-live commits stage the strongest required application`() {
		assertEquals(PendingApply.RESET, PendingApply.merge(PendingApply.NONE, OverlayNative.APPLY_RESET))
		assertEquals(PendingApply.RESTART, PendingApply.merge(PendingApply.RESET, OverlayNative.APPLY_RESTART))
		assertEquals(PendingApply.RESTART, PendingApply.merge(PendingApply.RESTART, OverlayNative.APPLY_LIVE))
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
