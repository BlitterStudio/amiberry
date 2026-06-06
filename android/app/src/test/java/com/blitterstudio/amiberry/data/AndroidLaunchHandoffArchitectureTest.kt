package com.blitterstudio.amiberry.data

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidLaunchHandoffArchitectureTest {

	@Test
	fun `SDL activity fallback arguments still rescan ROMs`() {
		val activity = File("src/main/java/com/blitterstudio/amiberry/AmiberryActivity.java")
			.readText()
		val getArguments = Regex("""protected String\[] getArguments\(\) \{[\s\S]*?\n\t\}""")
			.find(activity)
			?.value
			.orEmpty()

		assertTrue(
			"AmiberryActivity should still pass --rescan-roms if Android recreates it without SDL_ARGS.",
			getArguments.contains("\"--rescan-roms\"")
		)
		assertFalse(
			"AmiberryActivity should not fall back to an empty argv on Android.",
			getArguments.contains("new String[0]")
		)
	}

	@Test
	fun `SDL activity launch intent is safe for non activity contexts`() {
		val launcher = File("src/main/java/com/blitterstudio/amiberry/data/EmulatorLauncher.kt")
			.readText()

		assertTrue(
			"EmulatorLauncher should detect whether the supplied Context is activity-backed.",
			launcher.contains("import android.app.Activity") &&
				launcher.contains("import android.content.ContextWrapper") &&
				launcher.contains("isActivityBacked()")
		)
		assertTrue(
			"EmulatorLauncher should add FLAG_ACTIVITY_NEW_TASK only when the Context is not activity-backed.",
			Regex("""if \(!context\.isActivityBacked\(\)\) \{[\s\S]*intent\.addFlags\(Intent\.FLAG_ACTIVITY_NEW_TASK\)""")
				.containsMatchIn(launcher)
		)
	}
}
