package com.blitterstudio.amiberry.overlay

import androidx.compose.runtime.mutableStateSetOf

/**
 * A key of the on-screen Amiga keyboard, following apple/Shared/UI/
 * AmigaKeyboard.swift: the code is the Amiga raw key code the keyboard
 * hardware defines (src/include/keyboard.h's AK_* names), which reaches the
 * emulation through amiberry_actions_key.
 */
data class AmigaKey(
	/** The Amiga raw key code. */
	val code: Int,
	val label: String,
	/** What the key types with Shift, shown above the label. */
	val shifted: String? = null,
	/** Width in key units. */
	val width: Double = 1.0
) {
	enum class Behaviour {
		/** Down while touched. */
		PLAIN,
		/** Shift, Ctrl, Alt, Amiga: a tap holds it down for the next key. */
		MODIFIER,
		/** Caps Lock: down until tapped again, as its LED shows. */
		TOGGLE
	}

	val behaviour: Behaviour
		get() = when (code) {
			CAPS_LOCK -> Behaviour.TOGGLE
			0x60, 0x61, 0x63, 0x64, 0x65, 0x66, 0x67 -> Behaviour.MODIFIER
			else -> Behaviour.PLAIN
		}

	companion object {
		const val CAPS_LOCK = 0x62
	}
}

/** A keycap: the label, and what the key types with Shift above it. */
data class AmigaKeyLegend(val label: String, val shifted: String?)

/**
 * The keys, row by row, as on an Amiga 500 (US); every main row is 15 keys
 * wide. The codes are what the emulation sees; [legend] swaps only the
 * legends per language.
 */
object AmigaKeyboardLayout {
	val MAIN: List<List<AmigaKey>> = listOf(
		listOf(
			AmigaKey(0x45, "Esc", width = 1.5),
			AmigaKey(0x50, "F1"), AmigaKey(0x51, "F2"), AmigaKey(0x52, "F3"), AmigaKey(0x53, "F4"),
			AmigaKey(0x54, "F5"), AmigaKey(0x55, "F6"), AmigaKey(0x56, "F7"), AmigaKey(0x57, "F8"),
			AmigaKey(0x58, "F9"), AmigaKey(0x59, "F10"),
			AmigaKey(0x46, "Del", width = 1.5), AmigaKey(0x5F, "Help", width = 2.0),
		),
		listOf(
			AmigaKey(0x00, "`", "~"), AmigaKey(0x01, "1", "!"), AmigaKey(0x02, "2", "@"), AmigaKey(0x03, "3", "#"),
			AmigaKey(0x04, "4", "$"), AmigaKey(0x05, "5", "%"), AmigaKey(0x06, "6", "^"), AmigaKey(0x07, "7", "&"),
			AmigaKey(0x08, "8", "*"), AmigaKey(0x09, "9", "("), AmigaKey(0x0A, "0", ")"), AmigaKey(0x0B, "-", "_"),
			AmigaKey(0x0C, "=", "+"), AmigaKey(0x0D, "\\", "|"), AmigaKey(0x41, "⌫"),
		),
		listOf(
			AmigaKey(0x42, "Tab", width = 1.5),
			AmigaKey(0x10, "Q"), AmigaKey(0x11, "W"), AmigaKey(0x12, "E"), AmigaKey(0x13, "R"), AmigaKey(0x14, "T"),
			AmigaKey(0x15, "Y"), AmigaKey(0x16, "U"), AmigaKey(0x17, "I"), AmigaKey(0x18, "O"), AmigaKey(0x19, "P"),
			AmigaKey(0x1A, "[", "{"), AmigaKey(0x1B, "]", "}"), AmigaKey(0x44, "Return", width = 1.5),
		),
		listOf(
			AmigaKey(0x63, "Ctrl", width = 1.5), AmigaKey(AmigaKey.CAPS_LOCK, "Caps", width = 1.5),
			AmigaKey(0x20, "A"), AmigaKey(0x21, "S"), AmigaKey(0x22, "D"), AmigaKey(0x23, "F"), AmigaKey(0x24, "G"),
			AmigaKey(0x25, "H"), AmigaKey(0x26, "J"), AmigaKey(0x27, "K"), AmigaKey(0x28, "L"),
			AmigaKey(0x29, ";", ":"), AmigaKey(0x2A, "'", "\""), AmigaKey(0x2B, "#"),
		),
		listOf(
			AmigaKey(0x60, "Shift", width = 1.25), AmigaKey(0x30, "<", ">"),
			AmigaKey(0x31, "Z"), AmigaKey(0x32, "X"), AmigaKey(0x33, "C"), AmigaKey(0x34, "V"), AmigaKey(0x35, "B"),
			AmigaKey(0x36, "N"), AmigaKey(0x37, "M"), AmigaKey(0x38, ",", "<"), AmigaKey(0x39, ".", ">"),
			AmigaKey(0x3A, "/", "?"), AmigaKey(0x61, "Shift", width = 2.75),
		),
		listOf(
			AmigaKey(0x64, "Alt", width = 1.25), AmigaKey(0x66, "Amiga", width = 1.5),
			AmigaKey(0x40, "", width = 9.5),
			AmigaKey(0x67, "Amiga", width = 1.5), AmigaKey(0x65, "Alt", width = 1.25),
		),
	)

