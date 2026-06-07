package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Test

class QuickStartMediaStateTest {

	private val diskA = AmigaFile(
		path = "/files/DF0.adf",
		name = "DF0.adf",
		extension = "adf",
		size = 1,
		lastModified = 1,
		category = FileCategory.FLOPPIES
	)
	private val cd = AmigaFile(
		path = "/files/Game.iso",
		name = "Game.iso",
		extension = "iso",
		size = 1,
		lastModified = 1,
		category = FileCategory.CD_IMAGES
	)

	@Test
	fun `media selections are left alone while scanning`() {
		val settings = EmulatorSettings(
			floppy0 = "/missing/DF0.adf",
			floppy1 = "/missing/DF1.adf",
			floppy1Type = 0,
			cdImage = "/missing/Game.iso"
		)

		assertSame(
			settings,
			QuickStartMediaState.pruneMissingMedia(
				settings = settings,
				floppies = listOf(diskA),
				cds = listOf(cd),
				isScanning = true
			)
		)
	}

	@Test
	fun `missing selections are kept when a category has no known files`() {
		val settings = EmulatorSettings(floppy0 = "/missing/DF0.adf", cdImage = "/missing/Game.iso")

		assertSame(
			settings,
			QuickStartMediaState.pruneMissingMedia(
				settings = settings,
				floppies = emptyList(),
				cds = emptyList(),
				isScanning = false
			)
		)
	}

	@Test
	fun `missing selections are cleared when category has known files`() {
		val settings = EmulatorSettings(
			floppy0 = "/missing/DF0.adf",
			floppy1 = "/missing/DF1.adf",
			floppy1Type = 0,
			cdImage = "/missing/Game.iso"
		)

		val pruned = QuickStartMediaState.pruneMissingMedia(
			settings = settings,
			floppies = listOf(diskA),
			cds = listOf(cd),
			isScanning = false
		)

		assertEquals("", pruned.floppy0)
		assertEquals("", pruned.floppy1)
		assertEquals(-1, pruned.floppy1Type)
		assertEquals("", pruned.cdImage)
	}

	@Test
	fun `existing paths outside the repository scan are preserved`() {
		val settings = EmulatorSettings(
			floppy0 = "/external/DF0.adf",
			floppy1 = "/external/DF1.adf",
			floppy1Type = 0,
			cdImage = "/external/Game.iso"
		)

		assertSame(
			settings,
			QuickStartMediaState.pruneMissingMedia(
				settings = settings,
				floppies = listOf(diskA),
				cds = listOf(cd),
				isScanning = false,
				fileExists = { path -> path.startsWith("/external/") }
			)
		)
	}

	@Test
	fun `known selections are preserved`() {
		val settings = EmulatorSettings(
			floppy0 = diskA.path,
			floppy1 = diskA.path,
			floppy1Type = 0,
			cdImage = cd.path
		)

		assertSame(
			settings,
			QuickStartMediaState.pruneMissingMedia(
				settings = settings,
				floppies = listOf(diskA),
				cds = listOf(cd),
				isScanning = false
			)
		)
	}
}
