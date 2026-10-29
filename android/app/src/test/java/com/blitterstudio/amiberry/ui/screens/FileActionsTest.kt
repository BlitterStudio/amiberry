package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class FileActionsTest {

	@Test
	fun `disks go in a drive, games launch, and every file can be deleted`() {
		assertEquals(
			listOf(FileAction.InsertDf0AndStart, FileAction.InsertDf1, FileAction.Delete),
			fileActionsFor(FileCategory.FLOPPIES)
		)
		assertEquals(listOf(FileAction.LaunchWhdload, FileAction.Delete), fileActionsFor(FileCategory.WHDLOAD_GAMES))
		assertEquals(listOf(FileAction.Delete), fileActionsFor(FileCategory.ROMS))
		assertEquals(listOf(FileAction.Delete), fileActionsFor(FileCategory.CD_IMAGES))
		assertEquals(listOf(FileAction.Delete), fileActionsFor(FileCategory.HARD_DRIVES))
	}

	private val floppyModels = setOf("a500", "a1200", "a600")
	private val kickstarts = setOf("a1200")

	private fun choose(current: String, available: List<String>) = floppyLaunchModel(
		current = current,
		available = available,
		hasFloppy = { it in floppyModels },
		hasKickstart = { it in kickstarts }
	)

	@Test
	fun `a disk boots Quick Start's model when it has a drive`() {
		assertEquals("a500", choose("a500", listOf("a500", "a1200", "cd32")))
	}

	@Test
	fun `a model without a drive gives way to one with its Kickstart, then any with a drive`() {
		assertEquals("a1200", choose("cd32", listOf("a500", "a1200", "cd32")))
		assertEquals("a500", choose("cd32", listOf("a500", "a600", "cd32")))
		assertNull(choose("cd32", listOf("cd32")))
	}
}
