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

		return try {
			val file = ConfigGenerator.writeConfig(context, settings, target.name)

			// Append description if provided
			if (description.isNotBlank()) {
				file.appendText("config_description=$description\n")
			}

			// Append preserved unknown lines from original config
			if (unknownLines.isNotEmpty()) {
				file.appendText("\n; Preserved settings from original config\n")
				unknownLines.forEach { file.appendText("$it\n") }
			}

			ConfigurationSaveActions.SaveResult.Saved(file)
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
