package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
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
	fun `import offers both file and recursive folder pickers for all categories`() {
		val screen = source()

		assertTrue(
			"File Manager import should preserve batch selection.",
			screen.contains("ActivityResultContracts.OpenMultipleDocuments()")
		)
		assertTrue(
			"Users should be able to select a directory tree for any category.",
			screen.contains("ActivityResultContracts.OpenDocumentTree()")
		)
		assertTrue(
			"The selected folder should be imported through the view model for the current category.",
			screen.contains("viewModel.importFolder(uri, currentCategory)")
		)
		assertTrue(screen.contains("R.string.action_import_files"))
		assertFalse(
			"File Manager should not reference the replaced ROM-file import label.",
			screen.contains("R.string.action_import_rom_files")
		)
		assertTrue(screen.contains("R.string.action_import_folder"))
	}

	@Test
	fun `import wording is generic across categories`() {
		assertEquals("Import files", stringResourceValue("action_import_files"))
		assertEquals("Import folder", stringResourceValue("action_import_folder"))
		assertEquals("Import ROM", stringResourceValue("action_import_rom"))
		assertNull(stringResourceValue("action_import_rom_files"))
	}

	@Test
	fun `file list keeps its final row clear of the floating import action`() {
		val screen = source()

		assertTrue(
			"LazyColumn should use weight(1f) to fill remaining vertical space.",
			Regex("""LazyColumn\(\s*modifier = Modifier\.weight\(1f\)""").containsMatchIn(screen)
		)
		assertTrue(
			"LazyColumn content should retain 16dp horizontal, 4dp top, and 80dp bottom scroll clearance.",
			Regex(
				"""contentPadding = PaddingValues\(\s*start = 16\.dp,\s*top = 4\.dp,\s*end = 16\.dp,\s*bottom = 80\.dp\s*\)"""
			).containsMatchIn(screen)
		)
	}

	@Test
	fun `storage path is shown in top bar subtitle not a separate card`() {
		val screen = source()

		assertTrue(
			"TopAppBar should show the storage path as a subtitle.",
			screen.contains("viewModel.getStoragePath()")
		)
		assertFalse(
			"The storage path OutlinedCard should be removed from the body.",
			Regex("""OutlinedCard\([^{]*\{[^{]*getStoragePath""").containsMatchIn(screen)
		)
		assertTrue(
			"A copy-path action should remain accessible from the top bar.",
			screen.contains("clipboardLabelPath") && screen.contains("Icons.Default.ContentCopy")
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
			"Each file row should retain its own delete callback.",
			screen.contains("onDelete = { viewModel.deleteFile(file) }")
		)
		assertTrue(
			"Delete should continue to require local confirmation state.",
			screen.contains("var showDeleteDialog by remember { mutableStateOf(false) }") &&
				screen.contains("if (showDeleteDialog)") &&
				screen.contains("AlertDialog(") &&
				screen.contains("onDelete()")
		)
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

	private fun stringResourceValue(name: String): String? {
		val strings = File("src/main/res/values/strings.xml").readText()
		return Regex("""<string name="$name">([^<]*)</string>""")
			.find(strings)
			?.groupValues
			?.get(1)
	}
}
