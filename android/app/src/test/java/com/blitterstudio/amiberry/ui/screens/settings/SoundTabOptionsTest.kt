package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SoundTabOptionsTest {

	@Test
	fun `sound tab builds channel dropdown from the schema's options`() {
		val soundTab = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/SoundTab.kt").readText()

		assertTrue(
			"SoundTab should use the same sound channel list as the settings model and constraint layer (the schema's sound_channels choices).",
			soundTab.contains("id = \"sound_channels\"")
		)
	}
}
