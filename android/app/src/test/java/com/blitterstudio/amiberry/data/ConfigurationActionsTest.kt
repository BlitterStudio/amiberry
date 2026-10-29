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
	fun `delete message names the configuration and says when Settings keeps it unnamed`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_deleted_named, "Workbench"),
			ConfigurationActions.deleteMessage("Workbench", deleted = true, wasOpen = false)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_deleted_was_open, "Workbench"),
			ConfigurationActions.deleteMessage("Workbench", deleted = true, wasOpen = true)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_failed_delete_config),
			ConfigurationActions.deleteMessage("Workbench", deleted = false, wasOpen = false)
		)
	}

	@Test
	fun `duplicate message names the copy on success and explains a refused name`() {
		val copy = tempDir.newFile("Workbench copy.uae")

		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_duplicated_as, "Workbench copy"),
			ConfigurationActions.duplicateMessage(ConfigFiles.EditResult.Done(copy))
		)
		assertEquals(
			ConfigurationActions.Message(R.string.config_name_problem_taken, "Games"),
			ConfigurationActions.duplicateMessage(
				ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.Taken("Games"))
			)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_failed_duplicate_config),
			ConfigurationActions.duplicateMessage(ConfigFiles.EditResult.Failed)
		)
	}

	@Test
	fun `rename message names the new name or the reason it was refused`() {
		val renamed = tempDir.newFile("Lemmings.uae")

		assertEquals(
			ConfigurationActions.Message(R.string.msg_config_renamed_as, "Lemmings"),
			ConfigurationActions.renameMessage(ConfigFiles.EditResult.Done(renamed))
		)
		assertEquals(
			ConfigurationActions.Message(R.string.config_name_problem_empty),
			ConfigurationActions.renameMessage(ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.Empty))
		)
		assertEquals(
			ConfigurationActions.Message(R.string.config_name_problem_invalid),
			ConfigurationActions.renameMessage(
				ConfigFiles.EditResult.Refused(ConfigFiles.NameProblem.InvalidCharacters)
			)
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_failed_rename_config),
			ConfigurationActions.renameMessage(ConfigFiles.EditResult.Failed)
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
