package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import java.io.File

object AndroidLaunchConfig {
	const val QUICKSTART_CONFIG = ".quickstart_settings.uae"
	const val INTENT_QUICKSTART_CONFIG = ".intent_quickstart_settings.uae"
	const val CONTROL_CONFIG = ".android_controls.uae"

	data class PreparedWhdLoad(
		val configFile: File,
		val shaderOverride: String
	)

	fun loadLastSessionSettings(context: Context): EmulatorSettings {
		return try {
			val globalSettingsFile = ShaderCatalog.findSettingsFile(
				context.filesDir,
				context.getExternalFilesDir(null)
			)
			val sessionFile = ConfigGenerator.configFile(context, LAST_SESSION_FILE)
			val legacySessionFile = ConfigGenerator.legacyExternalConfigFile(context, LAST_SESSION_FILE)
			val readableSessionFile = when {
				sessionFile.exists() -> sessionFile
				legacySessionFile.exists() -> legacySessionFile
				else -> return ConfigSettingsResolver.defaults(globalSettingsFile)
			}
			ConfigSettingsResolver.parse(readableSessionFile, globalSettingsFile).settings
		} catch (_: Exception) {
			EmulatorSettings()
		}
	}

	fun buildQuickStartSettingsSnapshot(
		model: AmigaModel,
		roms: List<AmigaFile>,
		currentSettings: EmulatorSettings,
		floppy0: String,
		floppy1: String,
		cdImage: String
	): EmulatorSettings {
		val selectedRoms = ModelRomAvailability.selectRomsForModel(model, roms)

		return EmulatorSettings.fromModel(model).copy(
			romFile = selectedRoms.kick?.path.orEmpty(),
			romExtFile = selectedRoms.ext?.path.orEmpty(),
			floppy0 = floppy0,
			floppy1 = floppy1,
			floppy1Type = if (floppy1.isBlank()) -1 else 0,
			cdImage = cdImage,
			shader = currentSettings.shader,
			joyport0 = currentSettings.joyport0,
			joyport1 = currentSettings.joyport1,
			onScreenJoystick = currentSettings.onScreenJoystick,
			onScreenKeyboard = currentSettings.onScreenKeyboard,
			onScreenKeyboardNumpad = currentSettings.onScreenKeyboardNumpad
		)
	}

	private fun writeControlConfig(
		context: Context,
		settings: EmulatorSettings,
		filename: String = CONTROL_CONFIG
	): File = ConfigGenerator.writeControlConfig(context, settings, filename)

	fun prepareWhdLoad(
		context: Context,
		lhaPath: String,
		currentSettings: EmulatorSettings
	): PreparedWhdLoad {
		val shader = WhdLoadAutoConfig.resolveLaunchShader(
			externalFilesDir = context.getExternalFilesDir(null),
			lhaFile = File(lhaPath),
			currentShader = currentSettings.shader
		)
		return PreparedWhdLoad(
			configFile = writeControlConfig(context, currentSettings.copy(shader = shader)),
			shaderOverride = shader
		)
	}

	fun writeQuickStartConfig(
		context: Context,
		settings: EmulatorSettings,
		filename: String = INTENT_QUICKSTART_CONFIG
	): File = ConfigGenerator.writeConfig(context, settings, filename)

	private const val LAST_SESSION_FILE = ".last_session.uae"
}
