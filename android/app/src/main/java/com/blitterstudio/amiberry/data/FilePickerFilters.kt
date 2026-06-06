package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory

object FilePickerFilters {

	fun mimeTypesFor(category: FileCategory): Array<String> = when (category) {
		FileCategory.ROMS -> arrayOf("application/octet-stream", "application/x-binary")
		FileCategory.FLOPPIES -> arrayOf("application/octet-stream", "application/zip", "application/gzip")
		FileCategory.HARD_DRIVES -> arrayOf("application/octet-stream")
		FileCategory.CD_IMAGES -> arrayOf("application/x-iso9660-image", "application/octet-stream", "application/x-cue", "application/x-chd")
		FileCategory.WHDLOAD_GAMES -> arrayOf("application/x-lha", "application/x-lzh-compressed", "application/octet-stream")
	}

	fun extensionLabelsFor(category: FileCategory): Array<String> =
		category.extensions.map { ".$it" }.toTypedArray()
}
