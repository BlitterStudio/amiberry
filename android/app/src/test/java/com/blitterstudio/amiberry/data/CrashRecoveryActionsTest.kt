package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class CrashRecoveryActionsTest {

	private val entry = LaunchDiagnostics.Entry(
		requestType = "QuickStart",
		args = listOf("--rescan-roms", "--model", "A500", "--config", "/data/user/0/app/files/.quickstart_settings.uae", "-G"),
		configPath = "/data/user/0/app/files/.quickstart_settings.uae",
		trackSession = true,
		startedAtEpochMs = 123456789L
	)

	@Test
	fun `summary includes launch type arg count and config name`() {
		assertEquals(
			"QuickStart, 6 args, config .quickstart_settings.uae",
			CrashRecoveryActions.summary(entry)
		)
	}

	@Test
	fun `summary handles missing diagnostics`() {
		assertEquals(null, CrashRecoveryActions.summary(null))
	}

	@Test
	fun `copy payload includes app environment and last launch diagnostics`() {
		val payload = CrashRecoveryActions.copyPayload(
			entry = entry,
			packageName = "com.blitterstudio.amiberry",
			versionName = "7.0.0",
			versionCode = 7000000L,
			sdkInt = 36
		)

		assertTrue(payload.contains("Amiberry Android crash diagnostics"))
		assertTrue(payload.contains("Package: com.blitterstudio.amiberry"))
		assertTrue(payload.contains("Version: 7.0.0 (7000000)"))
		assertTrue(payload.contains("SDK: 36"))
		assertTrue(payload.contains("Last launch: QuickStart"))
		assertTrue(payload.contains("Args: --rescan-roms --model A500 --config /data/user/0/app/files/.quickstart_settings.uae -G"))
	}

	@Test
	fun `copy payload handles missing diagnostics`() {
		val payload = CrashRecoveryActions.copyPayload(
			entry = null,
			packageName = "com.blitterstudio.amiberry",
			versionName = "unknown",
			versionCode = 0L,
			sdkInt = 36
		)

		assertTrue(payload.contains("Last launch: unavailable"))
	}

	@Test
	fun `copy message is user-visible`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_crash_diagnostics_copied),
			CrashRecoveryActions.copyMessage()
		)
	}
}
