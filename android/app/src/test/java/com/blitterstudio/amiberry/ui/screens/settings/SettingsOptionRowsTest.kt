package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * The option-row registry: every option whose row a page gives a rule of its
 * own (a device list, a visibility or availability rule, a coupled pair, a
 * launcher list) resolves to that same row in All Settings, because both the
 * pages and All Settings render options through SettingsOptionRow.
 */
class SettingsOptionRowsTest {

	private val schema = SettingsSchema.current!!

	@Test
	fun `the registry names every option whose page row has a rule of its own`() {
		assertEquals(
			setOf(
				"joyport0", "joyport1", "joyport1mousemap", "input.joymouse_speed_analog",
				"amiberry.onscreen_joystick", "amiberry.vkbd_touch_button", "amiberry.vkbd_numpad",
				"amiberry.shader", "amiberry.scaling_method",
				"fpu_model", "chipmem_size", "bogomem_size", "sound_max_buff",
				"cpuboard_type", "cpuboardmem1_size"
			) + (0..3).map { "floppy${it}sound" },
			SettingsOptionRows.customIds
		)
	}

	@Test
	fun `every registered row is offered by all settings, on its owning group`() {
		SettingsOptionRows.customIds.forEach { id ->
			val group = schema.option(id)!!.group
			val offered = SettingsGroups.allSettingsRows(schema, group) { _ -> true }.map { it.id }
			assertTrue(id, id in offered)
		}
	}

	@Test
	fun `no registered row is managed or the hidden half of a coupled pair`() {
		SettingsOptionRows.customIds.forEach { id ->
			assertFalse(id, id in SettingsGroups.managed)
			assertTrue(id, SettingsGroups.coupledPairOf(id)?.let { it.first() == id } ?: true)
		}
	}

	@Test
	fun `every registered row is one the launcher's pages lay out`() {
		val laidOut = SettingsGroups.laidOut.values.flatten().toSet()
		SettingsOptionRows.customIds.forEach { id ->
			assertTrue(id, id in laidOut)
			assertTrue(id, schema.option(id)!!.let { !it.presetOnly && SettingsGroups.rowKind(it) != null })
		}
	}

	@Test
	fun `touch-only rows show only with a touchscreen, which the constraints enforce`() {
		val settings = EmulatorSettings()
		assertFalse(SettingsOptionRows.shows("amiberry.onscreen_joystick", settings, hasTouchScreen = false))
		assertFalse(SettingsOptionRows.shows("amiberry.vkbd_touch_button", settings, hasTouchScreen = false))
		assertTrue(SettingsOptionRows.shows("amiberry.onscreen_joystick", settings, hasTouchScreen = true))
		assertTrue(SettingsOptionRows.shows("amiberry.vkbd_touch_button", settings, hasTouchScreen = true))

		// Why they hide: without a touchscreen the constraints reset the switch,
		// so an enabled row would silently do nothing.
		val asked = EmulatorSettingsConstraints.apply(
			settings.withOption("amiberry.onscreen_joystick", "true"),
			hasTouchScreen = false
		)
		assertFalse(asked.onScreenJoystick)
	}

	@Test
	fun `the mouse-map rows show only for a port-1 controller with the mouse on port 0`() {
		fun shows(joyport1: String, joyport0: String) = SettingsOptionRows.shows(
			"joyport1mousemap",
			EmulatorSettings(joyport0 = joyport0, joyport1 = joyport1),
			hasTouchScreen = true
		)

		assertTrue(shows("joy0", "mouse"))
		assertFalse(shows("mouse", "mouse"))
		assertFalse(shows("joy0", "joy1"))
		assertEquals(shows("joy0", "mouse"), portSupportsMouseMap(1, "joy0", "mouse"))
	}

	@Test
	fun `options without a rule of their own show wherever they are`() {
		val settings = EmulatorSettings()
		listOf("cpu_model", "mmu_model", "sound_output", "use_gui").forEach { id ->
			assertTrue(id, SettingsOptionRows.shows(id, settings, hasTouchScreen = false))
			assertTrue(id, SettingsOptionRows.shows(id, settings, hasTouchScreen = true))
		}
	}
}
