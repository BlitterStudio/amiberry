package com.blitterstudio.amiberry.data

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class LaunchArchitectureTest {

	@Test
	fun `UI launch paths do not bypass typed launch requests with raw args`() {
		val productionSources = File("src/main/java")
			.walkTopDown()
			.filter { it.isFile && it.extension in setOf("kt", "java") }
			.filterNot { it.invariantSeparatorsPath.endsWith("data/LaunchRequest.kt") }
			.filterNot { it.invariantSeparatorsPath.endsWith("AmiberryActivity.java") }
			.joinToString(separator = "\n") { file ->
				"// ${file.invariantSeparatorsPath}\n${file.readText()}"
			}

		assertFalse(
			"UI code should launch through typed LaunchRequest helpers, not pre-built argv arrays.",
			productionSources.contains("launchWithArgs(")
		)
		assertFalse(
			"Settings launches should write a config and pass it to a typed launcher, not return raw argv.",
			productionSources.contains("generateLaunchArgs(")
		)
	}

	@Test
	fun `settings screen delegates config file writes to view model`() {
		val settingsScreen = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/SettingsScreen.kt")
			.readText()

		assertFalse(
			"SettingsScreen should not perform config file writes directly from UI code.",
			settingsScreen.contains("ConfigGenerator.writeConfig(")
		)
	}

	@Test
	fun `settings screen ROM import uses shared ROM picker filters`() {
		val settingsScreen = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/SettingsScreen.kt")
			.readText()

		assertFalse(
			"SettingsScreen ROM import should use FilePickerFilters.mimeTypesFor(FileCategory.ROMS), not a broad */* picker.",
			settingsScreen.contains("romPickerLauncher.launch(arrayOf(\"*/*\"))")
		)
	}

	@Test
	fun `main activity delegates quick start config writes to launch config helper`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()

		assertFalse(
			"MainActivity should not write emulator configs directly from ACTION_VIEW handling.",
			mainActivity.contains("ConfigGenerator.writeConfig(")
		)
		assertFalse(
			"MainActivity should use AndroidLaunchConfig instead of importing ConfigGenerator directly.",
			mainActivity.contains("import com.blitterstudio.amiberry.data.ConfigGenerator")
		)
	}

	@Test
	fun `main activity stores new intents before handling them`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()
		val newIntentBody = Regex(
			pattern = """override fun onNewIntent\(intent: Intent\) \{([\s\S]*?)\n\t\}"""
		).find(mainActivity)?.groupValues?.get(1).orEmpty()

		assertTrue(
			"MainActivity.onNewIntent() should call setIntent(intent) before handling ACTION_VIEW data.",
			newIntentBody.contains("setIntent(intent)")
		)
	}
}
