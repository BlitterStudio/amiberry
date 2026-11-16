package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class AndroidAutosaveTest {
	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `recorded autosave keeps its state path title date and launch options`() {
		val state = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME).apply { writeText("state") }
		val record = tempDir.newFile("session")
		val args = arrayOf("-G")

		val saved = AndroidAutosave.record(record, state, "Lotus II", args)
		val loaded = AndroidAutosave.load(record)

		assertEquals("Lotus II", saved?.title)
		assertEquals(state.absolutePath, loaded?.stateFile?.absolutePath)
		assertEquals("Lotus II", loaded?.title)
		assertEquals(state.lastModified(), loaded?.modifiedAt)
		assertTrue(args.contentEquals(loaded?.launchArgs))
	}

	@Test
	fun `recorded autosave snapshots transient config and skips WHDBooter on resume`() {
		val state = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME).apply { writeText("state") }
		val config = tempDir.newFile(".quickstart_settings.uae").apply { writeText("joyport0=mouse") }
		val record = tempDir.newFile("session")

		AndroidAutosave.record(
			record, state, "Game",
			arrayOf("--config", config.absolutePath, "--autoload", "/games/Game.lha", "-G")
		)
		config.writeText("joyport0=joy1")
		val loaded = AndroidAutosave.load(record)
		val snapshot = requireNotNull(state.parentFile).resolve("${state.name}.uae")

		assertTrue(snapshot.isFile)
		assertEquals("joyport0=mouse", snapshot.readText())
		assertTrue(loaded!!.launchArgs.contentEquals(arrayOf("--config", snapshot.absolutePath, "-G")))

		AndroidAutosave.discard(record)

		assertFalse(snapshot.exists())
	}

	@Test
	fun `recorded autosave keeps original state slot identity`() {
		val state = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME).apply { writeText("state") }
		val record = tempDir.newFile("session")
		val sources = arrayOf(
			"/states", "/screenshots", "/games/Lotus2.adf", "", "",
			"Lotus 2", "Amiga 500"
		)

		AndroidAutosave.record(record, state, "Lotus 2", emptyArray(), sources)

		assertTrue(sources.contentEquals(AndroidAutosave.load(record)?.slotSources))
	}

	@Test
	fun `missing autosave is not offered and its record is removed`() {
		val record = tempDir.newFile("session")
		val missing = tempDir.root.resolve(AndroidAutosave.STATE_FILE_NAME)

		assertNull(AndroidAutosave.record(record, missing, "Amiga"))
		assertFalse(record.length() > 0L)
		assertNull(AndroidAutosave.load(record))
	}

	@Test
	fun `corrupt record is discarded`() {
		val record = tempDir.newFile("session").apply { writeText("not a properties file") }

		assertNull(AndroidAutosave.load(record))
		assertFalse(record.exists())
	}

	@Test
	fun `empty state is not offered`() {
		val state = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME)
		val record = tempDir.newFile("session")

		assertNull(AndroidAutosave.record(record, state, "Amiga"))
		assertTrue(record.exists())
		assertNull(AndroidAutosave.load(record))
		assertFalse(record.exists())
	}

	@Test
	fun `discard removes a completed autosave and an interrupted partial save`() {
		val state = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME).apply { writeText("state") }
		val partial = tempDir.newFile("${AndroidAutosave.STATE_FILE_NAME}.partial").apply { writeText("partial") }
		val record = tempDir.newFile("session")
		AndroidAutosave.record(record, state, "Amiga")

		AndroidAutosave.discard(record)

		assertFalse(state.exists())
		assertFalse(partial.exists())
		assertFalse(record.exists())
	}

	@Test
	fun `discard leaves a user state with the autosave name alone`() {
		val userState = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME).apply { writeText("user state") }
		val recoveryDir = tempDir.newFolder(".recovery")
		val recoveryState = File(recoveryDir, AndroidAutosave.STATE_FILE_NAME).apply { writeText("recovery state") }
		val record = tempDir.newFile("session")
		AndroidAutosave.record(record, recoveryState, "Amiga")

		AndroidAutosave.discard(record)

		assertTrue(userState.exists())
		assertFalse(recoveryState.exists())
	}
}
