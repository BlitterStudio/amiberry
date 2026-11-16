package com.blitterstudio.amiberry.data

import android.util.Log
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.AcceleratorJumpers
import com.blitterstudio.amiberry.data.model.CustomControls
import com.blitterstudio.amiberry.data.model.MountEntry
import com.blitterstudio.amiberry.data.model.MountKind
import com.blitterstudio.amiberry.data.model.RtgConfig
import com.blitterstudio.amiberry.data.model.SettingsBoards
import com.blitterstudio.amiberry.data.model.SettingsSchema
import java.io.File
import java.io.IOException

/**
 * Parses .uae config files back into EmulatorSettings.
 * Schema options without a field go to EmulatorSettings.options; other unknown
 * keys are preserved verbatim so configs created by ImGui don't lose settings
 * on round-trip.
 */
object ConfigParser {

	data class ParsedConfig(
		val settings: EmulatorSettings,
		val unknownLines: List<String>,
		val description: String,
		/** The group the file declares itself part of (config_category), for the Library's filters. */
		val category: String = "",
		val explicitKeys: Set<String> = emptySet()
	)

	// Keys we parse into EmulatorSettings fields: the options with a field,
	// older spellings of some, and the launcher's own records.
	private val knownKeys = EmulatorSettings.FIELD_IDS + setOf(
		"floppy_volume",
		"gfx_width", "gfx_height", "gfx_correct_aspect", "gfx_auto_crop", "scaling_method",
		"input.default_osk",
		"amiberry.android_joyport1", "amiberry.android_preset",
		"config_description", "config_category", "config_hardware_path"
	)

	/** A schema option without a field: kept in EmulatorSettings.options. */
	private fun isMapOption(key: String): Boolean =
		key !in knownKeys && SettingsSchema.current?.option(key) != null

	private const val TAG = "Amiberry-ConfigParser"

	private val LEGACY_MIXED_STEREO = listOf("sound_stereo_separation" to "7", "sound_stereo_mixing_delay" to "5")

	private fun configKey(line: String): String? {
		val trimmed = line.trim()
		if (trimmed.startsWith(";")) return null
		val eqIndex = trimmed.indexOf('=')
		return if (eqIndex < 0) null else trimmed.substring(0, eqIndex).trim().lowercase()
	}

