package com.blitterstudio.amiberry.data

import java.io.File

/**
 * What changing amiberry.conf's base content folder does to the folder
 * lines derived from it, as the emulator's own setter does
 * (set_base_content_path and its merge in amiberry.cpp): every folder the
 * previous base derived takes the new base's spelling — the default
 * content root's when the base is cleared — while a folder the file had
 * set away from that baseline, a genuine per-folder override, keeps its
 * line. The keys are the ones native manages
 * (base_content_managed_path_options); retroarch_config, the floppy
 * sounds and the plugins folder are not derived and never change with it.
 */
object BaseContentPaths {

	/** The folder (and log file) suffixes a base derives, by amiberry.conf key. */
	internal val managedSuffixes: Map<String, String> = ShaderCatalog.managedPathSuffixes

	/** The set a base derives: each key to the base plus its suffix ("" is the base itself). */
	fun derivedSet(base: String?): Map<String, String> {
		if (base.isNullOrEmpty()) return managedSuffixes.keys.associateWith { "" }
		return managedSuffixes.mapValues { (key, suffix) ->
			// The empty suffix (save-data) is the base itself; the log file joins as one too.
			if (suffix.isEmpty()) base else File(base, suffix).path
		}
	}

	/**
	 * The folder lines a change of the base rewrites: [fileValues] as the
	 * file holds them (a key without a line sits at [defaultSet]), the
	 * previous base's derived set the baseline, and the new base's (or the
	 * defaults', when [newBase] is empty) the target. A value at its
	 * baseline follows the target; one away from it is an override and
	 * stays. Native compares normalized, case-insensitively
	 * (path_strings_match); so does this.
	 */
	fun mergedLines(
		newBase: String?,
		previousBase: String?,
		fileValues: Map<String, String>,
		defaultSet: Map<String, String>
	): Map<String, String> {
		val previous = managedSuffixes.keys.associateWith { key ->
			fileValues[key] ?: defaultSet[key].orEmpty()
		}
		val baseline = if (!previousBase.isNullOrEmpty()) derivedSet(previousBase) else defaultSet
		val target = if (!newBase.isNullOrEmpty()) derivedSet(newBase) else defaultSet
		return managedSuffixes.keys.associateWith { key ->
			val was = previous.getValue(key)
			if (!pathsEqual(was, baseline.getValue(key))) was else target.getValue(key)
		}
	}

	/** Native's path comparison: normalized, then case-insensitive. */
	private fun pathsEqual(first: String, second: String): Boolean =
		normalized(first) == normalized(second)

	private fun normalized(path: String): String {
		if (path.isEmpty()) return ""
		var value = try {
			File(path).toPath().normalize().toString()
		} catch (_: Exception) {
			path
		}
		if (File.separatorChar == '\\') value = value.replace('\\', '/')
		if (value.isEmpty()) value = "."
		while (value.length > 1 && value.endsWith('/')) value = value.dropLast(1)
		return value.lowercase()
	}
}
