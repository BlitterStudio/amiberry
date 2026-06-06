package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Test

class ImportBatchFeedbackTest {

	@Test
	fun `all imported batch uses existing imported multiple message`() {
		val message = ImportBatchFeedback.messageFor(
			listOf(
				FileManager.ImportResult.Imported("/files/a.adf", "a.adf"),
				FileManager.ImportResult.Imported("/files/b.adf", "b.adf")
			)
		)

		assertEquals(R.string.msg_imported_multiple, message.stringRes)
		assertEquals(listOf(2, 2), message.args)
	}

	@Test
	fun `mixed batch reports unsupported and failed counts`() {
		val message = ImportBatchFeedback.messageFor(
			listOf(
				FileManager.ImportResult.Imported("/files/a.adf", "a.adf"),
				FileManager.ImportResult.Unsupported("notes.txt", "txt"),
				FileManager.ImportResult.Failed("broken.adf")
			)
		)

		assertEquals(R.string.msg_imported_multiple_with_failures, message.stringRes)
		assertEquals(listOf(1, 3, 1, 1), message.args)
	}

	@Test
	fun `batch with no imports reports why nothing was imported`() {
		val message = ImportBatchFeedback.messageFor(
			listOf(
				FileManager.ImportResult.Unsupported("notes.txt", "txt"),
				FileManager.ImportResult.Unsupported("image.png", "png"),
				FileManager.ImportResult.Failed("broken.adf")
			)
		)

		assertEquals(R.string.msg_imported_none, message.stringRes)
		assertEquals(listOf(2, 1), message.args)
	}
}
