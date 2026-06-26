package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class ConfigurationsScreenArchitectureTest {

	@Test
	fun `configurations screen exposes a manual refresh action`() {
		val source = File("src/main/java/com/blitterstudio/amiberry/ui/screens/ConfigurationsScreen.kt").readText()

		assertTrue(
			"ConfigurationsScreen should provide an app-bar action to rescan saved configurations copied in while the app is open.",
			source.contains("Icons.Default.Refresh")
		)
		assertTrue(
			"ConfigurationsScreen should call the ViewModel refresh action from the app bar.",
			Regex("""TopAppBar\([\s\S]*actions = \{[\s\S]*viewModel\.refresh\(\)""")
				.containsMatchIn(source)
		)
		assertTrue(
			"ConfigurationsScreen refresh action should have an accessible label.",
			source.contains("R.string.configurations_refresh")
		)
	}

	@Test
	fun `editing a config passes its name so the editor can save it back`() {
		val source = File("src/main/java/com/blitterstudio/amiberry/ui/screens/ConfigurationsScreen.kt").readText()

		assertTrue(
			"Edit should hand the config name to the Settings editor so Save can overwrite it.",
			source.contains("settingsViewModel.loadConfig(result.value, config.name)")
		)
	}
}
