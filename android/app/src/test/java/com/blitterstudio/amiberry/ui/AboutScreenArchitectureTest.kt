package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AboutPageArchitectureTest {

	@Test
	fun `about screen uses centralized link actions instead of hardcoded urls`() {
		val source = aboutScreenSource()

		assertTrue(
			"AboutPage should render links from AboutActions so URL changes are testable outside Compose.",
			source.contains("AboutActions.externalLinks()")
		)
		assertFalse(
			"AboutPage should not hardcode the GitHub URL inline.",
			source.contains("https://github.com/BlitterStudio/amiberry")
		)
		assertFalse(
			"AboutPage should not hardcode the Discord URL inline.",
			source.contains("https://discord.gg/wWndKTGpGV")
		)
	}

	@Test
	fun `about support info includes storage context needed for Android troubleshooting`() {
		val source = aboutScreenSource()

		assertTrue(
			"AboutPage should report whether the app has All Files Access.",
			source.contains("hasFullStorageAccess = hasFullStorageAccess(context)")
		)
		assertTrue(
			"AboutPage should include the app external files directory in copied support info.",
			source.contains("externalFilesDir = context.getExternalFilesDir(null)?.absolutePath")
		)
	}

	private fun aboutScreenSource(): String =
		File("src/main/java/com/blitterstudio/amiberry/ui/screens/appsettings/AboutPage.kt").readText()
}
