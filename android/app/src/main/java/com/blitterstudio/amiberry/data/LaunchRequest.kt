package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints

sealed interface LaunchRequest {
	val trackSession: Boolean
		get() = true

	fun toArgs(): Array<String>

	/**
	 * The Settings screen's configuration. The launcher writes the whole
	 * machine into it (the preset's compatibility level included), and a
	 * configuration load starts from the defaults, so no preset is applied.
	 */
	data class SettingsConfig(
		val configPath: String
	) : LaunchRequest {
		override fun toArgs(): Array<String> =
			arrayOf("--rescan-roms", "--config", configPath, "-G")
	}

	/** A Quick Start launch: the configuration the launcher wrote for [model], then its media. */
	data class QuickStart(
		val model: AmigaModel,
		val configPath: String,
		val floppy0: String? = null,
		val floppy1: String? = null,
		val cd: String? = null
	) : LaunchRequest {
		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms", "--config", configPath)
			if (model.hasFloppy) {
				floppy0?.takeIf { it.isNotBlank() }?.let {
					args.addAll(listOf("-0", it))
				}
				floppy1?.takeIf { it.isNotBlank() }?.let {
					args.addAll(listOf("-1", it))
				}
			}
			if (model.hasCd) {
				cd?.takeIf { it.isNotBlank() }?.let {
					args.addAll(listOf("-s", "cdimage0=$it"))
				}
			}

			args.add("-G")
			return args.toTypedArray()
		}
	}

	data class AdvancedGui(
		val configPath: String? = null
	) : LaunchRequest {
		override val trackSession: Boolean = false

		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms")

			if (configPath != null) {
				args.addAll(listOf("--config", configPath))
			}
			args.addAll(listOf("-s", "use_gui=yes"))
			return args.toTypedArray()
		}
	}

	data class WhdLoad(
		val lhaPath: String,
		val configPath: String? = null,
		val shaderOverride: String? = null,
		val controlOverrides: AndroidControlOverrides? = null
	) : LaunchRequest {
		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms")
			configPath?.takeIf { it.isNotBlank() }?.let {
				args.addAll(listOf("--config", it))
			}
			args.addAll(listOf("--autoload", lhaPath))
			// WHDBooter rebuilds the machine (or loads the game's own configuration)
			// and assigns the joystick ports itself, so the shader and the Android
			// controls follow autoload, where they replace its choices.
			shaderOverride?.let { args.addAll(listOf("-s", "amiberry.shader=$it")) }
			controlOverrides?.let { args.addAll(it.toArgs()) }
			args.add("-G")
			return args.toTypedArray()
		}
	}

	data class Rp9(
		val path: String,
		val shaderOverride: String? = null,
		val controlOverrides: AndroidControlOverrides? = null
	) : LaunchRequest {
		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms", "--autoload", path)
			// RP9 rebuilds machine preferences, so Android-only overrides must follow autoload.
			shaderOverride?.let { args.addAll(listOf("-s", "amiberry.shader=$it")) }
			controlOverrides?.let { args.addAll(it.toArgs()) }
			args.add("-G")
			return args.toTypedArray()
		}
	}

	/**
	 * Android control keys passed as `-s` after the configuration. Only [groups] are
	 * emitted; each emitted group is fully explicit so the configuration cannot keep
	 * part of it.
	 */
	data class AndroidControlOverrides(
		val joyport0: String,
		val joyport1: String,
		val onScreenJoystick: Boolean,
		val onScreenKeyboardButton: Boolean,
		val onScreenKeyboardNumpad: Boolean = false,
		val onScreenKeyboardToggle: String? = null,
		val joyport0MouseMap: Boolean = false,
		val joyport1MouseMap: Boolean = false,
		val analogMouseSpeed: Int = EmulatorSettingsConstraints.DEFAULT_ANALOG_MOUSE_SPEED,
		val groups: Set<AndroidControlSettings.Group> = AndroidControlSettings.Group.entries.toSet()
	) {
		fun toArgs(): List<String> {
			val args = mutableListOf<String>()
			fun set(group: AndroidControlSettings.Group, value: String) {
				if (group in groups) args.addAll(listOf("-s", value))
			}

			set(AndroidControlSettings.Group.JOYPORT0, "joyport0=$joyport0")
			if (joyport1 != "onscreen_joy") {
				set(AndroidControlSettings.Group.JOYPORT1, "joyport1=$joyport1")
			}
			set(AndroidControlSettings.Group.ON_SCREEN_JOYSTICK, "amiberry.onscreen_joystick=${onScreenJoystick.toCfg()}")
			// The native master switch follows the button and the toggle, so it is
			// re-derived whenever either of them is overridden.
			val switchOverridden = AndroidControlSettings.Group.KEYBOARD_BUTTON in groups ||
				AndroidControlSettings.Group.KEYBOARD_TOGGLE in groups
			val keyboardAvailable = EmulatorSettings.onScreenKeyboardAvailable(
				onScreenKeyboardButton, onScreenKeyboardToggle
			).toCfg()
			if (switchOverridden) args.addAll(listOf("-s", "amiberry.vkbd_enabled=$keyboardAvailable"))
			set(AndroidControlSettings.Group.KEYBOARD_BUTTON, "amiberry.vkbd_touch_button=${onScreenKeyboardButton.toCfg()}")
			set(AndroidControlSettings.Group.KEYBOARD_NUMPAD, "amiberry.vkbd_numpad=${onScreenKeyboardNumpad.toCfg()}")
			if (switchOverridden) args.addAll(listOf("-s", "input.default_osk=$keyboardAvailable"))
			// null = "emulator default": emit the explicit reset sentinel so a
			// vkbd_toggle set in a backing config cannot survive the selection.
			// "" (explicitly disabled) and button names round-trip via the
			// target-prefixed key (cfgfile only routes amiberry.* to the
			// Amiberry option parser).
			set(AndroidControlSettings.Group.KEYBOARD_TOGGLE, "amiberry.vkbd_toggle=${onScreenKeyboardToggle ?: "default"}")
			// Mouse map overrides are always explicit (0 or 1) so a backing
			// config's enabled value cannot survive a disabled switch
			set(AndroidControlSettings.Group.JOYPORT0_MOUSE_MAP, "joyport0mousemap=${if (joyport0MouseMap) "1" else "0"}")
			set(AndroidControlSettings.Group.JOYPORT1_MOUSE_MAP, "joyport1mousemap=${if (joyport1MouseMap) "1" else "0"}")
			// Explicit too, so a backing config's speed cannot beat the slider
			set(AndroidControlSettings.Group.ANALOG_MOUSE_SPEED, "input.joymouse_speed_analog=$analogMouseSpeed")

			return args
		}

		companion object {
			fun fromSettings(settings: EmulatorSettings): AndroidControlOverrides =
				AndroidControlOverrides(
					joyport0 = settings.joyport0,
					joyport1 = settings.joyport1,
					onScreenJoystick = settings.onScreenJoystick,
					onScreenKeyboardButton = settings.onScreenKeyboardButton,
					onScreenKeyboardNumpad = settings.onScreenKeyboardNumpad,
					onScreenKeyboardToggle = settings.onScreenKeyboardToggle,
					joyport0MouseMap = settings.joyport0MouseMap,
					joyport1MouseMap = settings.joyport1MouseMap,
					analogMouseSpeed = settings.analogMouseSpeed
				)

			/**
			 * Overrides for a saved configuration: its own control keys win, and
			 * [remembered] only fills the groups it lacks (the editor's
			 * [AndroidControlSettings.withFallback] rule). Null when it sets them all.
			 */
			fun missingFrom(config: ConfigParser.ParsedConfig, remembered: EmulatorSettings): AndroidControlOverrides? {
				val missing = AndroidControlSettings.missingGroups(config.settings, config.explicitKeys)
				if (missing.isEmpty()) return null
				val effective = AndroidControlSettings.withFallback(config.settings, config.explicitKeys, remembered)
				return fromSettings(effective).copy(groups = missing)
			}
		}
	}

	data class SavedConfig(
		val configPath: String,
		val skipGui: Boolean = true,
		val controlOverrides: AndroidControlOverrides? = null
	) : LaunchRequest {
		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms", "--config", configPath)

			controlOverrides?.let { args.addAll(it.toArgs()) }
			if (skipGui) {
				args.add("-G")
			}
			return args.toTypedArray()
		}
	}
}

private fun Boolean.toCfg(): String = if (this) "true" else "false"
