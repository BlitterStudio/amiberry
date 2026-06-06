package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings

sealed interface LaunchRequest {
	val trackSession: Boolean
		get() = true

	fun toArgs(): Array<String>

	data class SettingsConfig(
		val model: AmigaModel,
		val configPath: String
	) : LaunchRequest {
		override fun toArgs(): Array<String> =
			arrayOf("--rescan-roms", "--model", model.cmdArg, "--config", configPath, "-G")
	}

	data class QuickStart(
		val model: AmigaModel,
		val configPath: String? = null,
		val floppy0: String? = null,
		val floppy1: String? = null,
		val cd: String? = null
	) : LaunchRequest {
		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms", "--model", model.cmdArg)

			configPath?.takeIf { it.isNotBlank() }?.let {
				args.addAll(listOf("--config", it))
			}
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
		val configPath: String? = null
	) : LaunchRequest {
		override fun toArgs(): Array<String> {
			val args = mutableListOf("--rescan-roms")
			configPath?.takeIf { it.isNotBlank() }?.let {
				args.addAll(listOf("--config", it))
			}
			args.addAll(listOf("--autoload", lhaPath, "-G"))
			return args.toTypedArray()
		}
	}

	data class AndroidControlOverrides(
		val joyport0: String,
		val joyport1: String,
		val onScreenJoystick: Boolean,
		val onScreenKeyboard: Boolean
	) {
		fun toArgs(): List<String> {
			val args = mutableListOf("-s", "joyport0=$joyport0")

			if (joyport1 != "onscreen_joy") {
				args.addAll(listOf("-s", "joyport1=$joyport1"))
			}
			args.addAll(
				listOf(
					"-s", "amiberry.onscreen_joystick=${onScreenJoystick.toCfg()}",
					"-s", "amiberry.vkbd_enabled=${onScreenKeyboard.toCfg()}",
					"-s", "input.default_osk=${onScreenKeyboard.toCfg()}"
				)
			)

			return args
		}

		companion object {
			fun fromSettings(settings: EmulatorSettings): AndroidControlOverrides =
				AndroidControlOverrides(
					joyport0 = settings.joyport0,
					joyport1 = settings.joyport1,
					onScreenJoystick = settings.onScreenJoystick,
					onScreenKeyboard = settings.onScreenKeyboard
				)
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
