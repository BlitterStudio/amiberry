package com.blitterstudio.amiberry.data.model

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

/**
 * Text for people, resolved where resources are at hand: [Plain] text that
 * comes from the settings layer (labels, choice labels) or from the user
 * (file names), or a launcher [Resource] whose arguments may themselves be
 * [SettingText].
 */
sealed interface SettingText {
	data class Plain(val text: String) : SettingText
	data class Resource(@StringRes val id: Int, val args: List<Any> = emptyList()) : SettingText
}

/**
 * Readable labels for values whose list the settings schema cannot carry:
 * the port devices (the layer lists the host's devices at run time), the
 * built-in shaders (listed with the shaders folder at run time) and the
 * gamepad buttons the on-screen keyboard toggle takes. Stored values keep
 * their .uae spelling; only what people read comes from here.
 */
object LauncherChoices {

	/** The port devices the launcher offers, by .uae token. */
	private val portDeviceLabels = mapOf(
		"none" to R.string.settings_option_none,
		"mouse" to R.string.settings_input_device_mouse,
		"joy0" to R.string.settings_input_device_joystick_0,
		"joy1" to R.string.settings_input_device_joystick_1,
		"onscreen_joy" to R.string.settings_input_device_on_screen_joystick,
		"kbd1" to R.string.settings_input_device_keyboard_layout_1,
		"kbd2" to R.string.settings_input_device_keyboard_layout_2,
		"kbd3" to R.string.settings_input_device_keyboard_layout_3,
		"kbd9" to R.string.settings_input_device_keyboard_layout_4
	)

	/** The label of a port device token; an unknown token (a hand-edited configuration) as it is. */
	fun portDevice(token: String): SettingText =
		portDeviceLabels[token]?.let { SettingText.Resource(it) } ?: SettingText.Plain(token)

	/**
	 * The gamepad buttons that toggle the on-screen keyboard (vkbd_toggle),
	 * by SDL button name: null leaves the key out (the emulator's default
	 * applies), "" unbinds it.
	 */
	val oskToggleButtons: List<Pair<String?, Int>> = listOf(
		null to R.string.settings_osk_toggle_default,
		"leftstick" to R.string.settings_osk_toggle_leftstick,
		"rightstick" to R.string.settings_osk_toggle_rightstick,
		"back" to R.string.settings_osk_toggle_back,
		"start" to R.string.settings_osk_toggle_start,
		"guide" to R.string.settings_osk_toggle_guide,
		"leftshoulder" to R.string.settings_osk_toggle_leftshoulder,
		"rightshoulder" to R.string.settings_osk_toggle_rightshoulder,
		"misc1" to R.string.settings_osk_toggle_misc1,
		"" to R.string.settings_osk_toggle_none
	)

	/** The label of a vkbd_toggle value; an unknown button name as it is. */
	fun oskToggle(value: String?): SettingText =
		oskToggleButtons.firstOrNull { it.first == value }?.let { SettingText.Resource(it.second) }
			?: SettingText.Plain(value.orEmpty())

	/** The built-in shaders, as the settings layer names them. */
	private val builtInShaderLabels = mapOf(
		"none" to R.string.settings_shader_none,
		"tv" to R.string.settings_shader_tv,
		"pc" to R.string.settings_shader_pc,
		"lite" to R.string.settings_shader_lite,
		"1084" to R.string.settings_shader_1084
	)

	/**
	 * The label of an amiberry.shader value: a built-in by name, a preset
	 * from the shaders folder by its file name (not its folder path).
	 */
	fun shader(value: String): SettingText =
		builtInShaderLabels[value]?.let { SettingText.Resource(it) }
			?: SettingText.Plain(fileName(value).ifBlank { value })

	/** The last component of a path ("Workbench.adf" for "/disks/Workbench.adf"). */
	fun fileName(path: String): String = path.substringAfterLast('/').substringAfterLast('\\')
}
