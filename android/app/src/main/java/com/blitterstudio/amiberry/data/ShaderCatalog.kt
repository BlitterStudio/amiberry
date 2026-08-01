package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.StoragePaths
import java.io.File
import java.nio.file.Files
import java.util.Locale

/**
 * Builds the Android Native shader choices using the same catalog contract as ImGui.
 *
 * The configured shader remains separate from this catalog so a temporarily missing
 * custom preset can still round-trip without being replaced.
 */
object ShaderCatalog {
	val BUILT_INS = listOf("none", "tv", "pc", "lite", "1084")

	private const val GLSL_SUFFIX = ".glsl"
	private const val GLSLP_SUFFIX = ".glslp"
	private const val SETTINGS_FILE_NAME = "amiberry.conf"
	private const val LEGACY_SETTINGS_DIRECTORY = "conf"

	private val managedPathSuffixes = mapOf(
		"config_path" to StoragePaths.CONFIGURATIONS,
		"controllers_path" to StoragePaths.CONTROLLERS,
		"whdboot_path" to StoragePaths.WHDBOOT,
		"whdload_arch_path" to StoragePaths.WHDLOAD_ARCHIVES,
		"floppy_path" to StoragePaths.FLOPPIES,
		"harddrive_path" to StoragePaths.HARD_DRIVES,
		"cdrom_path" to StoragePaths.CDROMS,
		"logfile_path" to "Amiberry.log",
		"rom_path" to StoragePaths.ROMS,
		"rp9_path" to StoragePaths.RP9,
		"saveimage_dir" to "",
		"savestate_dir" to StoragePaths.SAVE_STATES,
		"ripper_path" to "Ripper",
		"inputrecordings_dir" to StoragePaths.INPUT_RECORDINGS,
		"screenshot_dir" to StoragePaths.SCREENSHOTS,
		"nvram_dir" to StoragePaths.NVRAM,
		"video_dir" to StoragePaths.VIDEOS,
		"themes_path" to "${StoragePaths.VISUALS}/${StoragePaths.THEMES}",
		"shaders_path" to "${StoragePaths.VISUALS}/${StoragePaths.SHADERS}",
		"bezels_path" to "${StoragePaths.VISUALS}/${StoragePaths.BEZELS}"
	)

	private val visualDirectoryNames = mapOf(
		"themes_path" to StoragePaths.THEMES,
		"shaders_path" to StoragePaths.SHADERS,
		"bezels_path" to StoragePaths.BEZELS
	)

	/** Select the settings file native Android would use before legacy migration runs. */
	fun findSettingsFile(internalFilesDir: File, externalFilesDir: File?): File? {
		val candidates = buildList {
			add(File(internalFilesDir, SETTINGS_FILE_NAME))
			externalFilesDir?.let { externalRoot ->
				add(File(externalRoot, SETTINGS_FILE_NAME))
				add(File(File(externalRoot, LEGACY_SETTINGS_DIRECTORY), SETTINGS_FILE_NAME))
			}
		}
		return candidates.firstOrNull(::safeIsFile)
	}

	/** Read the native-display shader default from amiberry.conf. */
	fun resolveGlobalShader(globalSettingsFile: File?): String =
		readGlobalSettings(globalSettingsFile).shader.ifEmpty { "none" }

	/** Resolve the configuration directory used by native after loading amiberry.conf. */
	fun resolveConfigurationRoot(globalSettingsFile: File?, externalFilesDir: File?): File? {
		val paths = readGlobalSettings(globalSettingsFile)
		val inferredBasePath = inferSerializedBaseContentPath(paths.managedPaths)
		var resolvedRoot = when {
			paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() ->
				File(paths.baseContentPath, StoragePaths.CONFIGURATIONS)
			else -> externalFilesDir?.let { File(it, StoragePaths.CONFIGURATIONS) }
		}

		val serializedBasePathToSkip = inferredBasePath.takeIf {
			paths.hasBaseContentPath &&
				paths.baseContentPath.isNotEmpty() &&
				it.isNotEmpty() &&
				!pathsMatch(it, paths.baseContentPath)
		}
		for (line in paths.managedPaths) {
			if (line.key != "config_path") continue
			if (serializedBasePathToSkip != null &&
				pathsMatch(
					line.value,
					File(serializedBasePathToSkip, StoragePaths.CONFIGURATIONS).path
				)
			) {
				continue
			}
			resolvedRoot = line.value.takeIf { it.isNotEmpty() }?.let(::File)
		}
		return resolvedRoot
	}

	/**
	 * Resolve the shader root used by native Android.
	 *
	 * An explicit, empty shaders_path deliberately resolves to null: native treats that
	 * as a built-ins-only catalog instead of falling back to another content location.
	 */
	fun resolveRoot(globalSettingsFile: File?, externalFilesDir: File?): File? =
		resolveRoots(globalSettingsFile, externalFilesDir).configuredRoot

