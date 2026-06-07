package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
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
