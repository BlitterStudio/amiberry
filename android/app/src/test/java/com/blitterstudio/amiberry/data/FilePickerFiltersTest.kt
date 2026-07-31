package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertArrayEquals
import org.junit.Test

class FilePickerFiltersTest {

	@Test
	fun `rom picker shows all document types for extension validation`() {
		assertArrayEquals(
			arrayOf("*/*"),
			FilePickerFilters.mimeTypesFor(FileCategory.ROMS)
		)
	}

	@Test
	fun `floppy picker includes archive types`() {
		assertArrayEquals(
			arrayOf("application/octet-stream", "application/zip", "application/gzip"),
			FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES)
		)
	}

	@Test
	fun `category labels include accepted extensions`() {
		assertArrayEquals(
			arrayOf(".lha", ".lzx", ".lzh"),
			FilePickerFilters.extensionLabelsFor(FileCategory.WHDLOAD_GAMES)
		)
	}

	@Test
	fun `ROM labels include every supported extension`() {
		assertArrayEquals(
			arrayOf(".rom", ".bin", ".a500", ".a600", ".a1200", ".a3000", ".a4000", ".cdtv", ".cd32"),
			FilePickerFilters.extensionLabelsFor(FileCategory.ROMS)
		)
	}
}
