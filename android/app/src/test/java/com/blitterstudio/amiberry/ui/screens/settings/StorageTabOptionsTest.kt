package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class StorageTabOptionsTest {

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

		val aros = settings.withKickstartRom(ModelRomAvailability.AROS_ROM_FILE)
		assertEquals(ModelRomAvailability.AROS_ROM_FILE, aros.romFile)
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
	fun `storage tab exposes all four floppy drives`() {
		val storageTab = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/StorageTab.kt").readText()

		(0..3).forEach { index ->
			assertTrue(
				"StorageTab should render DF$index.",
				storageTab.contains("R.string.settings_storage_drive_df$index")
			)
			assertTrue(
				"DF$index should read its selected disk from settings.floppy$index.",
				storageTab.contains("selectedPath = settings.floppy$index")
			)
			assertTrue(
				"DF$index should read its drive type from settings.floppy${index}Type.",
				storageTab.contains("driveType = settings.floppy${index}Type")
			)
			assertTrue(
				"DF$index selection should update settings.floppy$index.",
				storageTab.contains("s.copy(floppy$index = it)")
			)
			assertTrue(
				"DF$index eject should clear settings.floppy$index.",
				storageTab.contains("s.copy(floppy$index = \"\")")
			)
			assertTrue(
				"DF$index drive type should update settings.floppy${index}Type.",
				storageTab.contains("s.copy(floppy${index}Type = it)")
			)
		}
	}
}
