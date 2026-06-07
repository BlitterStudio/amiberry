package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class ConfigurationActionsTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `load returns parsed value on success and failure message on failure`() {
		val parsed = ConfigParser.ParsedConfig(com.blitterstudio.amiberry.data.model.EmulatorSettings(), emptyList(), "")

		val loaded = ConfigurationActions.load(Result.success(parsed))
		assertTrue(loaded is ConfigurationActions.LoadResult.Loaded)
		assertSame(parsed, (loaded as ConfigurationActions.LoadResult.Loaded).value)

		val failed = ConfigurationActions.load<ConfigParser.ParsedConfig>(Result.failure(IllegalStateException()))
		assertTrue(failed is ConfigurationActions.LoadResult.Failed)
		assertEquals(R.string.msg_failed_load_config, (failed as ConfigurationActions.LoadResult.Failed).message.stringRes)
	}

	@Test
	fun `delete message reflects success or failure`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_deleted),
			ConfigurationActions.deleteMessage(deleted = true)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_failed_delete_config),
			ConfigurationActions.deleteMessage(deleted = false)
		)
	}

	@Test
	fun `duplicate message includes copied file name on success`() {
		val copy = tempDir.newFile("Workbench_copy.uae")

		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_duplicated_as, "Workbench_copy.uae"),
			ConfigurationActions.duplicateMessage(Result.success(copy))
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_failed_duplicate_config),
			ConfigurationActions.duplicateMessage(Result.failure(IllegalStateException()))
		)
	}

	@Test
	fun `share target requires an existing visible config file`() {
		val file = tempDir.newFile("A500.uae")
		assertEquals(
			ConfigurationActions.ShareTarget.Ready(file),
			ConfigurationActions.shareTarget(file.path)
		)

		val missing = ConfigurationActions.shareTarget(tempDir.root.resolve("missing.uae").path)
		assertTrue(missing is ConfigurationActions.ShareTarget.Failed)
		assertEquals(
			R.string.msg_failed_share_config,
			(missing as ConfigurationActions.ShareTarget.Failed).message.stringRes
		)

		listOf(
			tempDir.newFolder("Directory.uae").path,
			tempDir.newFile(".last_session.uae").path,
			tempDir.newFile("notes.txt").path
		).forEach { invalidPath ->
			val invalid = ConfigurationActions.shareTarget(invalidPath)
			assertTrue("$invalidPath should not be shareable as a saved configuration.", invalid is ConfigurationActions.ShareTarget.Failed)
		}
	}

	@Test
	fun `launch target requires an existing visible config file`() {
		val file = tempDir.newFile("A1200.uae")
		assertEquals(
			ConfigurationActions.LaunchTarget.Ready(file.path),
			ConfigurationActions.launchTarget(file.path)
		)

		val missing = ConfigurationActions.launchTarget(tempDir.root.resolve("missing.uae").path)
		assertTrue(missing is ConfigurationActions.LaunchTarget.Failed)
		assertEquals(
			R.string.msg_failed_launch_config,
			(missing as ConfigurationActions.LaunchTarget.Failed).message.stringRes
		)

		listOf(
			tempDir.newFolder("Directory.uae").path,
			tempDir.newFile(".temp_native_ui.uae").path,
			tempDir.newFile("readme.md").path
		).forEach { invalidPath ->
			val invalid = ConfigurationActions.launchTarget(invalidPath)
			assertTrue("$invalidPath should not be launchable as a saved configuration.", invalid is ConfigurationActions.LaunchTarget.Failed)
		}
	}
}
