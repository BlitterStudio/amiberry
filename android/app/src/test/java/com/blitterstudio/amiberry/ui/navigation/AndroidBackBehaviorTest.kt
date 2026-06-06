package com.blitterstudio.amiberry.ui.navigation

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class AndroidBackBehaviorTest {

	@Test
	fun `top-level destinations move the task to background`() {
		Screen.bottomNavItems.forEach { screen ->
			assertTrue(shouldMoveTaskToBack(screen.route))
		}
	}

	@Test
	fun `detail destinations keep normal navigation back behavior`() {
		assertFalse(shouldMoveTaskToBack(Screen.About.route))
	}

	@Test
	fun `visible dialogs keep normal back behavior`() {
		assertFalse(
			shouldMoveTaskToBack(
				route = Screen.QuickStart.route,
				crashDialogVisible = true,
				assetExtractionFailed = false
			)
		)
		assertFalse(
			shouldMoveTaskToBack(
				route = Screen.QuickStart.route,
				crashDialogVisible = false,
				assetExtractionFailed = true
			)
		)
	}
}
