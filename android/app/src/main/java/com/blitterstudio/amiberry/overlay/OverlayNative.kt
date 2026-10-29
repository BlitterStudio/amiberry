package com.blitterstudio.amiberry.overlay

/**
 * The JNI of the in-emulation menu (src/osdep/android_overlay.cpp), in
 * libamiberry, which SDLActivity loads before AmiberryActivity uses this.
 *
 * Threads: the action posts, [poll], [status], [swapperEntries], [close],
 * [setEnabled], [setRemoteJoystick] and [remoteKey] may be called from any
 * thread and never wait. The settings and state-name calls run on the
 * emulator thread and wait for it, so they must never be called from the
 * Android UI thread; they return null
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

	/** [remoteKey] buttons: the port 2 joystick's directions and fire. */
	const val REMOTE_UP = 1
	const val REMOTE_DOWN = 2
	const val REMOTE_LEFT = 4
	const val REMOTE_RIGHT = 8
	const val REMOTE_FIRE = 16

	@JvmStatic external fun setEnabled(enabled: Boolean)
	@JvmStatic external fun close(how: Int)

	/** Whether the remote drives port 2 (a TV with no gamepad); see [RemoteJoystick]. */
	@JvmStatic external fun setRemoteJoystick(enabled: Boolean)
	/**
	 * A remote button pressed or released; true when it moved the joystick
	 * (false while the classic settings window has the D-pad, or while the
	 * remote does not drive port 2). A release is always taken.
	 */
	@JvmStatic external fun remoteKey(button: Int, down: Boolean): Boolean

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
	/** Sets and commits options together; the apply class, -1 when one was refused. */
	@JvmStatic external fun changeSettings(ids: Array<String>, values: Array<String>): Int
	/** What state slots are named after, as [StateNameSources.parse] reads it. */
	@JvmStatic external fun stateNameSources(): Array<String>?
}
