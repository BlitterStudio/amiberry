package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
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

		assertEquals(ConfigFiles.EditResult.Failed, ConfigFiles.renameConfig(confDir, outside.absolutePath, "Renamed"))
		assertTrue(outside.exists())
		assertFalse(File(confDir, "Renamed.uae").exists())
	}

	@Test
	fun `rename config refuses to overwrite an existing config`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "Source.uae", "cpu_model=68000\n")
		val target = configFile(confDir, "Target.uae", "cpu_model=68020\n")

		assertEquals(
			ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.Taken("Target")),
			ConfigFiles.renameConfig(confDir, source.absolutePath, "Target")
		)
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

		assertEquals(ConfigFiles.EditResult.Failed, ConfigFiles.duplicateConfig(confDir, outside.absolutePath, "Copy"))
		assertEquals(
			ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.Taken("Copy")),
			ConfigFiles.duplicateConfig(confDir, source.absolutePath, "Copy")
		)
		assertEquals("cpu_model=68020\n", existing.readText())
	}

	@Test
	fun `rename moves the file and keeps its extension`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "Workbench.UAE", "cpu_model=68000\n")

		val result = ConfigFiles.renameConfig(confDir, source.absolutePath, "  Workbench 3.1  ")

		val renamed = (result as ConfigFiles.EditResult.Done).file
		assertEquals("Workbench 3.1.UAE", renamed.name)
		assertEquals("cpu_model=68000\n", renamed.readText())
		assertFalse(File(confDir, "Workbench.UAE").exists())
		assertEquals(listOf("Workbench 3.1.UAE"), ConfigFiles.listUserConfigFiles(confDir).map { it.name })
	}

	@Test
	fun `rename to the same name leaves the file and a case change is allowed`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "workbench.uae", "cpu_model=68000\n")

		assertEquals(
			ConfigFiles.EditResult.Done(source.canonicalFile),
			ConfigFiles.renameConfig(confDir, source.absolutePath, "workbench")
		)
		val result = ConfigFiles.renameConfig(confDir, source.absolutePath, "Workbench")
		assertTrue(result is ConfigFiles.EditResult.Done)
		assertEquals(listOf("Workbench.uae"), ConfigFiles.listUserConfigFiles(confDir).map { it.name })
	}

	@Test
	fun `rename refuses a name another configuration has in any case`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "Source.uae", "cpu_model=68000\n")
		configFile(confDir, "Games.UAE", "cpu_model=68020\n")

		val result = ConfigFiles.renameConfig(confDir, source.absolutePath, "games")

		assertTrue(result is ConfigFiles.EditResult.Refused)
		assertTrue((result as ConfigFiles.EditResult.Refused).problem is ConfigFiles.NameProblem.Taken)
		assertTrue(source.exists())
	}

	@Test
	fun `rename and duplicate refuse empty and unsafe names with their reason`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "Source.uae", "cpu_model=68000\n")

		assertEquals(
			ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.Empty),
			ConfigFiles.renameConfig(confDir, source.absolutePath, "   ")
		)
		listOf("A/B", "A:B", "A\\B", ".hidden").forEach { name ->
			assertEquals(
				name,
				ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.InvalidCharacters),
				ConfigFiles.duplicateConfig(confDir, source.absolutePath, name)
			)
		}
		assertEquals(listOf("Source.uae"), ConfigFiles.listUserConfigFiles(confDir).map { it.name })
	}

	@Test
	fun `duplicate copies the file under the new name with the source extension`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = configFile(confDir, "Games.UAE", "cpu_model=68020\n")

		val copy = (ConfigFiles.duplicateConfig(confDir, source.absolutePath, "Games copy") as ConfigFiles.EditResult.Done).file

		assertEquals("Games copy.UAE", copy.name)
		assertEquals(source.readText(), copy.readText())
		assertTrue(source.exists())
	}

	@Test
	fun `name problems match the rules Save and Rename share`() {
		val existing = listOf("Workbench", "Games")

		assertEquals(ConfigFiles.NameProblem.Empty, ConfigFiles.nameProblem("  ", existing))
		assertEquals(ConfigFiles.NameProblem.InvalidCharacters, ConfigFiles.nameProblem("A/B", existing))
		assertEquals(ConfigFiles.NameProblem.InvalidCharacters, ConfigFiles.nameProblem("A:B", existing))
		assertEquals(ConfigFiles.NameProblem.InvalidCharacters, ConfigFiles.nameProblem(".dot", existing))
		assertEquals(ConfigFiles.NameProblem.Taken("games"), ConfigFiles.nameProblem("games", existing))
		assertEquals(ConfigFiles.NameProblem.Taken("Games"), ConfigFiles.nameProblem("Games.uae", existing))
		assertNull(ConfigFiles.nameProblem("Lemmings", existing))
		// The configuration being renamed may keep its name or change only its case.
		assertNull(ConfigFiles.nameProblem("workbench", existing, currentName = "Workbench"))
	}

	@Test
	fun `copies are named as the Finder names them`() {
		assertEquals("Turrican copy", ConfigFiles.copyName("Turrican", listOf("Turrican")))
		assertEquals("Turrican copy 2", ConfigFiles.copyName("Turrican", listOf("Turrican", "Turrican copy")))
		assertEquals(
			"Turrican copy 3",
			ConfigFiles.copyName("Turrican copy", listOf("Turrican", "Turrican copy", "turrican COPY 2"))
		)
		assertEquals("Turrican copy 2", ConfigFiles.copyName("Turrican copy 7", listOf("Turrican copy")))
		assertEquals("copy", ConfigFiles.copyBase("copy"))
		assertEquals("Disk", ConfigFiles.copyBase("Disk copy 12"))
		assertEquals("Photocopy", ConfigFiles.copyBase("Photocopy"))
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

	@Test
	fun `userConfigFile preserves the exact filename and extension case`() {
		val confDir = tempDir.newFolder("Configurations")
		// Capture the name-based filename BEFORE the file exists so canonical resolution
		// cannot fold Workbench.uae → Workbench.UAE on case-insensitive filesystems (Windows).
		val nameBasedName = ConfigFiles.targetFileForName(confDir, "Workbench")!!.name
		val upper = File(confDir, "Workbench.UAE")
		upper.writeText("cpu_model=68000\n")

		val resolved = ConfigFiles.userConfigFile(confDir, upper.absolutePath)

		assertEquals(upper.canonicalFile, resolved)
		// A name-based target would have forced a different lowercase-extension file:
		assertNotEquals(resolved!!.name, nameBasedName)
	}

	@Test
	fun `userConfigFile rejects a path outside the configurations dir`() {
		val confDir = tempDir.newFolder("Configurations")
		val outside = tempDir.newFile("Outside.uae")
		assertNull(ConfigFiles.userConfigFile(confDir, outside.absolutePath))
	}

	private fun configFile(confDir: File, name: String, text: String): File =
		File(confDir, name).also { it.writeText(text) }
}
