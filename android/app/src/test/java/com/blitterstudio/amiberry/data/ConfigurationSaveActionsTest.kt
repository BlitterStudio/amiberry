package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class ConfigurationSaveActionsTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `targetForName rejects invalid names`() {
		val confDir = tempDir.newFolder("Configurations")

		val target = ConfigurationSaveActions.targetForName(confDir, "../bad")

		assertTrue(target is ConfigurationSaveActions.SaveTarget.InvalidName)
	}

	@Test
	fun `targetForName rejects existing configs without overwriting`() {
		val confDir = tempDir.newFolder("Configurations")
		File(confDir, "Workbench.uae").writeText("cpu_model=68000\n")

		val target = ConfigurationSaveActions.targetForName(confDir, "Workbench")

		assertTrue(target is ConfigurationSaveActions.SaveTarget.AlreadyExists)
	}

	@Test
	fun `targetForName accepts a safe new config name`() {
		val confDir = tempDir.newFolder("Configurations")

		val target = ConfigurationSaveActions.targetForName(confDir, "Workbench")

		assertEquals(
			ConfigurationSaveActions.SaveTarget.Ready(File(confDir, "Workbench.uae").canonicalFile),
			target
		)
	}

	@Test
	fun `messageFor maps save results to user facing messages`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_invalid_config_name),
			ConfigurationSaveActions.messageFor(ConfigurationSaveActions.SaveResult.InvalidName)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_already_exists),
			ConfigurationSaveActions.messageFor(ConfigurationSaveActions.SaveResult.AlreadyExists)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_failed_save_config),
			ConfigurationSaveActions.messageFor(ConfigurationSaveActions.SaveResult.Failed)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_saved_configuration, "Workbench.uae"),
			ConfigurationSaveActions.messageFor(ConfigurationSaveActions.SaveResult.Saved(File("Workbench.uae")))
		)
	}
}