	/**
	 * Resolve the directory that is available for the Kotlin catalog to scan.
	 *
	 * Native migrates legacy visual assets into the canonical root when SDL starts. Until
	 * then, scan one existing legacy directory if the canonical destination is unavailable.
	 * Explicit empty and custom shader paths remain authoritative.
	 */
	fun resolveScanRoot(globalSettingsFile: File?, externalFilesDir: File?): File? {
		val roots = resolveRoots(globalSettingsFile, externalFilesDir)
		val configuredRoot = roots.configuredRoot ?: return null
		if (safeIsDirectory(configuredRoot)) return configuredRoot

		val canonicalRoot = roots.canonicalRoot ?: return configuredRoot
		if (!pathsMatch(configuredRoot.path, canonicalRoot.path)) return configuredRoot

		return roots.legacyScanRoots.firstOrNull(::safeIsDirectory) ?: configuredRoot
	}

	private fun resolveRoots(
		globalSettingsFile: File?,
		externalFilesDir: File?
	): ResolvedRoots {
		val paths = readGlobalSettings(globalSettingsFile)
		val inferredBasePath = inferSerializedBaseContentPath(paths.managedPaths)
		val defaultRoot = externalFilesDir?.let(::canonicalShadersRoot)
		val usesInferredBasePath = !paths.hasBaseContentPath &&
			inferredBasePath.isNotEmpty() &&
			paths.managedPaths.any { it.matchesLegacyVisualPath(inferredBasePath) }
		val canonicalRoot = when {
			paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() ->
				canonicalShadersRoot(paths.baseContentPath)
			usesInferredBasePath ->
				canonicalShadersRoot(inferredBasePath)
			else -> defaultRoot
		}
		var resolvedRoot = canonicalRoot

		val serializedBasePathToSkip = inferredBasePath.takeIf {
			paths.hasBaseContentPath &&
				paths.baseContentPath.isNotEmpty() &&
				it.isNotEmpty() &&
				!pathsMatch(it, paths.baseContentPath)
		}
		val legacyVisualRoots = buildList {
			add(externalFilesDir?.let { File(it, StoragePaths.CONFIGURATIONS) }
				?: File(StoragePaths.CONFIGURATIONS))
			paths.baseContentPath.takeIf { it.isNotEmpty() }
				?.let { add(File(it, StoragePaths.CONFIGURATIONS)) }
			inferredBasePath.takeIf { it.isNotEmpty() }
				?.let { add(File(it, StoragePaths.CONFIGURATIONS)) }
		}

		for (line in paths.managedPaths) {
			if (line.key != "shaders_path") continue
			if (serializedBasePathToSkip != null &&
				pathsMatch(line.value, canonicalShadersRoot(serializedBasePathToSkip).path)
			) {
				continue
			}
			if (legacyVisualRoots.any { line.matchesLegacyVisualPath(it) }) continue
			resolvedRoot = line.value.takeIf { it.isNotEmpty() }?.let(::File)
		}

		val legacyScanRoots = buildList {
			externalFilesDir?.let { add(legacyShadersRoot(it)) }
			when {
				paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() ->
					add(legacyShadersRoot(File(paths.baseContentPath)))
				usesInferredBasePath -> add(legacyShadersRoot(File(inferredBasePath)))
			}
		}
		return ResolvedRoots(resolvedRoot, canonicalRoot, legacyScanRoots)
	}

	/** Return built-ins followed by sorted, deduplicated external shader names. */
	fun scan(root: File?): List<String> =
		scan(root) { directory -> directory.listFiles()?.toList() }

	internal fun scan(
		root: File?,
		listChildren: (File) -> List<File>?
	): List<String> {
		if (root == null || !safeIsDirectory(root)) return BUILT_INS

		val external = linkedSetOf<String>()
		val visitedDirectories = mutableSetOf<String>()

		fun visit(directory: File, relativeDirectory: String) {
			val traversalKey = safeTraversalKey(directory)
			if (traversalKey != null && !visitedDirectories.add(traversalKey)) return

			val children = try {
				listChildren(directory)
			} catch (_: Exception) {
				null
			} ?: return

			for (entry in children) {
				try {
					if (entry.isDirectory) {
						if (Files.isSymbolicLink(entry.toPath())) continue
						val childRelative = relativeName(relativeDirectory, entry.name)
						visit(entry, childRelative)
						continue
					}
					if (!entry.isFile) continue

					val name = entry.name
					val isGlslp = name.length > GLSLP_SUFFIX.length && name.endsWith(GLSLP_SUFFIX)
					val isGlsl = !isGlslp &&
						name.length > GLSL_SUFFIX.length &&
						name.endsWith(GLSL_SUFFIX)
					if (!isGlslp && !(isGlsl && relativeDirectory.isEmpty())) continue

					external.add(relativeName(relativeDirectory, name))
				} catch (_: Exception) {
					// Match native's non-fatal traversal: keep discoveries made so far.
				}
			}
		}

		visit(root, "")
		return BUILT_INS + external.sorted()
	}

