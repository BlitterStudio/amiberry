package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class FileManagerImportGuardArchitectureTest {

	@Test
	fun `file manager view model does not cancel active imports when another import is requested`() {
		val viewModel = File("src/main/java/com/blitterstudio/amiberry/ui/viewmodel/FileManagerViewModel.kt")
			.readText()

		assertTrue(
			"FileManagerViewModel.importFiles should ignore new import requests while an import is already active.",
			Regex("""fun importFiles\(uris: List<Uri>, category: FileCategory\) \{[\s\S]*if \(_isImporting\.value\) \{[\s\S]*return""")
				.containsMatchIn(viewModel)
		)
		assertFalse(
			"Starting a new import should not cancel an active import job and leave partially imported batches.",
			viewModel.contains("importJob?.cancel()")
		)
	}

	@Test
	fun `file manager import controls reflect active import state`() {
		val screen = File("src/main/java/com/blitterstudio/amiberry/ui/screens/FileManagerScreen.kt")
			.readText()

		assertTrue(
			"File picker callback should ignore selected URIs while a scan or import is already active.",
			screen.contains("if (uris.isNotEmpty() && !showProgress)")
		)
		assertTrue(
			"Floating import action should ignore repeated taps while a scan or import is active.",
			Regex("""ExtendedFloatingActionButton[\s\S]*if \(showProgress\) \{[\s\S]*return@ExtendedFloatingActionButton""")
				.containsMatchIn(screen)
		)
		assertTrue(
			"Floating import action should show progress feedback while importing.",
			screen.contains("CircularProgressIndicator")
		)
		assertTrue(
			"Floating import action should swap to an Importing label while busy.",
			screen.contains("R.string.action_importing")
		)
		assertTrue(
			"Empty-state import button should be disabled while scanning or importing unless it is clearing search results.",
			Regex("""Button\([\s\S]*enabled = searchHasNoResults \|\| !showProgress""")
				.containsMatchIn(screen)
		)
	}
}
