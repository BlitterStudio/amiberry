package com.blitterstudio.amiberry.data

import android.content.Context
import org.json.JSONArray
import java.io.File
import java.io.FileOutputStream
import java.util.Properties

/** The state and metadata kept when Android backgrounds an emulation. */
object AndroidAutosave {
	const val STATE_FILE_NAME = "Autosave.uss"
	private const val CONFIG_SUFFIX = ".uae"
	private const val RECORD_FILE_NAME = ".autosave_session"
	private const val PATH_KEY = "path"
	private const val TITLE_KEY = "title"
	private const val ARGS_KEY = "args"
	private const val SLOT_SOURCES_KEY = "slotSources"

	data class Session(
		val stateFile: File,
		val title: String,
		val modifiedAt: Long,
		val launchArgs: Array<String>,
		val slotSources: Array<String>
	)

	fun load(context: Context): Session? = load(recordFile(context))

	fun record(
		context: Context,
		path: String,
		title: String,
		launchArgs: Array<String>,
		slotSources: Array<String>
	): Session? = record(recordFile(context), File(path), title, launchArgs, slotSources)

	fun record(context: Context, path: String, title: String): Session? =
		record(context, path, title, emptyArray(), emptyArray())

	fun discard(context: Context) = discard(recordFile(context))

	internal fun discard(recordFile: File) {
		recordedStateFile(recordFile)?.let { stateFile ->
			stateFile.delete()
			File(stateFile.parentFile, "${stateFile.name}.partial").delete()
			configSnapshot(stateFile).delete()
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
			modifiedAt = stateFile.lastModified(),
			launchArgs = launchArgs(properties),
			slotSources = slotSources(properties)
		)
	}

	internal fun record(
		recordFile: File,
		stateFile: File,
		title: String,
		launchArgs: Array<String> = emptyArray(),
		slotSources: Array<String> = emptyArray()
	): Session? {
		if (!stateFile.isFile || stateFile.length() == 0L) return null
		val savedArgs = snapshotLaunchArgs(stateFile, launchArgs)
		val temporary = File(recordFile.parentFile, "${recordFile.name}.partial")
		val properties = Properties().apply {
			setProperty(PATH_KEY, stateFile.absolutePath)
			setProperty(TITLE_KEY, title.ifBlank { "Amiga" })
			setProperty(ARGS_KEY, JSONArray(savedArgs.toList()).toString())
			setProperty(SLOT_SOURCES_KEY, JSONArray(slotSources.toList()).toString())
		}
		return try {
			FileOutputStream(temporary).use { properties.store(it, null) }
			if (!temporary.renameTo(recordFile)) {
				recordFile.delete()
				if (!temporary.renameTo(recordFile)) return null
			}
			Session(
				stateFile,
				properties.getProperty(TITLE_KEY),
				stateFile.lastModified(),
				savedArgs,
				slotSources
			)
		} catch (_: Exception) {
			temporary.delete()
			null
		}
	}

	/**
	 * A state restores the machine but not Android-only launch overrides. Keep the
	 * configuration used to start it, since transient launch configs are overwritten.
	 * WHDBooter has already built the restored machine, so do not run --autoload again.
	 */
	private fun snapshotLaunchArgs(stateFile: File, launchArgs: Array<String>): Array<String> {
		val result = ArrayList<String>(launchArgs.size)
		var index = 0
		while (index < launchArgs.size) {
			when (launchArgs[index]) {
				"--autoload" -> index += 2
				"--config" -> {
					if (index + 1 >= launchArgs.size) {
						result.add(launchArgs[index++])
						continue
					}
					val source = File(launchArgs[index + 1])
					val snapshot = configSnapshot(stateFile)
					if (source.isFile && source.absolutePath != snapshot.absolutePath) {
						runCatching { source.copyTo(snapshot, overwrite = true) }
					}
					result.add("--config")
					result.add(if (snapshot.isFile) snapshot.absolutePath else source.absolutePath)
					index += 2
				}
				else -> result.add(launchArgs[index++])
			}
		}
		return result.toTypedArray()
	}

	private fun configSnapshot(stateFile: File): File =
		File(stateFile.parentFile, "${stateFile.name}$CONFIG_SUFFIX")

	private fun launchArgs(properties: Properties): Array<String> = try {
		val array = JSONArray(properties.getProperty(ARGS_KEY, "[]"))
		Array(array.length()) { index -> array.getString(index) }
	} catch (_: Exception) {
		emptyArray()
	}

	private fun slotSources(properties: Properties): Array<String> = try {
		val array = JSONArray(properties.getProperty(SLOT_SOURCES_KEY, "[]"))
		Array(array.length()) { index -> array.getString(index) }
	} catch (_: Exception) {
		emptyArray()
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
