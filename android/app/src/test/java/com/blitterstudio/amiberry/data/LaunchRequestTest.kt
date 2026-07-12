package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class LaunchRequestTest {

	@Test
	fun `settings config request includes model config and skip gui`() {
		val path = "/tmp/a1200.uae"

		val args = LaunchRequest.SettingsConfig(
			model = AmigaModel.A1200,
			configPath = path
		).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--model", "A1200", "--config", path, "-G"),
			args
		)
	}

	@Test
	fun `quick start request includes model config floppy and skip gui`() {
		val configPath = "/tmp/quickstart.uae"

		val args = LaunchRequest.QuickStart(
			model = AmigaModel.A500,
			configPath = configPath,
			floppy0 = "/tmp/game.adf",
			floppy1 = "",
			cd = ""
		).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--model", "A500", "--config", configPath, "-0", "/tmp/game.adf", "-G"),
			args
		)
	}

	@Test
	fun `quick start launches through generated config so Android controls are honored`() {
		val args = LaunchRequest.QuickStart(
			model = AmigaModel.A500,
			configPath = "/tmp/.quickstart_settings.uae"
		).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--model", "A500", "--config", "/tmp/.quickstart_settings.uae", "-G"),
			args
		)
	}

	@Test
	fun `quick start without generated config preserves direct media args`() {
		val args = LaunchRequest.QuickStart(
			model = AmigaModel.A500,
			configPath = null,
			floppy0 = "/tmp/game.adf"
		).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--model", "A500", "-0", "/tmp/game.adf", "-G"),
			args
		)
	}

	@Test
	fun `advanced gui request includes config and use gui override`() {
		val configPath = "/tmp/edit.uae"

		val args = LaunchRequest.AdvancedGui(configPath = configPath).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--config", configPath, "-s", "use_gui=yes"),
			args
		)
	}

	@Test
	fun `whdload request uses autoload and skip gui`() {
		val lhaPath = "/tmp/game.lha"

		val args = LaunchRequest.WhdLoad(lhaPath = lhaPath).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--autoload", lhaPath, "-G"),
			args
		)
	}

	@Test
	fun `whdload request can include control config before autoload`() {
		val lhaPath = "/tmp/game.lha"
		val configPath = "/tmp/android-controls.uae"

		val args = LaunchRequest.WhdLoad(lhaPath = lhaPath, configPath = configPath).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--config", configPath, "--autoload", lhaPath, "-G"),
			args
		)
	}

	@Test
	fun `saved config request with skip gui true adds skip gui flag`() {
		val configPath = "/tmp/saved.uae"

		val args = LaunchRequest.SavedConfig(configPath = configPath, skipGui = true).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--config", configPath, "-G"),
			args
		)
	}

	@Test
	fun `saved config request applies Android control overrides without loading another config`() {
		val configPath = "/tmp/saved.uae"

		val args = LaunchRequest.SavedConfig(
			configPath = configPath,
			controlOverrides = LaunchRequest.AndroidControlOverrides(
				joyport0 = "mouse",
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboard = false
			),
			skipGui = true
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--config", configPath,
				"-s", "joyport0=mouse",
				"-s", "amiberry.onscreen_joystick=true",
				"-s", "amiberry.vkbd_enabled=false",
				"-s", "amiberry.vkbd_numpad=false",
				"-s", "input.default_osk=false",
				"-G"
			),
			args
		)
	}

	@Test
	fun `saved config request applies physical port one override`() {
		val configPath = "/tmp/saved.uae"

		val args = LaunchRequest.SavedConfig(
			configPath = configPath,
			controlOverrides = LaunchRequest.AndroidControlOverrides(
				joyport0 = "mouse",
				joyport1 = "joy1",
				onScreenJoystick = false,
				onScreenKeyboard = true,
				onScreenKeyboardNumpad = true
			),
			skipGui = true
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--config", configPath,
				"-s", "joyport0=mouse",
				"-s", "joyport1=joy1",
				"-s", "amiberry.onscreen_joystick=false",
				"-s", "amiberry.vkbd_enabled=true",
				"-s", "amiberry.vkbd_numpad=true",
				"-s", "input.default_osk=true",
				"-G"
			),
			args
		)
	}

	@Test
	fun `saved config request with skip gui false omits skip gui flag`() {
		val configPath = "/tmp/saved.uae"

		val args = LaunchRequest.SavedConfig(configPath = configPath, skipGui = false).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--config", configPath),
			args
		)
	}

	@Test
	fun `launch matrix tracks emulator sessions but not advanced gui`() {
		assertTrue(LaunchRequest.QuickStart(AmigaModel.A500, configPath = "/tmp/quick.uae").trackSession)
		assertTrue(LaunchRequest.SavedConfig("/tmp/saved.uae").trackSession)
		assertTrue(LaunchRequest.SettingsConfig(AmigaModel.A1200, "/tmp/current.uae").trackSession)
		assertTrue(LaunchRequest.WhdLoad("/tmp/game.lha", configPath = "/tmp/controls.uae").trackSession)

		assertFalse(LaunchRequest.AdvancedGui(configPath = "/tmp/edit.uae").trackSession)
	}

	@Test
	fun `launch matrix does not mix advanced gui with skip gui`() {
		val requests = listOf(
			LaunchRequest.QuickStart(AmigaModel.A500, configPath = "/tmp/quick.uae"),
			LaunchRequest.SavedConfig("/tmp/saved.uae"),
			LaunchRequest.SettingsConfig(AmigaModel.A1200, "/tmp/current.uae"),
			LaunchRequest.WhdLoad("/tmp/game.lha", configPath = "/tmp/controls.uae"),
			LaunchRequest.AdvancedGui(configPath = "/tmp/edit.uae")
		)

		requests.forEach { request ->
			val args = request.toArgs().toList()
			val hasSkipGui = "-G" in args
			val hasUseGuiYes = args.windowed(2).any { it == listOf("-s", "use_gui=yes") }

			assertFalse("${request::class.simpleName} must not combine -G with use_gui=yes", hasSkipGui && hasUseGuiYes)
			if (request is LaunchRequest.AdvancedGui) {
				assertTrue("Advanced GUI must force native GUI mode", hasUseGuiYes)
				assertFalse("Advanced GUI must not skip native GUI", hasSkipGui)
			}
		}
	}
}
