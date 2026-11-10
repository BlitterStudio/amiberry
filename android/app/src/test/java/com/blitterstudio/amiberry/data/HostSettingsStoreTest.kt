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

	private fun store(content: String?, schema: SettingsSchema): Pair<HostSettingsStore, File> {
		val file = tempDir.newFile("amiberry.conf")
		if (content != null) file.writeText(content)
		else file.delete()
		return HostSettingsStore(file, schema) to file
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


	private fun path(id: String): String =
		"{\"id\":\"$id\",\"group\":\"g\",\"label\":\"$id\",\"help\":\"\",\"type\":\"path\",\"apply\":\"live\",\"default\":\"\"}"

	@Test
	fun `a clear drops the setting's lines and keeps everything else`() = runBlocking {
		val (store, file) = store("write_logfile=yes\nfloppy_path=/storage/disks\n; comment\nfloppy_path=/other\n", schemaOf(bool("write_logfile", "false"), path("floppy_path")))
		assertTrue(store.clear(listOf("floppy_path")))
		assertEquals("true", store.read()["write_logfile"])
		assertNull(store.read()["floppy_path"])
		val text = file.readText()
		assertFalse(text.contains("floppy_path"))
		assertTrue(text.contains("; comment"))
	}
}
