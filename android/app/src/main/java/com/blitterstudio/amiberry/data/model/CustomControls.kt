package com.blitterstudio.amiberry.data.model

/**
 * The custom controls of a configuration: the Amiga input event each
 * button and axis of the joystick on a port triggers, alone or with the
 * device's hotkey button held. The configuration keeps them per port, for
 * whatever joystick the port has when it loads, as
 * `joyport<N>_amiberry_custom_<none|hotkey>_<control>` lines (axes with
 * `_axis_` after `custom`), the event's name, "none" (the control does
 * nothing) or nothing (the default: the port's mode gives the control its
 * meaning).
 *
 * The launcher writes these lines itself (it runs in another process than
 * the emulator): the schema's custom_controls and input_events describe
 * what there is to map, the way the settings session does for the Apple
 * editor.
 */
data class CustomControls(
	/** The mapped controls by port, hotkey mode and control id; a value of "" leaves a control at the default. */
	val mappings: Map<Key, String> = emptyMap()
) {
	/** One control of one port, alone or with the hotkey held. */
	data class Key(val port: Int, val hotkey: Boolean, val axis: Boolean, val control: String) {
		/** The configuration line's key. */
		val id: String
			get() = "joyport$port" + CUSTOM_PREFIX + (if (axis) "axis_" else "") +
				(if (hotkey) "hotkey" else "none") + "_$control"
	}

	/** The event [key] triggers: "" the default, "none" no event, else an input event's name. */
	fun event(key: Key): String = mappings[key].orEmpty()

	/** [key] mapped to [event] ("" or "none" or an input event's name). */
	fun with(key: Key, event: String): CustomControls {
		val next = if (event.isEmpty()) mappings - key else mappings + (key to event)
		return copy(mappings = next)
	}

	/** Whether any control is mapped away from the default. */
	val anyMapped: Boolean get() = mappings.isNotEmpty()

	companion object {
		private const val CUSTOM_PREFIX = "_amiberry_custom_"

		/** A configuration line's [key], when it is a custom control's (any port, mode and control name). */
		fun lineKey(key: String): Key? {
			if (!key.startsWith("joyport")) return null
			val rest = key.substringAfter(CUSTOM_PREFIX, "")
			if (rest.isEmpty()) return null
			val port = key.removePrefix("joyport").substringBefore(CUSTOM_PREFIX).toIntOrNull() ?: return null
			if (port !in 0..3) return null
			val axis = rest.startsWith("axis_")
			val modes = rest.removePrefix(if (axis) "axis_" else "").split('_', limit = 2)
			if (modes.size != 2 || modes[1].isEmpty()) return null
			val hotkey = when (modes[0]) {
				"none" -> false
				"hotkey" -> true
				else -> return null
			}
			return Key(port, hotkey, axis, modes[1])
		}
	}
}
