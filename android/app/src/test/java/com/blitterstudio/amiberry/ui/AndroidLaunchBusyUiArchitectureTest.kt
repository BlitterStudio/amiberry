package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidLaunchBusyUiArchitectureTest {

	@Test
	fun `library launch actions are disabled while preparing SDL launch`() {
		val library = source("screens/library/LibraryScreen.kt")
		val machines = source("screens/library/LibraryMachinesPane.kt")

		assertTrue(
			"LibraryScreen should read the guard's observable launch state.",
			library.contains("val launchInProgress = launchGuard.isLaunching")
		)
		assertTrue(
			"LibraryScreen quick play should not start while a launch is already being prepared.",
			library.split("if (launchInProgress) return").size - 1 >= 3
		)
		assertTrue(
			"The Play a File toolbar action should be disabled while a launch is in progress.",
			Regex("""IconButton\([\s\S]*onClick = \{ if \(!launchInProgress\) playFilePicker\.pickFiles\(\) \}[\s\S]*enabled = !launchInProgress""")
				.containsMatchIn(library)
		)
		assertTrue(
			"LibraryMachinesPane should pass launch busy state into saved machine rows.",
			machines.contains("launchInProgress = launchInProgress")
		)
		assertTrue(
			"Saved machine Advanced launch menu item should be disabled while a launch is in progress.",
			Regex("""DropdownMenuItem\([\s\S]*text = \{ Text\(stringResource\(R\.string\.action_edit_advanced_imgui\)\) \}[\s\S]*enabled = !launchInProgress""")
				.containsMatchIn(machines)
		)
	}

	@Test
	fun `secondary launch actions are disabled while preparing SDL launch`() {
		val settings = source("screens/settings/SettingsScreen.kt")

		assertTrue(
			"Settings Advanced launch menu item should be disabled while a launch is in progress.",
			Regex("""DropdownMenuItem\([\s\S]*text = \{ Text\(stringResource\(R\.string\.action_advanced\)\) \}[\s\S]*enabled = !launchInProgress""")
				.containsMatchIn(settings)
		)
	}

	@Test
	fun `settings start launch reports SDL start failures separately from config write failures`() {
		val settings = source("screens/settings/SettingsScreen.kt")

		assertTrue(
			"Settings should keep a separate snackbar message for writing .current_settings.uae.",
			settings.contains("val configWriteFailedMessage = stringResource(R.string.msg_failed_save_config)")
		)
		assertTrue(
			"Settings should keep a separate snackbar message for SDL launch failures.",
			settings.contains("val launchFailedMessage = stringResource(R.string.msg_failed_launch_config)")
		)
		assertTrue(
			"Settings Start should report failures from EmulatorLauncher.launchSettingsConfig.",
			Regex("""runCatching\s*\{[\s\S]*EmulatorLauncher\.launchSettingsConfig\([\s\S]*\)[\s\S]*\}\.onFailure\s*\{[\s\S]*snackbarHostState\.showSnackbar\(launchFailedMessage\)""")
				.containsMatchIn(settings)
		)
	}

	private fun source(relativePath: String): String {
		val file = File("src/main/java/com/blitterstudio/amiberry/ui/$relativePath")
		return if (file.exists()) file.readText() else ""
	}
}
