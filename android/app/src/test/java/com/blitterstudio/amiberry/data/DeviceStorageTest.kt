package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class DeviceStorageTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `entries list folders first then supported files by name, without hidden ones`() {
		val root = tempDir.root
		File(root, "zeta").mkdir()
		File(root, "Alpha").mkdir()
		File(root, ".hidden").mkdir()
		File(root, "Game.ADF").writeText("disk")
		File(root, "b.lha").writeText("game")
		File(root, "notes.txt").writeText("text")
		File(root, ".secret.adf").writeText("disk")

		val entries = DeviceStorage.entries(root, setOf("adf", "lha"))!!

		assertEquals(listOf("Alpha", "zeta", "b.lha", "Game.ADF"), entries.map { it.file.name })
		assertEquals(listOf(true, true, false, false), entries.map { it.isDirectory })
	}

	@Test
	fun `an unreadable folder has no entries`() {
		assertNull(DeviceStorage.entries(File(tempDir.root, "missing"), setOf("adf")))
	}

	@Test
	fun `supported files below a folder include nested folders and skip hidden ones`() {
		val root = tempDir.root
		File(root, "Games/Action").mkdirs()
		File(root, ".cache").mkdirs()
		File(root, "kick.rom").writeText("rom")
		File(root, "Games/Action/Turrican.lha").writeText("game")
		File(root, "Games/readme.txt").writeText("text")
		File(root, ".cache/old.adf").writeText("disk")

		val files = DeviceStorage.supportedFilesUnder(root, setOf("rom", "lha", "adf"))

		assertEquals(listOf("kick.rom", "Turrican.lha"), files.map { it.name })
	}
}
