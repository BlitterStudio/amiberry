package com.blitterstudio.amiberry.overlay

import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.screens.settings.SettingsGroups
/**
 * The configuration options the menu changes over a running emulation. The
 * quick pages keep frequently used controls together; All settings adds the
 * generic rows that do not already belong to another in-game page.
 */
object OverlaySettings {
	const val ON_SCREEN_JOYSTICK = "amiberry.onscreen_joystick"
	const val ON_SCREEN_KEYBOARD = "amiberry.vkbd_enabled"
	const val PORT_1 = "joyport0"
	const val PORT_2 = "joyport1"
	const val SHADER = "amiberry.shader"
	const val CD_VOLUME = "sound_volume_cd"
	const val MOUSE_SPEED = "input.mouse_speed"
	const val ANALOG_MOUSE_SPEED = "input.joymouse_speed_analog"

	/** The Controls page's port devices (choices the layer lists at run time). */
	val PORTS = listOf(PORT_1, PORT_2)

	/** The Controls page's switches. */
	val CONTROLS = listOf(ON_SCREEN_JOYSTICK, ON_SCREEN_KEYBOARD)

	/**
	 * The Controls page's pointer speeds, set while a game runs: the mouse
	 * and touch pointer's, and the stick mouse's.
	 */
	val POINTER = listOf(MOUSE_SPEED, ANALOG_MOUSE_SPEED)

	/** The Settings page's options, in the order shown. */
	val SETTINGS = listOf(
		"sound_volume",
		"sound_volume_paula",
		CD_VOLUME,
		"cpu_speed",
		SHADER,
		"amiberry.gfx_auto_crop"
	)

	/** Every option the quick pages read. */
	val ALL: List<String> = PORTS + CONTROLS + POINTER + SETTINGS

	/**
	 * The generic options All settings shows. Ports and on-screen controls
	 * already have purpose-built Controls rows; the drive image and type
	 * fields stay on Drives, while its other settings (such as speed) remain
	 * available here.
	 */
	private val MEDIA = setOf("cdimage0") + (0..3).flatMap { listOf("floppy$it", "floppy${it}type") } +
		(0 until 20).map { "diskimage$it" }

	/**
	 * The in-game overlay changes settings through the live native bridge; it
	 * has no file browser or keyboard recorder, so paths, text and hotkeys
	 * stay in the machine editor.
	 */
	fun supportsInlineRow(option: SettingsSchema.Option): Boolean =
		SettingsGroups.rowKind(option) in setOf(
			SettingsGroups.RowKind.Switch,
			SettingsGroups.RowKind.Choice,
			SettingsGroups.RowKind.Level,
			SettingsGroups.RowKind.Number
		)

	fun allSettings(schema: SettingsSchema): List<SettingsSchema.Option> =
		schema.options.values.filter { option ->
			option.id !in MEDIA &&
				option.group != "diskswapper" &&
				option.id !in (SettingsGroups.managed + PORTS + CONTROLS) &&
				!option.presetOnly &&
				!option.hiddenOnAndroid &&
				supportsInlineRow(option)
		}

	/** Every option the overlay reads while its menu is open. */
	fun readIds(schema: SettingsSchema?): List<String> =
		(ALL + schema?.let(::allSettings).orEmpty().map { it.id }).distinct()

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

/** The strongest application still needed for committed non-live changes. */
enum class PendingApply {
	NONE, RESET, RESTART;

	companion object {
		fun merge(current: PendingApply, apply: Int): PendingApply = when {
			current == RESTART || apply == OverlayNative.APPLY_RESTART -> RESTART
			current == RESET || apply == OverlayNative.APPLY_RESET -> RESET
			else -> NONE
		}
	}
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
