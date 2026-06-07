package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class AssetExtractionFailureActionsTest {

	@Test
	fun `summary is hidden for missing or successful copy summary`() {
		assertEquals(null, AssetExtractionFailureActions.summary(null))
		assertEquals(null, AssetExtractionFailureActions.summary(AssetExtraction.CopySummary.copied()))
	}

	@Test
	fun `summary lists failed assets and truncates long lists`() {
		val copySummary = AssetExtraction.CopySummary(
			failedAssets = listOf(
				"data/fonts/font.ttf",
				"whdboot/boot-data.zip",
				"controllers/gamecontrollerdb.txt",
				"roms/kick13.rom"
			)
		)

		assertEquals(
			"data/fonts/font.ttf, whdboot/boot-data.zip, controllers/gamecontrollerdb.txt, +1 more",
			AssetExtractionFailureActions.summary(copySummary)
		)
	}

	@Test
	fun `copy payload includes environment and failed assets`() {
		val payload = AssetExtractionFailureActions.copyPayload(
			copySummary = AssetExtraction.CopySummary(
				copiedFiles = 3,
				preservedFiles = 2,
				failedAssets = listOf("data/fonts/font.ttf")
			),
			packageName = "com.blitterstudio.amiberry",
			versionName = "7.0.0",
			versionCode = 7000000L,
			sdkInt = 36,
			externalDirPath = "/storage/emulated/0/Android/data/com.blitterstudio.amiberry/files"
		)

		assertTrue(payload.contains("Amiberry Android setup failure"))
		assertTrue(payload.contains("Package: com.blitterstudio.amiberry"))
		assertTrue(payload.contains("Version: 7.0.0 (7000000)"))
		assertTrue(payload.contains("External files dir: /storage/emulated/0/Android/data/com.blitterstudio.amiberry/files"))
		assertTrue(payload.contains("Copied files: 3"))
		assertTrue(payload.contains("Preserved files: 2"))
		assertTrue(payload.contains("Failed assets: data/fonts/font.ttf"))
	}

	@Test
	fun `copy message is user-visible`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_asset_extraction_details_copied),
			AssetExtractionFailureActions.copyMessage()
		)
	}
}
