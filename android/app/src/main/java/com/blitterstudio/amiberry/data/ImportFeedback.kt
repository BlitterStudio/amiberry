package com.blitterstudio.amiberry.data

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

object ImportFeedback {
	data class Message(
		@param:StringRes val stringRes: Int,
		val argument: String
	)

	fun configImported(fileName: String): Message =
		Message(R.string.msg_intent_imported_config_as, cleanFileName(fileName))

	fun fileImported(fileName: String): Message =
		Message(R.string.msg_intent_imported_file, cleanFileName(fileName))

	fun unsupportedFileType(extension: String): Message =
		Message(R.string.msg_intent_unsupported_file_type, cleanExtension(extension))

	fun importFailed(fileName: String): Message =
		Message(R.string.msg_intent_import_failed, cleanFileName(fileName))

	fun fromImportResult(result: FileManager.ImportResult): Message =
		when (result) {
			is FileManager.ImportResult.Imported -> fileImported(result.safeName)
			is FileManager.ImportResult.Unsupported -> unsupportedFileType(result.extension)
			is FileManager.ImportResult.Failed -> importFailed(result.safeName)
		}

	private fun cleanFileName(fileName: String): String =
		fileName.trim().ifBlank { "file" }

	private fun cleanExtension(extension: String): String =
		extension.trim().trimStart('.').lowercase().ifBlank { "unknown" }
}
