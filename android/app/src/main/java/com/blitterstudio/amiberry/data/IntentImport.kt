package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory

object IntentImport {
	private const val RP9_MIME_TYPE = "application/vnd.cloanto.rp9"

	sealed interface Classification {
		val safeName: String

		data class Config(override val safeName: String) : Classification
		data class Rp9(override val safeName: String) : Classification
		data class Media(override val safeName: String, val category: FileCategory) : Classification
		data class Unsupported(override val safeName: String, val extension: String) : Classification
	}

	fun classify(rawName: String?, mimeType: String? = null): Classification {
		val safeName = FileManager.safeImportFileName(rawName)
		val extension = safeName.substringAfterLast('.', missingDelimiterValue = "").lowercase()
		if (extension == "uae") {
			return Classification.Config(safeName)
		}
		val normalizedMimeType = mimeType?.substringBefore(';')?.trim()
		if (extension == "rp9" || normalizedMimeType.equals(RP9_MIME_TYPE, ignoreCase = true)) {
			val rp9Name = if (extension == "rp9") safeName else "$safeName.rp9"
			return Classification.Rp9(rp9Name)
		}

		val category = FileCategory.fromExtension(extension)
		return if (category != null && FileManager.importFileNameForCategory(safeName, category) != null) {
			Classification.Media(safeName, category)
		} else {
			Classification.Unsupported(safeName, extension)
		}
	}
}