	/** The cursor keys, level with the main block's last two rows (null is a gap of one key). */
	val CURSOR: List<List<AmigaKey?>> = listOf(
		emptyList(), emptyList(), emptyList(), emptyList(),
		listOf(null, AmigaKey(0x4C, "↑")),
		listOf(AmigaKey(0x4F, "←"), AmigaKey(0x4D, "↓"), AmigaKey(0x4E, "→")),
	)

	/** The numeric keypad, level with the main block's first five rows. */
	val KEYPAD: List<List<AmigaKey>> = listOf(
		listOf(AmigaKey(0x5A, "("), AmigaKey(0x5B, ")"), AmigaKey(0x5C, "/"), AmigaKey(0x5D, "*")),
		listOf(AmigaKey(0x3D, "7"), AmigaKey(0x3E, "8"), AmigaKey(0x3F, "9"), AmigaKey(0x4A, "-")),
		listOf(AmigaKey(0x2D, "4"), AmigaKey(0x2E, "5"), AmigaKey(0x2F, "6"), AmigaKey(0x5E, "+")),
		listOf(AmigaKey(0x1D, "1"), AmigaKey(0x1E, "2"), AmigaKey(0x1F, "3"), AmigaKey(0x43, "Enter")),
		listOf(AmigaKey(0x0F, "0", width = 2.0), AmigaKey(0x3C, ".")),
	)

	/** Every key, in layout order. */
	val ALL_KEYS: List<AmigaKey> by lazy {
		MAIN.flatten() + CURSOR.flatten().filterNotNull() + KEYPAD.flatten()
	}

	/**
	 * The keycap of a key in a layout ("US", "UK", "DE", "FR", as the
	 * amiberry.vkbd_language option holds): the codes are the same, only
	 * the legends differ. An unknown layout is the US one.
	 */
	fun legend(key: AmigaKey, layout: String): AmigaKeyLegend =
		OVERRIDES[layout.uppercase()]?.get(key.code)
			?.let { AmigaKeyLegend(it.first, it.second) }
			?: AmigaKeyLegend(key.label, key.shifted)

	private val OVERRIDES: Map<String, Map<Int, Pair<String, String?>>> = mapOf(
		"UK" to mapOf(
			0x0D to ("#" to "~"), 0x2B to ("\\" to "|"), 0x02 to ("2" to "\""), 0x03 to ("3" to "£"),
			0x2A to ("'" to "@"), 0x30 to ("\\" to "|"),
		),
		"DE" to mapOf(
			0x15 to ("Z" to null), 0x31 to ("Y" to null), 0x1A to ("Ü" to null), 0x1B to ("+" to "*"),
			0x29 to ("Ö" to null), 0x2A to ("Ä" to null), 0x00 to ("^" to "°"),
			0x0B to ("ß" to "?"), 0x0C to ("´" to "`"), 0x3A to ("-" to "_"), 0x38 to ("," to ";"),
			0x39 to ("." to ":"), 0x0D to ("#" to "'"), 0x30 to ("<" to ">"), 0x02 to ("2" to "\""),
			0x03 to ("3" to "§"), 0x06 to ("6" to "&"), 0x07 to ("7" to "/"), 0x08 to ("8" to "("),
			0x09 to ("9" to ")"), 0x0A to ("0" to "="),
		),
		"FR" to mapOf(
			0x10 to ("A" to null), 0x11 to ("Z" to null), 0x20 to ("Q" to null), 0x31 to ("W" to null),
			0x29 to ("M" to null), 0x37 to ("," to "?"), 0x38 to (";" to "."), 0x39 to (":" to "/"),
			0x3A to ("!" to null), 0x01 to ("&" to "1"), 0x02 to ("é" to "2"), 0x03 to ("\"" to "3"),
			0x04 to ("'" to "4"), 0x05 to ("(" to "5"), 0x06 to ("-" to "6"), 0x07 to ("è" to "7"),
			0x08 to ("_" to "8"), 0x09 to ("ç" to "9"), 0x0A to ("à" to "0"), 0x0B to (")" to null),
			0x0C to ("=" to "+"), 0x1A to ("^" to null), 0x1B to ("$" to null), 0x00 to ("@" to "#"),
			0x2A to ("ù" to null), 0x30 to ("<" to ">"),
		),
	)
}

