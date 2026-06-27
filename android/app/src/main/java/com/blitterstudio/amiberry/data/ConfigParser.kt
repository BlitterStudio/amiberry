package com.blitterstudio.amiberry.data

import android.util.Log
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.HardDrive
import java.io.File
import java.io.IOException

/**
 * Parses .uae config files back into EmulatorSettings.
 * Preserves unknown keys so configs created by ImGui don't lose settings on round-trip.
 */
object ConfigParser {

	data class ParsedConfig(
		val settings: EmulatorSettings,
		val unknownLines: List<String>,
		val description: String,
		val explicitKeys: Set<String> = emptySet()
	)

	// Keys we know how to parse into EmulatorSettings
	private val knownKeys = setOf(
		"cpu_model", "cpu_compatible", "cpu_24bit_addressing", "cpu_speed",
		"fpu_model", "cachesize", "compfpu",
		"chipset", "immediate_blits", "collision_level", "cycle_exact", "ntsc",
		"chipmem_size", "bogomem_size", "fastmem_size", "z3mem_size",
		"kickstart_rom_file", "kickstart_ext_rom_file",
		"floppy0", "floppy0type", "floppy1", "floppy1type",
		"floppy2", "floppy2type", "floppy3", "floppy3type",
		"cdimage0",
		"sound_output", "sound_frequency", "sound_channels",
		"gfx_width", "gfx_height", "gfx_correct_aspect", "gfx_auto_crop",
		"amiberry.gfx_correct_aspect", "amiberry.gfx_auto_crop",
		"scaling_method", "amiberry.scaling_method", "gfx_autoresolution",
		"joyport0", "joyport1",
		"amiberry.onscreen_joystick", "amiberry.vkbd_enabled", "input.default_osk",
		"amiberry.android_joyport1",
		"use_gui", "config_description", "config_hardware_path"
	)

	private const val TAG = "Amiberry-ConfigParser"

	fun parse(file: File): ParsedConfig {
		if (!file.exists()) return ParsedConfig(EmulatorSettings(), emptyList(), "")

		val lines = try {
			file.readLines()
		} catch (e: IOException) {
			Log.e(TAG, "Failed to read config file: ${file.path}", e)
			return ParsedConfig(EmulatorSettings(), emptyList(), "")
		}
		val kvPairs = mutableMapOf<String, String>()
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

		val settings = buildSettings(kvPairs, hardDrives)
		val description = kvPairs["config_description"] ?: ""

		return ParsedConfig(settings, unknownLines, description, kvPairs.keys.toSet())
	}

	private fun buildSettings(kv: Map<String, String>, hardDrives: List<HardDrive>): EmulatorSettings {
		return EmulatorSettings(
			baseModel = guessModel(kv),
			cpuModel = kv["cpu_model"]?.toIntOrNull() ?: 68000,
			cpuCompatible = kv["cpu_compatible"].toBool(true),
			address24Bit = kv["cpu_24bit_addressing"].toBool(true),
			cpuSpeed = kv["cpu_speed"] ?: "real",
			fpuModel = kv["fpu_model"]?.toIntOrNull() ?: 0,
			jitCacheSize = kv["cachesize"]?.toIntOrNull() ?: 0,
			jitFpu = kv["compfpu"].toBool(false),

			chipset = kv["chipset"] ?: "ocs",
			immediateBlits = kv["immediate_blits"].toBool(false),
			collisionLevel = kv["collision_level"] ?: "playfields",
			cycleExact = kv["cycle_exact"].toBool(false),
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
			soundFreq = kv["sound_frequency"]?.toIntOrNull() ?: 44100,
			soundChannels = kv["sound_channels"] ?: "stereo",

			gfxWidth = kv["gfx_width"]?.toIntOrNull() ?: 720,
			gfxHeight = kv["gfx_height"]?.toIntOrNull() ?: 568,
			correctAspect = (kv["amiberry.gfx_correct_aspect"] ?: kv["gfx_correct_aspect"]).toBool(true),
			autoCrop = (kv["amiberry.gfx_auto_crop"] ?: kv["gfx_auto_crop"]).toBool(false),
			scalingMethod = (kv["amiberry.scaling_method"] ?: kv["scaling_method"])?.toIntOrNull() ?: -1,
			gfxAutoresolution = kv["gfx_autoresolution"]?.toIntOrNull() ?: 0,

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
			onScreenKeyboard = kv["amiberry.vkbd_enabled"]?.toBool(true) ?: kv["input.default_osk"].toBool(true)
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

	private fun String?.toBool(default: Boolean): Boolean {
		if (this == null) return default
		return this == "true" || this == "1" || this == "yes"
	}
}
