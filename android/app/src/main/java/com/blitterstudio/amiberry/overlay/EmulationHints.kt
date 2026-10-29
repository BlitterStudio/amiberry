package com.blitterstudio.amiberry.overlay

import android.content.Context
import android.widget.Toast
import androidx.annotation.StringRes
import androidx.core.content.edit
import com.blitterstudio.amiberry.R

/**
 * One-time hints as the emulation starts: how to open the in-game menu and,
 * on a TV with no gamepad, what the remote does. Each shows once per install.
 * Kept in their own preferences file: the emulator process writes it, and
 * the launcher process never rewrites it from a stale copy.
 */
object EmulationHints {
	/** The menu hint: Back opens the menu. */
	const val MENU = "menu"
	/** The TV remote hint: D-pad, OK and Back. */
	const val REMOTE = "remote"

	/**
	 * The hint to show, as its key; null when it was seen. The remote hint
	 * also tells about Back, so it stands for the menu hint too.
	 */
	fun next(remoteActive: Boolean, seen: Set<String>): String? = when {
		remoteActive -> REMOTE.takeUnless { it in seen }
		else -> MENU.takeUnless { it in seen || REMOTE in seen }
	}

	@StringRes
	fun text(hint: String, isTv: Boolean): Int = when {
		hint == REMOTE -> R.string.hint_remote_joystick
		isTv -> R.string.hint_menu_tv
		else -> R.string.hint_menu
	}

	/** Shows the hint the emulation's start or the remote taking port 2 calls for, once. */
	fun showIfNew(context: Context, remoteActive: Boolean, isTv: Boolean) {
		val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
		val seen = prefs.getStringSet(KEY_SEEN, emptySet()).orEmpty()
		val hint = next(remoteActive, seen) ?: return
		prefs.edit { putStringSet(KEY_SEEN, seen + hint) }
		Toast.makeText(context, text(hint, isTv), Toast.LENGTH_LONG).show()
	}

	private const val PREFS_NAME = "amiberry_emulation_hints"
	private const val KEY_SEEN = "seen"
}
