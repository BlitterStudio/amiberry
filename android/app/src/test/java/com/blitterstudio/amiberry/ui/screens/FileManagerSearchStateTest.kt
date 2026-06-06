package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Test

class FileManagerSearchStateTest {

	@Test
	fun `search is shown only when a category has more than five files`() {
		assertFalse(shouldShowFileManagerSearch(fileCount = 5))
		assertTrue(shouldShowFileManagerSearch(fileCount = 6))
	}

	@Test
	fun `hidden search ignores stale query while it is being cleared`() {
		assertEquals("", effectiveFileManagerSearchQuery("kick", fileCount = 5))
		assertEquals("kick", effectiveFileManagerSearchQuery("kick", fileCount = 6))
	}

	@Test
	fun `file filtering returns same list for blank query and matches names case insensitively`() {
		val files = listOf(
			file("Kick31.rom"),
			file("Workbench.adf"),
			file("Lotus.adf")
		)

		assertSame(files, filterFileManagerFiles(files, ""))
		assertEquals(listOf(file("Kick31.rom")), filterFileManagerFiles(files, "kick"))
		assertEquals(listOf(file("Workbench.adf")), filterFileManagerFiles(files, "BENCH"))
	}

	private fun file(name: String): AmigaFile =
		AmigaFile(
			path = "/files/$name",
			name = name,
			extension = name.substringAfterLast('.'),
			size = 1,
			lastModified = 1,
			category = FileCategory.FLOPPIES
		)
}
