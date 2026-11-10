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
	 * The folder lines a change of the base rewrites. First every managed
	 * folder's effective value, as the emulator resolves it when it loads
	 * the file: the line the file has, else what the previous base derives
	 * (a base is set), else the installation's default — a folder with no
	 * entry under a configured base sits at that base, not at the default.
	 * That value against the baseline (the previous base's derived set, or
	 * the defaults' without a base) decides: at it, the folder follows the
	 * target (the new base's set, or the defaults' when cleared); away from
	 * it, the folder is an override and keeps its line. Native compares
	 * normalized, case-insensitively (path_strings_match); so does this.
	 */
	fun mergedLines(
		newBase: String?,
		previousBase: String?,
		fileValues: Map<String, String>,
		defaultSet: Map<String, String>
	): Map<String, String> {
		val baseDerived = derivedSet(previousBase).takeIf { !previousBase.isNullOrEmpty() }
		val effective = managedSuffixes.keys.associateWith { key ->
			// A line the file has is the value, an empty one included (a
			// hand-edited unset); only a missing line falls through to the
			// previous base's derivation, else the default.
			val derived = baseDerived?.get(key).orEmpty()
			fileValues[key] ?: derived.ifEmpty { defaultSet[key].orEmpty() }
		}
		val baseline = baseDerived ?: defaultSet
		val target = if (!newBase.isNullOrEmpty()) derivedSet(newBase) else defaultSet
		return managedSuffixes.keys.associateWith { key ->
			val was = effective.getValue(key)
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
