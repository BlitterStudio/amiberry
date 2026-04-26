package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.StoragePaths
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class FileManagerTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	// --- FileCategory.fromExtension ---

	@Test
	fun `fromExtension returns ROMS for rom extension`() {
		assertEquals(FileCategory.ROMS, FileCategory.fromExtension("rom"))
		assertEquals(FileCategory.ROMS, FileCategory.fromExtension("bin"))
	}

	@Test
	fun `fromExtension returns FLOPPIES for floppy extensions`() {
		assertEquals(FileCategory.FLOPPIES, FileCategory.fromExtension("adf"))
		assertEquals(FileCategory.FLOPPIES, FileCategory.fromExtension("adz"))
		assertEquals(FileCategory.FLOPPIES, FileCategory.fromExtension("dms"))
		assertEquals(FileCategory.FLOPPIES, FileCategory.fromExtension("ipf"))
	}

	@Test
	fun `fromExtension returns HARD_DRIVES for hd extensions`() {
		assertEquals(FileCategory.HARD_DRIVES, FileCategory.fromExtension("hdf"))
		assertEquals(FileCategory.HARD_DRIVES, FileCategory.fromExtension("hdi"))
		assertEquals(FileCategory.HARD_DRIVES, FileCategory.fromExtension("vhd"))
	}

	@Test
	fun `fromExtension returns CD_IMAGES for cd extensions`() {
		assertEquals(FileCategory.CD_IMAGES, FileCategory.fromExtension("iso"))
		assertEquals(FileCategory.CD_IMAGES, FileCategory.fromExtension("cue"))
		assertEquals(FileCategory.CD_IMAGES, FileCategory.fromExtension("chd"))
		assertEquals(FileCategory.CD_IMAGES, FileCategory.fromExtension("nrg"))
		assertEquals(FileCategory.CD_IMAGES, FileCategory.fromExtension("mds"))
	}

	@Test
	fun `fromExtension returns WHDLOAD_GAMES for archive extensions`() {
		assertEquals(FileCategory.WHDLOAD_GAMES, FileCategory.fromExtension("lha"))
		assertEquals(FileCategory.WHDLOAD_GAMES, FileCategory.fromExtension("lzx"))
		assertEquals(FileCategory.WHDLOAD_GAMES, FileCategory.fromExtension("lzh"))
	}

	@Test
	fun `fromExtension is case insensitive`() {
		assertEquals(FileCategory.ROMS, FileCategory.fromExtension("ROM"))
		assertEquals(FileCategory.FLOPPIES, FileCategory.fromExtension("ADF"))
		assertEquals(FileCategory.CD_IMAGES, FileCategory.fromExtension("ISO"))
	}

	@Test
	fun `fromExtension returns null for unknown extension`() {
		assertNull(FileCategory.fromExtension("txt"))
		assertNull(FileCategory.fromExtension("jpg"))
		assertNull(FileCategory.fromExtension(""))
	}

	// --- scanDirectory ---

	@Test
	fun `scanDirectory returns empty for nonexistent dir`() {
		val dir = File(tempDir.root, "nonexistent")
		val result = FileManager.scanDirectory(dir, setOf("adf"))
		assertTrue(result.isEmpty())
	}

	@Test
	fun `scanDirectory returns empty for file instead of dir`() {
		val file = tempDir.newFile("notadir.txt")
		val result = FileManager.scanDirectory(file, setOf("adf"))
		assertTrue(result.isEmpty())
	}

	@Test
	fun `scanDirectory finds files matching extensions`() {
		val dir = tempDir.newFolder(StoragePaths.FLOPPIES)
		File(dir, "game1.adf").writeText("data")
		File(dir, "game2.dms").writeText("data")
		File(dir, "readme.txt").writeText("text")

		val result = FileManager.scanDirectory(dir, setOf("adf", "dms"))

		assertEquals(2, result.size)
		assertTrue(result.any { it.name == "game1.adf" })
		assertTrue(result.any { it.name == "game2.dms" })
	}

	@Test
	fun `scanDirectory ignores files with wrong extension`() {
		val dir = tempDir.newFolder("disks")
		File(dir, "game.adf").writeText("data")
		File(dir, "notes.txt").writeText("text")
		File(dir, "image.png").writeBytes(byteArrayOf(1, 2, 3))

		val result = FileManager.scanDirectory(dir, setOf("adf"))

		assertEquals(1, result.size)
		assertEquals("game.adf", result[0].name)
	}

	@Test
	fun `scanDirectory ignores subdirectories`() {
		val dir = tempDir.newFolder("files")
		File(dir, "game.adf").writeText("data")
		File(dir, "subdir").mkdir()

		val result = FileManager.scanDirectory(dir, setOf("adf"))

		assertEquals(1, result.size)
	}

	@Test
	fun `scanDirectory returns sorted by name case-insensitive`() {
		val dir = tempDir.newFolder("sorted")
		File(dir, "Zebra.adf").writeText("z")
		File(dir, "alpha.adf").writeText("a")
		File(dir, "Beta.adf").writeText("b")

		val result = FileManager.scanDirectory(dir, setOf("adf"))

		assertEquals(3, result.size)
		assertEquals("alpha.adf", result[0].name)
		assertEquals("Beta.adf", result[1].name)
		assertEquals("Zebra.adf", result[2].name)
	}

	@Test
	fun `scanDirectory sets correct AmigaFile fields`() {
		val dir = tempDir.newFolder("test")
		val file = File(dir, "game.adf")
		file.writeText("some floppy data here")

		val result = FileManager.scanDirectory(dir, setOf("adf"))

		assertEquals(1, result.size)
		val amigaFile = result[0]
		assertEquals("game.adf", amigaFile.name)
		assertEquals("adf", amigaFile.extension)
		assertEquals(file.absolutePath, amigaFile.path)
		assertEquals(file.length(), amigaFile.size)
		assertTrue(amigaFile.lastModified > 0)
	}

	@Test
	fun `scanDirectory matches extensions case-insensitively`() {
		val dir = tempDir.newFolder("mixed")
		File(dir, "game1.ADF").writeText("data")
		File(dir, "game2.Adf").writeText("data")
		File(dir, "game3.adf").writeText("data")

		val result = FileManager.scanDirectory(dir, setOf("adf"))

		assertEquals(3, result.size)
	}

	@Test
	fun `scanDirectory calculates CRC32 only for roms dir`() {
		val romsDir = tempDir.newFolder(StoragePaths.ROMS)
		File(romsDir, "kick.rom").writeText("kickstart data")

		val floppiesDir = tempDir.newFolder(StoragePaths.FLOPPIES)
		File(floppiesDir, "game.adf").writeText("floppy data")

		val romResults = FileManager.scanDirectory(romsDir, setOf("rom"))
		val floppyResults = FileManager.scanDirectory(floppiesDir, setOf("adf"))

		assertEquals(1, romResults.size)
		assertNotNull(romResults[0].crc32)

		assertEquals(1, floppyResults.size)
		assertNull(floppyResults[0].crc32)
	}

	@Test
	fun `scanDirectory CRC32 is consistent for same content`() {
		val dir = tempDir.newFolder(StoragePaths.ROMS)
		File(dir, "kick1.rom").writeText("identical content")
		File(dir, "kick2.rom").writeText("identical content")

		val result = FileManager.scanDirectory(dir, setOf("rom"))

		assertEquals(2, result.size)
		assertNotNull(result[0].crc32)
		assertNotNull(result[1].crc32)
		assertEquals(result[0].crc32, result[1].crc32)
	}

	@Test
	fun `scanDirectory CRC32 differs for different content`() {
		val dir = tempDir.newFolder(StoragePaths.ROMS)
		File(dir, "kick13.rom").writeText("kickstart 1.3")
		File(dir, "kick31.rom").writeText("kickstart 3.1")

		val result = FileManager.scanDirectory(dir, setOf("rom"))

		assertEquals(2, result.size)
		assertNotNull(result[0].crc32)
		assertNotNull(result[1].crc32)
		assertNotEquals(result[0].crc32, result[1].crc32)
	}

	// --- AmigaFile.sizeDisplay ---

	@Test
	fun `sizeDisplay formats bytes correctly`() {
		val dir = tempDir.newFolder("sizes")

		// Small file (bytes)
		File(dir, "tiny.adf").writeBytes(ByteArray(500))
		// Medium file (KB)
		File(dir, "medium.adf").writeBytes(ByteArray(50_000))

		val result = FileManager.scanDirectory(dir, setOf("adf"))
		val byName = result.associateBy { it.name }

		assertEquals("500 B", byName["tiny.adf"]!!.sizeDisplay)
		assertEquals("48 KB", byName["medium.adf"]!!.sizeDisplay)
	}

	// --- FileCategory properties ---

	@Test
	fun `FileCategory dirNames are unique`() {
		val dirNames = FileCategory.entries.map { it.dirName }
		assertEquals(dirNames.size, dirNames.toSet().size)
	}

	@Test
	fun `FileCategory extensions have no overlap between categories`() {
		val allExtensions = mutableSetOf<String>()
		for (category in FileCategory.entries) {
			for (ext in category.extensions) {
				assertTrue(
					"Extension '$ext' appears in multiple categories",
					allExtensions.add(ext)
				)
			}
		}
	}
}
