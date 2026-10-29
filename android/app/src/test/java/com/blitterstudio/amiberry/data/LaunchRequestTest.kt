package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class LaunchRequestTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `settings config request loads the complete config and skips gui`() {
		val path = "/tmp/a1200.uae"

		val args = LaunchRequest.SettingsConfig(configPath = path).toArgs()

		// No --apply-preset: the config holds the whole machine, and a config
		// load would discard a preset applied before it.
		assertArrayEquals(
			arrayOf("--rescan-roms", "--config", path, "-G"),
			args
		)
	}

	@Test
	fun `quick start request includes config floppy and skip gui`() {
		val configPath = "/tmp/quickstart.uae"

		val args = LaunchRequest.QuickStart(
			model = AmigaModel.A500,
			configPath = configPath,
			floppy0 = "/tmp/game.adf",
			floppy1 = "",
			cd = ""
		).toArgs()

		assertArrayEquals(
			arrayOf("--rescan-roms", "--config", configPath, "-0", "/tmp/game.adf", "-G"),
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
			arrayOf("--rescan-roms", "--config", "/tmp/.quickstart_settings.uae", "-G"),
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
	fun `rp9 request uses manifest autoload and skip gui`() {
		val path = "/tmp/game.rp9"
		assertArrayEquals(
			arrayOf("--rescan-roms", "--autoload", path, "-G"),
			LaunchRequest.Rp9(path).toArgs()
		)
	}

	@Test
	fun `rp9 request applies shader and Android controls after manifest autoload`() {
		val path = "/tmp/game.rp9"
		val args = LaunchRequest.Rp9(
			path = path,
			shaderOverride = "presets/custom.glslp",
			controlOverrides = LaunchRequest.AndroidControlOverrides(
				joyport0 = "mouse",
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboardButton = true,
				onScreenKeyboardNumpad = true
			)
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--autoload", path,
				"-s", "amiberry.shader=presets/custom.glslp",
				"-s", "joyport0=mouse",
				"-s", "amiberry.onscreen_joystick=true",
				"-s", "amiberry.vkbd_enabled=true",
				"-s", "amiberry.vkbd_touch_button=true",
				"-s", "amiberry.vkbd_numpad=true",
				"-s", "input.default_osk=true",
				"-s", "amiberry.vkbd_toggle=default",
				"-s", "joyport0mousemap=0",
				"-s", "joyport1mousemap=0",
				"-s", "input.joymouse_speed_analog=25",
				"-G"
			),
			args
		)
	}

	@Test
	fun `android control overrides include vkbd toggle when set`() {
		val args = LaunchRequest.AndroidControlOverrides(
			joyport0 = "mouse",
			joyport1 = "joy0",
			onScreenJoystick = false,
			onScreenKeyboardButton = true,
			onScreenKeyboardToggle = "rightstick"
		).toArgs()

		assertTrue(args.contains("amiberry.vkbd_toggle=rightstick"))
	}

	@Test
	fun `android control overrides emit default reset sentinel when toggle unset`() {
		val args = LaunchRequest.AndroidControlOverrides(
			joyport0 = "mouse",
			joyport1 = "joy0",
			onScreenJoystick = false,
			onScreenKeyboardButton = true
		).toArgs()

		assertTrue(args.contains("amiberry.vkbd_toggle=default"))
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
	fun `whdload request reapplies shader after autoload`() {
		val lhaPath = "/tmp/game.lha"
		val configPath = "/tmp/android-controls.uae"

		val args = LaunchRequest.WhdLoad(
			lhaPath = lhaPath,
			configPath = configPath,
			shaderOverride = "presets/custom.glslp"
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--config", configPath,
				"--autoload", lhaPath,
				"-s", "amiberry.shader=presets/custom.glslp",
				"-G"
			),
			args
		)
	}

	@Test
	fun `whdload request passes the launcher's ports after autoload so they replace WHDBooter's`() {
		val lhaPath = "/tmp/game.lha"
		val configPath = "/tmp/android-controls.uae"

		val args = LaunchRequest.WhdLoad(
			lhaPath = lhaPath,
			configPath = configPath,
			shaderOverride = "tv",
			controlOverrides = LaunchRequest.AndroidControlOverrides(
				joyport0 = "joy0",
				joyport1 = "kbd1",
				onScreenJoystick = false,
				onScreenKeyboardButton = false,
				groups = setOf(AndroidControlSettings.Group.JOYPORT0, AndroidControlSettings.Group.JOYPORT1)
			)
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--config", configPath,
				"--autoload", lhaPath,
				"-s", "amiberry.shader=tv",
				"-s", "joyport0=joy0",
				"-s", "joyport1=kbd1",
				"-G"
			),
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
				onScreenKeyboardButton = false
			),
			skipGui = true
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--config", configPath,
				"-s", "joyport0=mouse",
				"-s", "amiberry.onscreen_joystick=true",
				"-s", "amiberry.vkbd_enabled=true",
				"-s", "amiberry.vkbd_touch_button=false",
				"-s", "amiberry.vkbd_numpad=false",
				"-s", "input.default_osk=true",
				"-s", "amiberry.vkbd_toggle=default",
				"-s", "joyport0mousemap=0",
				"-s", "joyport1mousemap=0",
				"-s", "input.joymouse_speed_analog=25",
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
				onScreenKeyboardButton = true,
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
				"-s", "amiberry.vkbd_touch_button=true",
				"-s", "amiberry.vkbd_numpad=true",
				"-s", "input.default_osk=true",
				"-s", "amiberry.vkbd_toggle=default",
				"-s", "joyport0mousemap=0",
				"-s", "joyport1mousemap=0",
				"-s", "input.joymouse_speed_analog=25",
				"-G"
			),
			args
		)
	}

	@Test
	fun `saved config request applies enabled mouse map overrides`() {
		val configPath = "/tmp/saved.uae"

		val args = LaunchRequest.SavedConfig(
			configPath = configPath,
			controlOverrides = LaunchRequest.AndroidControlOverrides(
				joyport0 = "mouse",
				joyport1 = "joy0",
				onScreenJoystick = false,
				onScreenKeyboardButton = true,
				joyport1MouseMap = true,
				analogMouseSpeed = 50
			),
			skipGui = true
		).toArgs()

		assertArrayEquals(
			arrayOf(
				"--rescan-roms",
				"--config", configPath,
				"-s", "joyport0=mouse",
				"-s", "joyport1=joy0",
				"-s", "amiberry.onscreen_joystick=false",
				"-s", "amiberry.vkbd_enabled=true",
				"-s", "amiberry.vkbd_touch_button=true",
				"-s", "amiberry.vkbd_numpad=false",
				"-s", "input.default_osk=true",
				"-s", "amiberry.vkbd_toggle=default",
				"-s", "joyport0mousemap=0",
				"-s", "joyport1mousemap=1",
				"-s", "input.joymouse_speed_analog=50",
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

	private val remembered = EmulatorSettings(
		joyport0 = "mouse",
		joyport1 = "joy0",
		onScreenJoystick = false,
		onScreenKeyboardButton = false,
		onScreenKeyboardNumpad = true,
		onScreenKeyboardToggle = "select",
		joyport0MouseMap = false,
		joyport1MouseMap = true,
		analogMouseSpeed = 40
	)

	private fun savedConfigArgs(content: String, rememberedControls: EmulatorSettings = remembered): List<String> {
		val file = tempDir.newFile("saved.uae").apply { writeText(content) }
		val overrides = LaunchRequest.AndroidControlOverrides.missingFrom(ConfigParser.parse(file), rememberedControls)
		return LaunchRequest.SavedConfig(file.path, controlOverrides = overrides).toArgs()
			.drop(3).dropLast(1)
	}

	@Test
	fun `saved config keeps its own joyport and keyboard toggle over remembered controls`() {
		val args = savedConfigArgs("joyport0=joy1\namiberry.vkbd_toggle=start\n")

		assertEquals(
			listOf(
				"-s", "joyport1=joy0",
				"-s", "amiberry.onscreen_joystick=false",
				// Derived from the remembered button and the file's toggle.
				"-s", "amiberry.vkbd_enabled=true",
				"-s", "amiberry.vkbd_touch_button=false",
				"-s", "amiberry.vkbd_numpad=true",
				"-s", "input.default_osk=true",
				"-s", "joyport0mousemap=0",
				"-s", "joyport1mousemap=1",
				"-s", "input.joymouse_speed_analog=40"
			),
			args
		)
	}

	@Test
	fun `saved config without control keys gets every remembered control explicitly`() {
		val args = savedConfigArgs(
			"cpu_model=68020\n",
			remembered.copy(onScreenKeyboardToggle = null, joyport1MouseMap = false)
		)

		assertEquals(
			listOf(
				"-s", "joyport0=mouse",
				"-s", "joyport1=joy0",
				"-s", "amiberry.onscreen_joystick=false",
				"-s", "amiberry.vkbd_enabled=true",
				"-s", "amiberry.vkbd_touch_button=false",
				"-s", "amiberry.vkbd_numpad=true",
				"-s", "input.default_osk=true",
				// Remembered Default and disabled mouse maps stay explicit.
				"-s", "amiberry.vkbd_toggle=default",
				"-s", "joyport0mousemap=0",
				"-s", "joyport1mousemap=0",
				"-s", "input.joymouse_speed_analog=40"
			),
			args
		)
	}

	@Test
	fun `saved config with an explicitly disabled keyboard keeps button and toggle off`() {
		val args = savedConfigArgs(
			"amiberry.vkbd_enabled=false\njoyport0=mouse\njoyport1=joy1\namiberry.onscreen_joystick=false\n" +
				"joyport0mousemap=1\njoyport1mousemap=0\ninput.joymouse_speed_analog=10\n"
		)

		assertEquals(listOf("-s", "amiberry.vkbd_numpad=true"), args)
	}

	@Test
	fun `saved config keyboard switch follows a remembered toggle it lacks`() {
		val args = savedConfigArgs(
			"amiberry.vkbd_enabled=true\namiberry.vkbd_touch_button=false\namiberry.vkbd_numpad=false\n" +
				"joyport0=mouse\njoyport1=joy1\namiberry.onscreen_joystick=false\n" +
				"joyport0mousemap=0\njoyport1mousemap=0\ninput.joymouse_speed_analog=25\n",
			remembered.copy(onScreenKeyboardToggle = "")
		)

		assertEquals(
			listOf(
				"-s", "amiberry.vkbd_enabled=false",
				"-s", "input.default_osk=false",
				"-s", "amiberry.vkbd_toggle="
			),
			args
		)
	}

	@Test
	fun `saved config with every control key gets no overrides`() {
		val args = savedConfigArgs(
			"joyport0=mouse\njoyport1=joy1\namiberry.onscreen_joystick=false\n" +
				"amiberry.vkbd_enabled=true\namiberry.vkbd_touch_button=true\namiberry.vkbd_numpad=false\n" +
				"amiberry.vkbd_toggle=default\njoyport0mousemap=0\njoyport1mousemap=0\n" +
				"input.joymouse_speed_analog=25\n"
		)

		assertEquals(emptyList<String>(), args)
	}

	@Test
	fun `launch matrix tracks emulator sessions but not advanced gui`() {
		assertTrue(LaunchRequest.QuickStart(AmigaModel.A500, configPath = "/tmp/quick.uae").trackSession)
		assertTrue(LaunchRequest.SavedConfig("/tmp/saved.uae").trackSession)
		assertTrue(LaunchRequest.SettingsConfig("/tmp/current.uae").trackSession)
		assertTrue(LaunchRequest.WhdLoad("/tmp/game.lha", configPath = "/tmp/controls.uae").trackSession)
		assertTrue(LaunchRequest.Rp9("/tmp/game.rp9").trackSession)

		assertFalse(LaunchRequest.AdvancedGui(configPath = "/tmp/edit.uae").trackSession)
	}

	@Test
	fun `launch matrix does not mix advanced gui with skip gui`() {
		val requests = listOf(
			LaunchRequest.QuickStart(AmigaModel.A500, configPath = "/tmp/quick.uae"),
			LaunchRequest.SavedConfig("/tmp/saved.uae"),
			LaunchRequest.SettingsConfig("/tmp/current.uae"),
			LaunchRequest.WhdLoad("/tmp/game.lha", configPath = "/tmp/controls.uae"),
			LaunchRequest.Rp9("/tmp/game.rp9"),
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
