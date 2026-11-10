package com.blitterstudio.amiberry.data

import java.io.File
import java.io.IOException

/**
 * Classifies a save request against the config currently being edited.
 * Pure: no Android types, no IO beyond canonical-path resolution via ConfigFiles.
 */
object ConfigSaveResolver {

	sealed interface Resolution {
		/** The requested name cannot be used as a config filename. */
		data object InvalidName : Resolution
		/** No config with this name exists yet — write a brand new file. */
		data class WriteNew(val file: File) : Resolution
		/** The name resolves to the config currently open — overwrite it silently. */
		data class OverwriteTracked(val file: File) : Resolution
		/** The name collides with a *different* existing config — confirm before overwriting. */
		data class CollisionWithOther(val file: File) : Resolution
	}

	/**
	 * [openConfigPath] is the file the editor has open, by its whole path
	 * (wherever it was loaded from), not its name: the configurations
	 * folder may have changed since, and the same name under the new
	 * folder is another machine's config, not the open one's.
	 */
	fun resolve(confDir: File, requestedName: String, openConfigPath: String?): Resolution {
		val target = ConfigFiles.targetFileForName(confDir, requestedName)
			?: return Resolution.InvalidName
		val trackedTarget = openConfigPath?.let(::File)
		return when {
			trackedTarget != null && sameFile(target, trackedTarget) -> Resolution.OverwriteTracked(target)
			target.exists() -> Resolution.CollisionWithOther(target)
			else -> Resolution.WriteNew(target)
		}
	}

	private fun sameFile(first: File, second: File): Boolean = try {
		first.canonicalFile == second.canonicalFile
	} catch (_: IOException) {
		first.absolutePath == second.absolutePath
	}
}
