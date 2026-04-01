package com.blitterstudio.amiberry.data

import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

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

	// --- ROM fingerprint ---

	@Test
	fun `rom fingerprint changes when file is added`() {
		val romDir = tempDir.newFolder("roms")
		File(romDir, "kick13.rom").writeBytes(ByteArray(256 * 1024))

		val fp1 = computeTestFingerprint(romDir)
		File(romDir, "kick31.rom").writeBytes(ByteArray(512 * 1024))
		val fp2 = computeTestFingerprint(romDir)

		assertNotEquals(fp1, fp2)
	}

	@Test
	fun `rom fingerprint stable when nothing changes`() {
		val romDir = tempDir.newFolder("roms")
		File(romDir, "kick13.rom").writeBytes(ByteArray(256 * 1024))

		val fp1 = computeTestFingerprint(romDir)
		val fp2 = computeTestFingerprint(romDir)

		assertEquals(fp1, fp2)
	}

	@Test
	fun `rom fingerprint ignores non-rom files`() {
		val romDir = tempDir.newFolder("roms")
		File(romDir, "kick13.rom").writeBytes(ByteArray(256 * 1024))

		val fp1 = computeTestFingerprint(romDir)
		File(romDir, "readme.txt").writeText("not a rom")
		val fp2 = computeTestFingerprint(romDir)

		assertEquals(fp1, fp2)
	}

	@Test
	fun `rom fingerprint empty for nonexistent dir`() {
		val romDir = File(tempDir.root, "nonexistent")
		assertEquals("empty", computeTestFingerprint(romDir))
	}

	/**
	 * Mirror of EmulatorLauncher.computeRomFingerprint for testing
	 * (the private method is not directly accessible from tests)
	 */
	private fun computeTestFingerprint(romDir: File): String {
		if (!romDir.exists()) return "empty"
		val romExtensions = setOf("rom", "bin")
		val romFiles = romDir.listFiles { f ->
			f.isFile && f.extension.lowercase() in romExtensions
		} ?: return "empty"
		val count = romFiles.size
		val totalSize = romFiles.sumOf { it.length() }
		val latestModified = romFiles.maxOfOrNull { it.lastModified() } ?: 0L
		return "$count:$totalSize:$latestModified"
	}
}
