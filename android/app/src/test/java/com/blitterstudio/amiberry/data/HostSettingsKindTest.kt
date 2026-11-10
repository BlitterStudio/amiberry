package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * What the schema says about the content folders' settings: which ones name
 * a single file (a file picker) and which a folder, as the layer's own kind
 * exports it — the launcher's rows and pickers follow the type, not a list
 * of ids.
 */
class HostSettingsKindTest {

	private val schema = requireNotNull(SettingsSchema.current) { "the bundled settings schema did not load" }

	@Test
	fun `the log file and the retroarch config are the file settings`() {
		assertEquals(
			listOf("retroarch_config", "logfile_path"),
			schema.hostSettings.filter { it.type == SettingsSchema.TYPE_FILE }.map { it.id }
		)
	}

	@Test
	fun `every other content folder names a folder`() {
		val folders = schema.hostSettings
			.filter { it.type == SettingsSchema.TYPE_PATH }
			.map { it.id }
			.toSet()
		for (folder in listOf(
			"config_path", "controllers_path", "whdboot_path", "whdload_arch_path",
			"floppy_path", "harddrive_path", "cdrom_path", "rom_path", "rp9_path",
			"floppy_sounds_dir", "saveimage_dir", "savestate_dir", "ripper_path",
			"inputrecordings_dir", "screenshot_dir", "nvram_dir", "plugins_dir",
			"shaders_path", "bezels_path"
		)) {
			assertTrue("$folder should be a folder setting", folder in folders)
		}
	}
}