	private fun relativeName(directory: String, name: String): String =
		if (directory.isEmpty()) name else "$directory/$name"

	private fun canonicalShadersRoot(base: File): File =
		File(File(base, StoragePaths.VISUALS), StoragePaths.SHADERS)

	private fun canonicalShadersRoot(base: String): File = canonicalShadersRoot(File(base))

	private fun legacyShadersRoot(base: File): File =
		File(File(base, StoragePaths.CONFIGURATIONS), StoragePaths.SHADERS)

	private fun ManagedPath.matchesLegacyVisualPath(baseContentPath: String): Boolean =
		matchesLegacyVisualPath(File(baseContentPath, StoragePaths.CONFIGURATIONS))

	private fun ManagedPath.matchesLegacyVisualPath(configurationsRoot: File): Boolean {
		val directoryName = visualDirectoryNames[key] ?: return false
		return pathsMatch(value, File(configurationsRoot, directoryName).path)
	}

	private fun inferSerializedBaseContentPath(lines: List<ManagedPath>): String {
		val candidateCounts = sortedMapOf<String, Int>()
		for (line in lines) {
			val suffix = managedPathSuffixes[line.key] ?: continue
			val candidate = extractBaseContentRoot(line.value, suffix)
			if (candidate.isNotEmpty()) {
				candidateCounts[candidate] = candidateCounts.getOrDefault(candidate, 0) + 1
			}
		}

		var bestCandidate = ""
		var bestCount = 0
		for ((candidate, count) in candidateCounts) {
			if (count > bestCount) {
				bestCandidate = candidate
				bestCount = count
			}
		}
		return bestCandidate.takeIf { bestCount >= 3 }.orEmpty()
	}

	private fun extractBaseContentRoot(value: String, suffix: String): String {
		val normalizedValue = normalizePathForCompare(value)
		if (normalizedValue.isEmpty()) return ""

		val normalizedSuffix = normalizePathForCompare(suffix)
		if (normalizedSuffix.isEmpty()) return normalizedValue

		val suffixWithSeparator = "/${normalizedSuffix.lowercase(Locale.ROOT)}"
		if (normalizedValue.length <= suffixWithSeparator.length ||
			!normalizedValue.lowercase(Locale.ROOT).endsWith(suffixWithSeparator)
		) {
			return ""
		}
		return normalizePathForCompare(
			normalizedValue.dropLast(suffixWithSeparator.length)
		)
	}

	private fun pathsMatch(first: String, second: String): Boolean =
		normalizePathForCompare(first) == normalizePathForCompare(second)

	private fun normalizePathForCompare(path: String): String {
		if (path.isEmpty()) return ""
		var normalized = try {
			File(path).toPath().normalize().toString()
		} catch (_: Exception) {
			path
		}
		if (File.separatorChar == '\\') normalized = normalized.replace('\\', '/')
		if (normalized.isEmpty()) normalized = "."
		while (normalized.length > 1 && normalized.endsWith('/')) {
			if (normalized.length == 3 && normalized[1] == ':') break
			normalized = normalized.dropLast(1)
		}
		return normalized
	}

	private fun safeIsDirectory(file: File): Boolean =
		try {
			file.isDirectory
		} catch (_: Exception) {
			false
		}

	private fun safeIsFile(file: File): Boolean =
		try {
			file.isFile
		} catch (_: Exception) {
			false
		}

	private fun safeTraversalKey(directory: File): String? =
		try {
			directory.canonicalPath
		} catch (_: Exception) {
			try {
				directory.absolutePath
			} catch (_: Exception) {
				null
			}
		}

	private fun readGlobalSettings(file: File?): GlobalSettings {
		val settings = GlobalSettings()
		if (file == null) return settings

		try {
			file.forEachLine { rawLine ->
				val line = rawLine.trim()
				if (line.isEmpty() || line.startsWith(";")) return@forEachLine

				val separator = line.indexOf('=')
				if (separator < 0) return@forEachLine
				val key = line.substring(0, separator).trim()
				val value = line.substring(separator + 1).trim()
				if (key == "base_content_path") {
					settings.hasBaseContentPath = true
					settings.baseContentPath = value
				}
				if (key in managedPathSuffixes) {
					settings.managedPaths.add(ManagedPath(key, value))
				}
				if (key == "shader") {
					settings.shader = value
				}
			}
		} catch (_: Exception) {
			// Missing, unreadable, or partially readable settings are non-fatal.
		}
		return settings
	}

	private data class GlobalSettings(
		var baseContentPath: String = "",
		var hasBaseContentPath: Boolean = false,
		var shader: String = "none",
		val managedPaths: MutableList<ManagedPath> = mutableListOf()
	)

	private data class ResolvedRoots(
		val configuredRoot: File?,
		val canonicalRoot: File?,
		val legacyScanRoots: List<File>
	)

	private data class ManagedPath(val key: String, val value: String)
}
