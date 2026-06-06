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
