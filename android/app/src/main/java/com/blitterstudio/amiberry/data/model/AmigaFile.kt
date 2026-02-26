package com.blitterstudio.amiberry.data.model

data class AmigaFile(
	val path: String,
	val name: String,
	val extension: String,
	val size: Long,
	val lastModified: Long,
	val category: FileCategory
) {
	val sizeDisplay: String
		get() = when {
			size < 1024 -> "$size B"
			size < 1024 * 1024 -> "${size / 1024} KB"
			else -> String.format("%.1f MB", size / (1024.0 * 1024.0))
		}
}

enum class FileCategory(
	val dirName: String,
	val displayName: String,
	val extensions: Set<String>
) {
	ROMS("roms", "ROMs", setOf("rom", "bin")),
	FLOPPIES("floppies", "Floppies", setOf("adf", "adz", "dms", "ipf", "zip", "gz")),
	HARD_DRIVES("harddrives", "Hard Drives", setOf("hdf", "hdi", "vhd")),
	CD_IMAGES("cdroms", "CD Images", setOf("iso", "cue", "chd", "nrg", "mds")),
	WHDLOAD_GAMES("lha", "WHDLoad Games", setOf("lha", "lzx", "lzh"));

	companion object {
		fun fromExtension(ext: String): FileCategory? {
			val lower = ext.lowercase()
			return entries.firstOrNull { lower in it.extensions }
		}
	}
}
