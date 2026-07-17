package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory

object IntentImport {

	sealed interface Classification {
		val safeName: String

		data class Config(override val safeName: String) : Classification
		data class Rp9(override val safeName: String) : Classification
		data class Media(override val safeName: String, val category: FileCategory) : Classification
		data class Unsupported(override val safeName: String, val extension: String) : Classification
	}

	fun classify(rawName: String?): Classification {
		val safeName = FileManager.safeImportFileName(rawName)
		val extension = safeName.substringAfterLast('.', missingDelimiterValue = "").lowercase()
		if (extension == "uae") {
			return Classification.Config(safeName)
		}
		if (extension == "rp9") {
			return Classification.Rp9(safeName)
		}

		val category = FileCategory.fromExtension(extension)
		return if (category != null && FileManager.importFileNameForCategory(safeName, category) != null) {
			Classification.Media(safeName, category)
		} else {
			Classification.Unsupported(safeName, extension)
		}
	}
}
