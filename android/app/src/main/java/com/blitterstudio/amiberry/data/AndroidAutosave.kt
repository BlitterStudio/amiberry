package com.blitterstudio.amiberry.data

import android.content.Context
import java.io.File
import java.io.FileOutputStream
import java.util.Properties

/** The state and metadata kept when Android backgrounds an emulation. */
object AndroidAutosave {
	const val STATE_FILE_NAME = "Autosave.uss"
	private const val RECORD_FILE_NAME = ".autosave_session"
	private const val PATH_KEY = "path"
	private const val TITLE_KEY = "title"

	data class Session(val stateFile: File, val title: String, val modifiedAt: Long)

	fun load(context: Context): Session? = load(recordFile(context))

	fun record(context: Context, path: String, title: String): Session? =
		record(recordFile(context), File(path), title)

	fun discard(context: Context) = discard(recordFile(context))

	internal fun discard(recordFile: File) {
		recordedStateFile(recordFile)?.let { stateFile ->
			stateFile.delete()
			File(stateFile.parentFile, "${stateFile.name}.partial").delete()
		}
		recordFile.delete()
	}

	internal fun load(recordFile: File): Session? {
		val properties = Properties()
		val stateFile = try {
			recordFile.inputStream().use(properties::load)
			File(properties.getProperty(PATH_KEY).orEmpty())
		} catch (_: Exception) {
			recordFile.delete()
			return null
		}
		if (!stateFile.isFile || stateFile.length() == 0L) {
			recordFile.delete()
			return null
		}
		return Session(
			stateFile = stateFile,
			title = properties.getProperty(TITLE_KEY).orEmpty().ifBlank { "Amiga" },
			modifiedAt = stateFile.lastModified()
		)
	}

	internal fun record(recordFile: File, stateFile: File, title: String): Session? {
		if (!stateFile.isFile || stateFile.length() == 0L) return null
		val temporary = File(recordFile.parentFile, "${recordFile.name}.partial")
		val properties = Properties().apply {
			setProperty(PATH_KEY, stateFile.absolutePath)
			setProperty(TITLE_KEY, title.ifBlank { "Amiga" })
		}
		return try {
			FileOutputStream(temporary).use { properties.store(it, null) }
			if (!temporary.renameTo(recordFile)) {
				recordFile.delete()
				if (!temporary.renameTo(recordFile)) return null
			}
			Session(stateFile, properties.getProperty(TITLE_KEY), stateFile.lastModified())
		} catch (_: Exception) {
			temporary.delete()
			null
		}
	}

	private fun recordedStateFile(recordFile: File): File? {
		val properties = Properties()
		return try {
			recordFile.inputStream().use(properties::load)
			properties.getProperty(PATH_KEY)?.takeIf { it.isNotBlank() }?.let(::File)
		} catch (_: Exception) {
			null
		}
	}

	private fun recordFile(context: Context): File = File(context.filesDir, RECORD_FILE_NAME)
}
