package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.StoragePaths
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

	/**
	 * Resolve the shader root used by native Android.
	 *
	 * An explicit, empty shaders_path deliberately resolves to null: native treats that
	 * as a built-ins-only catalog instead of falling back to another content location.
	 */
	fun resolveRoot(globalSettingsFile: File?, externalFilesDir: File?): File? {
		val paths = readGlobalPaths(globalSettingsFile)
		if (paths.hasShaderPath) {
			return paths.shaderPath.takeIf { it.isNotEmpty() }?.let(::File)
		}
		if (paths.baseContentPath.isNotEmpty()) {
			return File(
				File(File(paths.baseContentPath), StoragePaths.VISUALS),
				StoragePaths.SHADERS
			)
		}
		return externalFilesDir?.let {
			File(File(it, StoragePaths.VISUALS), StoragePaths.SHADERS)
		}
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
				when (key) {
					"base_content_path" -> paths.baseContentPath = value
					"shaders_path" -> {
						paths.hasShaderPath = true
						paths.shaderPath = value
					}
				}
			}
		} catch (_: Exception) {
			// Missing, unreadable, or partially readable settings are non-fatal.
		}
		return paths
	}

	private data class GlobalPaths(
		var baseContentPath: String = "",
		var hasShaderPath: Boolean = false,
		var shaderPath: String = ""
	)
}
