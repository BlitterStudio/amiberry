package com.blitterstudio.amiberry.data

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import android.provider.OpenableColumns
import android.util.Log
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.StoragePaths
import java.io.File
import java.io.IOException
import java.util.ArrayDeque
import java.util.zip.CRC32

object FileManager {

	private const val TAG = "Amiberry-FileManager"

	sealed interface ImportCandidate {
		val safeName: String

		data class Importable(override val safeName: String) : ImportCandidate
		data class Unsupported(
			override val safeName: String,
			val extension: String
		) : ImportCandidate
	}

	sealed interface ImportResult {
		val safeName: String

		data class Imported(
			val path: String,
			override val safeName: String,
			/** An .lha/.lzh imported as a WHDLoad game that holds no `.slave`, so it won't start as one. */
			val notWhdloadGame: Boolean = false
		) : ImportResult

		data class Unsupported(
			override val safeName: String,
			val extension: String
		) : ImportResult

		data class Failed(override val safeName: String) : ImportResult
	}

	internal data class DocumentTreeChild(
		val documentId: String,
		val mimeType: String,
		val displayName: String?
	)

	internal data class DocumentTreeDocument(
		val documentId: String,
		val displayName: String?
	)

	internal data class DocumentTreeFailure(
		val documentId: String,
		val cause: Exception?
	)

