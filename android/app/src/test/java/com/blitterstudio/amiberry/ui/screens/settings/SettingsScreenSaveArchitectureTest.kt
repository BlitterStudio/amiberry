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
	fun `top bar has a Save action that saves the tracked config`() {
		assertTrue(
			Regex("""actions = \{[\s\S]*viewModel\.saveTracked\(\)""").containsMatchIn(source)
		)
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
	fun `save dialog is seeded with the current name and description`() {
		assertTrue(
			Regex("""SaveConfigDialog\([\s\S]*initialName[\s\S]*initialDescription""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `settings screen exposes dependency feedback and intent presets`() {
		assertTrue(source.contains("SettingsAdjustmentBanner("))
		assertTrue(source.contains("SettingsPresetSelector("))
		assertTrue(source.contains("viewModel.applyIntentPreset"))
	}
}
