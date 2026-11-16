package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

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
			arrayOf(
				"application/octet-stream",
				"application/zip",
				"application/gzip",
				"application/x-7z-compressed"
			),
			FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES)
		)
	}

	@Test
	fun `content imports accept every floppy archive MIME type`() {
		val manifest = File("src/main/AndroidManifest.xml").readText()
		val contentFilter = Regex("""<intent-filter>[\s\S]*?<data android:scheme="content" />[\s\S]*?</intent-filter>""")
			.find(manifest)
			?.value
			.orEmpty()

		FilePickerFilters.mimeTypesFor(FileCategory.FLOPPIES)
			.filterNot { it == "application/octet-stream" }
			.forEach { mimeType ->
				assertTrue(contentFilter.contains("""android:mimeType="$mimeType""""))
			}
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
			arrayOf(".rom", ".bin", ".roz", ".a1000", ".a500", ".a600", ".a1200", ".a3000", ".a4000", ".cdtv", ".cd32"),
			FilePickerFilters.extensionLabelsFor(FileCategory.ROMS)
		)
	}
}
