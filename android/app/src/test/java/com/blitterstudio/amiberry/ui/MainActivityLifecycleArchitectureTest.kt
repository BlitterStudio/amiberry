package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class MainActivityLifecycleArchitectureTest {

	@Test
	fun `main activity processes deferred resume recovery after assets become ready`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()

		assertTrue(
			"MainActivity should route resume marker handling through a reusable lifecycle method.",
			mainActivity.contains("private fun processResumeRecovery()")
		)
		assertTrue(
			"MainActivity should retry deferred resume recovery after setting isReady=true.",
			Regex("""isReady = true\s+processResumeRecovery\(\)""")
				.containsMatchIn(mainActivity)
		)
	}

	@Test
	fun `main activity defers resume recovery while paused`() {
		val mainActivity = File("src/main/java/com/blitterstudio/amiberry/ui/MainActivity.kt")
			.readText()

		assertTrue(
			"MainActivity should track whether it is currently resumed before showing dialogs or requesting review.",
			mainActivity.contains("private var isActivityResumed = false")
		)
		assertTrue(
			"MainActivity should update resumed state from onPause().",
			mainActivity.contains("override fun onPause()")
		)
		assertTrue(
			"MainActivity should use EmulatorResumeRecovery.shouldDefer() for lifecycle gating.",
			mainActivity.contains("EmulatorResumeRecovery.shouldDefer(")
		)
	}

	@Test
	fun `main activity launch mode matches onNewIntent handler`() {
		val manifest = File("src/main/AndroidManifest.xml").readText()

		assertTrue(
			"MainActivity should be singleTop so foreground ACTION_VIEW intents use onNewIntent() instead of stacking launcher instances.",
			Regex("""<activity android:name="com\.blitterstudio\.amiberry\.ui\.MainActivity"[\s\S]*?android:launchMode="singleTop"""")
				.containsMatchIn(manifest)
		)
	}
}
