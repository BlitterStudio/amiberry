package com.blitterstudio.amiberry.ui.screens.appsettings

import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * App Settings' pages and sections (audit §3d): every host setting the
 * schema still offers on Android lands in exactly one section, the
 * ImGui-only ones the layer hides land nowhere, and a search reaches the
 * settings a collapsed section holds.
 */
class AppSettingsSectionsTest {

	private val schema = SettingsSchema.current!!

	/** The section a setting lands in, computed over the pages' sections. */
	private fun sectionOf(id: String): AppSettingsSection? =
		AppSettingsSections.pages.asSequence()
			.flatMap { page -> page.sections.map { page to it } }
			.filter { (_, section) -> AppSettingsSections.settingsOf(schema, section).any { it.id == id } }
			.map { it.second }
			.singleOrNull()

	@Test
	fun `every host setting that shows on Android lands in exactly one section`() {
		val landed = HashMap<String, String>()
		for (page in AppSettingsSections.pages) {
			for (section in page.sections) {
				for (setting in AppSettingsSections.settingsOf(schema, section)) {
					assertNull("setting ${setting.id} shows in two sections", landed[setting.id])
					landed[setting.id] = section.key
				}
			}
		}
		val offered = schema.hostSettings.filter { !it.hiddenOnAndroid }.map { it.id }
		assertEquals(offered.toSet(), landed.keys)
	}

	@Test
	fun `the ImGui-only settings are hidden on Android and land in no section`() {
		val hidden = schema.hostSettings.filter { it.hiddenOnAndroid }.map { it.id }
		assertEquals(
			setOf(
				"Quickstart", "read_config_descriptions", "gui_joystick_control", "disable_shutdown_button",
				"default_quickstart_compatibility", "gui_theme", "gui_layout_scale"
			),
			hidden.toSet()
		)
		hidden.forEach { id -> assertNull(id, sectionOf(id)) }
	}

	@Test
	fun `the input defaults split into controllers and mouse, shortcuts, and the RetroArch buttons disclosed`() {
		val defaults = AppSettingsSections.pages.single { it.id == "defaults" }
		fun ids(section: AppSettingsSection) = AppSettingsSections.settingsOf(schema, section).map { it.id }

		assertEquals(
			listOf(
				"default_controller1", "default_controller2", "default_controller3", "default_controller4",
				"default_mouse1", "default_mouse2", "input_default_mouse_speed", "default_joystick_deadzone",
				"input_keyboard_as_joystick_stop_keypresses", "rctrl_as_ramiga"
			),
			ids(defaults.sections.single { it.key == "default_controller1,default_controller2,default_controller3,default_controller4,default_mouse1,default_mouse2,input_default_mouse_speed,default_joystick_deadzone,input_keyboard_as_joystick_stop_keypresses,rctrl_as_ramiga" })
		)
		assertEquals(
			listOf("default_open_gui_key", "default_quit_key", "default_ar_key", "default_fullscreen_toggle_key"),
			ids(defaults.sections.single { it.group == "global_input" })
		)
		val retroarch = defaults.sections.single { it.key == "default_retroarch_quit,default_retroarch_menu,default_retroarch_reset" }
		assertEquals(
			listOf("default_retroarch_quit", "default_retroarch_menu", "default_retroarch_reset"),
			ids(retroarch)
		)
		assertTrue(retroarch.collapsed)
	}

	@Test
	fun `a search reaches the settings a collapsed section holds`() {
		val retroarch = AppSettingsSections.pages.single { it.id == "defaults" }
			.sections.single { it.collapsed }
		assertEquals(
			listOf("default_retroarch_quit"),
			AppSettingsSections.settingsOf(schema, retroarch) { it.matches("retroarch quit") }.map { it.id }
		)
		// A query that matches nothing shows nothing, wherever it looks.
		assertTrue(AppSettingsSections.settingsOf(schema, retroarch) { it.matches("nothing matches this") }.isEmpty())
		// The hidden settings never match, not even by their id.
		assertTrue(AppSettingsSections.settingsOf(schema, retroarch) { it.matches("gui_theme") }.isEmpty())
	}

	@Test
	fun `general became appearance, and the startup settings left with the classic window`() {
		assertEquals(
			listOf("appearance", "defaults", "advanced"),
			AppSettingsSections.pages.map { it.id }
		)
		// The whole global_startup group is hidden on Android.
		assertFalse(AppSettingsSections.pages.any { page -> page.sections.any { it.group == "global_startup" } })
		// The emulator's appearance (shaders and bezels) is the page's one section.
		val appearance = AppSettingsSections.pages.single { it.id == "appearance" }
		assertEquals(
			listOf("shader", "shader_rtg", "use_bezel", "use_custom_bezel", "custom_bezel"),
			appearance.sections.flatMap { AppSettingsSections.settingsOf(schema, it).map { option -> option.id } }
		)
	}
}
