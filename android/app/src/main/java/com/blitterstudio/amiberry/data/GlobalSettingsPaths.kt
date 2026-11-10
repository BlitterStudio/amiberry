package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.StoragePaths
import java.io.File
import java.util.Locale

/**
 * How the emulator resolves the content folders amiberry.conf names, as the
 * Kotlin side needs it: one shared resolver (once ShaderCatalog's own) for
 * every managed path setting — the folder a setting's own line names, else
 * the base content root's (an explicit `base_content_path`, the one the
 * file's own lines imply, or the app's storage) with the folder's default
 * name.
 *
 * The mirror of amiberry.cpp's base content path set: its defaults
 * (`get_base_content_path_set`) are [managedPathSuffixes], and a line whose
 * value spells the base's own folder is the base coming along, not an
 * override (`merge_base_content_path_overrides`), with the legacy visual
 * folders (a `Configurations/Shaders` spelling) skipped the same way.
 */
internal object GlobalSettingsPaths {

	internal val managedPathSuffixes = mapOf(
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

	/**
	 * Every amiberry.conf key that names a path: the managed set above,
	 * whose folders the base content root derives, and the three the
	 * loader reads on their own (the RetroArch config, the floppy
	 * sounds, the plugins override), whose defaults it computes itself.
	 */
	internal val pathSettingKeys = managedPathSuffixes.keys +
		setOf("retroarch_config", "floppy_sounds_dir", "plugins_dir")

	/** The `Visuals` subfolders, whose folders had a legacy spelling beside `Configurations`. */
	private val visualDirectoryNames = mapOf(
		"themes_path" to StoragePaths.THEMES,
		"shaders_path" to StoragePaths.SHADERS,
		"bezels_path" to StoragePaths.BEZELS
	)

	internal data class ManagedPath(val key: String, val value: String)

	internal class GlobalSettings {
		var baseContentPath: String = ""
		var hasBaseContentPath: Boolean = false
		var shader: String = "none"
		val managedPaths: MutableList<ManagedPath> = mutableListOf()
	}

	/** The amiberry.conf [file] holds, read non-fatally: missing keys keep their defaults. */
	internal fun readGlobalSettings(file: File?): GlobalSettings {
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
				if (key in pathSettingKeys) {
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

	/**
	 * The content root the file's own managed lines imply: three or more of
	 * them spelling folders under one root (amiberry.cpp's
	 * `infer_serialized_base_content_path`).
	 */
	internal fun inferSerializedBaseContentPath(lines: List<ManagedPath>): String {
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

	internal fun pathsMatch(first: String, second: String): Boolean =
		normalizePathForCompare(first) == normalizePathForCompare(second)

	internal fun normalizePathForCompare(path: String): String {
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

	internal fun safeIsDirectory(file: File): Boolean =
		try {
			file.isDirectory
		} catch (_: Exception) {
			false
		}

	internal fun safeIsFile(file: File): Boolean =
		try {
			file.isFile
		} catch (_: Exception) {
			false
		}

	internal data class ResolvedRoots(
		val configuredRoot: File?,
		val canonicalRoot: File?,
		val legacyScanRoots: List<File>
	)

	/**
	 * A `./x` or `../x` value, as native loads it: resolved against the
	 * relative path base (target_resolve_relative_path), which on Android
	 * is the content home — the app's own storage the launcher passes as
	 * [externalFilesDir]. `./` alone names the base; a bare `x` (no `./`
	 * prefix) is not one of the base-relative spellings and stays as it is.
	 * The app's working directory is that same base, so a relative path
	 * existing only there cannot win over the base join (native's
	 * working-directory exception cannot fire).
	 */
	internal fun resolveBaseRelative(value: String, base: File?): File? {
		val baseRelative = value.startsWith("./") || value.startsWith("../")
		if (base == null || !baseRelative) return File(value)
		return try {
			File(base, value).toPath().normalize().toFile()
		} catch (_: Exception) {
			File(value)
		}
	}

	/**
	 * The resolver for one reading of the settings: every managed path
	 * resolved over the same lines, so a caller resolving many folders
	 * reads the file once. What [resolveRoots] and its companions compute.
	 */
	internal class Resolver private constructor(
		private val paths: GlobalSettings,
		private val inferredBasePath: String,
		private val externalFilesDir: File?
	) {
		/** One managed path's folders, as [resolveRoots] describes them. */
		fun roots(key: String): ResolvedRoots {
			// A path outside the base content set (the RetroArch config, the
			// floppy sounds, the plugins override) has no folder of the
			// base's own to default to: the core computes its default.
			val suffix = managedPathSuffixes[key]
			val defaultRoot = suffix?.let { s -> externalFilesDir?.let { canonicalRoot(it, s) } }
			// The implied base redirects only the visual catalogs (native
			// applies it to them alone); every other path's default stays
			// the app's own storage.
			val usesInferredBasePath = key in visualDirectoryNames &&
				!paths.hasBaseContentPath &&
				inferredBasePath.isNotEmpty() &&
				paths.managedPaths.any { it.matchesLegacyVisualPath(inferredBasePath) }
			val canonicalRoot = when {
				paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() ->
					suffix?.let { canonicalRoot(paths.baseContentPath, it) }
				usesInferredBasePath ->
					suffix?.let { canonicalRoot(inferredBasePath, it) }
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
				if (line.key != key) continue
				if (suffix != null && serializedBasePathToSkip != null &&
					pathsMatch(line.value, canonicalRoot(serializedBasePathToSkip, suffix).path)
				) {
					continue
				}
				if (legacyVisualRoots.any { line.matchesLegacyVisualPath(it) }) continue
				resolvedRoot = line.value.takeIf { it.isNotEmpty() }
					?.let { resolveBaseRelative(it, externalFilesDir) }
			}

			val legacyScanRoots = if (key in visualDirectoryNames && suffix != null) {
				buildList {
					externalFilesDir?.let { add(legacyVisualRoot(it, suffix)) }
					when {
						paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() ->
							add(legacyVisualRoot(File(paths.baseContentPath), suffix))
						usesInferredBasePath -> add(legacyVisualRoot(File(inferredBasePath), suffix))
					}
				}
			} else {
				emptyList()
			}
			return ResolvedRoots(resolvedRoot, canonicalRoot, legacyScanRoots)
		}

		/** The configuration directory, as [resolveConfigurationRoot] resolves it. */
		fun configurationRoot(): File? {
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
				resolvedRoot = line.value.takeIf { it.isNotEmpty() }
					?.let { resolveBaseRelative(it, externalFilesDir) }
			}
			return resolvedRoot
		}

		/** The base content root in effect: the one named, implied, or the app's own storage. */
		fun baseRoot(): File? = when {
			paths.hasBaseContentPath && paths.baseContentPath.isNotEmpty() -> File(paths.baseContentPath)
			inferredBasePath.isNotEmpty() -> File(inferredBasePath)
			else -> externalFilesDir
		}

		companion object {
			/** The resolver for the settings [globalSettingsFile] holds. */
			fun of(globalSettingsFile: File?, externalFilesDir: File?): Resolver {
				val paths = readGlobalSettings(globalSettingsFile)
				return Resolver(paths, inferSerializedBaseContentPath(paths.managedPaths), externalFilesDir)
			}
		}
	}

	/**
	 * One managed path's folders, as native resolves it:
	 * [ResolvedRoots.configuredRoot] is the folder in effect (the setting's
	 * own line, else the base's; null when an explicit empty line or a
	 * missing base leaves none), [ResolvedRoots.canonicalRoot] the base's
	 * own folder, and [ResolvedRoots.legacyScanRoots] the legacy visual
	 * folders a catalog scans until native has merged them away.
	 */
	internal fun resolveRoots(
		key: String,
		globalSettingsFile: File?,
		externalFilesDir: File?
	): ResolvedRoots = Resolver.of(globalSettingsFile, externalFilesDir).roots(key)

	/**
	 * Resolve the configuration directory used by native after loading amiberry.conf.
	 */
	internal fun resolveConfigurationRoot(globalSettingsFile: File?, externalFilesDir: File?): File? =
		Resolver.of(globalSettingsFile, externalFilesDir).configurationRoot()

	/**
	 * The base content root in effect: the one the file names, the one its
	 * own lines imply, or the app's own storage.
	 */
	internal fun baseContentRoot(globalSettingsFile: File?, externalFilesDir: File?): File? =
		Resolver.of(globalSettingsFile, externalFilesDir).baseRoot()

	private fun canonicalRoot(base: File, suffix: String): File =
		if (suffix.isEmpty()) base else File(base, suffix)

	private fun canonicalRoot(base: String, suffix: String): File = canonicalRoot(File(base), suffix)

	/** A visual catalog's legacy spelling: the folder inside `Configurations`. */
	private fun legacyVisualRoot(base: File, suffix: String): File {
		val directoryName = suffix.substringAfterLast('/')
		return File(File(base, StoragePaths.CONFIGURATIONS), directoryName)
	}

	private fun ManagedPath.matchesLegacyVisualPath(baseContentPath: String): Boolean =
		matchesLegacyVisualPath(File(baseContentPath, StoragePaths.CONFIGURATIONS))

	private fun ManagedPath.matchesLegacyVisualPath(configurationsRoot: File): Boolean {
		val directoryName = visualDirectoryNames[key] ?: return false
		return pathsMatch(value, File(configurationsRoot, directoryName).path)
	}
}
