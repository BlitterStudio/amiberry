package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidNativeLifecycleArchitectureTest {

	@Test
	fun `android native clean exit preserves session marker for Kotlin recovery`() {
		val amiberryCpp = File("../../src/osdep/amiberry.cpp").readText()
		val targetQuit = Regex("""void target_quit\(\)[\s\S]*?#endif""")
			.find(amiberryCpp)
			?.value
			.orEmpty()

		assertTrue(
			"target_quit() should still write the Android clean-exit marker.",
			targetQuit.contains("/.clean_exit")
		)
		assertFalse(
			"target_quit() should not delete .emulator_session; Kotlin resume recovery uses it to distinguish tracked emulator sessions from untracked Advanced GUI exits.",
			targetQuit.contains("remove(session.c_str())")
		)
	}

	@Test
	fun `SDL activity clean exit fallback preserves session marker for Kotlin recovery`() {
		val activity = File("src/main/java/com/blitterstudio/amiberry/AmiberryActivity.java")
			.readText()
		val onDestroy = Regex("""protected void onDestroy\(\) \{[\s\S]*?super\.onDestroy\(\);""")
			.find(activity)
			?.value
			.orEmpty()

		assertTrue(
			"AmiberryActivity.onDestroy() should still write the clean-exit marker as a fallback.",
			onDestroy.contains("writeCleanExitMarker(this)")
		)
		assertFalse(
			"AmiberryActivity.onDestroy() should not clear .emulator_session; Kotlin resume recovery consumes it after seeing .clean_exit.",
			onDestroy.contains("clearSessionMarker(this)")
		)
	}
}
