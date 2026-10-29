package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.screens.settings.InputSettingsActions
import java.io.File
import java.util.Properties

/**
 * The on-screen control switches turned in the in-game menu, kept for the
 * next launch. The menu runs in the emulator process, which records them in
 * a file; the launcher process takes them into its settings and remembered
 * controls ([adopt]) when it starts and when the emulation returns to it.
 */
object InGameControls {
	const val ON_SCREEN_JOYSTICK = "amiberry.onscreen_joystick"
	const val ON_SCREEN_KEYBOARD = "amiberry.vkbd_enabled"

	/** Records that the menu turned [id] (one of the switches above) [on]. Emulator process. */
	@Synchronized
	fun record(context: Context, id: String, on: Boolean) {
		if (id != ON_SCREEN_JOYSTICK && id != ON_SCREEN_KEYBOARD) return
		val file = file(context)
		val values = read(file)
		values[id] = on.toString()
		val temporary = File(file.parentFile, "${file.name}.tmp")
		temporary.outputStream().use { values.store(it, null) }
		temporary.renameTo(file)
	}

	/** The switches recorded since the last call, which forgets them. Launcher process. */
	fun take(context: Context): Map<String, Boolean> {
		val file = file(context)
		if (!file.isFile) return emptyMap()
		val values = read(file)
		file.delete()
		return values.stringPropertyNames()
			.filter { it == ON_SCREEN_JOYSTICK || it == ON_SCREEN_KEYBOARD }
			.associateWith { values.getProperty(it) == "true" }
	}

	/**
	 * [settings] with the recorded switches: the joystick as the launcher's
	 * own switch sets it (it takes port 2), the keyboard on as its touch
	 * button (where [touchControls]) and gamepad toggle, off as neither.
	 */
	fun applyTo(settings: EmulatorSettings, changes: Map<String, Boolean>, touchControls: Boolean): EmulatorSettings {
		var result = settings
		changes[ON_SCREEN_JOYSTICK]?.let { result = InputSettingsActions.setOnScreenJoystick(result, it) }
		changes[ON_SCREEN_KEYBOARD]?.let { on ->
			result = if (on) {
				result.copy(
					onScreenKeyboardButton = result.onScreenKeyboardButton || touchControls,
					onScreenKeyboardToggle = result.onScreenKeyboardToggle.takeUnless { it == "" }
				)
			} else {
				result.copy(onScreenKeyboardButton = false, onScreenKeyboardToggle = "")
			}
		}
		return result
	}

	private fun file(context: Context) = File(context.filesDir, FILE_NAME)

	private fun read(file: File): Properties = Properties().also { values ->
		if (file.isFile) runCatching { file.inputStream().use { values.load(it) } }
	}

	private const val FILE_NAME = "in_game_controls.properties"
}
