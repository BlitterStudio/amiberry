package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class FileListingTest {

	private fun rom(path: String, crc: Long?) = AmigaFile(
		path = path,
		name = path.substringAfterLast('/'),
		extension = path.substringAfterLast('.'),
		size = 524288,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc
	)

	private val isArosExtension: (AmigaFile) -> Boolean = { it.name == "aros-ext.bin" }

	@Test
	fun `ROMs show one AROS row and no WHDBooter copies of a listed Kickstart`() {
		val roms = listOf(
			rom("/data/ROMs/amiga-os-310-a1200.rom", 0x1483a091),
			rom("/data/ROMs/aros-ext.bin", 1),
			rom("/data/ROMs/aros-rom.bin", 2),
			rom("/data/WHDBoot/save-data/Kickstarts/kick40068.A1200", 0x1483a091)
		)

		val visible = FileListing.visibleRoms(roms, isArosExtension)

		assertEquals(listOf("amiga-os-310-a1200.rom", "aros-rom.bin"), visible.map { it.name })
	}

	@Test
	fun `a WHDBooter copy whose original is gone stays listed`() {
		val roms = listOf(
			rom("/data/ROMs/amiga-os-130.rom", 0xc4f0f55f),
			rom("/data/WHDBoot/save-data/Kickstarts/kick40068.A1200", 0x1483a091)
		)

		assertEquals(roms, FileListing.visibleRoms(roms, isArosExtension))
	}

	@Test
	fun `import summary names the categories that received files and reveals a single other one`() {
		val results = listOf(
			FileManager.ImportResult.Imported("/data/Floppies/Lotus.adf", "Lotus.adf"),
			FileManager.ImportResult.Imported("/data/LHA/dynAMIte.lha", "dynAMIte.lha", notWhdloadGame = true),
			FileManager.ImportResult.Unsupported("notes.txt", "txt")
		)

		val mixed = FileListing.importSummary(results, openCategory = FileCategory.ROMS)
		assertEquals(listOf(FileCategory.FLOPPIES, FileCategory.WHDLOAD_GAMES), mixed.categories)
		assertNull(mixed.revealCategory)
		assertEquals(1, mixed.notWhdloadGames)

		val floppiesOnly = FileListing.importSummary(results.take(1), openCategory = FileCategory.ROMS)
		assertEquals(FileCategory.FLOPPIES, floppiesOnly.revealCategory)

		val alreadyOpen = FileListing.importSummary(results.take(1), openCategory = FileCategory.FLOPPIES)
		assertNull(alreadyOpen.revealCategory)
	}
}