/**
 * What the on-screen keyboard has down (apple/Shared/UI/AmigaKeyboard.swift
 * is the model this follows). A modifier tapped stays down until the next
 * other key is released (Shift then A types a capital A); Caps Lock stays
 * down until it is tapped again. Every press is released: [releaseAll] lets
 * go of everything when the keyboard hides.
 */
class AmigaKeyboardModel(private val post: (code: Int, pressed: Boolean) -> Unit) {
	/** Keys down, whether touched or latched. */
	val down = mutableStateSetOf<Int>()
	/** Modifiers and Caps Lock held down by a tap. */
	val latched = mutableStateSetOf<Int>()

	/** For a modifier or Caps Lock being touched: whether the touch latched it (true) or let it go (false), undone if the touch is cancelled. */
	private val toggledByTouch = mutableMapOf<Int, Boolean>()

	/** A finger went down on the key. */
	fun touchDown(key: AmigaKey) {
		when (key.behaviour) {
			AmigaKey.Behaviour.PLAIN ->
				if (key.code !in down) send(key.code, pressed = true)
			AmigaKey.Behaviour.MODIFIER, AmigaKey.Behaviour.TOGGLE -> {
				val latching = key.code !in latched
				toggledByTouch[key.code] = latching
				setLatched(key.code, latching)
			}
		}
	}

	private fun setLatched(code: Int, on: Boolean) {
		if (on) latched.add(code) else latched.remove(code)
		send(code, pressed = on)
	}

	/** The finger lifted (or left the key). */
	fun touchUp(key: AmigaKey) {
		toggledByTouch.remove(key.code)
		if (key.behaviour != AmigaKey.Behaviour.PLAIN || key.code !in down) return
		send(key.code, pressed = false)
		// One key typed with the latched modifiers: they come up after it.
		for (code in latched.toSortedSet()) {
			if (code == AmigaKey.CAPS_LOCK) continue
			latched.remove(code)
			send(code, pressed = false)
		}
	}

	/**
	 * The system took the touch away (a system gesture, too many fingers):
	 * nothing was typed. A plain key comes up; a modifier or Caps Lock goes
	 * back to how it was before the touch; the other latched modifiers stay
	 * for the next key.
	 */
	fun touchCancelled(key: AmigaKey) {
		when (key.behaviour) {
			AmigaKey.Behaviour.PLAIN ->
				if (key.code in down) send(key.code, pressed = false)
			AmigaKey.Behaviour.MODIFIER, AmigaKey.Behaviour.TOGGLE -> {
				val latchedByTouch = toggledByTouch.remove(key.code) ?: return
				if ((key.code in latched) != latchedByTouch) return
				setLatched(key.code, !latchedByTouch)
			}
		}
	}

	/**
	 * Follows the emulation's Caps Lock (its LED), which a hardware
	 * keyboard or a restored state can change: the key shows it, and the
	 * next tap turns it the other way. Nothing is sent.
	 */
	fun syncCapsLock(on: Boolean) {
		if ((AmigaKey.CAPS_LOCK in latched) == on) return
		if (on) {
			latched.add(AmigaKey.CAPS_LOCK)
			down.add(AmigaKey.CAPS_LOCK)
		} else {
			latched.remove(AmigaKey.CAPS_LOCK)
			down.remove(AmigaKey.CAPS_LOCK)
		}
	}

	/**
	 * Lets go of every key, latched modifiers included, when the keyboard
	 * hides or the emulation stops taking input. Caps Lock stays as it is:
	 * it is the Amiga's state, not a key held.
	 */
	fun releaseAll() {
		for (code in down.toSortedSet()) {
			if (code == AmigaKey.CAPS_LOCK) continue
			send(code, pressed = false)
		}
		latched.retainAll { it == AmigaKey.CAPS_LOCK }
		toggledByTouch.clear()
	}

	private fun send(code: Int, pressed: Boolean) {
		if (pressed) down.add(code) else down.remove(code)
		post(code, pressed)
	}
}
