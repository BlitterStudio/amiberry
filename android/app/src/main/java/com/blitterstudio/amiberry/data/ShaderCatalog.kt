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

	/**
	 * Resolve the shader root used by native Android.
	 *
	 * An explicit, empty shaders_path deliberately resolves to null: native treats that
	 * as a built-ins-only catalog instead of falling back to another content location.
	 */
	fun resolveRoot(globalSettingsFile: File?, externalFilesDir: File?): File? {
		val paths = readGlobalPaths(globalSettingsFile)
		val inferredBasePath = inferSerializedBaseContentPath(paths.managedPaths)
		val defaultRoot = externalFilesDir?.let(::canonicalShadersRoot)
		var resolvedRoot = when {
			paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() ->
				canonicalShadersRoot(paths.baseContentPath)
			!paths.hasBaseContentPath &&
				inferredBasePath.isNotEmpty() &&
				paths.managedPaths.any { it.matchesLegacyVisualPath(inferredBasePath) } ->
				canonicalShadersRoot(inferredBasePath)
			else -> defaultRoot
		}

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

		return resolvedRoot
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

	private fun readGlobalPaths(file: File?): GlobalPaths {
		val paths = GlobalPaths()
		if (file == null) return paths

		try {
			file.forEachLine { rawLine ->
				val line = rawLine.trim()
				if (line.isEmpty() || line.startsWith(";")) return@forEachLine

				val separator = line.indexOf('=')
				if (separator < 0) return@forEachLine
				val key = line.substring(0, separator).trim()
				val value = line.substring(separator + 1).trim()
				if (key == "base_content_path") {
					paths.hasBaseContentPath = true
					paths.baseContentPath = value
				}
				if (key in managedPathSuffixes) {
					paths.managedPaths.add(ManagedPath(key, value))
				}
			}
		} catch (_: Exception) {
			// Missing, unreadable, or partially readable settings are non-fatal.
		}
		return paths
	}

	private data class GlobalPaths(
		var baseContentPath: String = "",
		var hasBaseContentPath: Boolean = false,
		val managedPaths: MutableList<ManagedPath> = mutableListOf()
	)

	private data class ManagedPath(val key: String, val value: String)
}
