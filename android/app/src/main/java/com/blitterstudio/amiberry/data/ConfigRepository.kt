package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import java.io.File

data class ConfigInfo(
	val path: String,
	val name: String,
	val lastModified: Long,
	val description: String
)

class ConfigRepository(private val context: Context) {

	private val confDir: File
		get() = File(context.getExternalFilesDir(null), "conf").also {
			if (!it.exists()) it.mkdirs()
		}

	fun listConfigs(): List<ConfigInfo> {
		return confDir.listFiles { f -> f.extension == "uae" && !f.name.startsWith(".") }
			?.map { file ->
				val parsed = ConfigParser.parse(file)
				ConfigInfo(
					path = file.absolutePath,
					name = file.nameWithoutExtension,
					lastModified = file.lastModified(),
					description = parsed.description
				)
			}
			?.sortedByDescending { it.lastModified }
			?: emptyList()
	}

	fun loadConfig(path: String): ConfigParser.ParsedConfig {
		return ConfigParser.parse(File(path))
	}

	fun saveConfig(settings: EmulatorSettings, name: String, unknownLines: List<String> = emptyList()): File {
		val file = ConfigGenerator.writeConfig(context, settings, "$name.uae")

		// Append preserved unknown lines from original config
		if (unknownLines.isNotEmpty()) {
			file.appendText("\n; Preserved settings from original config\n")
			unknownLines.forEach { file.appendText("$it\n") }
		}

		return file
	}

	/**
	 * Validate that a config name is safe for use as a filename.
	 * Rejects path separators, parent directory references, and blank names.
	 */
	private fun isValidConfigName(name: String): Boolean {
		if (name.isBlank()) return false
		if (name.contains('/') || name.contains('\\') || name.contains("..")) return false
		val testFile = File(confDir, "$name.uae")
		return testFile.parentFile?.canonicalPath == confDir.canonicalPath
	}

	fun deleteConfig(path: String): Boolean {
		return File(path).delete()
	}

	fun renameConfig(path: String, newName: String): File? {
		if (!isValidConfigName(newName)) return null
		val oldFile = File(path)
		if (!oldFile.exists()) return null
		val newFile = File(oldFile.parentFile, "$newName.uae")
		return if (oldFile.renameTo(newFile)) newFile else null
	}

	fun duplicateConfig(path: String, newName: String): File? {
		if (!isValidConfigName(newName)) return null
		val source = File(path)
		if (!source.exists()) return null
		val target = File(source.parentFile, "$newName.uae")
		source.copyTo(target, overwrite = false)
		return target
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
