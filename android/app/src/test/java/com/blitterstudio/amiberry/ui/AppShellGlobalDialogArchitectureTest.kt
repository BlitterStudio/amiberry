package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AppShellGlobalDialogArchitectureTest {

	@Test
	fun `app shell uses one prioritized global dialog state`() {
		val source = File("src/main/java/com/blitterstudio/amiberry/ui/AmiberryApp.kt").readText()

		assertTrue(
			"AmiberryApp should centralize global dialog priority instead of rendering independent modal dialogs.",
			source.contains("GlobalDialogState.visibleDialog(")
		)
		assertTrue(
			"AmiberryApp should render the selected global dialog from a single when expression.",
			Regex("""when \(visibleGlobalDialog\)[\s\S]*GlobalDialogState\.VisibleDialog\.AssetExtractionFailure[\s\S]*GlobalDialogState\.VisibleDialog\.CrashRecovery""")
				.containsMatchIn(source)
		)
		assertFalse(
			"Crash recovery should not be guarded by an independent top-level if that can stack with setup failure.",
			source.contains("if (activity?.emulatorCrashDetected == true)")
		)
	}
}
