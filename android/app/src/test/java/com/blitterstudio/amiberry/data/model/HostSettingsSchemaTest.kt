package com.blitterstudio.amiberry.data.model

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * The host settings the bundled schema carries: the Android build's own
 * (the update checker left out, the Android-only ones carried), the
 * defaults that differ there, and the shape the App Settings rows render
 * (every choice a list, the device lists empty for the launcher to fill).
 */
class HostSettingsSchemaTest {

	private val schema = requireNotNull(SettingsSchema.current) { "the bundled settings schema did not load" }

	@Test
	fun `the schema lists the host settings in the layer's groups`() {
		assertTrue(schema.hostSettings.isNotEmpty())
		val groups = schema.hostSettingGroups
		assertEquals(
			listOf(
				"global_startup", "global_input", "global_display", "global_sound", "global_whdload",
				"global_touch", "global_visual", "global_paths", "global_advanced"
			),
			groups
		)
	}

	@Test
	fun `settings the Android build has no use for are not listed`() {
		val ids = schema.hostSettings.map { it.id }
		assertFalse("update_check" in ids)
		assertFalse("update_channel" in ids)
		assertFalse("skipped_version" in ids)
		assertFalse("macos_left_command_q" in ids)
	}

	@Test
	fun `the Android-only settings travel with the desktop export`() {
		val adpf = schema.hostSetting("use_adpf")
		assertNotNull(adpf)
		assertEquals("bool", adpf?.type)
		assertEquals("global_advanced", adpf?.group)
		assertEquals("true", adpf?.default)
	}

	@Test
	fun `defaults that differ on Android read as the Android build's`() {
		assertEquals("48000", schema.hostSetting("default_sound_frequency")?.defaultOnAndroid)
		assertEquals("8192", schema.hostSetting("default_sound_buffer")?.defaultOnAndroid)
		assertEquals("true", schema.hostSetting("default_onscreen_joystick")?.defaultOnAndroid)
		assertEquals("true", schema.hostSetting("default_vkbd_enabled")?.defaultOnAndroid)
		assertEquals("leftstick", schema.hostSetting("default_vkbd_toggle")?.defaultOnAndroid)
		assertEquals("70", schema.hostSetting("gui_layout_scale")?.defaultOnAndroid)
		// The desktop export's own default stays beside it.
		assertEquals("100", schema.hostSetting("gui_layout_scale")?.default)
		assertEquals("44100", schema.hostSetting("default_sound_frequency")?.default)
	}

	@Test
	fun `every host setting carries a label, help and its group`() {
		for (setting in schema.hostSettings) {
			assertTrue(setting.label.isNotBlank())
			assertTrue(setting.help.isNotBlank())
			assertTrue(setting.group.isNotBlank())
		}
	}

	@Test
	fun `a choice setting has its list, empty only where the launcher lists the host's own`() {
		val dynamic = setOf(
			"default_controller1", "default_controller2", "default_controller3", "default_controller4",
			"default_mouse1", "default_mouse2", "default_soundcard", "shader", "shader_rtg", "custom_bezel"
		)
		for (setting in schema.hostSettings) {
			if (setting.type != SettingsSchema.TYPE_CHOICE) continue
			if (setting.id in dynamic) {
				assertTrue(setting.choices.isEmpty())
			} else {
				assertTrue("${setting.id} has choices", setting.choices.isNotEmpty())
				// A choice's default is one of its values.
				assertTrue(setting.choices.any { it.value == setting.default })
			}
		}
	}

	@Test
	fun `host setting keys never collide with the machine options`() {
		val optionIds = schema.options.keys
		val shared = schema.hostSettings.map { it.id }.filter { it in optionIds }
		assertTrue("the same key cannot be both: $shared", shared.isEmpty())
	}

	@Test
	fun `the levels carry their unit and slider marks`() {
		val deadzone = schema.hostSetting("default_joystick_deadzone")
		assertNotNull(deadzone)
		assertEquals("%", deadzone?.unit)
		assertEquals(true, deadzone?.level)
		assertEquals(0.0, deadzone?.min)
		assertEquals(100.0, deadzone?.max)
	}

	@Test
	fun `the defaults map covers every host setting`() {
		assertEquals(schema.hostSettings.map { it.id }.toSet(), schema.hostDefaults.keys)
	}
}
