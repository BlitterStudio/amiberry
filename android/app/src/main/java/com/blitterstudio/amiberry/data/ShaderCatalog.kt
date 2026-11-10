package com.blitterstudio.amiberry.data

import java.io.File
import java.nio.file.Files

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

	/** Select the settings file native Android would use before legacy migration runs. */
	fun findSettingsFile(internalFilesDir: File, externalFilesDir: File?): File? {
		val candidates = buildList {
			add(File(internalFilesDir, SETTINGS_FILE_NAME))
			externalFilesDir?.let { externalRoot ->
				add(File(externalRoot, SETTINGS_FILE_NAME))
				add(File(File(externalRoot, LEGACY_SETTINGS_DIRECTORY), SETTINGS_FILE_NAME))
			}
		}
		return candidates.firstOrNull(GlobalSettingsPaths::safeIsFile)
	}

	/** Read the native-display shader default from amiberry.conf. */
	fun resolveGlobalShader(globalSettingsFile: File?): String =
		GlobalSettingsPaths.readGlobalSettings(globalSettingsFile).shader.ifEmpty { "none" }

	/** Resolve the configuration directory used by native after loading amiberry.conf. */
	fun resolveConfigurationRoot(globalSettingsFile: File?, externalFilesDir: File?): File? =
		GlobalSettingsPaths.resolveConfigurationRoot(globalSettingsFile, externalFilesDir)

	/**
	 * Resolve the shader root used by native Android.
	 *
	 * An explicit, empty shaders_path deliberately resolves to null: native treats that
	 * as a built-ins-only catalog instead of falling back to another content location.
	 */
	fun resolveRoot(globalSettingsFile: File?, externalFilesDir: File?): File? =
		resolveRoots(SHADERS_KEY, globalSettingsFile, externalFilesDir).configuredRoot

	/**
	 * Resolve the directories that are available for the Kotlin catalog to scan.
	 *
	 * Native merges every applicable legacy visual directory into the canonical root when
	 * SDL starts. Until then, scan the canonical directory and every existing legacy source
	 * so Kotlin exposes the same eventual catalog. Explicit empty and custom shader paths
	 * remain authoritative.
	 */
	fun resolveScanRoots(globalSettingsFile: File?, externalFilesDir: File?): List<File> =
		resolveVisualScanRoots(SHADERS_KEY, globalSettingsFile, externalFilesDir)

	/**
	 * The bezels folder the emulator resolves its bezel names in
	 * (get_bezels_path, display_catalog.cpp): the folder amiberry.conf
	 * names, the base content's, or the default — with the same legacy
	 * sources the shaders catalog scans until native has merged them.
	 */
	fun resolveBezelScanRoots(globalSettingsFile: File?, externalFilesDir: File?): List<File> =
		resolveVisualScanRoots(BEZELS_KEY, globalSettingsFile, externalFilesDir)

	/** One visuals folder's scan roots, as the shaders ones are resolved. */
	private fun resolveVisualScanRoots(
		key: String,
		globalSettingsFile: File?,
		externalFilesDir: File?
	): List<File> {
		val roots = resolveRoots(key, globalSettingsFile, externalFilesDir)
		val configuredRoot = roots.configuredRoot ?: return emptyList()

		val canonicalRoot = roots.canonicalRoot ?: return listOf(configuredRoot)
		if (!GlobalSettingsPaths.pathsMatch(configuredRoot.path, canonicalRoot.path)) return listOf(configuredRoot)

		val availableRoots = (listOf(configuredRoot) + roots.legacyScanRoots)
			.distinctBy { GlobalSettingsPaths.normalizePathForCompare(it.path) }
			.filter(GlobalSettingsPaths::safeIsDirectory)
		return availableRoots.ifEmpty { listOf(configuredRoot) }
	}

	/** The amiberry.conf key of each catalog the launcher mirrors. */
	private const val SHADERS_KEY = "shaders_path"
	private const val BEZELS_KEY = "bezels_path"

	private fun resolveRoots(
		key: String,
		globalSettingsFile: File?,
		externalFilesDir: File?
	) = GlobalSettingsPaths.resolveRoots(key, globalSettingsFile, externalFilesDir)

	/**
	 * The bezel images of the bezels folder, by their name from the root —
	 * how the emulator lists them (scan_bezels in display_catalog.cpp:
	 * png/jpg/jpeg, nested folders included, sorted).
	 */
	fun scanBezels(roots: List<File>): List<String> {
		val images = linkedSetOf<String>()
		val visitedDirectories = mutableSetOf<String>()

		fun visit(directory: File, relativeDirectory: String) {
			val traversalKey = safeTraversalKey(directory)
			if (traversalKey != null && !visitedDirectories.add(traversalKey)) return

			val children = try {
				directory.listFiles()?.toList()
			} catch (_: Exception) {
				null
			} ?: return

			for (entry in children) {
				try {
					if (entry.isDirectory) {
						if (Files.isSymbolicLink(entry.toPath())) continue
						visit(entry, relativeName(relativeDirectory, entry.name))
						continue
					}
					if (!entry.isFile) continue
					val name = entry.name.lowercase()
					if (!name.endsWith(".png") && !name.endsWith(".jpg") && !name.endsWith(".jpeg")) continue
					images.add(relativeName(relativeDirectory, entry.name))
				} catch (_: Exception) {
					// Match native's non-fatal traversal: keep discoveries made so far.
				}
			}
		}

		for (root in roots) {
			if (GlobalSettingsPaths.safeIsDirectory(root)) visit(root, "")
		}
		return images.sorted()
	}

	/** Return built-ins followed by sorted, deduplicated external shader names. */
	fun scan(root: File?): List<String> =
		scan(root) { directory -> directory.listFiles()?.toList() }

	/** Return one catalog merged from all applicable shader roots. */
	fun scan(roots: List<File>): List<String> =
		scan(roots) { directory -> directory.listFiles()?.toList() }

	internal fun scan(
		root: File?,
		listChildren: (File) -> List<File>?
	): List<String> = scan(listOfNotNull(root), listChildren)

	internal fun scan(
		roots: List<File>,
		listChildren: (File) -> List<File>?
	): List<String> {
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

		for (root in roots) {
			if (GlobalSettingsPaths.safeIsDirectory(root)) visit(root, "")
		}
		return BUILT_INS + external.sorted()
	}

	private fun relativeName(directory: String, name: String): String =
		if (directory.isEmpty()) name else "$directory/$name"

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
}