	fun parse(file: File): ParsedConfig {
		if (!file.exists()) return ParsedConfig(EmulatorSettings(), emptyList(), "")

		val lines = try {
			file.readLines()
		} catch (e: IOException) {
			Log.e(TAG, "Failed to read config file: ${file.path}", e)
			return ParsedConfig(EmulatorSettings(), emptyList(), "")
		}
		val kvPairs = mutableMapOf<String, String>()
		val options = LinkedHashMap<String, String>()
		val unknownLines = mutableListOf<String>()
		val mounts = mutableListOf<MountEntry>()
		var gotFs2Hdf2 = false
		val deferredTwins = mutableListOf<Pair<String, MountEntry>>()
		val filesystemExtras = mutableListOf<Pair<String, String>>()
		// Expansion board lines (SettingsBoards) and the custom control
		// lines, collected by their line keys; the accelerator's packed
		// jumper line waits for the board it belongs to.
		val boardLines = LinkedHashMap<String, String>()
		val identityLines = mutableListOf<String>()
		val customLines = LinkedHashMap<String, String>()
		var packedJumpers: String? = null
		var acceleratorRomIdentity: String? = null

		for (line in lines) {
			val trimmed = line.trim()
			if (trimmed.isEmpty() || trimmed.startsWith(";")) continue

			val eqIndex = trimmed.indexOf('=')
			if (eqIndex < 0) {
				unknownLines.add(line)
				continue
			}

			val key = trimmed.substring(0, eqIndex).trim().lowercase()
			val value = trimmed.substring(eqIndex + 1).trim()
			val boardLine = SettingsBoards.line(key)

			when {
				key == "filesystem2" -> {
					val mount = parseDirectory(value)
					if (mount == null) unknownLines.add(line) else {
						mounts += mount
						gotFs2Hdf2 = true
					}
				}
				key == "hardfile2" -> {
					val mount = parseHardfile(value, MountKind.Hardfile)
					if (mount == null) unknownLines.add(line) else {
						mounts += mount
						gotFs2Hdf2 = true
					}
				}
				key == "filesystem_extra" -> parseFilesystemExtra(value)?.let(filesystemExtras::add) ?: unknownLines.add(line)
				isUaehf(key) -> {
					val mount = parseUaehf(value)
					if (mount == null) {
						unknownLines.add(line)
					} else if (mount.kind == MountKind.Directory || mount.kind == MountKind.Hardfile) {
						deferredTwins += line to mount
					} else {
						mounts += mount
					}
				}
				// The expansion boards and custom controls keep their own
				// lines (SettingsBoards, CustomControls): consumed here, and
				// written back by ConfigGenerator. What the schema does not
				// know stays verbatim below.
				boardLine != null && boardLine.kind == SettingsBoards.Line.Kind.Entry ->
					boardLines[key] = value
				boardLine != null && boardLine.kind == SettingsBoards.Line.Kind.Options ->
					boardLines[key] = value
				boardLine != null && (boardLine.kind == SettingsBoards.Line.Kind.Memory ||
					boardLine.kind == SettingsBoards.Line.Kind.Flag) ->
					boardLines[key] = value
				// A unit's ROM identity lines are dropped when the codec
				// owns the unit (its entry or options line is there) and
				// kept verbatim when it does not, so a board a file
				// identifies only by its ROM checksum survives a save
				// (cfgfile_rom resolves the checksum into the entry,
				// cfgfile.cpp:6393-6399).
				boardLine != null && boardLine.kind == SettingsBoards.Line.Kind.Identity ->
					identityLines.add(line)
				CustomControls.lineKey(key) != null -> if (value.isNotEmpty()) customLines[key] = value
				key == "cpuboard_settings" -> packedJumpers = value
				// The accelerator ROM's checksum: see below.
				key == "cpuboard_rom_file_id" -> acceleratorRomIdentity = line
				key in knownKeys -> kvPairs[key] = value
				isMapOption(key) -> options[key] = value
				else -> unknownLines.add(line)
			}
		}

		// filesystem2/hardfile2 make their uaehfN twins redundant to the loader.
		// An unsupported primary line and its twin stay together as unknown data.
		for ((_, mount) in deferredTwins) {
			if (!gotFs2Hdf2) mounts += mount
		}
		for ((device, extra) in filesystemExtras) {
			val index = mounts.indexOfFirst { it.kind == MountKind.Directory && it.device.equals(device, ignoreCase = true) }
			if (index < 0) {
				unknownLines.add("filesystem_extra=$device,$extra")
			} else if (extra.equals("inject_icons=true", ignoreCase = true)) {
				mounts[index] = mounts[index].copy(injectIcons = true)
			} else {
				unknownLines.add("filesystem_extra=$device,$extra")
			}
		}

		// sound_channels=mixed is a legacy spelling the emulator loads as
		// stereo with stereo separation 7 and a mixing delay of 5 (cfgfile.cpp);
		// keep that sound on the next save. A later line for either still wins.
		if (kvPairs["sound_channels"] == "mixed") {
			kvPairs["sound_channels"] = "stereo"
			val mixedLine = lines.indexOfLast { configKey(it) == "sound_channels" }
			for ((key, value) in LEGACY_MIXED_STEREO) {
				if (lines.drop(mixedLine + 1).any { configKey(it) == key }) continue
				unknownLines.removeAll { configKey(it) == key }
				if (isMapOption(key)) options[key] = value else unknownLines.add("$key=$value")
			}
		}

		// rtg_modes and gfxcard_options are the .uae keys. Their editor
		// controls are views of those packed values, never .uae keys.
		val settings = buildSettings(kvPairs, mounts)
			.copy(options = RtgConfig.unpack(legacyPresetOptions(kvPairs["amiberry.android_preset"]) + options))
		val schema = SettingsSchema.current
		val boards = schema?.let { SettingsBoards.parse(it, boardLines) }.orEmpty()
		// The units the codec parsed own their identity lines.
		val ownedTypes = boards.map { it.typeId }.toSet()
		for (identity in identityLines) {
			if (SettingsBoards.line(configKey(identity) ?: "")?.typeId !in ownedTypes)
				unknownLines.add(identity)
		}
		// The accelerator ROM's checksum stays only for a file that names
		// its ROM by nothing else: with the ROM file named, the file line
		// decides (the writer spells both alike), and a picked or cleared
		// ROM must not come back from the old checksum, which
		// cfgfile_read_board_rom resolves into the entry after the path.
		acceleratorRomIdentity?.let { identity ->
			if (settings.optionValue("cpuboard_rom_file").isNullOrEmpty()) unknownLines.add(identity)
		}
		val customControls = CustomControls(
			customLines.entries.associate { (line, event) ->
				CustomControls.lineKey(line)!! to event
			}
		)
		// The packed jumper line unpacks against the board the file names
		// (a preset record's line where the file has none); one the schema
		// does not know keeps its line verbatim.
		val packed = packedJumpers ?: settings.options["cpuboard_settings"]
		val board = schema?.acceleratorBoard(settings.optionValue("cpuboard_type") ?: "none")
		var jumpers: Map<String, String> = emptyMap()
		if (board != null && packed != null)
			jumpers = AcceleratorJumpers.unpack(board, packed)
		else if (packed != null)
			unknownLines.add("cpuboard_settings=$packed")
		val description = kvPairs["config_description"] ?: ""
		val category = kvPairs["config_category"] ?: ""

		return ParsedConfig(
			settings.copy(
				options = settings.options - "cpuboard_settings",
				boards = boards,
				customControls = customControls,
				acceleratorJumpers = jumpers
			),
			unknownLines,
			description,
			category,
			kvPairs.keys + options.keys
		)
	}

