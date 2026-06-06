package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidLaunchBusyUiArchitectureTest {

	@Test
	fun `main launch actions expose busy state while preparing SDL launch`() {
		val quickStart = source("screens/QuickStartScreen.kt")
		val settings = source("screens/settings/SettingsScreen.kt")

		listOf(
			"QuickStartScreen" to quickStart,
			"SettingsScreen" to settings
		).forEach { (name, text) ->
			assertTrue(
				"$name should read the guard's observable launch state.",
				text.contains("val launchInProgress = launchGuard.isLaunching")
			)
			assertTrue(
				"$name should ignore repeated Start presses while a launch is already being prepared.",
				Regex("""ExtendedFloatingActionButton[\s\S]*if \(launchInProgress\) \{[\s\S]*return@ExtendedFloatingActionButton""")
					.containsMatchIn(text)
			)
			assertTrue(
				"$name should show progress feedback while launch preparation is running.",
				text.contains("CircularProgressIndicator")
			)
			assertTrue(
				"$name should swap the Start label to a Launching label while busy.",
				text.contains("R.string.action_launching")
			)
		}
	}

	@Test
	fun `secondary launch actions are disabled while preparing SDL launch`() {
		val quickStart = source("screens/QuickStartScreen.kt")
		val configurations = source("screens/ConfigurationsScreen.kt")
		val settings = source("screens/settings/SettingsScreen.kt")

		assertTrue(
			"QuickStart recent launch buttons should be disabled while a launch is in progress.",
			Regex("""TextButton\([\s\S]*enabled = !launchInProgress""")
				.containsMatchIn(quickStart)
		)
		assertTrue(
			"Settings Advanced launch menu item should be disabled while a launch is in progress.",
			Regex("""DropdownMenuItem\([\s\S]*text = \{ Text\(stringResource\(R\.string\.action_advanced\)\) \}[\s\S]*enabled = !launchInProgress""")
				.containsMatchIn(settings)
		)
		assertTrue(
			"ConfigurationsScreen should pass launch busy state into saved configuration rows.",
			configurations.contains("launchInProgress = launchInProgress")
		)
		assertTrue(
			"Saved configuration launch buttons should be disabled while a launch is in progress.",
			Regex("""IconButton\(onClick = onLaunch, enabled = !launchInProgress\)""")
				.containsMatchIn(configurations)
		)
		assertTrue(
			"Saved configuration Advanced launch menu item should be disabled while a launch is in progress.",
			Regex("""DropdownMenuItem\([\s\S]*text = \{ Text\(stringResource\(R\.string\.action_edit_advanced_imgui\)\) \}[\s\S]*enabled = !launchInProgress""")
				.containsMatchIn(configurations)
		)
	}

	private fun source(relativePath: String): String {
		val file = File("src/main/java/com/blitterstudio/amiberry/ui/$relativePath")
		return if (file.exists()) file.readText() else ""
	}
}
