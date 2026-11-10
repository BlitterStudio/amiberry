package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class ConfigSettingsResolverTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun newMachinesStartFromAmiberryConfDefaults() {
		val global = tempDir.newFile("defaults-global.conf").also {
			it.writeText(
				"shader=pc\n" +
					"default_sound_frequency=11025\n" +
					"default_sound_buffer=1024\n" +
					"default_correct_aspect_ratio=no\n" +
					"default_scaling_method=2\n"
			)
		}

		val settings = ConfigSettingsResolver.defaults(global)

		assertEquals("pc", settings.shader)
		assertEquals(11025, settings.soundFreq)
		assertEquals(1024, settings.soundBuffer)
		assertEquals(false, settings.correctAspect)
		assertEquals(2, settings.scalingMethod)
	}

	@Test
	fun newMachinesUseAndroidDefaultsWithoutAmiberryConf() {
		val settings = ConfigSettingsResolver.defaults(null)

		// The Android build's own defaults (the schema's android_default).
		assertEquals(48000, settings.soundFreq)
		assertEquals(8192, settings.soundBuffer)
	}

	@Test
	fun legacySessionFallsBackToNativeGlobalShader() {
		val session = tempDir.newFile(".last_session.uae").also {
			it.writeText("cpu_model=68020")
		}
		val global = tempDir.newFile("amiberry.conf").also {
			it.writeText("shader=presets/global.glslp")
		}

		assertEquals(
			"presets/global.glslp",
			ConfigSettingsResolver.parse(session, global).settings.shader
		)
	}

	@Test
	fun legacySavedConfigFallsBackToNativeGlobalShader() {
		val config = tempDir.newFile("Workbench.uae").also {
			it.writeText("cpu_model=68040")
		}
		val global = tempDir.newFile("saved-global.conf").also {
			it.writeText("shader=presets/global.glslp")
		}

		assertEquals(
			"presets/global.glslp",
			ConfigSettingsResolver.parse(config, global).settings.shader
		)
	}

	@Test
	fun explicitSavedShaderWinsOverNativeGlobalShader() {
		val config = tempDir.newFile("explicit.uae").also {
			it.writeText("amiberry.shader=tv")
		}
		val global = tempDir.newFile("explicit-global.conf").also {
			it.writeText("shader=pc")
		}

		assertEquals("tv", ConfigSettingsResolver.parse(config, global).settings.shader)
	}

	@Test
	fun explicitEmptySavedShaderNormalizesToNoneAndWins() {
		val config = tempDir.newFile("empty.uae").also {
			it.writeText("amiberry.shader=")
		}
		val global = tempDir.newFile("empty-global.conf").also {
			it.writeText("shader=1084")
		}

		assertEquals("none", ConfigSettingsResolver.parse(config, global).settings.shader)
	}

	@Test
	fun missingSessionStartsWithNativeGlobalShader() {
		val global = tempDir.newFile("defaults.conf").also {
			it.writeText("shader=lite")
		}

		assertEquals("lite", ConfigSettingsResolver.defaults(global).shader)
	}

	@Test
	fun missingMalformedAndUnreadableGlobalSettingsFallBackToNone() {
		val malformed = tempDir.newFile("malformed.conf").also {
			it.writeText("shader without a value")
		}
		val unreadable = tempDir.newFolder("settings-directory")

		assertEquals("none", ConfigSettingsResolver.defaults(null).shader)
		assertEquals("none", ConfigSettingsResolver.defaults(malformed).shader)
		assertEquals("none", ConfigSettingsResolver.defaults(unreadable).shader)
	}
}