	/**
	 * The machine options of a preset record written before levels existed
	 * ("<preset id>:<configuration>", no level): those launches started the
	 * emulator from the preset (--apply-preset) and the file holds only the
	 * fields, so the rest of the preset's machine is level 0's values without
	 * a field. Empty for a record with a level, which wrote its whole machine.
	 */
	private fun legacyPresetOptions(record: String?): Map<String, String> {
		val parts = record?.split(':') ?: return emptyMap()
		if (parts.size != 2) return emptyMap()
		val model = AmigaModel.find(parts[0]) ?: return emptyMap()
		val config = parts[1].toIntOrNull()?.takeIf { it in model.configs.indices } ?: return emptyMap()
		return model.levelValues(config, 0).filterKeys { it !in EmulatorSettings.FIELD_IDS }
	}

	private fun buildSettings(kv: Map<String, String>, hardDrives: List<MountEntry>): EmulatorSettings {
		// vkbd_enabled is the native master switch; the KB touch button and the
		// gamepad toggle are only reachable while it is on (see ConfigGenerator).
		val keyboardAvailable = kv["amiberry.vkbd_enabled"]?.toBool(true) ?: kv["input.default_osk"].toBool(true)
		// amiberry.android_preset is "<preset id>:<configuration>:<level>";
		// configurations written before levels existed have no level and were
		// written from level 0's values. Configurations written before the
		// record existed are recognised by their hardware.
		val preset = kv["amiberry.android_preset"]?.split(':')
		val presetModel = AmigaModel.find(preset?.getOrNull(0))
		val presetConfig = preset?.getOrNull(1)?.toIntOrNull()
			?.takeIf { presetModel != null && it in presetModel.configs.indices } ?: 0
		val model = presetModel ?: guessModel(kv)
		val presetLevel = when {
			presetModel == null -> model.defaultLevel
			preset == null || preset.size < 3 -> 0
			else -> preset[2].toIntOrNull()?.takeIf { it in 0..model.compatibilityMax } ?: model.defaultLevel
		}
		val defaults = EmulatorSettings()
		// floppy_volume (older configurations) sets every drive's click
		// volumes; the per-drive keys the emulator writes after it win.
		val allDrivesVolume = kv["floppy_volume"]?.toIntOrNull()
		fun floppyVolume(key: String, fallback: Int) = kv[key]?.toIntOrNull() ?: allDrivesVolume ?: fallback
		fun int(key: String, fallback: Int) = kv[key]?.toIntOrNull() ?: fallback
		return EmulatorSettings(
			baseModel = model,
			baseConfig = presetConfig,
			baseLevel = presetLevel,
			cpuModel = kv["cpu_model"]?.toIntOrNull() ?: 68000,
			cpuCompatible = kv["cpu_compatible"].toBool(true),
			address24Bit = kv["cpu_24bit_addressing"].toBool(true),
			cpuSpeed = kv["cpu_speed"] ?: "real",
			fpuModel = kv["fpu_model"]?.toIntOrNull() ?: 0,
			jitCacheSize = kv["cachesize"]?.toIntOrNull() ?: 0,
			jitFpu = kv["compfpu"].toBool(false),

			chipset = kv["chipset"] ?: "ocs",
			immediateBlits = kv["immediate_blits"].toBool(false),
			collisionLevel = kv["collision_level"] ?: "full",
			cycleExact = parseCycleExact(kv["cycle_exact"]),
			ntsc = kv["ntsc"].toBool(false),

			chipRam = kv["chipmem_size"]?.toIntOrNull() ?: 1,
			slowRam = kv["bogomem_size"]?.toIntOrNull() ?: 2,
			fastRam = kv["fastmem_size"]?.toIntOrNull() ?: 0,
			z3Ram = kv["z3mem_size"]?.toIntOrNull() ?: 0,

			romFile = kv["kickstart_rom_file"] ?: "",
			romExtFile = kv["kickstart_ext_rom_file"] ?: "",

			floppy0 = kv["floppy0"] ?: "",
			floppy0Type = kv["floppy0type"]?.toIntOrNull() ?: 0,
			floppy1 = kv["floppy1"] ?: "",
			floppy1Type = kv["floppy1type"]?.toIntOrNull() ?: -1,
			floppy2 = kv["floppy2"] ?: "",
			floppy2Type = kv["floppy2type"]?.toIntOrNull() ?: -1,
			floppy3 = kv["floppy3"] ?: "",
			floppy3Type = kv["floppy3type"]?.toIntOrNull() ?: -1,

			cdImage = kv["cdimage0"] ?: "",
			hardDrives = hardDrives,

			soundOutput = kv["sound_output"] ?: "exact",
			soundFreq = kv["sound_frequency"]?.toIntOrNull()
				?: EmulatorSettingsConstraints.DEFAULT_SOUND_FREQUENCY,
			soundBuffer = kv["sound_max_buff"]?.toIntOrNull()
				?: EmulatorSettingsConstraints.DEFAULT_SOUND_BUFFER,
			soundChannels = kv["sound_channels"] ?: "stereo",
			soundVolume = int("sound_volume", defaults.soundVolume),
			soundVolumePaula = int("sound_volume_paula", defaults.soundVolumePaula),
			soundVolumeCd = int("sound_volume_cd", defaults.soundVolumeCd),
			soundVolumeAhi = int("sound_volume_ahi", defaults.soundVolumeAhi),
			soundVolumeMidi = int("sound_volume_midi", defaults.soundVolumeMidi),
			floppy0Sound = int("floppy0sound", defaults.floppy0Sound),
			floppy1Sound = int("floppy1sound", defaults.floppy1Sound),
			floppy2Sound = int("floppy2sound", defaults.floppy2Sound),
			floppy3Sound = int("floppy3sound", defaults.floppy3Sound),
			floppy0SoundVolumeDisk = floppyVolume("floppy0soundvolume_disk", defaults.floppy0SoundVolumeDisk),
			floppy1SoundVolumeDisk = floppyVolume("floppy1soundvolume_disk", defaults.floppy1SoundVolumeDisk),
			floppy2SoundVolumeDisk = floppyVolume("floppy2soundvolume_disk", defaults.floppy2SoundVolumeDisk),
			floppy3SoundVolumeDisk = floppyVolume("floppy3soundvolume_disk", defaults.floppy3SoundVolumeDisk),
			floppy0SoundVolumeEmpty = floppyVolume("floppy0soundvolume_empty", defaults.floppy0SoundVolumeEmpty),
			floppy1SoundVolumeEmpty = floppyVolume("floppy1soundvolume_empty", defaults.floppy1SoundVolumeEmpty),
			floppy2SoundVolumeEmpty = floppyVolume("floppy2soundvolume_empty", defaults.floppy2SoundVolumeEmpty),
			floppy3SoundVolumeEmpty = floppyVolume("floppy3soundvolume_empty", defaults.floppy3SoundVolumeEmpty),

			gfxWidth = kv["gfx_width"]?.toIntOrNull() ?: 720,
			gfxHeight = kv["gfx_height"]?.toIntOrNull() ?: 568,
			correctAspect = (kv["amiberry.gfx_correct_aspect"] ?: kv["gfx_correct_aspect"]).toBool(true),
			autoCrop = (kv["amiberry.gfx_auto_crop"] ?: kv["gfx_auto_crop"]).toBool(false),
			scalingMethod = (kv["amiberry.scaling_method"] ?: kv["scaling_method"])?.toIntOrNull() ?: -1,
			gfxAutoresolution = kv["gfx_autoresolution"]?.toIntOrNull() ?: 0,
			shader = kv["amiberry.shader"]?.ifEmpty { "none" } ?: "none",

			joyport0 = kv["joyport0"] ?: "mouse",
			// Round-trip: prefer the explicit Android joyport1 key if present,
			// otherwise infer from on-screen joystick state
			joyport1 = kv["amiberry.android_joyport1"] ?: run {
				val onScreenJoy = kv["amiberry.onscreen_joystick"].toBool(true)
				if (onScreenJoy && (kv["joyport1"] == null || kv["joyport1"] == "joy0")) {
					"onscreen_joy"
				} else {
					kv["joyport1"] ?: "joy0"
				}
			},
			onScreenJoystick = kv["amiberry.onscreen_joystick"].toBool(true),
			onScreenKeyboardButton = keyboardAvailable && kv["amiberry.vkbd_touch_button"].toBool(true),
			onScreenKeyboardNumpad = kv["amiberry.vkbd_numpad"].toBool(false),
			// "default" is the explicit reset sentinel (see ConfigGenerator): parse it
			// back to the emulator default (null) while the key stays explicit, so the
			// remembered-control fallback cannot override a saved Default choice.
			// "" (explicitly disabled) and button names round-trip unchanged. With the
			// keyboard switched off the toggle is inert, so it reads as disabled.
			onScreenKeyboardToggle = if (keyboardAvailable) {
				kv["amiberry.vkbd_toggle"]?.takeIf { it != "default" }
			} else {
				""
			},
			joyport0MouseMap = (kv["joyport0mousemap"]?.toIntOrNull() ?: 0) > 0,
			joyport1MouseMap = (kv["joyport1mousemap"]?.toIntOrNull() ?: 0) > 0,
			analogMouseSpeed = kv["input.joymouse_speed_analog"]?.toIntOrNull()
				?: EmulatorSettingsConstraints.DEFAULT_ANALOG_MOUSE_SPEED
		)
	}

