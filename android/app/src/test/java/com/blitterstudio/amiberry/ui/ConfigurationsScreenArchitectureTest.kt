package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class ConfigurationsScreenArchitectureTest {

	@Test
	fun `editing a config passes its name so the editor can save it back`() {
		val source = File("src/main/java/com/blitterstudio/amiberry/ui/screens/ConfigurationsScreen.kt").readText()

		assertTrue(
			"Edit should hand the config name to the Settings editor so Save can overwrite it.",
			source.contains("settingsViewModel.loadConfig(result.value, config.name, config.path)")
		)
	}

	@Test
	fun `editing a config switches to the Settings tab so other tabs stay reachable`() {
		val source = File("src/main/java/com/blitterstudio/amiberry/ui/screens/ConfigurationsScreen.kt").readText()
		assertTrue(
			"Navigation to Settings must use the standard tab-switch options (saveState + restoreState), not stack Settings on the Configs entry.",
			source.contains("popUpTo(graph.findStartDestination().id) { saveState = true }") &&
				source.contains("restoreState = true")
		)
		assertFalse(
			"Edit must not push Settings on top of the Configurations entry — that breaks returning to the Configs tab.",
			source.contains("popUpTo(Screen.Configurations.route)")
		)
	}

	@Test
	fun `configurations screen exposes search and list filters`() {
		val source = File("src/main/java/com/blitterstudio/amiberry/ui/screens/ConfigurationsScreen.kt").readText()

		assertTrue(source.contains("ConfigurationSearchControls("))
		assertTrue(source.contains("filterConfigurations("))
		assertTrue(source.contains("ConfigurationListFilter"))
	}
}
