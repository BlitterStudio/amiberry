package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class WhdLoadPerGameConfigTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private fun launchShader(
		globalSettingsFile: File?,
		externalFilesDir: File?,
		lhaFile: File,
		currentShader: String
	): String = WhdLoadPerGameConfig.launch(
		WhdLoadPerGameConfig.file(globalSettingsFile, externalFilesDir, lhaFile),
		EmulatorSettings(shader = currentShader)
	).settings.shader

	@Test
	fun `launch shader uses explicit per-game selection`() {
		val externalDir = tempDir.newFolder("external")
		val configDir = File(externalDir, "Configurations").apply { mkdirs() }
		File(configDir, "Lotus.uae").writeText("amiberry.shader=presets/game.glslp")

		val shader = launchShader(
			globalSettingsFile = null,
			externalFilesDir = externalDir,
			lhaFile = File("/games/Lotus.lha"),
			currentShader = "presets/current.glslp"
		)

		assertEquals("presets/game.glslp", shader)
	}

	@Test
	fun `launch shader normalizes explicit empty per-game selection`() {
		val externalDir = tempDir.newFolder("empty-shader")
		val configDir = File(externalDir, "Configurations").apply { mkdirs() }
		File(configDir, "Lotus.uae").writeText("amiberry.shader=")

		val shader = launchShader(
			globalSettingsFile = null,
			externalFilesDir = externalDir,
			lhaFile = File("/games/Lotus.lha"),
			currentShader = "presets/current.glslp"
		)

		assertEquals("none", shader)
	}

	@Test
	fun `launch shader falls back when per-game selection is absent`() {
		val externalDir = tempDir.newFolder("fallback")
		val configDir = File(externalDir, "Configurations").apply { mkdirs() }
		File(configDir, "Lotus.uae").writeText("cpu_model=68020")

		val fromConfigWithoutShader = launchShader(
			globalSettingsFile = null,
			externalFilesDir = externalDir,
			lhaFile = File("/games/Lotus.lha"),
			currentShader = "presets/current.glslp"
		)
		val withoutConfig = launchShader(
			globalSettingsFile = null,
			externalFilesDir = externalDir,
			lhaFile = File("/games/Another.lha"),
			currentShader = "presets/current.glslp"
		)
		val withoutExternalDirectory = launchShader(
			globalSettingsFile = null,
			externalFilesDir = null,
			lhaFile = File("/games/Lotus.lha"),
			currentShader = "presets/current.glslp"
		)

		assertEquals("presets/current.glslp", fromConfigWithoutShader)
		assertEquals("presets/current.glslp", withoutConfig)
		assertEquals("presets/current.glslp", withoutExternalDirectory)
	}

	@Test
	fun `launch shader uses configured WHDLoad configuration directory`() {
		val externalDir = tempDir.newFolder("configured-external")
		val customConfigDir = tempDir.newFolder("configured-root")
		val settingsFile = tempDir.newFile("configured-amiberry.conf").also {
			it.writeText("config_path=${customConfigDir.path}")
		}
		File(customConfigDir, "Lotus.uae").writeText("amiberry.shader=presets/game.glslp")

		val shader = launchShader(
			globalSettingsFile = settingsFile,
			externalFilesDir = externalDir,
			lhaFile = File("/games/Lotus.lha"),
			currentShader = "presets/current.glslp"
		)

		assertEquals("presets/game.glslp", shader)
	}

	@Test
	fun `launch without a per-game configuration passes every launcher control after autoload`() {
		val current = EmulatorSettings(joyport0 = "joy0", joyport1 = "kbd1", onScreenJoystick = false)

		val launch = WhdLoadPerGameConfig.launch(perGameConfig = null, current = current)
		val args = LaunchRequest.WhdLoad(
			lhaPath = "/games/Lotus.lha",
			controlOverrides = launch.controls
		).toArgs().toList()

		assertEquals(current, launch.settings)
		val autoload = args.indexOf("--autoload")
		assertTrue(args.indexOf("joyport0=joy0") > autoload)
		assertTrue(args.indexOf("joyport1=kbd1") > autoload)
	}

	@Test
	fun `per-game configuration ports stay the game's and the rest come from the launcher`() {
		val config = tempDir.newFile("Lotus.uae")
		config.writeText("cpu_model=68020\njoyport0=kbd2\n")
		val current = EmulatorSettings(joyport0 = "joy0", joyport1 = "kbd1", onScreenJoystick = false)

		val launch = WhdLoadPerGameConfig.launch(config, current)
		val args = requireNotNull(launch.controls).toArgs()

		assertEquals("kbd2", launch.settings.joyport0)
		assertEquals("kbd1", launch.settings.joyport1)
		assertFalse(args.any { it.startsWith("joyport0=") })
		assertTrue("joyport1=kbd1" in args)
	}
}
