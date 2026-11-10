package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.SettingsSchema
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.io.File
import java.util.concurrent.atomic.AtomicLong

/**
 * The emulator's own settings, read and written as the [SettingsSchema]'s
 * host settings describe them: the launcher edits amiberry.conf (the file
 * the emulator process reads at start-up and its Advanced settings window
 * rewrites), so it only writes while no emulation runs, and every write
 * re-reads the file first — nothing else in it (comments, paths, the
 * recent-file lists) is touched, and lines the schema does not know stay
 * verbatim.
 *
 * A value keeps its amiberry.conf spelling, except that a bool reads and
 * writes as "true"/"false" here whatever spelling the file carries (the
 * loader takes yes/no/true/false/1/0); the file is written with the yes/no
 * the emulator's own writer uses.
 */
class HostSettingsStore(
	private val file: File,
	private val schema: SettingsSchema,
	/** The default content root (the app's external files dir); what a cleared base returns the derived folders to. */
	private val defaultContentRoot: File? = null
) {
	/** The values the file holds, by setting ID; a setting the file lacks is absent. */
	fun read(): Map<String, String> {
		val values = LinkedHashMap<String, String>()
		val lines = readLines()
		for ((key, value) in lines.mapNotNull(::parseLine)) {
			val option = schema.hostSetting(key) ?: continue
			values[option.id] = normalize(option, value)
		}
		return values
	}

	/** One change at a time: each write re-reads a file no other write is mid-way through. */
	private val writeMutex = Mutex()

	/**
	 * Writes [values] into the file: each setting's line (the last one when
	 * the file repeats it, the repeats dropped) takes its value, a setting
	 * without a line is appended, and everything else stays as it is. The
	 * whole read-modify-write is serialized ([writeMutex]) so concurrent
	 * changes cannot lose each other, the file is read again before
	 * anything is written, and the new content lands in a uniquely named
	 * temporary file renamed over it, so a failure leaves the old one.
	 * False when a value is refused; the file is then not written.
	 */
	suspend fun write(values: Map<String, String>): Boolean = writeMutex.withLock {
		if (values.any { (id, value) -> !accepts(id, value) }) return@withLock false
		val fileLines = readLines()
		val expanded = expandBaseContentChange(values, fileLines)
		val lines = fileLines.toMutableList()
		for ((id, value) in expanded) {
			val option = schema.hostSetting(id) ?: continue
			val key = option.id
			val written = fileText(option, value)
			val at = lines.indexOfLast { line -> parseKey(line) == key }
			if (at < 0) {
				if (written != null) lines += written
			} else {
				// Earlier repeats would fight the last line, which the
				// loader honours; the emulator's writer keeps one of each.
				val repeats = lines.indices.count { it < at && parseKey(lines[it]) == key }
				lines.indices.reversed()
					.filter { index -> index != at && parseKey(lines[index]) == key }
					.forEach(lines::removeAt)
				val position = at - repeats
				if (written != null) lines[position] = written else lines.removeAt(position)
			}
		}
		writeAtomically(lines)
	}

	/**
	 * The line [value] writes, null to have the setting's line gone: a
	 * missing line is the built-in default, which is what an empty folder
	 * means (an empty text or hotkey value — no key, no quit key — is a
	 * value of its own, so its line stays).
	 */
	private fun fileText(option: SettingsSchema.Option, value: String): String? {
		if (value.isEmpty() && option.type == TYPE_PATH) return null
		return when (option.type) {
			TYPE_BOOL -> if (value == "true") "yes" else "no"
			else -> value
		}.let { "${option.id}=$it" }
	}

	/** Whether [value] is one the loader keeps for [id] (the UI offers only such). */
	fun accepts(id: String, value: String): Boolean {
		val option = schema.hostSetting(id) ?: return false
		if (value.contains('\n') || value.contains('\r')) return false
		// The loader strips whitespace at both ends, so a value with either
		// would not come back as it was set.
		if (value != value.trim()) return false
		return when (option.type) {
			TYPE_BOOL -> value == "true" || value == "false"
			TYPE_INT, TYPE_REAL -> {
				// The choice spelling of a 0/1 number is also accepted.
				val number = value.toDoubleOrNull() ?: return false
				val min = option.min ?: Double.NEGATIVE_INFINITY
				val max = option.max ?: Double.POSITIVE_INFINITY
				number in min..max
			}
			TYPE_CHOICE -> option.choices.any { it.value == value } || acceptsDynamicChoice(option, value)
			// A path at none, a key unbound: empty is a value (the write drops
			// a folder's line and keeps a text one).
			else -> value.length + id.length < 900
		}
	}

	/** The choices the schema carries empty because they name the host's own things. */
	private fun acceptsDynamicChoice(option: SettingsSchema.Option, value: String): Boolean = when (option.id) {
		// A default port device: a joyport token, or empty for the fallback
		// the layer names (settings_host.cpp's port devices).
		"default_controller1", "default_controller2", "default_controller3", "default_controller4",
		"default_mouse1", "default_mouse2" ->
			value.isEmpty() || PORT_DEVICE_TOKENS.any { it == value } ||
				(value.startsWith("custom") && value.drop(6).all(Char::isDigit))
		// The audio device's index; any in range is kept (the device it
		// names may not be connected now).
		"default_soundcard" -> value.toIntOrNull() in 0..255
		// A shader or bezel: "none" names none, anything else a file in its folder.
		"shader", "shader_rtg", "custom_bezel" -> value.isNotEmpty()
		else -> false
	}

	private fun normalize(option: SettingsSchema.Option, value: String): String = when (option.type) {
		TYPE_BOOL -> when (value.lowercase()) {
			"yes", "y", "true", "t", "1" -> "true"
			else -> "false"
		}
		else -> value
	}

	/** key=value as the loader reads it: ';' a comment, no '=' not a setting, both ends trimmed. */
	private fun parseLine(line: String): Pair<String, String>? {
		val key = parseKey(line) ?: return null
		val separator = line.indexOf('=')
		if (separator < 0) return null
		val value = line.substring(separator + 1).trim()
		return key to value
	}

	private fun parseKey(line: String): String? {
		val trimmed = line.trimStart()
		if (trimmed.isEmpty() || trimmed.startsWith(";")) return null
		val separator = trimmed.indexOf('=')
		if (separator < 0) return null
		val key = trimmed.substring(0, separator).trim()
		return key.ifEmpty { null }
	}

	/**
	 * A write of base_content_path rewrites the folder lines the base
	 * derives, exactly as the emulator's setter does
	 * (set_base_content_path: the merge in BaseContentPaths) — only
	 * clearing a set base changes anything, and a folder the file had set
	 * away from its baseline keeps its line. Explicit writes of the
	 * derived folders still win.
	 */
	private fun expandBaseContentChange(values: Map<String, String>, lines: List<String>): Map<String, String> {
		if (!values.containsKey("base_content_path")) return values
		val fileValues = lines.mapNotNull(::parseLine).toMap()
		val previousBase = fileValues["base_content_path"].orEmpty()
		val newBase = values.getValue("base_content_path")
		// Clearing with no base set is the setter's no-op.
		if (newBase.isEmpty() && previousBase.isEmpty()) return values
		val defaults = BaseContentPaths.derivedSet(defaultContentRoot?.path)
		val merged = BaseContentPaths.mergedLines(newBase, previousBase, fileValues, defaults)
		return merged + values
	}

	private fun readLines(): List<String> = try {
		val text = file.readText()
		when {
			text.isEmpty() -> emptyList()
			else -> text.split('\n')
				.let { lines -> if (text.endsWith("\n")) lines.dropLast(1) else lines }
				.map { it.trimEnd('\r') }
		}
	} catch (_: Exception) {
		emptyList()
	}

	private fun writeAtomically(lines: List<String>): Boolean {
		val text = if (lines.isEmpty()) "" else lines.joinToString(separator = "\n", postfix = "\n")
		return try {
			file.parentFile?.mkdirs()
			val temp = File(file.parentFile, "${file.name}.${tempCounter.incrementAndGet()}.tmp")
			temp.writeText(text)
			if (!temp.renameTo(file)) {
				// A rename across filesystems cannot happen (the temporary
				// file sits beside the settings), but fall back to the
				// direct write rather than lose the change.
				file.writeText(text)
				temp.delete()
			}
			true
		} catch (_: Exception) {
			false
		}
	}

	companion object {
		private const val SETTINGS_FILE_NAME = "amiberry.conf"

		private val TYPE_BOOL = "bool"
		private val TYPE_INT = "int"
		private val TYPE_REAL = "real"
		private val TYPE_CHOICE = "choice"
		private val TYPE_PATH = "path"
		private val TYPE_STRING = "string"
		private val TYPE_HOTKEY = "hotkey"

		/**
		 * The joyport device tokens the core reads besides "none" and the
		 * on-screen devices (inputdevice_joyport_config): joysticks, mice
		 * and keyboard layouts by index.
		 */
		private val PORT_DEVICE_TOKENS = buildSet {
			for (i in 0..3) add("joy$i")
			for (i in 0..3) add("mouse$i")
			addAll(listOf("mouse", "onscreen_joy", "onscreen_mouse", "kbd1", "kbd2", "kbd3", "kbd9"))
		}

		/**
		 * The amiberry.conf the emulator reads and writes on this
		 * installation: the one the launcher already resolves for the
		 * shader catalog, else the internal files directory, where the
		 * emulator's settings directory is on Android (SDL's pref path).
		 */
		fun settingsFile(internalFilesDir: File, externalFilesDir: File?): File =
			ShaderCatalog.findSettingsFile(internalFilesDir, externalFilesDir)
				?: File(internalFilesDir, SETTINGS_FILE_NAME)

		private val tempCounter = AtomicLong()

		/**
		 * The machine options a fresh configuration starts from, as
		 * amiberry.conf's defaults set them (the schema's sets_option
		 * mapping): the file's value, else the Android default. A missing
		 * or unreadable file gives the Android defaults; a missing schema,
		 * an empty map.
		 */
		fun machineOptionDefaults(globalSettingsFile: File?, schema: SettingsSchema?): Map<String, String> {
			if (schema == null || globalSettingsFile == null) return emptyMap()
			val values = try {
				HostSettingsStore(globalSettingsFile, schema).read()
			} catch (_: Exception) {
				emptyMap()
			}
			return buildMap {
				for (setting in schema.hostSettings) {
					val option = setting.setsOption?.takeIf { it in schema.options } ?: continue
					val value = values[setting.id] ?: setting.defaultOnAndroid
					// A switch option the host spells as a bool ("true"/"false")
					// stores 1 and 0; everything else keeps its value.
					val machine = schema.options.getValue(option)
					val stored = when {
						machine.isSwitch && value == "true" -> "1"
						machine.isSwitch && value == "false" -> "0"
						else -> value
					}
					put(option, stored)
				}
			}
		}
	}
}
