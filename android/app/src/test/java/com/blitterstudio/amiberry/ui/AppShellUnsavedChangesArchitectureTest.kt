package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AppShellUnsavedChangesArchitectureTest {

	private val source =
		File("src/main/java/com/blitterstudio/amiberry/ui/AmiberryApp.kt").readText()

	@Test
	fun `app shell observes the shared settings view model`() {
		assertTrue(source.contains("SettingsViewModel"))
		assertTrue(source.contains("settingsViewModel.isDirty"))
	}

	@Test
	fun `navigation away from a dirty Settings screen is guarded`() {
		// Bottom-nav/rail clicks route through a guard rather than navigating directly.
		assertTrue(source.contains("guardedNavigate("))
		assertTrue(
			Regex("""Screen\.Settings\.route[\s\S]*settingsViewModel\.isDirty""")
				.containsMatchIn(source) ||
			Regex("""settingsViewModel\.isDirty[\s\S]*Screen\.Settings\.route""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `shows an unsaved-changes dialog with save and discard`() {
		assertTrue(source.contains("R.string.dialog_unsaved_changes_title"))
		assertTrue(source.contains("settingsViewModel.saveTracked()"))
		assertTrue(source.contains("settingsViewModel.discardChanges()"))
		assertTrue(source.contains("R.string.action_discard"))
	}
}
