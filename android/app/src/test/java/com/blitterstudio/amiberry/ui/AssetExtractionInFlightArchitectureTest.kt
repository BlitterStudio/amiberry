package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AssetExtractionInFlightArchitectureTest {

	@Test
	fun `asset extraction startup and retry share an in-flight guard`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()

		assertTrue(
			"MainActivity should expose asset extraction in-flight state for retry UI.",
			mainActivity.contains("var assetExtractionInProgress by mutableStateOf(false)")
		)
		assertTrue(
			"startAssetExtraction should ignore duplicate starts while extraction is active.",
			Regex("""private fun startAssetExtraction\(\) \{[\s\S]*if \(assetExtractionInProgress\) \{[\s\S]*return""")
				.containsMatchIn(mainActivity)
		)
		assertTrue(
			"startAssetExtraction should mark extraction active before launching background work.",
			Regex("""private fun startAssetExtraction\(\) \{[\s\S]*assetExtractionInProgress = true[\s\S]*lifecycleScope\.launch\(Dispatchers\.IO\)""")
				.containsMatchIn(mainActivity)
		)
		assertTrue(
			"startAssetExtraction should always clear the in-flight state in a finally block.",
			Regex("""lifecycleScope\.launch\(Dispatchers\.IO\) \{[\s\S]*try \{[\s\S]*extractAssetsIfNeeded\(\)[\s\S]*\} finally \{[\s\S]*assetExtractionInProgress = false""")
				.containsMatchIn(mainActivity)
		)
	}

	@Test
	fun `asset extraction retry dialog reflects in-flight state`() {
		val app = File("src/main/java/com/blitterstudio/amiberry/ui/AmiberryApp.kt")
			.readText()

		assertTrue(
			"AmiberryApp should read extraction retry in-flight state from MainActivity.",
			app.contains("val assetExtractionInProgress = activity.assetExtractionInProgress == true")
		)
		assertTrue(
			"Retry button should be disabled while extraction retry is already running.",
			Regex("""TextButton\(onClick = \{ activity\.retryAssetExtraction\(\) \}, enabled = !assetExtractionInProgress\)""")
				.containsMatchIn(app)
		)
		assertTrue(
			"Retry button should show a retrying label while extraction retry is active.",
			app.contains("R.string.action_retrying")
		)
	}
}
