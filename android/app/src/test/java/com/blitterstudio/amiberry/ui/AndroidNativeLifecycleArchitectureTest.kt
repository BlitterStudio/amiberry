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
	fun `SDL activity saves its autosave before SDL pauses the native thread`() {
		val activity = File("src/main/java/com/blitterstudio/amiberry/AmiberryActivity.java").readText()
		val onPause = Regex("""protected void onPause\(\) \{[\s\S]*?super\.onPause\(\);""")
			.find(activity)
			?.value
			.orEmpty()

		assertTrue("AmiberryActivity should request an autosave on pause.", onPause.contains("saveAutosave();"))
		assertTrue(
			"AmiberryActivity must save before SDLActivity pauses the native thread.",
			onPause.indexOf("saveAutosave();") < onPause.indexOf("super.onPause();")
		)
	}

	@Test
	fun `background autosave does not race a normal quit`() {
		val overlay = File("../../src/osdep/android_overlay.cpp").readText()

		assertTrue(
			"An exiting emulator must not start an autosave after target_quit clears it.",
			overlay.contains("savestate_state != 0 || quit_program != 0")
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