	/**
	 * Guess the AmigaModel from chipset + CPU combination.
	 */
	private fun guessModel(kv: Map<String, String>): AmigaModel {
		val cpu = kv["cpu_model"]?.toIntOrNull() ?: 68000
		val chipset = kv["chipset"] ?: "ocs"
		val hasCd = kv["cdimage0"]?.isNotEmpty() == true
		val chipRam = kv["chipmem_size"]?.toIntOrNull() ?: 1
		val slowRam = kv["bogomem_size"]?.toIntOrNull() ?: 0
		val hwPath = kv["config_hardware_path"] ?: ""

		return when {
			chipset == "aga" && cpu >= 68040 -> AmigaModel.A4000
			chipset == "aga" && hasCd -> AmigaModel.CD32
			chipset == "aga" -> AmigaModel.A1200
			chipset == "ecs" && cpu >= 68030 -> AmigaModel.A3000
			chipset == "ecs" && hwPath.contains("A600", ignoreCase = true) -> AmigaModel.A600
			chipset == "ecs" && hwPath.contains("A500", ignoreCase = true) -> AmigaModel.A500_PLUS
			// Without config_hardware_path, A500+ has 1MB chip + no slow RAM,
			// while A600 has 2MB chip + no slow RAM. But both have chipRam=2 in defaults.
			// Default to A500+ (more common) when we can't distinguish.
			chipset == "ecs" -> AmigaModel.A500_PLUS
			hasCd -> AmigaModel.CDTV
			// OCS with 512KB chip + 512KB slow = A500 (or A2000), default A500
			else -> AmigaModel.A500
		}
	}

