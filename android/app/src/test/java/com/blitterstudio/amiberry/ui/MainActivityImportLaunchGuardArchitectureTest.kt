package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class MainActivityImportLaunchGuardArchitectureTest {

	@Test
	fun `action view import launches are guarded against overlapping SDL starts`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()

		assertTrue(
			"MainActivity should own a launch guard for ACTION_VIEW import launches.",
			mainActivity.contains("private val importLaunchGuard = LaunchInFlightGuard()")
		)
		assertTrue(
			"importAndLaunch should reject a second incoming file launch while one is already being prepared.",
			Regex("""fun importAndLaunch\(uri: Uri, mimeType: String\? = null\) \{[\s\S]*if \(!importLaunchGuard\.begin\(\)\) \{[\s\S]*return""")
				.containsMatchIn(mainActivity)
		)
		assertTrue(
			"importAndLaunch should release the import launch guard after feedback and launch handoff complete.",
			Regex("""lifecycleScope\.launch\(Dispatchers\.IO\) \{[\s\S]*try \{[\s\S]*launchPreparedImport\(preparedImport\.launch\)[\s\S]*\} finally \{[\s\S]*importLaunchGuard\.finish\(\)""")
				.containsMatchIn(mainActivity)
		)
	}

	@Test
	fun `pending action view imports wait for the current import launch to finish`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()
		val app = File("src/main/java/com/blitterstudio/amiberry/ui/AmiberryApp.kt")
			.readText()

		assertTrue(
			"MainActivity should expose the direct import launch guard as observable shell state.",
			mainActivity.contains("val importLaunchInProgress: Boolean")
		)
		assertTrue(
			"MainActivity should back importLaunchInProgress with the ACTION_VIEW launch guard.",
			mainActivity.contains("get() = importLaunchGuard.isLaunching")
		)
		assertTrue(
			"AmiberryApp should observe whether a direct import launch is still being prepared.",
			app.contains("val importLaunchInProgress = activity?.importLaunchInProgress == true")
		)
		assertTrue(
			"AmiberryApp should include importLaunchInProgress in pending import effect keys.",
			Regex("""LaunchedEffect\(pendingUri, pendingMimeType, assetsReady, crashDetected, assetExtractionFailed, importLaunchInProgress\)""")
				.containsMatchIn(app)
		)
		assertTrue(
			"MainActivity should preserve the MIME type that routed the ACTION_VIEW intent.",
			mainActivity.contains("pendingFileMimeType = intent.type")
		)
		assertTrue(
			"Pending imports should pass the original MIME type to classification.",
			app.contains("mainActivity.importAndLaunch(uri, pendingMimeType)") &&
				mainActivity.contains("IntentImportExecutor.importAndPrepare(this@MainActivity, uri, mimeType)")
		)
		assertTrue(
			"PendingImportState should block processing while a direct import launch is already active.",
			app.contains("importLaunchInProgress = importLaunchInProgress")
		)
		assertTrue(
			"Pending ACTION_VIEW imports should wait while the crash recovery dialog is visible.",
			app.contains("crashRecoveryVisible = crashDetected")
		)
	}
}
