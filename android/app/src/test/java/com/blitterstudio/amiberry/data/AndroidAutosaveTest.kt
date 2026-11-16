package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class AndroidAutosaveTest {
	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun `recorded autosave keeps its state path title and date`() {
		val state = tempDir.newFile(AndroidAutosave.STATE_FILE_NAME).apply { writeText("state") }
		val record = tempDir.newFile("session")

		val saved = AndroidAutosave.record(record, state, "Lotus II")
		val loaded = AndroidAutosave.load(record)

		assertEquals("Lotus II", saved?.title)
		assertEquals(state.absolutePath, loaded?.stateFile?.absolutePath)
		assertEquals("Lotus II", loaded?.title)
		assertEquals(state.lastModified(), loaded?.modifiedAt)
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
}