	private val uaehfKeyRegex = Regex("""uaehf(\d{1,2})""")

	/** uaehf0..uaehf49: cfgfile reads one line per possible mount (MAX_FILESYSTEM_UNITS). */
	private fun isUaehf(key: String): Boolean =
		uaehfKeyRegex.matchEntire(key)?.groupValues?.get(1)?.toInt()?.let { it < MountEntry.MAX_MOUNTS } == true

	private fun parseFilesystemExtra(value: String): Pair<String, String>? {
		val parts = splitMountFields(value)
		return if (parts.size == 2 && parts[0].isNotBlank()) parts[0] to parts[1] else null
	}

	private fun parseUaehf(value: String): MountEntry? {
		val split = splitMountFields(value)
		val kindToken = split.firstOrNull()?.lowercase() ?: return null
		val body = value.substringAfter(',', missingDelimiterValue = "")
		if (body.isEmpty()) return null
		return when {
			kindToken == "dir" -> parseDirectory(body)
			kindToken == "hdf" -> parseHardfile(body, MountKind.Hardfile)
			kindToken.matches(Regex("""cd\d?""")) ->
				parseHardfile(body, MountKind.Cd, kindToken.removePrefix("cd").toIntOrNull() ?: 0)
			kindToken.matches(Regex("""tape\d?""")) ->
				parseHardfile(body, MountKind.Tape, kindToken.removePrefix("tape").toIntOrNull() ?: 0)
			else -> null
		}
	}

