package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SoundTabOptionsTest {

	@Test
	fun `sound tab builds channel dropdown from model options`() {
		val soundTab = File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/SoundTab.kt").readText()

		assertTrue(
			"SoundTab should use the same sound channel list as the settings model and constraint layer.",
			soundTab.contains("EmulatorSettings.soundChannelOptions")
		)
	}
}
