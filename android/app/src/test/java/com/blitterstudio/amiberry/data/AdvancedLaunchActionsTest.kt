package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class AdvancedLaunchActionsTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `saved config target requires an existing visible config file`() {
		val config = tempDir.newFile("A1200.uae")

		assertEquals(
			AdvancedLaunchActions.ConfigTarget.Ready(config.path),
			AdvancedLaunchActions.savedConfigTarget(config.path)
		)

		val missing = AdvancedLaunchActions.savedConfigTarget(tempDir.root.resolve("missing.uae").path)

		assertTrue(missing is AdvancedLaunchActions.ConfigTarget.Failed)
		assertEquals(
			R.string.msg_failed_launch_advanced,
			(missing as AdvancedLaunchActions.ConfigTarget.Failed).message.stringRes
		)

		listOf(
			tempDir.newFolder("Directory.uae").path,
			tempDir.newFile(".temp_native_ui.uae").path,
			tempDir.newFile("notes.txt").path
		).forEach { invalidPath ->
			val invalid = AdvancedLaunchActions.savedConfigTarget(invalidPath)
			assertTrue("$invalidPath should not be editable as a saved configuration.", invalid is AdvancedLaunchActions.ConfigTarget.Failed)
		}
	}

	@Test
	fun `settings advanced config uses transient config filename`() {
		assertEquals(".temp_native_ui.uae", AdvancedLaunchActions.SETTINGS_ADVANCED_CONFIG)
	}
}
