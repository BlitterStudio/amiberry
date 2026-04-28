package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.StoragePaths
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class ConfigRepositoryTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	// --- Config name validation ---

	@Test
	fun `isValidConfigName rejects blank names`() {
		assertFalse(isValidName(""))
		assertFalse(isValidName("   "))
	}

	@Test
	fun `isValidConfigName rejects path separators`() {
		assertFalse(isValidName("path/to/config"))
		assertFalse(isValidName("path\\to\\config"))
	}

	@Test
	fun `isValidConfigName rejects parent directory traversal`() {
		assertFalse(isValidName("../evil"))
		assertFalse(isValidName(".."))
	}

	@Test
	fun `isValidConfigName accepts normal names`() {
		assertTrue(isValidName("MyConfig"))
		assertTrue(isValidName("A500 Default"))
		assertTrue(isValidName("config-v2"))
		assertTrue(isValidName("test_config.1"))
	}

	// --- Config round-trip with unknown lines ---

	@Test
	fun `config save and reload preserves unknown lines`() {
		val confDir = tempDir.newFolder(StoragePaths.CONFIGURATIONS)

		val settings = EmulatorSettings(cpuModel = 68020, chipset = "aga")
		val unknownLines = listOf("custom_option=value", "another_key=42")

		// Write config
		val configText = ConfigGenerator.generate(settings)
		val file = File(confDir, "test.uae")
		file.writeText(configText)
		file.appendText("\n; Preserved settings from original config\n")
		unknownLines.forEach { file.appendText("$it\n") }

		// Parse it back
		val parsed = ConfigParser.parse(file)

		assertEquals(68020, parsed.settings.cpuModel)
		assertEquals("aga", parsed.settings.chipset)
		assertEquals(2, parsed.unknownLines.size)
		assertTrue(parsed.unknownLines.any { it.contains("custom_option=value") })
		assertTrue(parsed.unknownLines.any { it.contains("another_key=42") })
	}

	@Test
	fun `config round-trip preserves all floppy drives`() {
		val settings = EmulatorSettings(
			floppy0 = "/path/disk1.adf",
			floppy0Type = 0,
			floppy1 = "/path/disk2.adf",
			floppy1Type = 0,
			floppy2 = "",
			floppy2Type = -1,
			floppy3 = "",
			floppy3Type = -1
		)

		val configText = ConfigGenerator.generate(settings)
		val file = tempDir.newFile("floppy_test.uae")
		file.writeText(configText)
		val parsed = ConfigParser.parse(file)

		assertEquals("/path/disk1.adf", parsed.settings.floppy0)
		assertEquals(0, parsed.settings.floppy0Type)
		assertEquals("/path/disk2.adf", parsed.settings.floppy1)
		assertEquals(0, parsed.settings.floppy1Type)
		assertEquals("", parsed.settings.floppy2)
		assertEquals(-1, parsed.settings.floppy2Type)
	}

	// --- Duplicate config ---

	@Test
	fun `duplicating a config creates an identical copy`() {
		val confDir = tempDir.newFolder(StoragePaths.CONFIGURATIONS)
		val original = File(confDir, "original.uae")
		original.writeText("cpu_model=68000\nchipset=ocs\n")

		val target = File(confDir, "copy.uae")
		original.copyTo(target, overwrite = false)

		assertTrue(target.exists())
		assertEquals(original.readText(), target.readText())
	}

	@Test
	fun `duplicate does not overwrite existing file`() {
		val confDir = tempDir.newFolder(StoragePaths.CONFIGURATIONS)
		val original = File(confDir, "original.uae")
		original.writeText("cpu_model=68000\n")

		val existing = File(confDir, "copy.uae")
		existing.writeText("cpu_model=68020\n")

		try {
			original.copyTo(existing, overwrite = false)
			fail("Should throw FileAlreadyExistsException")
		} catch (_: FileAlreadyExistsException) {
			// Expected
		}

		// Existing file should be unchanged
		assertTrue(existing.readText().contains("68020"))
	}

	/**
	 * Mirror of ConfigRepository.isValidConfigName for testing without Context.
	 */
	private fun isValidName(name: String): Boolean {
		if (name.isBlank()) return false
		if (name.contains('/') || name.contains('\\') || name.contains("..")) return false
		return true
	}
}
