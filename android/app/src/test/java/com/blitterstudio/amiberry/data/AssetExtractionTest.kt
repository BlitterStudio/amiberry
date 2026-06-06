package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class AssetExtractionTest {

	@Test
	fun `storage paths map bundled asset roots to Android storage roots`() {
		assertEquals("Controllers/gamecontrollerdb.txt", AssetExtraction.storagePathForAsset("controllers/gamecontrollerdb.txt"))
		assertEquals("ROMs/kick13.rom", AssetExtraction.storagePathForAsset("roms/kick13.rom"))
		assertEquals("WHDBoot/boot-data.zip", AssetExtraction.storagePathForAsset("whdboot/boot-data.zip"))
		assertEquals("data/fonts/font.ttf", AssetExtraction.storagePathForAsset("data/fonts/font.ttf"))
	}

	@Test
	fun `existing user modifiable assets are preserved`() {
		assertTrue(AssetExtraction.shouldPreserveExistingAsset("Controllers/gamecontrollerdb.txt"))
		assertTrue(AssetExtraction.shouldPreserveExistingAsset("WHDBoot/boot-data.zip"))
		assertTrue(AssetExtraction.shouldPreserveExistingAsset("Configurations/default.uae"))
		assertFalse(AssetExtraction.shouldPreserveExistingAsset("data/fonts/font.ttf"))
	}

	@Test
	fun `copy summary fails when any asset failed`() {
		val summary = AssetExtraction.CopySummary.Empty +
			AssetExtraction.CopySummary.copied() +
			AssetExtraction.CopySummary.failed("data/fonts/font.ttf") +
			AssetExtraction.CopySummary.preserved()

		assertFalse(summary.isSuccess)
		assertEquals(1, summary.copiedFiles)
		assertEquals(1, summary.preservedFiles)
		assertEquals(listOf("data/fonts/font.ttf"), summary.failedAssets)
	}

	@Test
	fun `copy summary succeeds with copied and preserved files only`() {
		val summary = AssetExtraction.CopySummary.Empty +
			AssetExtraction.CopySummary.copied() +
			AssetExtraction.CopySummary.preserved()

		assertTrue(summary.isSuccess)
		assertEquals(1, summary.copiedFiles)
		assertEquals(1, summary.preservedFiles)
		assertTrue(summary.failedAssets.isEmpty())
	}
}
