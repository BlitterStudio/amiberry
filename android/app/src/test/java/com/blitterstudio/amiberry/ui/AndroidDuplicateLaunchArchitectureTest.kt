package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidDuplicateLaunchArchitectureTest {

	@Test
	fun `launch screens share an in-flight guard`() {
		val quickStart = source("screens/QuickStartScreen.kt")
		val settings = source("screens/settings/SettingsScreen.kt")
		val configurations = source("screens/ConfigurationsScreen.kt")

		listOf(
			"QuickStartScreen" to quickStart,
			"SettingsScreen" to settings,
			"ConfigurationsScreen" to configurations
		).forEach { (name, text) ->
			assertTrue(
				"$name should remember a launch guard so repeated taps cannot start concurrent SDL launches.",
				text.contains("rememberLaunchInFlightGuard()")
			)
			assertTrue(
				"$name should start emulator launches through launchGuarded().",
				text.contains("launchGuarded(launchGuard)")
			)
		}
	}

	@Test
	fun `launch guard exposes begin finish and observable busy state`() {
		val guard = source("LaunchInFlightGuard.kt")

		assertTrue(
			"LaunchInFlightGuard should own the launch-in-flight state.",
			guard.contains("class LaunchInFlightGuard")
		)
		assertTrue(
			"LaunchInFlightGuard should expose an observable isLaunching state for UI controls.",
			guard.contains("var isLaunching by mutableStateOf(false)")
		)
		assertTrue(
			"LaunchInFlightGuard should reject a second begin while a launch is active.",
			Regex("""fun begin\(\): Boolean[\s\S]*if \(isLaunching\)[\s\S]*return false""")
				.containsMatchIn(guard)
		)
		assertTrue(
			"launchGuarded should always finish the guard after the launch block completes.",
			Regex("""fun CoroutineScope\.launchGuarded[\s\S]*try\s*\{[\s\S]*block\(\)[\s\S]*\}\s*finally\s*\{[\s\S]*guard\.finish\(\)""")
				.containsMatchIn(guard)
		)
	}

	private fun source(relativePath: String): String {
		val file = File("src/main/java/com/blitterstudio/amiberry/ui/$relativePath")
		return if (file.exists()) file.readText() else ""
	}
}
