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
	fun `import wording is generic across categories`() {
		assertEquals("Import files", stringResourceValue("action_import_files"))
		assertEquals("Import folder", stringResourceValue("action_import_folder"))
		assertEquals("Import ROM", stringResourceValue("action_import_rom"))
		assertNull(stringResourceValue("action_import_rom_files"))
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
