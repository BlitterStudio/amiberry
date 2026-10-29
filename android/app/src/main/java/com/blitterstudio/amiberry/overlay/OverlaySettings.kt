package com.blitterstudio.amiberry.overlay

import com.blitterstudio.amiberry.data.model.SettingsSchema

/**
 * The configuration options the menu changes over a running emulation. Only
 * options the settings layer applies live are offered: the menu commits each
 * change at once and the running machine picks it up between frames, so an
 * option needing a reset or restart would silently wait.
 */
object OverlaySettings {
	const val ON_SCREEN_JOYSTICK = "amiberry.onscreen_joystick"
	const val ON_SCREEN_KEYBOARD = "amiberry.vkbd_enabled"
	const val PORT_1 = "joyport0"
	const val PORT_2 = "joyport1"
	const val SHADER = "amiberry.shader"
	const val CD_VOLUME = "sound_volume_cd"

	/** The Controls page's port devices (choices the layer lists at run time). */
	val PORTS = listOf(PORT_1, PORT_2)

	/** The Controls page's switches. */
	val CONTROLS = listOf(ON_SCREEN_JOYSTICK, ON_SCREEN_KEYBOARD)

	/** The Settings page's options, in the order shown. */
	val SETTINGS = listOf(
		"sound_volume",
		"sound_volume_paula",
		CD_VOLUME,
		"cpu_speed",
		SHADER,
		"amiberry.gfx_auto_crop"
	)

	/** Every option the menu reads. */
	val ALL: List<String> = PORTS + CONTROLS + SETTINGS

	/** The values of the two ports swapped, as [OverlayNative.changeSettings] sets them; null unless both are known. */
	fun swappedPorts(settings: Map<String, SettingState>): Map<String, String>? {
		val port1 = settings[PORT_1]?.value ?: return null
		val port2 = settings[PORT_2]?.value ?: return null
		return mapOf(PORT_1 to port2, PORT_2 to port1)
	}

	private const val APPLY_LIVE = "live"

	/** The options of [ids] the schema lists as live, in order; the rest are not offered. */
	fun offered(schema: SettingsSchema?, ids: List<String>): List<SettingsSchema.Option> =
		ids.mapNotNull { schema?.option(it) }.filter { it.apply == APPLY_LIVE }
}

/** An option's pending value and whether the layer offers it now. */
data class SettingState(val value: String, val available: Boolean) {
	companion object {
		/** Reads [OverlayNative.readSettings] for [ids]; empty when the answer does not fit them. */
		fun parse(ids: List<String>, fields: Array<String>?): Map<String, SettingState> {
			if (fields == null || fields.size != ids.size * 2) return emptyMap()
			return ids.indices.associate { i -> ids[i] to SettingState(fields[i * 2], fields[i * 2 + 1] == "1") }
		}
	}
}
