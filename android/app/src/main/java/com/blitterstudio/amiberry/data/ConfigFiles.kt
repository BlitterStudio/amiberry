package com.blitterstudio.amiberry.data

import java.io.File
import java.io.IOException

object ConfigFiles {

	fun listUserConfigFiles(confDir: File): List<File> {
		return confDir.listFiles()
			?.filter { isUserConfigFile(confDir, it) }
			.orEmpty()
	}

	fun targetFileForName(confDir: File, name: String): File? {
		val baseName = normalizedBaseName(name) ?: return null
		return try {
			val target = File(confDir, "$baseName.uae").canonicalFile
			val canonicalDir = confDir.canonicalFile
			if (target.parentFile?.canonicalFile == canonicalDir) target else null
		} catch (_: IOException) {
			null
		}
	}

	fun isValidConfigName(name: String): Boolean =
		normalizedBaseName(name) != null

	fun deleteConfig(confDir: File, path: String): Boolean {
		val file = userConfigFileFromPath(confDir, path) ?: return false
		return file.delete()
	}

	fun renameConfig(confDir: File, path: String, newName: String): File? {
		val source = userConfigFileFromPath(confDir, path)?.takeIf { it.exists() } ?: return null
		val target = targetFileForName(confDir, newName) ?: return null
		if (target.exists()) return null
		return if (source.renameTo(target)) target else null
	}

	fun duplicateConfig(confDir: File, path: String, newName: String): File? {
		val source = userConfigFileFromPath(confDir, path)?.takeIf { it.exists() } ?: return null
		val target = targetFileForName(confDir, newName) ?: return null
		if (target.exists()) return null
		return try {
			source.copyTo(target, overwrite = false)
			target
		} catch (_: IOException) {
			null
		}
	}

	private fun userConfigFileFromPath(confDir: File, path: String): File? {
		if (path.isBlank()) return null
		return try {
			val canonicalDir = confDir.canonicalFile
			val file = File(path).canonicalFile
			if (file.parentFile?.canonicalFile == canonicalDir && isUserConfigFileName(file.name)) file else null
		} catch (_: IOException) {
			null
		}
	}

	private fun isUserConfigFile(confDir: File, file: File): Boolean {
		return try {
			file.isFile &&
				file.parentFile?.canonicalFile == confDir.canonicalFile &&
				isUserConfigFileName(file.name)
		} catch (_: IOException) {
			false
		}
	}

	fun isUserConfigFileName(name: String): Boolean =
		!name.startsWith(".") && name.substringAfterLast('.', "").equals("uae", ignoreCase = true)

	private fun normalizedBaseName(name: String): String? {
		val trimmed = name.trim()
		if (trimmed.isBlank()) return null
		val baseName = trimmed.removeSuffixIgnoreCase(".uae")
		if (baseName.isBlank()) return null
		if (baseName.startsWith(".")) return null
		if (baseName.contains('/') || baseName.contains('\\') || baseName.contains("..")) return null
		return baseName
	}

	private fun String.removeSuffixIgnoreCase(suffix: String): String =
		if (endsWith(suffix, ignoreCase = true)) dropLast(suffix.length) else this
}
