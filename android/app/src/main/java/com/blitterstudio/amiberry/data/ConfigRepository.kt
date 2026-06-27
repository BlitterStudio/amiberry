package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.StoragePaths
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import java.io.File
import java.io.IOException

data class ConfigInfo(
	val path: String,
	val name: String,
	val lastModified: Long,
	val description: String
)

class ConfigRepository(private val context: Context) {

	private val confDir: File
		get() = File(context.getExternalFilesDir(null), StoragePaths.CONFIGURATIONS).also {
			if (!it.exists()) it.mkdirs()
		}

	fun listConfigs(): List<ConfigInfo> {
		return ConfigFiles.listUserConfigFiles(confDir)
			.map { file ->
				val parsed = ConfigParser.parse(file)
				ConfigInfo(
					path = file.absolutePath,
					name = file.nameWithoutExtension,
					lastModified = file.lastModified(),
					description = parsed.description
				)
			}
			.sortedByDescending { it.lastModified }
	}

	fun loadConfig(path: String): ConfigParser.ParsedConfig {
		return ConfigParser.parse(File(path))
	}

	fun saveConfig(settings: EmulatorSettings, name: String, unknownLines: List<String> = emptyList(), description: String = ""): File? {
		return when (val result = saveConfigResult(settings, name, unknownLines, description)) {
			is ConfigurationSaveActions.SaveResult.Saved -> result.file
			ConfigurationSaveActions.SaveResult.AlreadyExists,
			ConfigurationSaveActions.SaveResult.Failed,
			ConfigurationSaveActions.SaveResult.InvalidName -> null
		}
	}

	fun saveConfigResult(
		settings: EmulatorSettings,
		name: String,
		unknownLines: List<String> = emptyList(),
		description: String = ""
	): ConfigurationSaveActions.SaveResult {
		val target = when (val saveTarget = ConfigurationSaveActions.targetForName(confDir, name)) {
			is ConfigurationSaveActions.SaveTarget.Ready -> saveTarget.file
			ConfigurationSaveActions.SaveTarget.InvalidName -> return ConfigurationSaveActions.SaveResult.InvalidName
			ConfigurationSaveActions.SaveTarget.AlreadyExists -> return ConfigurationSaveActions.SaveResult.AlreadyExists
		}
		return writeConfigFile(target, settings, unknownLines, description)
	}

	/**
	 * Save honoring the config currently being edited: silently overwrites the open
	 * config, writes brand-new names, and only reports AlreadyExists when the name
	 * collides with a *different* existing config and [allowOverwrite] is false.
	 */
	fun saveResolved(
		settings: EmulatorSettings,
		requestedName: String,
		currentConfigName: String?,
		unknownLines: List<String> = emptyList(),
		description: String = "",
		allowOverwrite: Boolean = false
	): ConfigurationSaveActions.SaveResult {
		val target = when (val resolution = ConfigSaveResolver.resolve(confDir, requestedName, currentConfigName)) {
			ConfigSaveResolver.Resolution.InvalidName -> return ConfigurationSaveActions.SaveResult.InvalidName
			is ConfigSaveResolver.Resolution.WriteNew -> resolution.file
			is ConfigSaveResolver.Resolution.OverwriteTracked -> resolution.file
			is ConfigSaveResolver.Resolution.CollisionWithOther ->
				if (allowOverwrite) resolution.file
				else return ConfigurationSaveActions.SaveResult.AlreadyExists
		}
		return writeConfigFile(target, settings, unknownLines, description)
	}

	/**
	 * Overwrite the exact config file at [path], preserving its original filename and
	 * extension case — used when re-saving the config the user opened. Returns InvalidName
	 * if [path] is not a user config inside the configurations directory.
	 */
	fun overwriteConfigAtPath(
		path: String,
		settings: EmulatorSettings,
		unknownLines: List<String> = emptyList(),
		description: String = ""
	): ConfigurationSaveActions.SaveResult {
		val target = ConfigFiles.userConfigFile(confDir, path)
			?: return ConfigurationSaveActions.SaveResult.InvalidName
		return writeConfigFile(target, settings, unknownLines, description)
	}

	private fun writeConfigFile(
		target: File,
		settings: EmulatorSettings,
		unknownLines: List<String>,
		description: String
	): ConfigurationSaveActions.SaveResult {
		return try {
			target.writeText(ConfigGenerator.generate(settings))
			if (description.isNotBlank()) {
				target.appendText("config_description=$description\n")
			}
			if (unknownLines.isNotEmpty()) {
				target.appendText("\n; Preserved settings from original config\n")
				unknownLines.forEach { target.appendText("$it\n") }
			}
			ConfigurationSaveActions.SaveResult.Saved(target)
		} catch (_: IOException) {
			ConfigurationSaveActions.SaveResult.Failed
		}
	}

	/**
	 * Validate that a config name is safe for use as a filename.
	 * Rejects path separators, parent directory references, and blank names.
	 */
	fun isValidConfigName(name: String): Boolean {
		return ConfigFiles.targetFileForName(confDir, name) != null
	}

	fun deleteConfig(path: String): Boolean {
		return ConfigFiles.deleteConfig(confDir, path)
	}

	fun renameConfig(path: String, newName: String): File? {
		return ConfigFiles.renameConfig(confDir, path, newName)
	}

	fun duplicateConfig(path: String, newName: String): File? {
		return ConfigFiles.duplicateConfig(confDir, path, newName)
	}

	companion object {
		@Volatile
		private var instance: ConfigRepository? = null

		fun getInstance(context: Context): ConfigRepository {
			return instance ?: synchronized(this) {
				instance ?: ConfigRepository(context.applicationContext).also { instance = it }
			}
		}
	}
}
