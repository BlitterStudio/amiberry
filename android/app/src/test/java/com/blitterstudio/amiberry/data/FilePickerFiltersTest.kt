package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertArrayEquals
import org.junit.Test

class FilePickerFiltersTest {

	@Test
	fun `rom picker prefers binary document types`() {
		assertArrayEquals(
			arrayOf("application/octet-stream", "application/x-binary"),
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
}
