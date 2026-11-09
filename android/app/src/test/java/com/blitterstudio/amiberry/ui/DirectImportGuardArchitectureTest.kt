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
	fun `library direct imports are guarded and controls show busy state`() {
		val library = source("screens/library/LibraryScreen.kt")

		assertTrue(
			"LibraryScreen should remember a direct import guard for ROM imports.",
			library.contains("val importGuard = rememberImportInFlightGuard()")
		)
		assertTrue(
			"LibraryScreen should read import busy state for its import controls.",
			library.contains("val importInProgress = importGuard.isImporting")
		)
		assertTrue(
			"LibraryScreen should guard its direct ROM import callbacks (files and folders).",
			library.split("launchImportGuarded(importGuard)").size - 1 >= 2
		)
		assertTrue(
			"The ROM notice import button should be disabled while a direct import is active.",
			Regex("""Button\(onClick = onImport, enabled = !importInProgress""")
				.containsMatchIn(library)
		)
	}

	@Test
	fun `settings direct imports are guarded and controls show busy state`() {
		val settings = source("screens/settings/SettingsScreen.kt")
		val storage = source("screens/settings/StoragePage.kt")

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
			"StoragePage should remember a direct import guard for floppy imports.",
			storage.contains("val importGuard = rememberImportInFlightGuard()")
		)
		assertTrue(
			"StoragePage should guard direct floppy import callbacks.",
			storage.contains("launchImportGuarded(importGuard)")
		)
		assertTrue(
			"StoragePage should show an importing label while direct floppy import is active.",
			storage.contains("R.string.action_importing")
		)
	}

	private fun source(relativePath: String): String {
		val file = File("src/main/java/com/blitterstudio/amiberry/ui/$relativePath")
		return if (file.exists()) file.readText() else ""
	}
}