	internal data class DocumentTreeTraversal(
		val documents: List<DocumentTreeDocument>,
		val failures: List<DocumentTreeFailure>
	)

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
		return when (val result = importFileWithResult(context, uri, category)) {
			is ImportResult.Imported -> result.path
			is ImportResult.Failed,
			is ImportResult.Unsupported -> null
		}
	}

	fun importFileWithResult(context: Context, uri: Uri, category: FileCategory): ImportResult {
		return importFileWithResult(context, uri, category, getFileName(context, uri))
	}

	/**
	 * Imports each file into the category its extension belongs to (an .adf to Floppies);
	 * a .bin goes by its size and the other files picked with it (see [sortedImportCategory]).
	 */
	fun importFilesSortedWithResults(context: Context, uris: List<Uri>): List<ImportResult> {
		val names = uris.map { getFileName(context, it) }
		val withCue = importIncludesCue(names)
		return uris.zip(names) { uri, name ->
			importFileWithResult(context, uri, category = null, displayName = name, importIncludesCue = withCue)
		}
	}

	/** [category] null sorts the file (see [sortedImportCategory]). */
	private fun importFileWithResult(
		context: Context,
		uri: Uri,
		category: FileCategory?,
		displayName: String?,
		importIncludesCue: Boolean = false
	): ImportResult {
		val target = category ?: sortedImportCategory(displayName, importIncludesCue) { fileSize(context, uri) }
		if (target == null) {
			val safeName = safeImportFileName(displayName)
			return ImportResult.Unsupported(safeName, safeName.substringAfterLast('.', missingDelimiterValue = "").lowercase())
		}
		// A sorted file's category already accepts it (a CD track .bin among CD images too).
		if (category == null) return copyImportFile(context, uri, target, safeImportFileName(displayName))
		return when (val candidate = importCandidateForCategory(displayName, target)) {
			is ImportCandidate.Importable -> copyImportFile(context, uri, target, candidate.safeName)
			is ImportCandidate.Unsupported -> ImportResult.Unsupported(candidate.safeName, candidate.extension)
		}
	}

	/**
	 * The category a file of this name is imported into when sorting by extension. A
	 * .bin is a Kickstart when it is ROM-sized ([size], read only for a .bin; unknown
	 * counts as ROM-sized); a larger one is a CD track when the import includes a .cue
	 * sheet, and unsupported otherwise.
	 */
	internal fun sortedImportCategory(
		rawName: String?,
		importIncludesCue: Boolean = false,
		size: () -> Long? = { null }
	): FileCategory? {
		val extension = safeImportFileName(rawName).substringAfterLast('.', missingDelimiterValue = "").lowercase()
		if (extension == "bin") {
			val bytes = size()
			return when {
				bytes == null || bytes <= MAX_ROM_BIN_SIZE -> FileCategory.ROMS
				importIncludesCue -> FileCategory.CD_IMAGES
				else -> null
			}
		}
		return FileCategory.fromExtension(extension)
	}

	internal fun importIncludesCue(names: List<String?>): Boolean =
		names.any { it?.substringAfterLast('.', missingDelimiterValue = "")?.lowercase() == "cue" }

	/** The size of the file behind [uri], or null when the provider does not say. */
	private fun fileSize(context: Context, uri: Uri): Long? {
		if (uri.scheme == "file") return uri.path?.let { File(it).length() }
		return try {
			context.contentResolver.query(uri, arrayOf(OpenableColumns.SIZE), null, null, null)?.use { cursor ->
				val index = cursor.getColumnIndex(OpenableColumns.SIZE)
				if (index >= 0 && cursor.moveToFirst() && !cursor.isNull(index)) cursor.getLong(index) else null
			}
		} catch (e: Exception) {
			Log.w(TAG, "Failed to read the size of $uri", e)
			null
		}
	}

	/**
	 * Import every supported file below a Storage Access Framework directory tree, each
	 * into the category of its extension. Subdirectories are traversed so users can select
	 * a collection root once.
	 */
	fun importFolderWithResults(
		context: Context,
		treeUri: Uri
	): List<ImportResult> {
		val traversal = documentsInTree(context, treeUri)
		val withCue = importIncludesCue(traversal.documents.map { it.displayName })
		val results = traversal.documents.mapTo(mutableListOf()) { document ->
			val uri = try {
				DocumentsContract.buildDocumentUriUsingTree(treeUri, document.documentId)
			} catch (e: IllegalArgumentException) {
				Log.w(TAG, "Failed to build document URI: ${document.documentId}", e)
				return@mapTo ImportResult.Failed(
					safeImportFileName(document.displayName ?: document.documentId)
				)
			}
			val displayName = document.displayName
				?.takeIf { it.isNotBlank() }
				?: document.documentId.substringAfterLast('/').substringAfterLast(':')
			importFileWithResult(context, uri, category = null, displayName = displayName, importIncludesCue = withCue)
		}
		results += traversal.failures.map { ImportResult.Failed(FOLDER_IMPORT_FAILURE_NAME) }
		return results
	}

	/**
	 * Import the supported files below a folder of the device's storage (the in-app
	 * browser, with All Files Access), each into the category of its extension. The
	 * app's own storage is left out: importing Internal storage would otherwise import
	 * the whole library again into itself.
	 */
	fun importDirectoryWithResults(context: Context, dir: File): List<ImportResult> {
		val files = DeviceStorage.supportedFilesUnder(
			dir,
			FileCategory.allExtensions,
			excluded = listOfNotNull(context.getExternalFilesDir(null))
		)
		val withCue = importIncludesCue(files.map { it.name })
		return files.map { file ->
			importFileWithResult(context, Uri.fromFile(file), category = null, displayName = file.name, importIncludesCue = withCue)
		}
	}

	private fun copyImportFile(
		context: Context,
		uri: Uri,
		category: FileCategory,
		fileName: String
	): ImportResult {
		val targetDir = getCategoryDir(context, category)
		if (!targetDir.exists()) targetDir.mkdirs()

		val finalFile = uniqueImportTarget(targetDir, fileName)

		return try {
			val inputStream = context.contentResolver.openInputStream(uri)
			if (inputStream == null) {
				Log.e(TAG, "Failed to open input stream for URI: $uri")
				return ImportResult.Failed(fileName)
			}
			inputStream.use { input ->
				finalFile.outputStream().use { output ->
					input.copyTo(output)
				}
			}
			Log.d(TAG, "Imported ${finalFile.name} to ${finalFile.absolutePath}")
			val notWhdloadGame = category == FileCategory.WHDLOAD_GAMES &&
				WhdloadArchive.hasSlave(finalFile) == false
			ImportResult.Imported(finalFile.absolutePath, finalFile.name, notWhdloadGame)
		} catch (e: Exception) {
			Log.e(TAG, "Failed to import file from URI: $uri", e)
			ImportResult.Failed(fileName)
		}
	}

	/**
	 * Scan a directory for files matching the given extensions.
	 */
	fun scanDirectory(dir: File, extensions: Set<String>, categoryOverride: FileCategory? = null): List<AmigaFile> {
		if (!dir.exists() || !dir.isDirectory) return emptyList()

		val category = categoryOverride ?: FileCategory.entries.firstOrNull { it.dirName == dir.name }

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
		for (file in scanDirectory(categoryDir, category.extensions, category)) {
			if (seenPaths.add(file.path)) results.add(file)
		}

		// Also scan the root app directory for matching files
		val rootDir = File(getAppStoragePath(context))
		for (file in scanDirectory(rootDir, category.extensions, category)) {
			if (seenPaths.add(file.path)) results.add(file)
		}

		// For ROMs, also check WHDBoot/Kickstarts
		if (category == FileCategory.ROMS) {
			val kickstartsDir = File(rootDir, StoragePaths.WHDBOOT_KICKSTARTS)
			for (file in scanDirectory(kickstartsDir, category.extensions, category)) {
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
		File(base, StoragePaths.CONFIGURATIONS).let { if (!it.exists()) it.mkdirs() }
	}

	/**
	 * Extract the display name from a content:// URI.
	 * Public so callers can validate file extensions before importing.
	 */
	fun getDisplayName(context: Context, uri: Uri): String? = getFileName(context, uri)

	internal fun safeImportFileName(rawName: String?, fallbackName: String = "imported_file"): String {
		val leafName = rawName
			.orEmpty()
			.trim()
			.replace('\\', '/')
			.substringAfterLast('/')
			.trim()

		val safeName = leafName
			.map { char ->
				if (char.isLetterOrDigit() || char in SAFE_FILENAME_CHARS) char else '_'
			}
			.joinToString(separator = "")
			.trim(' ', '.')

		return safeName.ifBlank { fallbackName }
	}

	internal fun importFileNameForCategory(rawName: String?, category: FileCategory): String? {
		return when (val candidate = importCandidateForCategory(rawName, category)) {
			is ImportCandidate.Importable -> candidate.safeName
			is ImportCandidate.Unsupported -> null
		}
	}

	internal fun importCandidateForCategory(rawName: String?, category: FileCategory): ImportCandidate {
		val fileName = safeImportFileName(rawName)
		val extension = fileName.substringAfterLast('.', missingDelimiterValue = "").lowercase()
		if (extension.isBlank() || extension !in category.extensions) {
			return ImportCandidate.Unsupported(fileName, extension)
		}
		return ImportCandidate.Importable(fileName)
	}

	internal fun uniqueImportTarget(targetDir: File, fileName: String): File {
		val targetFile = File(targetDir, fileName)
		if (!targetFile.exists()) return targetFile

		val baseName = targetFile.nameWithoutExtension
		val ext = targetFile.extension
		var counter = 1
		var candidate = File(targetDir, importTargetName(baseName, ext, counter))
		while (candidate.exists()) {
			counter++
			candidate = File(targetDir, importTargetName(baseName, ext, counter))
		}
		return candidate
	}

	internal fun isManagedCategoryFile(baseDir: File, file: AmigaFile): Boolean {
		return try {
			val canonicalBase = baseDir.canonicalFile
			val target = File(file.path).canonicalFile
			target.isFile &&
				target.isUnder(canonicalBase) &&
				target.extension.lowercase() in file.category.extensions
		} catch (_: IOException) {
			false
		}
	}

	fun deleteManagedFile(baseDir: File, file: AmigaFile): Boolean {
		if (!isManagedCategoryFile(baseDir, file)) return false
		return try {
			File(file.path).canonicalFile.delete()
		} catch (_: IOException) {
			false
		}
	}

	private fun getFileName(context: Context, uri: Uri): String? {
		// Try ContentResolver query first (works for most SAF URIs)
		context.contentResolver.query(
			uri,
			arrayOf(OpenableColumns.DISPLAY_NAME),
			null,
			null,
			null
		)?.use { cursor ->
			val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
			if (nameIndex >= 0 && cursor.moveToFirst()) {
				val name = cursor.getString(nameIndex)
				if (!name.isNullOrBlank()) return name
			}
		}
		// Fallback: extract from URI path
		return uri.lastPathSegment?.substringAfterLast('/')
	}

	private fun documentsInTree(context: Context, treeUri: Uri): DocumentTreeTraversal {
		val resolver = context.contentResolver

		val rootDocumentId = try {
			DocumentsContract.getTreeDocumentId(treeUri)
		} catch (e: IllegalArgumentException) {
			Log.e(TAG, "Invalid document tree URI: $treeUri", e)
			return DocumentTreeTraversal(
				documents = emptyList(),
				failures = listOf(DocumentTreeFailure(FOLDER_IMPORT_FAILURE_NAME, e))
			)
		}

		val traversal = traverseDocumentTree(rootDocumentId) { documentId ->
			val childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, documentId)
			val cursor = resolver.query(
				childrenUri,
				arrayOf(
					DocumentsContract.Document.COLUMN_DOCUMENT_ID,
					DocumentsContract.Document.COLUMN_MIME_TYPE,
					DocumentsContract.Document.COLUMN_DISPLAY_NAME
				),
				null,
				null,
				null
			) ?: return@traverseDocumentTree null

			cursor.use {
				val idColumn = it.getColumnIndexOrThrow(
					DocumentsContract.Document.COLUMN_DOCUMENT_ID
				)
				val mimeTypeColumn = it.getColumnIndexOrThrow(
					DocumentsContract.Document.COLUMN_MIME_TYPE
				)
				val displayNameColumn = it.getColumnIndex(
					DocumentsContract.Document.COLUMN_DISPLAY_NAME
				)
				buildList {
					while (it.moveToNext()) {
						add(
							DocumentTreeChild(
								documentId = it.getString(idColumn),
								mimeType = it.getString(mimeTypeColumn),
								displayName = if (displayNameColumn >= 0) {
									it.getString(displayNameColumn)
								} else {
									null
								}
							)
						)
					}
				}
			}
		}

		traversal.failures.forEach { failure ->
			if (failure.cause == null) {
				Log.w(TAG, "Document provider returned no cursor for directory: ${failure.documentId}")
			} else {
				Log.w(TAG, "Failed to list document tree directory: ${failure.documentId}", failure.cause)
			}
		}
		return traversal
	}

	internal fun traverseDocumentTree(
		rootDocumentId: String,
		queryChildren: (String) -> List<DocumentTreeChild>?
	): DocumentTreeTraversal {
		val pendingDirectories = ArrayDeque<String>()
		val visitedDirectories = mutableSetOf<String>()
		val documents = mutableListOf<DocumentTreeDocument>()
		val failures = mutableListOf<DocumentTreeFailure>()
		pendingDirectories.add(rootDocumentId)

		while (pendingDirectories.isNotEmpty()) {
			val documentId = pendingDirectories.removeFirst()
			if (!visitedDirectories.add(documentId)) continue

			val children = try {
				queryChildren(documentId)
			} catch (e: Exception) {
				failures += DocumentTreeFailure(documentId, e)
				continue
			}
			if (children == null) {
				failures += DocumentTreeFailure(documentId, null)
				continue
			}

			children.forEach { child ->
				if (child.mimeType == DocumentsContract.Document.MIME_TYPE_DIR) {
					pendingDirectories.add(child.documentId)
				} else {
					documents += DocumentTreeDocument(child.documentId, child.displayName)
				}
			}
		}

		return DocumentTreeTraversal(documents, failures)
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

	private val SAFE_FILENAME_CHARS = setOf(' ', '.', '_', '-', '(', ')', '+')
	private const val FOLDER_IMPORT_FAILURE_NAME = "folder"
	/** The largest .bin taken for a Kickstart: the biggest ROMs (CD32 with its extension) are 1 MiB. */
	private const val MAX_ROM_BIN_SIZE = 2L * 1024 * 1024

	private fun importTargetName(baseName: String, extension: String, counter: Int): String =
		if (extension.isBlank()) "${baseName}_$counter" else "${baseName}_$counter.$extension"

	private fun File.isUnder(baseDir: File): Boolean {
		val basePath = baseDir.path.trimEnd(File.separatorChar)
		val targetPath = path
		return targetPath == basePath || targetPath.startsWith(basePath + File.separator)
	}
}
