package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.SettingsSchema
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import java.io.File

/**
 * The host settings store's behaviour on amiberry.conf: values read from
 * the file and the Android defaults, the spelling the file keeps (bools as
 * yes/no), and a write that touches nothing but the setting it sets —
 * comments, paths and the emulator's own keys stay verbatim, the last line
 * of a repeated key wins, and the new content lands through a temporary
 * file renamed over the old one.
 */
class HostSettingsStoreTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private fun schemaOf(vararg settings: String): SettingsSchema {
		val base = """
			{"schema_version":1,"api_version":1,"options":[],"presets":[],"compatibility_levels":[],
			"roms":[],"resolutions":{"inputs":[],"free":[],"rows":[]},
			"memory_resolutions":{"inputs":[],"free":[],"rows":[]},
			"host_settings":[${settings.joinToString(",")}]}
		""".trimIndent()
		return SettingsSchema.parse(JSONObject(base))
	}

	private fun bool(id: String, def: String, android: String? = null): String {
		val extra = android?.let { ",\"android_default\":\"$it\"" } ?: ""
		return "{\"id\":\"$id\",\"group\":\"g\",\"label\":\"$id\",\"help\":\"\",\"type\":\"bool\"," +
			"\"apply\":\"live\",\"default\":\"$def\"$extra}"
	}

	private fun intSetting(id: String, def: String, min: Int, max: Int): String =
		 "{\"id\":\"$id\",\"group\":\"g\",\"label\":\"$id\",\"help\":\"\",\"type\":\"int\"," +
			"\"apply\":\"live\",\"default\":\"$def\",\"min\":$min,\"max\":$max,\"step\":1}"

	private fun choice(id: String, vararg values: String): String {
		val choices = values.joinToString(",") { "{\"value\":\"$it\",\"label\":\"$it\"}" }
		return "{\"id\":\"$id\",\"group\":\"g\",\"label\":\"$id\",\"help\":\"\",\"type\":\"choice\"," +
			"\"apply\":\"live\",\"default\":\"${values.first()}\",\"choices\":[$choices]}"
	}

	private fun path(id: String): String =
		"{\"id\":\"$id\",\"group\":\"g\",\"label\":\"$id\",\"help\":\"\",\"type\":\"path\"," +
			"\"apply\":\"live\",\"default\":\"\"}"

	private fun store(
		content: String?,
		schema: SettingsSchema,
		defaultContentRoot: File? = null
	): Pair<HostSettingsStore, File> {
		val file = tempDir.newFile("amiberry.conf")
		if (content != null) file.writeText(content)
		else file.delete()
		return HostSettingsStore(file, schema, defaultContentRoot) to file
	}

	@Test
	fun `reads values the file holds and defaults for the rest`() {
		val schema = schemaOf(bool("write_logfile", "false"), intSetting("input_default_mouse_speed", "100", 1, 1000))
		val (store, _) = store("write_logfile=yes\n", schema)
		assertEquals("true", store.read()["write_logfile"])
		assertNull(store.read()["input_default_mouse_speed"])
	}

	@Test
	fun `every spelling a bool can take in the file reads as true or false`() {
		val schema = schemaOf(bool("a", "false"), bool("b", "false"), bool("c", "false"), bool("d", "true"))
		val (store, _) = store("a=YES\nb=1\nc=True\nd=no\n", schema)
		val values = store.read()
		assertEquals("true", values["a"])
		assertEquals("true", values["b"])
		assertEquals("true", values["c"])
		assertEquals("false", values["d"])
	}

	@Test
	fun `a write spells bools as the emulator's own writer does`() {
		val schema = schemaOf(bool("write_logfile", "false"))
		val (store, file) = store("; the settings\nDiskfile=/keep/disk.adf\n", schema)
		assertTrue(store.writeSync(mapOf("write_logfile" to "true")))
		assertEquals("; the settings\nDiskfile=/keep/disk.adf\nwrite_logfile=yes\n", file.readText())
	}

	@Test
	fun `a write preserves unknown lines and comments verbatim`() {
		val schema = schemaOf(bool("write_logfile", "false"))
		val content = "; Amiberry settings\nDiskfile=/a.adf\nDiskfile=/b.adf\nconfig_path=/conf\nwrite_logfile=no\n; tail\n"
		val (store, file) = store(content, schema)
		assertTrue(store.writeSync(mapOf("write_logfile" to "true")))
		val lines = file.readText().lines()
		assertTrue("; Amiberry settings" in lines)
		assertTrue("Diskfile=/a.adf" in lines)
		assertTrue("Diskfile=/b.adf" in lines)
		assertTrue("config_path=/conf" in lines)
		assertTrue("; tail" in lines)
		assertTrue("write_logfile=yes" in lines)
	}

	@Test
	fun `a setting without a line is appended and reads back`() {
		val schema = schemaOf(intSetting("default_whd_configdelay", "0", 0, 1000))
		val (store, file) = store("perf_log=no\n", schema)
		assertTrue(store.writeSync(mapOf("default_whd_configdelay" to "5")))
		assertEquals("perf_log=no\ndefault_whd_configdelay=5\n", file.readText())
		assertEquals("5", store.read()["default_whd_configdelay"])
	}

	@Test
	fun `the last line of a repeated key wins and the write keeps one`() {
		val schema = schemaOf(bool("write_logfile", "false"))
		val (store, file) = store("write_logfile=yes\nwrite_logfile=no\n", schema)
		assertEquals("false", store.read()["write_logfile"])
		assertTrue(store.writeSync(mapOf("write_logfile" to "true")))
		assertEquals(1, file.readText().lines().count { it.startsWith("write_logfile=") })
		assertEquals("true", store.read()["write_logfile"])
	}

	@Test
	fun `an empty folder value is the line gone`() {
		val schema = schemaOf(path("base_content_path"))
		val (store, file) = store("base_content_path=/storage/content\n", schema)
		assertTrue(store.writeSync(mapOf("base_content_path" to "")))
		assertEquals("", file.readText())
		assertTrue(store.read().isEmpty())
	}

	@Test
	fun `a refused value leaves the file as it was`() {
		val schema = schemaOf(intSetting("input_default_mouse_speed", "100", 1, 1000), choice("default_line_mode", "0", "1", "2"))
		val content = "input_default_mouse_speed=100\n"
		val (store, file) = store(content, schema)
		assertFalse(store.writeSync(mapOf("input_default_mouse_speed" to "2000")))
		assertFalse(store.writeSync(mapOf("input_default_mouse_speed" to "fast")))
		assertFalse(store.writeSync(mapOf("default_line_mode" to "7")))
		assertFalse(store.writeSync(mapOf("input_default_mouse_speed" to " 100")))
		assertEquals(content, file.readText())
	}

	@Test
	fun `a write re-reads the file, so a change made meanwhile stays`() {
		val schema = schemaOf(bool("write_logfile", "false"), bool("perf_log", "false"))
		val (store, file) = store("write_logfile=no\n", schema)
		store.read()
		// The emulator's own window saved while the settings were open.
		file.writeText("write_logfile=no\nperf_log=yes\n")
		assertTrue(store.writeSync(mapOf("write_logfile" to "true")))
		assertTrue("perf_log=yes" in file.readText())
	}

	@Test
	fun `the write lands through a temporary file that is gone after`() {
		val schema = schemaOf(bool("write_logfile", "false"))
		val (store, file) = store(null, schema)
		assertTrue(store.writeSync(mapOf("write_logfile" to "true")))
		assertEquals("write_logfile=yes\n", file.readText())
		assertNull(file.parentFile!!.list { _, name -> name.endsWith(".tmp") }?.firstOrNull())
	}

	@Test
	fun `a comment or a line without a value is not a setting`() {
		val schema = schemaOf(bool("write_logfile", "false"))
		val (store, file) = store("; write_logfile=yes\nnot a setting\n\n", schema)
		assertTrue(store.read().isEmpty())
		assertTrue(store.writeSync(mapOf("write_logfile" to "true")))
		val lines = file.readText().lines()
		assertTrue("; write_logfile=yes" in lines)
		assertTrue("not a setting" in lines)
	}

	@Test
	fun `a port device's empty value stays a line, distinct from the default`() {
		val schema = schemaOf(choice("default_controller1", "joy1"))
		val (store, file) = store("", schema)
		assertTrue(store.writeSync(mapOf("default_controller1" to "")))
		assertEquals("default_controller1=\n", file.readText())
		assertEquals("", store.read()["default_controller1"])
	}

	@Test
	fun `concurrent writes each keep their change`() = runBlocking {
		val schema = schemaOf(bool("a", "false"), bool("b", "false"), bool("c", "false"), bool("d", "false"))
		val (store, _) = store("", schema)
		val writes = listOf("a", "b", "c", "d").associateWith { "true" }
		coroutineScope {
			writes.map { (id, value) -> launch(Dispatchers.IO) { assertTrue(store.write(mapOf(id to value))) } }
		}
		val values = store.read()
		writes.forEach { (id, _) -> assertEquals("true", values[id]) }
	}

	/** The store's suspending write, run to completion (the UI awaits it on IO). */
	private fun HostSettingsStore.writeSync(values: Map<String, String>): Boolean = runBlocking { write(values) }

	/**
	 * A base content folder as the emulator saves it: the base, the folders
	 * it derives and the folders set away from it, plus one the base does
	 * not manage.
	 */
	private fun nativeSavedBaseFixture(): String {
		val base = "/storage/emulated/0/BASE"
		return listOf(
			"base_content_path=$base",
			"config_path=$base/Configurations",
			"rom_path=$base/ROMs",
			"controllers_path=$base/Controllers",
			"shaders_path=/storage/emulated/0/CUSTOM/Shaders",
			"retroarch_config=/storage/emulated/0/retroarch.cfg"
		).joinToString("\n", postfix = "\n")
	}

	private fun baseSchema(): SettingsSchema = schemaOf(
		path("base_content_path"), path("config_path"), path("rom_path"), path("controllers_path"),
		path("shaders_path"), path("retroarch_config")
	)

	@Test
	fun `clearing the base content folder returns the folders it derived to the defaults`() {
		val root = tempDir.newFolder("content-defaults")
		val (store, file) = store(nativeSavedBaseFixture(), baseSchema(), root)

		assertTrue(store.writeSync(mapOf("base_content_path" to "")))

		val lines = file.readText().lines()
		// The base is gone; the folders it derived are the default root's…
		assertFalse(lines.any { it.startsWith("base_content_path=") })
		assertTrue(lines.contains("config_path=" + File(root, "Configurations").path))
		assertTrue("rom_path=${File(root, "ROMs").path}" in lines)
		assertTrue("controllers_path=${File(root, "Controllers").path}" in lines)
		// …a folder set away from the base's spelling keeps its override…
		assertTrue("shaders_path=/storage/emulated/0/CUSTOM/Shaders" in lines)
		// …and a folder the base does not manage is untouched.
		assertTrue("retroarch_config=/storage/emulated/0/retroarch.cfg" in lines)
	}

	@Test
	fun `a new base content folder rederives its folders and keeps the overrides`() {
		val root = tempDir.newFolder("content-new-defaults")
		val (store, file) = store(nativeSavedBaseFixture(), baseSchema(), root)

		assertTrue(store.writeSync(mapOf("base_content_path" to "/storage/emulated/0/NEW")))

		val lines = file.readText().lines()
		assertTrue("base_content_path=/storage/emulated/0/NEW" in lines)
		assertTrue("config_path=/storage/emulated/0/NEW/Configurations" in lines)
		assertTrue("rom_path=/storage/emulated/0/NEW/ROMs" in lines)
		assertTrue("controllers_path=/storage/emulated/0/NEW/Controllers" in lines)
		assertTrue("shaders_path=/storage/emulated/0/CUSTOM/Shaders" in lines)
		assertTrue("retroarch_config=/storage/emulated/0/retroarch.cfg" in lines)
	}

	@Test
	fun `clearing without a base set changes nothing`() {
		val content = "config_path=/storage/emulated/0/Configurations\n"
		val (store, file) = store(content, baseSchema())

		assertTrue(store.writeSync(mapOf("base_content_path" to "")))

		assertEquals(content, file.readText())
	}

	@Test
	fun `a folder with no entry under a base follows the base, not the default root`() {
		// The emulator saved the base and its Configurations, then the user
		// cleared ROMs: the line is gone, so ROMs sit at the base.
		val root = tempDir.newFolder("content-missing-base")
		val base = "/storage/emulated/0/BASE"
		val content = "base_content_path=$base\nconfig_path=$base/Configurations\n"
		val (store, file) = store(content, baseSchema(), root)

		assertTrue(store.writeSync(mapOf("base_content_path" to "/storage/emulated/0/NEW")))

		val lines = file.readText().lines()
		assertTrue(lines.contains("rom_path=/storage/emulated/0/NEW/ROMs"))
		// Clearing the base takes it to the default root, the same way.
		assertTrue(store.writeSync(mapOf("base_content_path" to "")))
		assertTrue("rom_path=${File(root, "ROMs").path}" in storeRead(file))
	}

	@Test
	fun `a folder with no entry and no previous base takes the new base`() {
		val root = tempDir.newFolder("content-missing-default")
		val content = "config_path=/storage/emulated/0/Configurations\n"
		val (store, file) = store(content, baseSchema(), root)

		assertTrue(store.writeSync(mapOf("base_content_path" to "/storage/emulated/0/NEW")))

		// No base was set, so a missing line means the default root — at its
		// baseline, and the folder follows the new base.
		assertTrue("rom_path=/storage/emulated/0/NEW/ROMs" in file.readText().lines())
	}

	private fun storeRead(file: File): List<String> = file.readText().lines()

	@Test
	fun `a base change leaves the folders an explicit write names to that write`() {
		val root = tempDir.newFolder("content-explicit")
		val (store, file) = store(nativeSavedBaseFixture(), baseSchema(), root)

		assertTrue(store.writeSync(mapOf("base_content_path" to "/storage/emulated/0/NEW", "rom_path" to "/my/roms")))

		val lines = file.readText().lines()
		assertTrue("rom_path=/my/roms" in lines)
		assertTrue("config_path=/storage/emulated/0/NEW/Configurations" in lines)
	}
}
