package com.blitterstudio.amiberry

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performScrollTo
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.blitterstudio.amiberry.ui.MainActivity
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class AndroidAppSmokeTest {

	@get:Rule
	val composeRule = createAndroidComposeRule<MainActivity>()

	@Test
	fun launcherShellShowsStableTopLevelLabels() {
		waitForText("Quick Start")

		composeRule.onNodeWithText("Quick Start").assertIsDisplayed()
		composeRule.onNodeWithText("Settings").assertIsDisplayed()
		composeRule.onNodeWithText("Files").assertIsDisplayed()
		composeRule.onNodeWithText("Configs").assertIsDisplayed()
	}

	@Test
	fun onScreenJoystickCanOnlyBeAssignedToJoystickPort() {
		waitForText("Quick Start")

		composeRule.onNodeWithText("Settings").performClick()
		composeRule.onNodeWithTag("settings-tab-5").performScrollTo().performClick()

		composeRule.onNodeWithTag("input-port0-dropdown").performClick()
		assertTagIsNotDisplayed("input-port0-option-onscreen_joy")
		composeRule.onNodeWithTag("input-port0-option-mouse").performClick()

		composeRule.onNodeWithTag("input-port1-dropdown").performClick()
		composeRule.onNodeWithTag("input-port1-option-onscreen_joy").assertIsDisplayed()
	}

	private fun waitForText(text: String) {
		composeRule.waitUntil(timeoutMillis = 10_000) {
			isTextDisplayed(text)
		}
	}

	private fun isTextDisplayed(text: String): Boolean =
		try {
			composeRule.onNodeWithText(text).assertIsDisplayed()
			true
		} catch (_: AssertionError) {
			false
		}

	private fun assertTagIsNotDisplayed(tag: String) {
		if (isTagDisplayed(tag)) {
			throw AssertionError("Found unexpected visible node with tag $tag")
		}
	}

	private fun isTagDisplayed(tag: String): Boolean =
		try {
			composeRule.onNodeWithTag(tag).assertIsDisplayed()
			true
		} catch (_: AssertionError) {
			false
		}
}
