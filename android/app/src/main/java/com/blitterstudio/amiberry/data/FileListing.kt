package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.StoragePaths
import java.io.File

/** What the Files tab lists, and what an import there did. */
object FileListing {

	/**
	 * The ROM rows: one AROS row (the two AROS files work only together, so the extension
	 * has no row of its own), and none for the copies WHDBooter makes of a listed Kickstart
	 * under the names WHDLoad expects (`WHDBoot/save-data/Kickstarts/kick40068.A1200`).
	 * A copy whose original is gone stays listed, so it can still be seen and deleted.
	 */
	fun visibleRoms(
		roms: List<AmigaFile>,
		isArosExtension: (AmigaFile) -> Boolean = ModelRomAvailability::isBuiltInArosExtension
	): List<AmigaFile> {
		val originalCrcs = roms.filterNot(::isWhdbootKickstartCopy).mapNotNullTo(mutableSetOf()) { it.crc32 }
		return roms.filter { rom ->
			!isArosExtension(rom) && !(isWhdbootKickstartCopy(rom) && rom.crc32 != null && rom.crc32 in originalCrcs)
		}
	}

	private fun isWhdbootKickstartCopy(rom: AmigaFile): Boolean =
		File(rom.path).parentFile?.path?.replace(File.separatorChar, '/')
			?.endsWith("/" + StoragePaths.WHDBOOT_KICKSTARTS) == true

	/** Where the files of one Files-tab import went. */
	data class ImportSummary(
		/** The categories that received files, in FileCategory order. */
		val categories: List<FileCategory>,
		/** The one category every imported file went to, when it is not the open tab. */
		val revealCategory: FileCategory?,
		/** Imported .lha/.lzh archives without a WHDLoad `.slave`. */
		val notWhdloadGames: Int
	)

	fun importSummary(results: List<FileManager.ImportResult>, openCategory: FileCategory): ImportSummary {
		val imported = results.filterIsInstance<FileManager.ImportResult.Imported>()
		val categories = imported.mapNotNullTo(mutableSetOf()) { importedCategory(it) }
		return ImportSummary(
			categories = FileCategory.entries.filter { it in categories },
			revealCategory = categories.singleOrNull()?.takeIf { it != openCategory },
			notWhdloadGames = imported.count { it.notWhdloadGame }
		)
	}

	/** Imports are sorted by extension, so the file's extension names its category. */
	fun importedCategory(result: FileManager.ImportResult.Imported): FileCategory? =
		FileCategory.fromExtension(File(result.path).extension)
}
