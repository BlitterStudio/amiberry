package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SettingsScreenSaveArchitectureTest {

	private val source =
		File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/SettingsScreen.kt").readText()

	@Test
	fun `title shows the open config name or a new-configuration label`() {
		assertTrue(source.contains("viewModel.currentConfigName"))
		assertTrue(source.contains("R.string.settings_new_configuration"))
	}

	@Test
	fun `shows an unsaved-changes indicator driven by isDirty`() {
		assertTrue(source.contains("viewModel.isDirty"))
		assertTrue(source.contains("R.string.settings_unsaved_changes"))
	}

	@Test
	fun `overflow offers Save As`() {
		assertTrue(source.contains("R.string.action_save_as"))
	}

	@Test
	fun `a different-config collision triggers an overwrite confirmation`() {
		assertTrue(source.contains("R.string.dialog_overwrite_config_title"))
		assertTrue(source.contains("allowOverwrite = true"))
		assertTrue(source.contains("ConfigurationSaveActions.SaveResult.AlreadyExists"))
	}

	@Test
	fun `save dialog is seeded with the current name description and category`() {
		assertTrue(
			Regex("""SaveConfigDialog\([\s\S]*initialName[\s\S]*initialDescription[\s\S]*initialCategory""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `save dialog offers existing machine categories`() {
		assertTrue(source.contains("machineCategories(configs.map { it.category })"))
		assertTrue(source.contains("categorySuggestions = categorySuggestions"))
	}

	@Test
	fun `importing an Amiga Forever key preserves the selected Kickstart`() {
		assertTrue(source.contains("!result.isRomKey"))
	}

	@Test
	fun `settings screen exposes a review changes dialog`() {
		assertTrue(source.contains("R.string.action_review_changes"))
		assertTrue(source.contains("SettingsChangesDialog("))
		assertTrue(source.contains("viewModel.changeSummary"))
	}
}
