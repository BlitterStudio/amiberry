package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class ConfigFilesTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `list user config files includes uae case insensitively and excludes hidden or non config files`() {
		val confDir = tempDir.newFolder("Configurations")
		val lower = configFile(confDir, "Workbench.uae", "cpu_model=68000\n")
		val upper = configFile(confDir, "Games.UAE", "cpu_model=68020\n")
		configFile(confDir, ".last_session.uae", "transient=true\n")
		configFile(confDir, "notes.txt", "not a config\n")

		val listed = ConfigFiles.listUserConfigFiles(confDir).map { it.name }

		assertEquals(listOf(upper.name, lower.name).sorted(), listed.sorted())
	}

	@Test
	fun `delete config refuses paths outside the configurations directory`() {
		val confDir = tempDir.newFolder("Configurations")
		val outside = tempDir.newFile("outside.uae")
		outside.writeText("cpu_model=68000\n")

		assertFalse(ConfigFiles.deleteConfig(confDir, outside.absolutePath))
		assertTrue(outside.exists())
	}

	@Test
	fun `delete config refuses hidden and non config files inside the configurations directory`() {
		val confDir = tempDir.newFolder("Configurations")
		val hidden = configFile(confDir, ".last_session.uae", "transient=true\n")
		val text = configFile(confDir, "notes.txt", "not a config\n")

		assertFalse(ConfigFiles.deleteConfig(confDir, hidden.absolutePath))
		assertFalse(ConfigFiles.deleteConfig(confDir, text.absolutePath))
		assertTrue(hidden.exists())
		assertTrue(text.exists())
	}

	@Test
	fun `rename config refuses outside sources and does not create a target`() {
		val confDir = tempDir.newFolder("Configurations")
		val outside = tempDir.newFile("outside.uae")
		outside.writeText("cpu_model=68000\n")

		assertNull(ConfigFiles.renameConfig(confDir, outside.absolutePath, "Renamed"))
		assertTrue(outside.exists())
		assertFalse(File(confDir, "Renamed.uae").exists())
	}

	@Test
	fun `rename config refuses to overwrite an existing config`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "Source.uae", "cpu_model=68000\n")
		val target = configFile(confDir, "Target.uae", "cpu_model=68020\n")

		assertNull(ConfigFiles.renameConfig(confDir, source.absolutePath, "Target"))
		assertTrue(source.exists())
		assertEquals("cpu_model=68020\n", target.readText())
	}

	@Test
	fun `duplicate config refuses outside sources and existing targets`() {
		val confDir = tempDir.newFolder("Configurations")
		val outside = tempDir.newFile("outside.uae")
		outside.writeText("cpu_model=68000\n")
		val source = configFile(confDir, "Source.uae", "cpu_model=68000\n")
		val existing = configFile(confDir, "Copy.uae", "cpu_model=68020\n")

		assertNull(ConfigFiles.duplicateConfig(confDir, outside.absolutePath, "Copy"))
		assertNull(ConfigFiles.duplicateConfig(confDir, source.absolutePath, "Copy"))
		assertEquals("cpu_model=68020\n", existing.readText())
	}

	@Test
	fun `target file strips an optional uae suffix from user entered names`() {
		val confDir = tempDir.newFolder("Configurations")

		assertEquals(File(confDir, "Workbench.uae").canonicalFile, ConfigFiles.targetFileForName(confDir, "Workbench.uae"))
		assertEquals(File(confDir, "Workbench.uae").canonicalFile, ConfigFiles.targetFileForName(confDir, " Workbench "))
	}

	@Test
	fun `target file rejects unsafe names`() {
		val confDir = tempDir.newFolder("Configurations")

		assertNull(ConfigFiles.targetFileForName(confDir, ""))
		assertNull(ConfigFiles.targetFileForName(confDir, "../outside"))
		assertNull(ConfigFiles.targetFileForName(confDir, ".hidden"))
		assertNull(ConfigFiles.targetFileForName(confDir, "a/b"))
		assertNull(ConfigFiles.targetFileForName(confDir, "a\\b"))
	}

	private fun configFile(confDir: File, name: String, text: String): File =
		File(confDir, name).also { it.writeText(text) }
}
