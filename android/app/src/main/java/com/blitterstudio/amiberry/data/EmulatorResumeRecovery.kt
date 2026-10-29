package com.blitterstudio.amiberry.data

object EmulatorResumeRecovery {
	data class Decision(
		val showCrashDialog: Boolean = false,
		val clearSessionMarker: Boolean = false,
		val countSuccessfulSession: Boolean = false,
		val keepLaunchTracked: Boolean = false
	)

	fun shouldDefer(isReady: Boolean, isResumed: Boolean): Boolean =
		!isReady || !isResumed

	/**
	 * Whether a left-over session marker can mean a crash: not while the emulator
	 * process is still running (its session is live, the launcher just came forward).
	 */
	fun shouldCheckForCrash(wasCleanExit: Boolean, emulatorRunning: Boolean): Boolean =
		!wasCleanExit && !emulatorRunning

	/**
	 * Whether a new launcher instance is a second one stacked over a running emulator
	 * (an explicit start, a shortcut or "Open with" while a game runs): it is not its
	 * task's root and is not being recreated. It should step aside for the emulator.
	 */
	fun isDuplicateLauncher(isTaskRoot: Boolean, restoring: Boolean, emulatorRunning: Boolean): Boolean =
		!isTaskRoot && !restoring && emulatorRunning

	fun decide(
		wasCleanExit: Boolean,
		wasCrash: Boolean,
		emulatorWasLaunched: Boolean,
		hadTrackedSession: Boolean
	): Decision =
		when {
			wasCleanExit -> Decision(
				clearSessionMarker = true,
				countSuccessfulSession = emulatorWasLaunched || hadTrackedSession
			)
			wasCrash -> Decision(showCrashDialog = true)
			emulatorWasLaunched -> Decision(countSuccessfulSession = true)
			else -> Decision(keepLaunchTracked = false)
		}
}
