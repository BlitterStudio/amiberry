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
	fun `shows an unsaved-changes dialog with save and discard`() {
		assertTrue(source.contains("R.string.dialog_unsaved_changes_title"))
		assertTrue(source.contains("settingsViewModel.saveTracked()"))
		assertTrue(source.contains("settingsViewModel.discardChanges()"))
	}

	@Test
	fun `guard save failure surfaces feedback instead of getting stuck`() {
		assertTrue(source.contains("Toast"))
		assertTrue(source.contains("R.string.msg_failed_save_config"))
	}
}
