package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Test

class ImportFeedbackTest {

	@Test
	fun `config import feedback identifies the imported config file`() {
		val message = ImportFeedback.configImported("Workbench.uae")

		assertEquals(R.string.msg_intent_imported_config_as, message.stringRes)
		assertEquals("Workbench.uae", message.argument)
	}

	@Test
	fun `file import feedback identifies the imported file`() {
		val message = ImportFeedback.fileImported("Lotus.adf")

		assertEquals(R.string.msg_intent_imported_file, message.stringRes)
		assertEquals("Lotus.adf", message.argument)
	}

	@Test
	fun `unsupported file feedback reports the extension`() {
		val message = ImportFeedback.unsupportedFileType("txt")

		assertEquals(R.string.msg_intent_unsupported_file_type, message.stringRes)
		assertEquals("txt", message.argument)
	}

	@Test
	fun `unsupported file feedback falls back when extension is blank`() {
		val message = ImportFeedback.unsupportedFileType("")

		assertEquals(R.string.msg_intent_unsupported_file_type, message.stringRes)
		assertEquals("unknown", message.argument)
	}

	@Test
	fun `failed import feedback identifies the file`() {
		val message = ImportFeedback.importFailed("bad.uae")

		assertEquals(R.string.msg_intent_import_failed, message.stringRes)
		assertEquals("bad.uae", message.argument)
	}

	@Test
	fun `import result feedback reports imported picker files`() {
		val message = ImportFeedback.fromImportResult(
			FileManager.ImportResult.Imported("C:/storage/floppies/Lotus.adf", "Lotus.adf")
		)

		assertEquals(R.string.msg_intent_imported_file, message.stringRes)
		assertEquals("Lotus.adf", message.argument)
	}

	@Test
	fun `import result feedback reports unsupported picker files`() {
		val message = ImportFeedback.fromImportResult(
			FileManager.ImportResult.Unsupported("notes.txt", "txt")
		)

		assertEquals(R.string.msg_intent_unsupported_file_type, message.stringRes)
		assertEquals("txt", message.argument)
	}

	@Test
	fun `import result feedback reports failed picker files`() {
		val message = ImportFeedback.fromImportResult(
			FileManager.ImportResult.Failed("broken.adf")
		)

		assertEquals(R.string.msg_intent_import_failed, message.stringRes)
		assertEquals("broken.adf", message.argument)
	}
}
