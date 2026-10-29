package com.blitterstudio.amiberry.data

import android.util.Log
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.HardDrive
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
		val explicitKeys: Set<String> = emptySet()
	)

	// Keys we parse into EmulatorSettings fields: the options with a field,
	// older spellings of some, and the launcher's own records.
	private val knownKeys = EmulatorSettings.FIELD_IDS + setOf(
		"floppy_volume",
		"gfx_width", "gfx_height", "gfx_correct_aspect", "gfx_auto_crop", "scaling_method",
		"input.default_osk",
		"amiberry.android_joyport1", "amiberry.android_preset",
		"config_description", "config_hardware_path"
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
		val hardDrives = mutableListOf<HardDrive>()
		// UAE controller units of the simple hardfile2 lines we adopted into hardDrives.
		val adoptedControllers = mutableSetOf<String>()
		// uaehf=hdf lines, deferred until the whole file is scanned: a uaehf only duplicates
		// a hardfile2 mount when we actually adopted a hardfile2 on the same controller.
		val uaehfHdfLines = mutableListOf<Pair<String, String?>>()

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

			when {
				key == "hardfile2" -> {
					val drive = parseHardfile2(value)
					if (drive != null) {
						hardDrives.add(drive)
						hardfile2Controller(value)?.let { adoptedControllers.add(it) }
					} else {
						unknownLines.add(line)
					}
				}
				isUaehfHdf(key, value) -> uaehfHdfLines.add(line to uaehfController(value))
				key in knownKeys -> kvPairs[key] = value
				isMapOption(key) -> options[key] = value
				else -> unknownLines.add(line)
			}
		}

		// Drop a uaehf=hdf line only when it twins a hardfile2 we adopted (same controller);
		// otherwise preserve it verbatim so standalone/legacy mounts survive a save.
		for ((line, controller) in uaehfHdfLines) {
			if (controller == null || controller !in adoptedControllers) {
				unknownLines.add(line)
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

		val settings = buildSettings(kvPairs, hardDrives).copy(options = options)
		val description = kvPairs["config_description"] ?: ""

		return ParsedConfig(settings, unknownLines, description, kvPairs.keys + options.keys)
	}

	private fun buildSettings(kv: Map<String, String>, hardDrives: List<HardDrive>): EmulatorSettings {
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

	private val uaehfKeyRegex = Regex("""uaehf[0-7]""")
	private val uaeControllerRegex = Regex("""uae\d+""")

	/** A uaehf<n>=hdf,... line (the twin native also writes for an HDF mount). */
	private fun isUaehfHdf(key: String, value: String): Boolean =
		uaehfKeyRegex.matches(key) && value.trimStart().startsWith("hdf,")

	/** UAE controller unit (e.g. "uae0") of an adopted simple hardfile2 line. */
	private fun hardfile2Controller(value: String): String? =
		value.split(',').getOrNull(8)?.trim()?.takeIf { uaeControllerRegex.matches(it) }

	/** UAE controller unit of a uaehf=hdf line: one of its fields holds a `uae<n>` token. */
	private fun uaehfController(value: String): String? =
		value.split(',').map { it.trim() }.firstOrNull { uaeControllerRegex.matches(it) }

	/**
	 * Parse a `hardfile2` value ONLY when it is the exact simple UAE-controller RDB form
	 * this app generates: `<ro|rw>,:<path>,0,0,0,512,<bootpri>,,uae<n>`. Any other form
	 * (custom geometry/blocksize, an explicit filesystem, a device name, extra trailing
	 * fields/flags, or a non-UAE controller) returns null so the caller preserves the line
	 * verbatim — otherwise those mount parameters would be lost on a parse/save round trip.
	 */
	private fun parseHardfile2(value: String): HardDrive? {
		val parts = value.split(',')
		if (parts.size != 9) return null
		val access = parts[0].trim()
		if (!access.equals("ro", ignoreCase = true) && !access.equals("rw", ignoreCase = true)) return null
		if (!parts[1].startsWith(":")) return null                       // empty device name only
		if (parts[2].trim() != "0" || parts[3].trim() != "0" ||
			parts[4].trim() != "0" || parts[5].trim() != "512") return null  // RDB / auto geometry
		if (parts[7].trim().isNotEmpty()) return null                    // no explicit filesystem
		if (!uaeControllerRegex.matches(parts[8].trim())) return null
		val path = parts[1].substring(1).trim()
		if (path.isEmpty()) return null
		val bootPriority = parts[6].trim().toIntOrNull() ?: return null
		val readOnly = access.equals("ro", ignoreCase = true)
		return HardDrive(path = path, readOnly = readOnly, bootPriority = bootPriority)
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
