package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class ConfigSaveResolverTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private fun confDir(): File = tempDir.newFolder("Configurations")

	@Test
	fun `invalid name resolves to InvalidName`() {
		val resolution = ConfigSaveResolver.resolve(confDir(), "../bad", currentConfigName = null)
		assertTrue(resolution is ConfigSaveResolver.Resolution.InvalidName)
	}

	@Test
	fun `new name with no open config resolves to WriteNew`() {
		val dir = confDir()
		val resolution = ConfigSaveResolver.resolve(dir, "Workbench", currentConfigName = null)
		assertEquals(
			ConfigSaveResolver.Resolution.WriteNew(File(dir, "Workbench.uae").canonicalFile),
			resolution
		)
	}

	@Test
	fun `saving to the open config resolves to OverwriteTracked even though the file exists`() {
		val dir = confDir()
		File(dir, "Workbench.uae").writeText("cpu_model=68000\n")
		val resolution = ConfigSaveResolver.resolve(dir, "Workbench", currentConfigName = "Workbench")
		assertEquals(
			ConfigSaveResolver.Resolution.OverwriteTracked(File(dir, "Workbench.uae").canonicalFile),
			resolution
		)
	}

	@Test
	fun `open config name with uae suffix still resolves to OverwriteTracked`() {
		val dir = confDir()
		File(dir, "Workbench.uae").writeText("cpu_model=68000\n")
		val resolution = ConfigSaveResolver.resolve(dir, "Workbench.uae", currentConfigName = "Workbench")
		assertTrue(resolution is ConfigSaveResolver.Resolution.OverwriteTracked)
	}

	@Test
	fun `name of a different existing config resolves to CollisionWithOther`() {
		val dir = confDir()
		File(dir, "Games.uae").writeText("cpu_model=68020\n")
		val resolution = ConfigSaveResolver.resolve(dir, "Games", currentConfigName = "Workbench")
		assertEquals(
			ConfigSaveResolver.Resolution.CollisionWithOther(File(dir, "Games.uae").canonicalFile),
			resolution
		)
	}

	@Test
	fun `third new name while a config is open resolves to WriteNew`() {
		val dir = confDir()
		File(dir, "Workbench.uae").writeText("cpu_model=68000\n")
		val resolution = ConfigSaveResolver.resolve(dir, "Demos", currentConfigName = "Workbench")
		assertEquals(
			ConfigSaveResolver.Resolution.WriteNew(File(dir, "Demos.uae").canonicalFile),
			resolution
		)
	}
}
