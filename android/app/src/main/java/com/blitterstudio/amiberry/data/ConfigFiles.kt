package com.blitterstudio.amiberry.data

import java.io.File
import java.io.IOException
import java.nio.file.Files

object ConfigFiles {

	/** Why a configuration cannot take a name; the rules Save, Rename and Duplicate share. */
	sealed interface NameProblem {
		data object Empty : NameProblem
		data object InvalidCharacters : NameProblem
		data class Taken(val name: String) : NameProblem
	}

	/** The outcome of renaming or duplicating a configuration. */
	sealed interface EditResult {
		data class Done(val file: File) : EditResult
		data class Refused(val problem: NameProblem) : EditResult
		data object Failed : EditResult
	}

	fun listUserConfigFiles(confDir: File): List<File> {
		return confDir.listFiles()
			?.filter { isUserConfigFile(confDir, it) }
			.orEmpty()
	}

	/**
	 * The file a configuration named [name] is saved as in [confDir], or null when the name is
	 * not allowed. The name keeps the case it was typed in, so a case-only rename changes the
	 * file name even on a case-insensitive volume, whose canonical path reports the existing case.
	 */
	fun targetFileForName(confDir: File, name: String, extension: String = "uae"): File? {
		val baseName = normalizedBaseName(name) ?: return null
		return try {
			val canonicalDir = confDir.canonicalFile
			val target = File(canonicalDir, "$baseName.$extension")
			if (target.canonicalFile.parentFile?.canonicalFile == canonicalDir) target else null
		} catch (_: IOException) {
			null
		}
	}

	fun isValidConfigName(name: String): Boolean =
		normalizedBaseName(name) != null

	/**
	 * The problem with naming a configuration [name], or null when it can be used.
	 * [existingNames] are the configurations already in the folder; [currentName] is the
	 * configuration being renamed, which may keep its own name or change only its case.
	 * Names compare without case, as configurations are shared with case-insensitive systems.
	 */
	fun nameProblem(name: String, existingNames: Collection<String>, currentName: String? = null): NameProblem? {
		val trimmed = name.trim()
		if (trimmed.isEmpty()) return NameProblem.Empty
		val baseName = normalizedBaseName(trimmed) ?: return NameProblem.InvalidCharacters
		if (currentName != null && baseName.equals(currentName, ignoreCase = true)) return null
		return if (existingNames.any { it.equals(baseName, ignoreCase = true) }) NameProblem.Taken(baseName) else null
	}

	/**
	 * The name the Finder would give a copy of [name]: "X copy", then "X copy 2" and so on
	 * past [existingNames]. A copy's copy is numbered from the original ("X copy 2"), not "X copy copy".
	 */
	fun copyName(name: String, existingNames: Collection<String>): String {
		val base = copyBase(name)
		var number = 1
		while (true) {
			val candidate = if (number == 1) "$base copy" else "$base copy $number"
			if (existingNames.none { it.equals(candidate, ignoreCase = true) }) return candidate
			number++
		}
	}

	/** The name copies of [name] are numbered from: "Disk copy 12" gives "Disk". */
	fun copyBase(name: String): String {
		val match = COPY_SUFFIX.find(name) ?: return name
		return if (match.range.first > 0) name.substring(0, match.range.first) else name
	}

	fun deleteConfig(confDir: File, path: String): Boolean {
		val file = userConfigFileFromPath(confDir, path) ?: return false
		return file.delete()
	}

