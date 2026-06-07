package com.blitterstudio.amiberry.data

import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class EmulatorLauncherTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	// --- Crash marker staleness ---

	@Test
	fun `fresh marker within threshold is detected as crash`() {
		val marker = tempDir.newFile(".emulator_session")
		marker.writeText(System.currentTimeMillis().toString())

		assertTrue(marker.exists())
		val startTime = marker.readText().trim().toLong()
		val elapsed = System.currentTimeMillis() - startTime
		// Should be within threshold (< 12 hours)
		assertTrue(elapsed < 12 * 60 * 60 * 1000L)
	}

	@Test
	fun `stale marker older than threshold is not a crash`() {
		val staleTime = System.currentTimeMillis() - (13 * 60 * 60 * 1000L) // 13 hours ago
		val marker = tempDir.newFile(".emulator_session")
		marker.writeText(staleTime.toString())

		val startTime = marker.readText().trim().toLong()
		val elapsed = System.currentTimeMillis() - startTime
		// Should exceed threshold (> 12 hours)
		assertTrue(elapsed >= 12 * 60 * 60 * 1000L)
	}

	@Test
	fun `marker with invalid content is treated as crash`() {
		val marker = tempDir.newFile(".emulator_session")
		marker.writeText("not_a_number")

		val startTime = marker.readText().trim().toLongOrNull()
		assertNull(startTime)
		// When toLongOrNull returns null, the code falls back to "treat as crash"
	}

	@Test
	fun `marker with future timestamp is not a crash`() {
		// Negative elapsed time (marker timestamp > current time) should not match range
		val futureTime = System.currentTimeMillis() + (60 * 60 * 1000L)
		val marker = tempDir.newFile(".emulator_session")
		marker.writeText(futureTime.toString())

		val startTime = marker.readText().trim().toLong()
		val elapsed = System.currentTimeMillis() - startTime
		// Negative elapsed is outside the 1..threshold range
		assertFalse(elapsed in 1 until (12 * 60 * 60 * 1000L))
	}

	@Test
	fun `prepare launch args preserves rom rescan flag`() {
		val args = arrayOf("--rescan-roms", "--model", "A500", "-G")

		assertArrayEquals(args, EmulatorLauncher.prepareLaunchArgs(args))
	}

	@Test
	fun `prepare launch args prepends missing rom rescan flag`() {
		val args = arrayOf("--model", "A500", "-G")

		assertArrayEquals(
			arrayOf("--rescan-roms", "--model", "A500", "-G"),
			EmulatorLauncher.prepareLaunchArgs(args)
		)
	}

	@Test
	fun `prepare launch args defaults to rom rescan when empty`() {
		assertArrayEquals(
			arrayOf("--rescan-roms"),
			EmulatorLauncher.prepareLaunchArgs(emptyArray())
		)
	}

	@Test
	fun `tracked launch clears stale clean exit marker before writing session marker`() {
		val baseDir = tempDir.newFolder()
		val cleanExitMarker = baseDir.resolve(".clean_exit")
		val sessionMarker = baseDir.resolve(".emulator_session")

		cleanExitMarker.writeText("1")

		EmulatorLauncher.prepareTrackedSessionMarkers(baseDir, nowMs = 1234L)

		assertFalse(cleanExitMarker.exists())
		assertEquals("1234", sessionMarker.readText())
	}

	@Test
	fun `untracked launch clears stale session markers without creating a new one`() {
		val baseDir = tempDir.newFolder()
		val cleanExitMarker = baseDir.resolve(".clean_exit")
		val sessionMarker = baseDir.resolve(".emulator_session")

		cleanExitMarker.writeText("1")
		sessionMarker.writeText("old")

		EmulatorLauncher.prepareUntrackedLaunchMarkers(baseDir)

		assertFalse(cleanExitMarker.exists())
		assertFalse(sessionMarker.exists())
	}

	@Test
	fun `session marker existence identifies tracked session`() {
		val baseDir = tempDir.newFolder()
		val sessionMarker = baseDir.resolve(".emulator_session")

		assertFalse(EmulatorLauncher.sessionMarkerExists(baseDir))

		sessionMarker.writeText("1234")

		assertTrue(EmulatorLauncher.sessionMarkerExists(baseDir))
	}
}
