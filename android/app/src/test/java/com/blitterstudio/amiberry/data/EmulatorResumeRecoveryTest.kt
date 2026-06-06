package com.blitterstudio.amiberry.data

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class EmulatorResumeRecoveryTest {

	@Test
	fun `resume recovery waits until activity is ready and resumed`() {
		assertTrue(EmulatorResumeRecovery.shouldDefer(isReady = false, isResumed = true))
		assertTrue(EmulatorResumeRecovery.shouldDefer(isReady = true, isResumed = false))
		assertFalse(EmulatorResumeRecovery.shouldDefer(isReady = true, isResumed = true))
	}

	@Test
	fun `clean exit clears markers and counts as successful session`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = true,
			wasCrash = false,
			emulatorWasLaunched = true,
			hadTrackedSession = false
		)

		assertTrue(decision.clearSessionMarker)
		assertTrue(decision.countSuccessfulSession)
		assertFalse(decision.showCrashDialog)
		assertFalse(decision.keepLaunchTracked)
	}

	@Test
	fun `tracked clean exit after process restart counts as successful session`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = true,
			wasCrash = false,
			emulatorWasLaunched = false,
			hadTrackedSession = true
		)

		assertTrue(decision.clearSessionMarker)
		assertTrue(decision.countSuccessfulSession)
		assertFalse(decision.showCrashDialog)
	}

	@Test
	fun `untracked clean exit does not count as successful session`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = true,
			wasCrash = false,
			emulatorWasLaunched = false,
			hadTrackedSession = false
		)

		assertTrue(decision.clearSessionMarker)
		assertFalse(decision.countSuccessfulSession)
		assertFalse(decision.showCrashDialog)
	}

	@Test
	fun `clean exit has priority over stale crash marker`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = true,
			wasCrash = true,
			emulatorWasLaunched = false,
			hadTrackedSession = true
		)

		assertTrue(decision.clearSessionMarker)
		assertTrue(decision.countSuccessfulSession)
		assertFalse(decision.showCrashDialog)
	}

	@Test
	fun `crash shows dialog and does not count as successful session`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = false,
			wasCrash = true,
			emulatorWasLaunched = true,
			hadTrackedSession = true
		)

		assertTrue(decision.showCrashDialog)
		assertFalse(decision.countSuccessfulSession)
		assertFalse(decision.keepLaunchTracked)
	}

	@Test
	fun `in-memory launch without markers still counts as successful session`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = false,
			wasCrash = false,
			emulatorWasLaunched = true,
			hadTrackedSession = false
		)

		assertTrue(decision.countSuccessfulSession)
		assertFalse(decision.showCrashDialog)
		assertFalse(decision.keepLaunchTracked)
	}

	@Test
	fun `no marker and no in-memory launch has no resume action`() {
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = false,
			wasCrash = false,
			emulatorWasLaunched = false,
			hadTrackedSession = false
		)

		assertFalse(decision.countSuccessfulSession)
		assertFalse(decision.showCrashDialog)
		assertFalse(decision.clearSessionMarker)
		assertFalse(decision.keepLaunchTracked)
	}
}
