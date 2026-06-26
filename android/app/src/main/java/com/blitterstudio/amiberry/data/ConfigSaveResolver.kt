package com.blitterstudio.amiberry.data

import java.io.File

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

	fun resolve(confDir: File, requestedName: String, currentConfigName: String?): Resolution {
		val target = ConfigFiles.targetFileForName(confDir, requestedName)
			?: return Resolution.InvalidName
		val trackedTarget = currentConfigName?.let { ConfigFiles.targetFileForName(confDir, it) }
		return when {
			trackedTarget != null && target == trackedTarget -> Resolution.OverwriteTracked(target)
			target.exists() -> Resolution.CollisionWithOther(target)
			else -> Resolution.WriteNew(target)
		}
	}
}
