package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

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
}
