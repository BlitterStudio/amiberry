package com.blitterstudio.amiberry.data

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import android.util.Log
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import java.util.zip.CRC32
import java.io.File

object FileManager {

	private const val TAG = "Amiberry-FileManager"

	fun getAppStoragePath(context: Context): String {
		return context.getExternalFilesDir(null)?.absolutePath ?: ""
	}

	fun getCategoryDir(context: Context, category: FileCategory): File {
		return File(getAppStoragePath(context), category.dirName)
	}

	/**
	 * Import a file from a SAF content:// URI into the appropriate app storage directory.
	 * Returns the local file path on success, or null on failure.
	 */
	fun importFile(context: Context, uri: Uri, category: FileCategory): String? {
		val fileName = getFileName(context, uri) ?: "imported_file"
		val targetDir = getCategoryDir(context, category)
		if (!targetDir.exists()) targetDir.mkdirs()

		val targetFile = File(targetDir, fileName)

		// Avoid overwriting: add suffix if file exists
		val finalFile = if (targetFile.exists()) {
			val baseName = targetFile.nameWithoutExtension
			val ext = targetFile.extension
			var counter = 1
			var candidate = File(targetDir, "${baseName}_$counter.$ext")
			while (candidate.exists()) {
				counter++
				candidate = File(targetDir, "${baseName}_$counter.$ext")
			}
			candidate
		} else {
			targetFile
		}

		return try {
			val inputStream = context.contentResolver.openInputStream(uri)
			if (inputStream == null) {
				Log.e(TAG, "Failed to open input stream for URI: $uri")
				return null
			}
			inputStream.use { input ->
				finalFile.outputStream().use { output ->
					input.copyTo(output)
				}
			}
			Log.d(TAG, "Imported ${finalFile.name} to ${finalFile.absolutePath}")
			finalFile.absolutePath
		} catch (e: Exception) {
			Log.e(TAG, "Failed to import file from URI: $uri", e)
			null
		}
	}

	/**
	 * Scan a directory for files matching the given extensions.
	 */
	fun scanDirectory(dir: File, extensions: Set<String>): List<AmigaFile> {
		if (!dir.exists() || !dir.isDirectory) return emptyList()

		val category = FileCategory.entries.firstOrNull { it.dirName == dir.name }

		return dir.listFiles()
			?.filter { file ->
				file.isFile && file.extension.lowercase() in extensions
			}
			?.map { file ->
				val crc = if (category == FileCategory.ROMS) calculateCrc32(file) else null
				AmigaFile(
					path = file.absolutePath,
					name = file.name,
					extension = file.extension.lowercase(),
					size = file.length(),
					lastModified = file.lastModified(),
					category = category ?: FileCategory.fromExtension(file.extension) ?: FileCategory.FLOPPIES,
					crc32 = crc
				)
			}
			?.sortedBy { it.name.lowercase() }
			?: emptyList()
	}

	/**
	 * Scan all known directories for files of a given category.
	 * Also checks the root app storage dir for matching files (users may put files there).
	 */
	fun scanForCategory(context: Context, category: FileCategory): List<AmigaFile> {
		val results = mutableListOf<AmigaFile>()
		val seenPaths = mutableSetOf<String>()

		// Scan the category-specific directory
		val categoryDir = getCategoryDir(context, category)
		for (file in scanDirectory(categoryDir, category.extensions)) {
			if (seenPaths.add(file.path)) results.add(file)
		}

		// Also scan the root app directory for matching files
		val rootDir = File(getAppStoragePath(context))
		for (file in scanDirectory(rootDir, category.extensions)) {
			if (seenPaths.add(file.path)) results.add(file)
		}

		// For ROMs, also check whdboot/Kickstarts
		if (category == FileCategory.ROMS) {
			val kickstartsDir = File(rootDir, "whdboot/game-data/Kickstarts")
			for (file in scanDirectory(kickstartsDir, category.extensions)) {
				if (seenPaths.add(file.path)) results.add(file)
			}
		}

		return results.sortedBy { it.name.lowercase() }
	}

	fun ensureDirectories(context: Context) {
		val base = context.getExternalFilesDir(null) ?: return
		FileCategory.entries.forEach { category ->
			File(base, category.dirName).let { if (!it.exists()) it.mkdirs() }
		}
		File(base, "conf").let { if (!it.exists()) it.mkdirs() }
	}

	/**
	 * Extract the display name from a content:// URI.
	 * Public so callers can validate file extensions before importing.
	 */
	fun getDisplayName(context: Context, uri: Uri): String? = getFileName(context, uri)

	private fun getFileName(context: Context, uri: Uri): String? {
		// Try ContentResolver query first (works for most SAF URIs)
		context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
			val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
			if (nameIndex >= 0 && cursor.moveToFirst()) {
				val name = cursor.getString(nameIndex)
				if (!name.isNullOrBlank()) return name
			}
		}
		// Fallback: extract from URI path
		return uri.lastPathSegment?.substringAfterLast('/')
	}

	private fun calculateCrc32(file: File): Long? {
		return try {
			val crc = CRC32()
			file.inputStream().use { input ->
				val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
				while (true) {
					val read = input.read(buffer)
					if (read <= 0) break
					crc.update(buffer, 0, read)
				}
			}
			crc.value and 0xffffffffL
		} catch (e: Exception) {
			Log.w(TAG, "Failed to calculate CRC32 for ${file.absolutePath}", e)
			null
		}
	}
}