	private fun parseDirectory(value: String): MountEntry? {
		val parts = splitMountFields(value)
		if (parts.size != 3) return null
		val access = readOnly(parts[0]) ?: return null
		val prefix = parts[1]
		val first = prefix.indexOf(':')
		val second = prefix.indexOf(':', first + 1)
		if (first < 0 || second < 0) return null
		val device = prefix.substring(0, first)
		val volume = prefix.substring(first + 1, second)
		val path = decodeFilename(prefix.substring(second + 1))
		val boot = parts[2].toIntOrNull()?.takeIf { it in MountEntry.BOOTPRI_NOAUTOMOUNT..127 } ?: return null
		if (path.isEmpty()) return null
		return MountEntry(
			kind = MountKind.Directory, path = path, device = device, volume = volume,
			readOnly = access, bootPriority = boot, controller = "uae0"
		)
	}

	private fun parseHardfile(value: String, kind: MountKind, emuUnit: Int = 0): MountEntry? {
		val parts = splitMountFields(value)
		if (parts.size < 9) return null
		val access = readOnly(parts[0]) ?: return null
		val pathField = parts[1]
		val separator = pathField.indexOf(':')
		if (separator < 0) return null
		val device = pathField.substring(0, separator)
		val rawPath = decodeFilename(pathField.substring(separator + 1))
		// A second colon starts cfgfile's physical-drive `:name` form.
		if (kind == MountKind.Hardfile && rawPath.startsWith(':')) return null
		val numbers = parts.subList(2, 7).map { it.toIntOrNull() ?: return null }
		if (numbers[4] !in MountEntry.BOOTPRI_NOAUTOMOUNT..127) return null
		val controller = parts[8].takeIf { it.isNotBlank() } ?: "uae0"
		val tail = parts.drop(9)
		var highCyl = 0
		var physical = ""
		var geometry = ""
		var compactFlash = false
		var feature = ""
		var flags = ""
		var lock = false
		var identity = false
		val extras = mutableListOf<String>()
		for (token in tail) when {
			token.toIntOrNull() != null && highCyl == 0 -> highCyl = token.toInt()
			token.matches(Regex("""\d+/\d+/\d+""")) && physical.isEmpty() -> physical = token
			token.equals("CF", true) -> compactFlash = true
			token in setOf("SCSI1", "SASI", "SASIE", "SASI_CHS", "ATA1", "ATA2+S") -> feature = token
			token.startsWith("flags=", true) -> flags = token.substringAfter('=')
			token.equals("lock", true) -> lock = true
			token.equals("identity", true) -> identity = true
			// The loader takes the geometry file only in its quoted spelling.
			geometry.isEmpty() && token.startsWith('"') -> geometry = decodeGeometry(token)
			else -> extras += token
		}
		return MountEntry(
			kind = kind, path = rawPath, device = device, readOnly = access,
			bootPriority = numbers[4], sectors = numbers[0], surfaces = numbers[1],
			reserved = numbers[2], blockSize = numbers[3], filesystem = decodeFilename(parts[7]),
			controller = controller, unit = emuUnit, highCyl = highCyl, physicalGeometry = physical,
			geometry = geometry, compactFlash = compactFlash, featureLevel = feature, flags = flags,
			lock = lock, identity = identity, extras = extras
		)
	}

