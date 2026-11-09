package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class StoragePageOptionsTest {

	private val rom = AmigaFile(
		path = "/roms/kick31.rom",
		name = "kick31.rom",
		extension = "rom",
		size = 1,
		lastModified = 1,
		category = FileCategory.ROMS
	)

	private fun romFile(name: String) = rom.copy(path = "/roms/$name", name = name)

	@Test
	fun `the Kickstart list leaves out the AROS files, which do not boot as a Kickstart`() {
		val roms = listOf(rom, romFile("aros-rom.bin"), romFile("AROS-EXT.BIN"))

		assertEquals(listOf(rom), kickstartRomFiles(roms))
	}

	@Test
	fun `picking AROS stores the built-in AROS value and drops the extended ROM`() {
		val settings = EmulatorSettings(romFile = "/roms/kick31.rom", romExtFile = "/roms/ext.rom")

		val aros = settings.withKickstartRom(ModelRomAvailability.arosRomFile)
		assertEquals(ModelRomAvailability.arosRomFile, aros.romFile)
		assertEquals("", aros.romExtFile)

		val kick = settings.withKickstartRom("/roms/kick13.rom")
		assertEquals("/roms/kick13.rom", kick.romFile)
		assertEquals("/roms/ext.rom", kick.romExtFile)
	}

	@Test
	fun `file dropdown is hidden only when no files and no selected path exist`() {
		assertFalse(shouldShowStorageFileDropdown(emptyList(), ""))
		assertTrue(shouldShowStorageFileDropdown(emptyList(), "/external/kick13.rom"))
		assertTrue(shouldShowStorageFileDropdown(listOf(rom), ""))
	}

	@Test
	fun `the storage page reads and writes each of the four drives by number`() {
		val settings = EmulatorSettings(
			floppy0 = "df0.adf", floppy1 = "df1.adf", floppy2 = "df2.adf", floppy3 = "df3.adf",
			floppy0Type = 0, floppy1Type = 1, floppy2Type = -1, floppy3Type = 0
		)

		(0..3).forEach { index ->
			assertEquals("df$index.adf", settings.drivePath(index))
			assertEquals("other.adf", settings.withDrivePath(index, "other.adf").drivePath(index))
			assertEquals("", settings.withDrivePath(index, "").drivePath(index))
		}
		assertEquals(1, settings.driveType(1))
		assertEquals(-1, settings.driveType(2))
		assertEquals(0, settings.withDriveType(2, 0).driveType(2))

		// Writing one drive leaves the others alone.
		assertEquals("df0.adf", settings.withDrivePath(3, "x.adf").drivePath(0))
	}
}
