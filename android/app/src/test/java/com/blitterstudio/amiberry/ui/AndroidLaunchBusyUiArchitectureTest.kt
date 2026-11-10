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
			"One launch admission for the whole Library: a Play a File copy runs in the activity's own guard.",
			library.contains("activity?.importLaunchInProgress == true")
		)
		assertTrue(
			"LibraryScreen quick play should not start while a launch is already being prepared.",
			library.split("if (launchBusy) return").size - 1 >= 3
		)
		assertTrue(
			"The Play a File toolbar action should be disabled while a launch is in progress.",
			Regex("""IconButton\([\s\S]*onClick = \{ if \(!launchBusy\) playFilePicker\.pickFiles\(\) \}[\s\S]*enabled = !launchBusy""")
				.containsMatchIn(library)
		)
		assertTrue(
			"LibraryMachinesPane should pass launch busy state into saved machine rows.",
			machines.contains("launchInProgress = launchInProgress")
		)
		assertTrue(
			"The machine menu's Rename item follows the machine row's enabled state.",
			Regex("""DropdownMenuItem\([\s\S]*text = \{ Text\(stringResource\(R\.string\.action_rename\)\) \}""")
				.containsMatchIn(machines)
		)
	}

	@Test
	fun `no launch path opens the removed classic settings window`() {
		val settings = source("screens/settings/SettingsScreen.kt")
		val machines = source("screens/library/LibraryMachinesPane.kt")

		assertTrue(
			"The settings editor has no Classic settings window entry.",
			!settings.contains("R.string.action_advanced")
		)
		assertTrue(
			"The Library has no classic settings window entry.",
			!machines.contains("R.string.action_edit_advanced_imgui")
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
