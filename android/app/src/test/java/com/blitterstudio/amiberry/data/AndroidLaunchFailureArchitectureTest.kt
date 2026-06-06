package com.blitterstudio.amiberry.data

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidLaunchFailureArchitectureTest {

	private val launcher = File("src/main/java/com/blitterstudio/amiberry/data/EmulatorLauncher.kt")
		.readText()

	@Test
	fun `failed tracked SDL activity start clears crash marker before surfacing failure`() {
		val launchSdlActivity = functionBody("private fun launchSdlActivity")

		assertTrue(
			"launchSdlActivity should catch startActivity failures so failed starts do not look like emulator crashes.",
			Regex("""try\s*\{[\s\S]*context\.startActivity\(intent\)[\s\S]*\}\s*catch""")
				.containsMatchIn(launchSdlActivity)
		)
		assertTrue(
			"Tracked launch failures should clear the session marker before rethrowing to the UI.",
			Regex("""catch\s*\([^)]*\)\s*\{[\s\S]*if \(trackSession\)[\s\S]*clearSessionMarker\(context\)[\s\S]*throw""")
				.containsMatchIn(launchSdlActivity)
		)
	}

	@Test
	fun `recent launches are recorded only after SDL activity start succeeds`() {
		assertLaunchBeforeRecentWrite("fun launchQuickStart")
		assertLaunchBeforeRecentWrite("fun launchWithConfig")
		assertLaunchBeforeRecentWrite("fun launchWhdload")
	}

	private fun assertLaunchBeforeRecentWrite(signature: String) {
		val body = functionBody(signature)
		val launchIndex = body.indexOf("launchRequest(context, request)")
		val recentIndex = body.indexOf("AppPreferences.getInstance(context).addRecentLaunch")

		assertTrue("$signature should call launchRequest().", launchIndex >= 0)
		assertTrue("$signature should record a recent launch.", recentIndex >= 0)
		assertTrue(
			"$signature should write Recents only after launchRequest() returns successfully.",
			launchIndex < recentIndex
		)
	}

	private fun functionBody(signature: String): String {
		val start = launcher.indexOf(signature)
		require(start >= 0) { "Missing function signature: $signature" }
		val braceStart = launcher.indexOf('{', start)
		require(braceStart >= 0) { "Missing function body for: $signature" }

		var depth = 0
		for (i in braceStart until launcher.length) {
			when (launcher[i]) {
				'{' -> depth++
				'}' -> {
					depth--
					if (depth == 0) {
						return launcher.substring(braceStart, i + 1)
					}
				}
			}
		}
		error("Unterminated function body for: $signature")
	}
}