	private fun readOnly(value: String): Boolean? = when (value.lowercase()) {
		"ro" -> true
		"rw" -> false
		else -> null
	}

	/**
	 * The comma-separated fields of a mount value, each as written: a quoted
	 * field keeps its quotes and escapes for its own decoder. Inside quotes a
	 * backslash pairs with the next character, so `\"` does not end the field.
	 */
	private fun splitMountFields(value: String): List<String> {
		val fields = mutableListOf<String>()
		val field = StringBuilder()
		var quoted = false
		var i = 0
		while (i < value.length) {
			val char = value[i]
			when {
				quoted && char == '\\' && i + 1 < value.length && (value[i + 1] == '"' || value[i + 1] == '\\') -> {
					field.append(char).append(value[i + 1])
					i++
				}
				char == '"' -> {
					quoted = !quoted
					field.append(char)
				}
				char == ',' && !quoted -> {
					fields += field.toString()
					field.clear()
				}
				else -> field.append(char)
			}
			i++
		}
		if (quoted) return emptyList()
		fields += field.toString()
		return fields
	}

	/**
	 * A file name field as cfgfile_escape_min wrote it and
	 * cfgfile_unescape_quoted_filename reads it: quoted only when needed,
	 * backslashes literal (Windows paths), `\"` a quote in the name.
	 */
	internal fun decodeFilename(value: String): String {
		if (!value.startsWith('"') || value.length < 2 || !value.endsWith('"')) return value
		return value.substring(1, value.length - 1).replace("\\\"", "\"")
	}

	/**
	 * The geometry file field, which cfgfile_escape(…, nullptr, true, false)
	 * always quotes with every backslash escaped; cfgfile_unescape reads it
	 * (\r and \n for those characters, any other escaped character as it is).
	 */
	internal fun decodeGeometry(value: String): String {
		val inner = value.removePrefix("\"").removeSuffix("\"")
		val out = StringBuilder()
		var i = 0
		while (i < inner.length) {
			val char = inner[i]
			if (char == '\\' && i + 1 < inner.length) {
				out.append(when (val next = inner[i + 1]) { 'r' -> '\r'; 'n' -> '\n'; else -> next })
				i += 2
			} else {
				out.append(char)
				i++
			}
		}
		return out.toString()
	}

	/**
	 * cycle_exact as the emulator reads it (cfgfile_strval in cfgfile.cpp):
	 * "false", "memory" (DMA/memory) or "true" (full), and "yes"/"no" as the
	 * table's entries 1 and 0, so "yes" is the DMA/memory mode.
	 */
	private fun parseCycleExact(value: String?): String = when (value?.lowercase()) {
		"true" -> "true"
		"memory", "yes" -> "memory"
		else -> "false"
	}

	private fun String?.toBool(default: Boolean): Boolean {
		if (this == null) return default
		return this == "true" || this == "1" || this == "yes"
	}
}
