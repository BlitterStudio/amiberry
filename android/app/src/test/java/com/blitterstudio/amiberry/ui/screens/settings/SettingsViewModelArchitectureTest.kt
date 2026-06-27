package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SettingsViewModelArchitectureTest {

	private val source =
		File("src/main/java/com/blitterstudio/amiberry/ui/viewmodel/SettingsViewModel.kt").readText()

	@Test
	fun `tracks the open config name and description`() {
		assertTrue(source.contains("var currentConfigName"))
		assertTrue(source.contains("var currentConfigDescription"))
	}

	@Test
	fun `exposes dirty state derived from a baseline snapshot`() {
		assertTrue(source.contains("baselineSettings"))
		assertTrue(source.contains("baselineUnknownLines"))
		assertTrue(
			Regex("""val isDirty[\s\S]*currentConfigName != null[\s\S]*settings != baselineSettings""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `loadConfig takes the config name`() {
		assertTrue(
			Regex("""fun loadConfig\(\s*parsed: ConfigParser\.ParsedConfig,\s*name: String""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `exposes save and discard operations`() {
		assertTrue(source.contains("suspend fun saveTracked()"))
		assertTrue(source.contains("suspend fun saveAs("))
		assertTrue(source.contains("fun discardChanges()"))
		assertTrue(source.contains("configRepository.saveResolved("))
	}

	@Test
	fun `tracks the exact config path and overwrites it on save`() {
		assertTrue(source.contains("var currentConfigPath"))
		assertTrue(source.contains("configRepository.overwriteConfigAtPath("))
		assertTrue(Regex("""fun loadConfig\([\s\S]*name: String,\s*path: String""").containsMatchIn(source))
	}
}
