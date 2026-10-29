package com.blitterstudio.amiberry.overlay

/**
 * The JNI of the in-emulation menu (src/osdep/android_overlay.cpp), in
 * libamiberry, which SDLActivity loads before AmiberryActivity uses this.
 *
 * Threads: the action posts, [poll], [status], [swapperEntries], [close]
 * and [setEnabled] may be called from any thread and never wait. The
 * settings and state-name calls run on the emulator thread and wait for it,
 * so they must never be called from the Android UI thread; they return null
 * (or -1) when the emulator thread could not run them.
 */
object OverlayNative {
	/** [close]: back to the emulation. */
	const val CLOSE_RESUME = 0
	/** [close]: open the built-in (ImGui) settings within the same pause. */
	const val CLOSE_ADVANCED = 1
	/** [close]: back to the emulation with the on-screen keyboard shown. */
	const val CLOSE_KEYBOARD = 2

	/** [poll] results (amiberry_action_result). */
	const val RESULT_PENDING = 0
	const val RESULT_DONE = 1
	const val RESULT_FAILED = 2
	const val RESULT_UNKNOWN = 3

	@JvmStatic external fun setEnabled(enabled: Boolean)
	@JvmStatic external fun close(how: Int)

	// Runtime actions; each returns the action's id, 0 when it was rejected.
	@JvmStatic external fun insertFloppy(drive: Int, path: String): Long
	@JvmStatic external fun ejectFloppy(drive: Int): Long
	@JvmStatic external fun insertCd(path: String): Long
	@JvmStatic external fun ejectCd(): Long
	@JvmStatic external fun insertSwapperEntry(index: Int, drive: Int): Long
	@JvmStatic external fun reset(hard: Boolean): Long
	@JvmStatic external fun pause(): Long
	@JvmStatic external fun quit(): Long
	@JvmStatic external fun saveState(path: String): Long
	@JvmStatic external fun loadState(path: String): Long
	@JvmStatic external fun screenshot(path: String): Long
	@JvmStatic external fun poll(id: Long): Int

	/** The status snapshot, as [OverlayStatus.parse] reads it. */
	@JvmStatic external fun status(): Array<String>
	/** The disk swapper's entries, "" for an empty one. */
	@JvmStatic external fun swapperEntries(): Array<String>

	// Emulator thread.
	/** For each id its pending value, then "1" when the option is offered now. */
	@JvmStatic external fun readSettings(ids: Array<String>): Array<String>?
	/** The choices offered now as value, label pairs. */
	@JvmStatic external fun settingChoices(id: String): Array<String>?
	/** Sets and commits one option; the apply class, -1 when it was refused. */
	@JvmStatic external fun changeSetting(id: String, value: String): Int
	/** What state slots are named after, as [StateNameSources.parse] reads it. */
	@JvmStatic external fun stateNameSources(): Array<String>?
}
