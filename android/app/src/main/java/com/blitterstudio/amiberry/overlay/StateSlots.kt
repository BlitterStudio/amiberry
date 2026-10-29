package com.blitterstudio.amiberry.overlay

/**
 * What the built-in GUI names a session's state slots after (gui_update in
 * src/osdep/amiberry_gui.cpp), as [OverlayNative.stateNameSources] reports
 * it: the state and screenshot folders, then DF0's image, the CD image, the
 * WHDLoad game and the last configuration's name ("" where there is none),
 * then the machine's description.
 */
data class StateNameSources(
	val stateDir: String,
	val screenshotDir: String,
	val floppy0: String,
	val cd: String,
	val whdload: String,
	val lastConfig: String,
	val description: String
) {
	companion object {
		fun parse(fields: Array<String>?): StateNameSources? {
			if (fields == null || fields.size < 7) return null
			return StateNameSources(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6])
		}
	}
}

/** Where a slot's state and its thumbnail (a screenshot taken as it is saved) go. */
data class SlotPaths(val slot: Int, val state: String, val thumbnail: String)

/**
 * The menu's save state slots: the built-in GUI's slots 1 to [COUNT], with
 * its file names, so either UI loads what the other saved. The GUI names a
 * session's states after its media or configuration and has no slots for a
 * session with none (a Quick Start machine without a disk); the menu names
 * those after the machine's description instead.
 */
object StateSlots {
	const val FIRST = 1
	const val COUNT = 10
	val slots: IntRange = FIRST until FIRST + COUNT

	private const val FALLBACK_NAME = "default"

	/**
	 * The file name states are named after, as gui_update picks it: DF0's
	 * image, else the CD, else the WHDLoad game, else the configuration
	 * (whose name gets ".uss" first, so a dot in it survives the extension
	 * being cut). A session with none of them uses its description.
	 */
	fun baseFileName(sources: StateNameSources): String = when {
		sources.floppy0.isNotEmpty() -> fileName(sources.floppy0)
		sources.cd.isNotEmpty() -> fileName(sources.cd)
		sources.whdload.isNotEmpty() -> fileName(sources.whdload)
		sources.lastConfig.isNotEmpty() -> sources.lastConfig + ".uss"
		else -> safeName(sources.description) + ".uss"
	}

	/**
	 * Slot [slot]'s paths: the base name without its extension, "-<slot>"
	 * and ".uss" in the state folder, ".png" in the screenshot folder.
	 */
	fun paths(sources: StateNameSources, slot: Int): SlotPaths {
		val name = withoutExtension(baseFileName(sources)) + "-" + slot
		return SlotPaths(
			slot = slot,
			state = folder(sources.stateDir) + name + ".uss",
			thumbnail = folder(sources.screenshotDir) + name + ".png"
		)
	}

	private fun fileName(path: String): String = path.substringAfterLast('/')

	private fun withoutExtension(name: String): String =
		if ('.' in name) name.substringBeforeLast('.') else name

	private fun folder(path: String): String = if (path.isEmpty() || path.endsWith('/')) path else "$path/"

	/** A description as a file name: no path separators or dots, never empty. */
	internal fun safeName(description: String): String {
		val name = description.trim().map { c ->
			if (c.isLetterOrDigit() || c == ' ' || c == '-' || c == '_' || c == '(' || c == ')' || c == ',' || c == '+') c else '_'
		}.joinToString("").trim()
		return name.ifEmpty { FALLBACK_NAME }
	}
}
