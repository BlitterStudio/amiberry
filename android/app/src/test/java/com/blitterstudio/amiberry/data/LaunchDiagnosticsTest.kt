package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class LaunchDiagnosticsTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `write and read last launch diagnostics`() {
		val entry = LaunchDiagnostics.Entry(
			requestType = "QuickStart",
			args = listOf("--rescan-roms", "--model", "A500", "-G"),
			configPath = "/tmp/.quickstart_settings.uae",
			trackSession = true,
			startedAtEpochMs = 123456789L
		)

		val file = LaunchDiagnostics.write(tempDir.root, entry)
		val raw = file.readText()
		val parsed = LaunchDiagnostics.read(file)

		assertEquals(".last_launch.json", file.name)
		assertTrue(raw.contains("\"schemaVersion\": 1"))
		assertTrue(raw.contains("\"requestType\": \"QuickStart\""))
		assertTrue(raw.contains("\"args\": [\"--rescan-roms\", \"--model\", \"A500\", \"-G\"]"))
		assertEquals(entry, parsed)
	}

	@Test
	fun `write and read launch diagnostics with punctuation in file paths`() {
		val configPath = "/storage/emulated/0/Amiberry/Configurations/Workbench [AGA] \"test\".uae"
		val entry = LaunchDiagnostics.Entry(
			requestType = "SavedConfig",
			args = listOf(
				"--rescan-roms",
				"--config",
				configPath,
				"-s",
				"filesystem2=rw,DH0:Workbench:/storage/emulated/0/Amiberry/Hard Drives/Workbench ] [ 3.2,0"
			),
			configPath = configPath,
			trackSession = true,
			startedAtEpochMs = 123456789L
		)

		val file = LaunchDiagnostics.write(tempDir.root, entry)

		assertEquals(entry, LaunchDiagnostics.read(file))
	}

	@Test
	fun `entry from request records request type args and config path`() {
		val request = LaunchRequest.QuickStart(
			model = AmigaModel.A500,
			configPath = "/tmp/.quickstart_settings.uae"
		)

		val entry = LaunchDiagnostics.fromRequest(
			request = request,
			args = request.toArgs().toList(),
			trackSession = true,
			startedAtEpochMs = 123456789L
		)

		assertEquals("QuickStart", entry.requestType)
		assertEquals(listOf("--rescan-roms", "--model", "A500", "--config", "/tmp/.quickstart_settings.uae", "-G"), entry.args)
		assertEquals("/tmp/.quickstart_settings.uae", entry.configPath)
		assertEquals(true, entry.trackSession)
		assertEquals(123456789L, entry.startedAtEpochMs)
	}
}
