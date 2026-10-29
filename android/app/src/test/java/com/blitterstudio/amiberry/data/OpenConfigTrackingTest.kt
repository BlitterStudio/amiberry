package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class OpenConfigTrackingTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `renaming the open configuration moves Settings to the new file`() {
		val confDir = tempDir.newFolder("Configurations")
		val source = File(confDir, "Workbench.uae").also { it.writeText("cpu_model=68000\n") }
		val open = OpenConfigTracking.Tracked(name = "Workbench", path = source.absolutePath)

		val renamed = (ConfigFiles.renameConfig(confDir, source.absolutePath, "Workbench 3.1") as ConfigFiles.EditResult.Done).file
		val tracked = OpenConfigTracking.afterRename(open, source.absolutePath, renamed)

		assertEquals("Workbench 3.1", tracked.name)
		assertEquals(renamed.absolutePath, tracked.path)
		assertTrue(File(tracked.path!!).exists())
	}

	@Test
	fun `renaming another configuration leaves Settings where it is`() {
		val open = OpenConfigTracking.Tracked(name = "Workbench", path = "/configs/Workbench.uae")

		val tracked = OpenConfigTracking.afterRename(open, "/configs/Games.uae", File("/configs/Arcade.uae"))

		assertEquals(open, tracked)
	}

	@Test
	fun `deleting the open configuration leaves its values unnamed`() {
		val confDir = tempDir.newFolder("Configurations")
		val file = File(confDir, "Workbench.uae").also { it.writeText("cpu_model=68000\n") }
		// The path Settings stored may be spelled differently from the one the list deletes.
		val open = OpenConfigTracking.Tracked(name = "Workbench", path = File(confDir, "./Workbench.uae").path)

		assertTrue(ConfigFiles.deleteConfig(confDir, file.absolutePath))
		val tracked = OpenConfigTracking.afterDelete(open, file.absolutePath)

		assertEquals(OpenConfigTracking.Unnamed, tracked)
	}

	@Test
	fun `deleting another configuration keeps the open one`() {
		val open = OpenConfigTracking.Tracked(name = "Workbench", path = "/configs/Workbench.uae")

		assertEquals(open, OpenConfigTracking.afterDelete(open, "/configs/Games.uae"))
		assertEquals(OpenConfigTracking.Unnamed, OpenConfigTracking.afterDelete(OpenConfigTracking.Unnamed, "/configs/Games.uae"))
	}
}
