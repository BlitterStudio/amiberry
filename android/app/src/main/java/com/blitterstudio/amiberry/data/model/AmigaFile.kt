package com.blitterstudio.amiberry.data.model

data class AmigaFile(
	val path: String,
	val name: String,
	val extension: String,
	val size: Long,
	val lastModified: Long,
	val category: FileCategory,
	val crc32: Long? = null
) {
	val sizeDisplay: String
		get() = when {
			size < 1024 -> "$size B"
			size < 1024 * 1024 -> "${size / 1024} KB"
			else -> String.format(java.util.Locale.getDefault(), "%.1f MB", size / (1024.0 * 1024.0))
		}
}

enum class FileCategory(
	val dirName: String,
	val displayName: String,
	val extensions: Set<String>
) {
	ROMS(StoragePaths.ROMS, "ROMs", setOf("rom", "bin", "roz", "a1000", "a500", "a600", "a1200", "a3000", "a4000", "cdtv", "cd32")),
	FLOPPIES(StoragePaths.FLOPPIES, "Floppies", setOf("adf", "adz", "dms", "ipf", "zip", "gz", "7z")),
	HARD_DRIVES(StoragePaths.HARD_DRIVES, "Hard Drives", setOf("hdf", "hdi", "vhd")),
	CD_IMAGES(StoragePaths.CDROMS, "CD Images", setOf("iso", "cue", "chd", "nrg", "mds")),
	WHDLOAD_GAMES(StoragePaths.WHDLOAD_ARCHIVES, "WHDLoad Games", setOf("lha", "lzx", "lzh"));

	fun acceptsFileName(name: String): Boolean {
		val lowerName = name.lowercase()
		val extension = lowerName.substringAfterLast('.', missingDelimiterValue = "")
		return extension in extensions ||
			(this == ROMS && isRomKeyFileName(lowerName)) ||
			(this == ROMS && ROM_CHIP_EXTENSION.matches(extension))
	}

	companion object {
		private val ROM_CHIP_EXTENSION = Regex("""u\d+""")

		fun isRomKeyFileName(name: String): Boolean = name.equals("rom.key", ignoreCase = true)
		fun fromExtension(ext: String): FileCategory? {
			val lower = ext.lowercase()
			return entries.firstOrNull { lower in it.extensions }
		}

		fun fromFileName(name: String): FileCategory? =
			entries.firstOrNull { it.acceptsFileName(name) }

		/** Every extension some category imports. */
		val allExtensions: Set<String> by lazy { entries.flatMapTo(mutableSetOf()) { it.extensions } }
	}
}
