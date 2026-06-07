package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class DirectImportGuardArchitectureTest {

	@Test
	fun `direct import guard exposes observable busy state and guarded coroutine helper`() {
		val guard = source("ImportInFlightGuard.kt")

		assertTrue(
			"ImportInFlightGuard should own direct file-import state separately from emulator launch state.",
			guard.contains("class ImportInFlightGuard")
		)
		assertTrue(
			"ImportInFlightGuard should expose an observable isImporting state for import controls.",
			guard.contains("var isImporting by mutableStateOf(false)")
		)
		assertTrue(
			"ImportInFlightGuard should reject a second begin while an import is active.",
			Regex("""fun begin\(\): Boolean[\s\S]*if \(isImporting\)[\s\S]*return false""")
				.containsMatchIn(guard)
		)
		assertTrue(
			"launchImportGuarded should always finish the guard after the import block completes.",
			Regex("""fun CoroutineScope\.launchImportGuarded[\s\S]*try\s*\{[\s\S]*block\(\)[\s\S]*\}\s*finally\s*\{[\s\S]*guard\.finish\(\)""")
				.containsMatchIn(guard)
		)
	}

	@Test
	fun `quick start direct imports are guarded and controls show busy state`() {
		val quickStart = source("screens/QuickStartScreen.kt")
		val mediaSelector = source("components/MediaSelector.kt")

		assertTrue(
			"QuickStartScreen should remember a direct import guard.",
			quickStart.contains("val importGuard = rememberImportInFlightGuard()")
		)
		assertTrue(
			"QuickStartScreen should read import busy state for its import controls.",
			quickStart.contains("val importInProgress = importGuard.isImporting")
		)
		assertTrue(
			"QuickStartScreen should guard all direct media and ROM import callbacks.",
			quickStart.split("launchImportGuarded(importGuard)").size - 1 >= 4
		)
		assertTrue(
			"QuickStartScreen should pass import busy state to its media selector import buttons.",
			quickStart.split("importInProgress = importInProgress").size - 1 >= 4
		)
		assertTrue(
			"QuickStartScreen setup ROM import button should be disabled while a direct import is active.",
			Regex("""Button\([\s\S]*enabled = !importInProgress""")
				.containsMatchIn(quickStart)
		)
		assertMediaSelectorBusyState(mediaSelector)
	}

	@Test
	fun `settings direct imports are guarded and controls show busy state`() {
		val settings = source("screens/settings/SettingsScreen.kt")
		val storage = source("screens/settings/StorageTab.kt")

		assertTrue(
			"SettingsScreen should remember a direct import guard for ROM imports.",
			settings.contains("val importGuard = rememberImportInFlightGuard()")
		)
		assertTrue(
			"SettingsScreen should guard direct ROM import callback.",
			settings.contains("launchImportGuarded(importGuard)")
		)
		assertTrue(
			"SettingsScreen setup ROM import button should be disabled while a direct import is active.",
			Regex("""Button\([\s\S]*enabled = !importInProgress""")
				.containsMatchIn(settings)
		)
		assertTrue(
			"StorageTab should remember a direct import guard for floppy imports.",
			storage.contains("val importGuard = rememberImportInFlightGuard()")
		)
		assertTrue(
			"StorageTab should guard direct floppy import callbacks.",
			storage.contains("launchImportGuarded(importGuard)")
		)
		assertTrue(
			"StorageTab should disable floppy import buttons while a direct import is active.",
			Regex("""TextButton\(onClick = \{ importLauncher\.launch\(FilePickerFilters\.mimeTypesFor\(FileCategory\.FLOPPIES\)\) \}, enabled = !importInProgress\)""")
				.containsMatchIn(storage)
		)
		assertTrue(
			"StorageTab should show an importing label while direct floppy import is active.",
			storage.contains("R.string.action_importing")
		)
	}

	private fun assertMediaSelectorBusyState(mediaSelector: String) {
		assertTrue(
			"MediaSelector should accept direct import busy state from its caller.",
			mediaSelector.contains("importInProgress: Boolean = false")
		)
		assertTrue(
			"MediaSelector import button should be disabled while importing.",
			Regex("""TextButton\(onClick = onImport, enabled = !importInProgress\)""")
				.containsMatchIn(mediaSelector)
		)
		assertTrue(
			"MediaSelector should show an importing label while busy.",
			mediaSelector.contains("R.string.action_importing")
		)
	}

	private fun source(relativePath: String): String {
		val file = File("src/main/java/com/blitterstudio/amiberry/ui/$relativePath")
		return if (file.exists()) file.readText() else ""
	}
}