	/**
	 * Rename the configuration at [path] to [newName], keeping its extension (Workbench.UAE
	 * stays .UAE). Refuses a source outside [confDir] and a name another configuration has;
	 * a change of case only is allowed, and the same name leaves the file as it is.
	 */
	fun renameConfig(confDir: File, path: String, newName: String): EditResult {
		val source = userConfigFileFromPath(confDir, path)?.takeIf { it.exists() } ?: return EditResult.Failed
		val problem = nameProblem(newName, emptyList())
		if (problem != null) return EditResult.Refused(problem)
		val target = targetFileForName(confDir, newName, source.extension)
			?: return EditResult.Refused(NameProblem.InvalidCharacters)
		if (target.name == source.name) return EditResult.Done(source)
		if (nameTakenByOther(confDir, target, source)) return EditResult.Refused(NameProblem.Taken(target.nameWithoutExtension))
		if (target.name.equals(source.name, ignoreCase = true)) {
			// A change of case only: a case-insensitive volume (macOS, shared
			// storage) can take that rename as a no-op, so it goes through a
			// temporary name.
			val step = File(source.parentFile, ".${source.name}.renaming")
			if (!source.renameTo(step)) return EditResult.Failed
			return if (step.renameTo(target)) EditResult.Done(target) else {
				step.renameTo(source)
				EditResult.Failed
			}
		}
		return if (source.renameTo(target)) EditResult.Done(target) else EditResult.Failed
	}

	/**
	 * Copy the configuration at [path] to [newName] beside it, with the source's extension.
	 * Refuses a source outside [confDir] and a name another configuration has.
	 */
	fun duplicateConfig(confDir: File, path: String, newName: String): EditResult {
		val source = userConfigFileFromPath(confDir, path)?.takeIf { it.exists() } ?: return EditResult.Failed
		val problem = nameProblem(newName, emptyList())
		if (problem != null) return EditResult.Refused(problem)
		val target = targetFileForName(confDir, newName, source.extension)
			?: return EditResult.Refused(NameProblem.InvalidCharacters)
		if (nameTakenByOther(confDir, target, source = null)) return EditResult.Refused(NameProblem.Taken(target.nameWithoutExtension))
		return try {
			source.copyTo(target, overwrite = false)
			EditResult.Done(target)
		} catch (_: IOException) {
			EditResult.Failed
		}
	}

	/** Whether two stored paths point at the same location once links and relative parts are resolved. */
	fun sameConfigPath(first: String, second: String): Boolean {
		if (first.isBlank() || second.isBlank()) return false
		return try {
			File(first).canonicalPath == File(second).canonicalPath
		} catch (_: IOException) {
			false
		}
	}

	/**
	 * Whether [target]'s name belongs to a configuration other than [source]: an existing file
	 * there that is not [source] itself (on a case-insensitive volume a case-only rename finds
	 * the source), or another configuration whose name differs from it only in case.
	 */
	private fun nameTakenByOther(confDir: File, target: File, source: File?): Boolean {
		if (target.exists() && (source == null || !isSameFile(source, target))) return true
		val baseName = target.nameWithoutExtension
		return listUserConfigFiles(confDir).any { file ->
			file.nameWithoutExtension.equals(baseName, ignoreCase = true) &&
				(source == null || !isSameFile(source, file))
		}
	}

	private fun isSameFile(first: File, second: File): Boolean =
		try {
			Files.isSameFile(first.toPath(), second.toPath())
		} catch (_: IOException) {
			false
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

	/** Resolve a stored absolute [path] to its File only if it is a user config inside [confDir]. */
	fun userConfigFile(confDir: File, path: String): File? = userConfigFileFromPath(confDir, path)

	private fun normalizedBaseName(name: String): String? {
		val trimmed = name.trim()
		if (trimmed.isBlank()) return null
		val baseName = trimmed.removeSuffixIgnoreCase(".uae").trim()
		if (baseName.isBlank()) return null
		if (baseName.startsWith(".")) return null
		if (baseName.any { it in FORBIDDEN_NAME_CHARACTERS }) return null
		return baseName
	}

	private val FORBIDDEN_NAME_CHARACTERS = setOf('/', '\\', ':')
	private val COPY_SUFFIX = Regex(""" copy( [0-9]+)?$""")

	private fun String.removeSuffixIgnoreCase(suffix: String): String =
		if (endsWith(suffix, ignoreCase = true)) dropLast(suffix.length) else this
}
