package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class FileManagerScreenArchitectureTest {

	@Test
	fun `file manager exposes a manual refresh action`() {
		val screen = source()

		assertTrue(
			"FileManagerScreen should show a refresh icon for files copied into app storage while the app is open.",
			screen.contains("Icons.Default.Refresh")
		)
		assertTrue(
			"FileManagerScreen refresh action should rescan through the view model.",
			screen.contains("viewModel.rescan()")
		)
		assertTrue(
			"FileManagerScreen refresh action should have an accessible label.",
			screen.contains("R.string.file_manager_refresh")
		)
	}

	@Test
	fun `file manager does not mutate search state during composition`() {
		val screen = source()

		assertFalse(
			"Search cleanup should run from an effect, not by assigning searchQuery during composition.",
			Regex("""if \(allFiles\.size <= 5 && searchQuery\.isNotEmpty\(\)\) \{[\s\S]*searchQuery = ""[\s\S]*\}""")
				.containsMatchIn(screen)
		)
		assertTrue(
			"Search cleanup should be coordinated by LaunchedEffect.",
			screen.contains("LaunchedEffect(showSearch, searchQuery)")
		)
	}

	@Test
	fun `file manager disables delete actions while scan or import is in progress`() {
		val screen = source()

		assertTrue(
			"File list rows should receive the same busy state used by refresh/import controls.",
			screen.contains("deleteEnabled = !showProgress")
		)
		assertTrue(
			"Delete icon buttons should be disabled while file operations are already running.",
			Regex("""IconButton\([\s\S]*enabled = deleteEnabled""")
				.containsMatchIn(screen)
		)
		assertTrue(
			"Delete confirmation should also be disabled if a scan/import starts while the dialog is open.",
			Regex("""TextButton\([\s\S]*enabled = deleteEnabled""")
				.containsMatchIn(screen)
		)
	}

	@Test
	fun `file manager disables import actions while scan or import is in progress`() {
		val screen = source()

		assertTrue(
			"File picker callback should ignore selected URIs while any file operation is already active.",
			screen.contains("if (uris.isNotEmpty() && !showProgress)")
		)
		assertTrue(
			"Floating import action should ignore taps while a scan or import is active.",
			Regex("""ExtendedFloatingActionButton[\s\S]*if \(showProgress\) \{[\s\S]*return@ExtendedFloatingActionButton""")
				.containsMatchIn(screen)
		)
		assertTrue(
			"Floating import action should expose disabled semantics while file operations are active.",
			Regex("""ExtendedFloatingActionButton\([\s\S]*Modifier\.semantics \{ if \(showProgress\) disabled\(\) \}""")
				.containsMatchIn(screen)
		)
		assertTrue(
			"Empty-state import button should be disabled while scanning or importing unless it is clearing search results.",
			Regex("""Button\([\s\S]*enabled = searchHasNoResults \|\| !showProgress""")
				.containsMatchIn(screen)
		)
	}

	private fun source(): String =
		File("src/main/java/com/blitterstudio/amiberry/ui/screens/FileManagerScreen.kt").readText()
}
