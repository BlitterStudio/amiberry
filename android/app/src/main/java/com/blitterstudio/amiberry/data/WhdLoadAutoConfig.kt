package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.StoragePaths
import org.json.JSONObject
import java.io.File
import java.security.MessageDigest

object WhdLoadAutoConfig {

	fun settingsFor(
		context: Context,
		whdLoadFile: AmigaFile,
		availableRoms: List<AmigaFile>,
		currentSettings: EmulatorSettings
	): EmulatorSettings? {
		val lhaFile = File(whdLoadFile.path)
		val externalDir = context.getExternalFilesDir(null) ?: return null
		val configFile = File(File(externalDir, StoragePaths.CONFIGURATIONS), "${lhaFile.nameWithoutExtension}.uae")
		if (configFile.exists()) {
			val parsed = ConfigParser.parse(configFile)
			return AndroidControlSettings.withFallback(
				settings = parsed.settings,
				explicitKeys = parsed.explicitKeys,
				fallback = currentSettings
			)
		}

		val databaseFile = File(externalDir, "${StoragePaths.WHDBOOT}/game-data/whdload_db.json")
		if (!databaseFile.exists()) return null
		return try {
			settingsFromDatabaseJson(
				json = databaseFile.readText(),
				lhaFile = lhaFile,
				availableRoms = availableRoms,
				currentSettings = currentSettings
			)
		} catch (_: Exception) {
			null
		}
	}

	fun settingsFromDatabaseJson(
		json: String,
		lhaFile: File,
		availableRoms: List<AmigaFile>,
		currentSettings: EmulatorSettings,
		hasTouchScreen: Boolean = true
	): EmulatorSettings? {
		val doc = try {
			JSONObject(json)
		} catch (_: Exception) {
			return null
		}
		val game = findGame(doc, lhaFile) ?: return null
		val hardware = game.optJSONObject("hardware") ?: JSONObject()
		val gameName = lhaFile.nameWithoutExtension
		val chipset = hardware.stringOrNull("chipset")
		val isAga = gameName.contains("AGA", ignoreCase = true) || chipset.equalsIgnoringCase("aga")
		val isCd32 = gameName.contains("CD32", ignoreCase = true) || chipset.equalsIgnoringCase("cd32")
		val a600Available = ModelRomAvailability.isModelAvailable(AmigaModel.A600, availableRoms)
		val baseModel = if (isAga || isCd32 || !a600Available) AmigaModel.A1200 else AmigaModel.A600
		val selectedRoms = ModelRomAvailability.selectRomsForModel(baseModel, availableRoms)

		var settings = EmulatorSettings.fromModel(baseModel).copy(
			fastRam = 8,
			romFile = selectedRoms.kick?.path.orEmpty(),
			romExtFile = selectedRoms.ext?.path.orEmpty(),
			floppy0 = "",
			floppy0Type = -1,
			floppy1 = "",
			floppy1Type = -1,
			joyport0 = currentSettings.joyport0,
			joyport1 = currentSettings.joyport1,
			onScreenJoystick = currentSettings.onScreenJoystick,
			onScreenKeyboard = currentSettings.onScreenKeyboard,
			onScreenKeyboardNumpad = currentSettings.onScreenKeyboardNumpad
		)

		settings = applyHardware(settings, hardware, a600Available, useAgaCpuProfile = isAga || isCd32)

		return EmulatorSettingsConstraints.apply(settings, hasTouchScreen = hasTouchScreen)
	}

	private fun findGame(doc: JSONObject, lhaFile: File): JSONObject? {
		val games = doc.optJSONArray("games") ?: return null
		val gameName = lhaFile.nameWithoutExtension
		val sha1 = sha1Of(lhaFile)
		for (i in 0 until games.length()) {
			val game = games.optJSONObject(i) ?: continue
			val entryFilename = game.optString("filename", "")
			val entrySha1 = game.optString("sha1", "").lowercase()
			if (entryFilename == gameName || (sha1.isNotEmpty() && entrySha1 == sha1)) {
				return game
			}
		}
		return null
	}

	private fun applyHardware(
		initial: EmulatorSettings,
		hardware: JSONObject,
		a600Available: Boolean,
		useAgaCpuProfile: Boolean
	): EmulatorSettings {
		var settings = initial
		val cpu = hardware.stringOrNull("cpu")?.lowercase()
		settings = when {
			cpu == "68020" || cpu == "68040" || useAgaCpuProfile ->
				settings.copy(cpuModel = if (useAgaCpuProfile) 68020 else cpu!!.toInt())
			(cpu == "68000" || cpu == "68010") && a600Available ->
				settings.copy(cpuModel = cpu.toInt(), chipRam = 4)
			a600Available ->
				settings.copy(cpuModel = 68000)
			else ->
				settings.copy(cpuModel = 68020)
		}

		when (hardware.stringOrNull("clock")?.lowercase()) {
			"max" -> settings = settings.copy(cpuSpeed = "max")
		}

		hardware.boolOrNull("cpu_compatible")?.let { settings = settings.copy(cpuCompatible = it) }
		hardware.boolOrNull("cpu_exact")?.let { settings = settings.copy(cycleExact = it) }

		val z3Ram = hardware.intOrNull("z3_ram")
		if (hardware.boolOrNull("cpu_24bitaddressing") == false || z3Ram != null) {
			settings = settings.copy(address24Bit = false)
		}
		hardware.intOrNull("fast_ram")?.let { settings = settings.copy(fastRam = it) }
		z3Ram?.let { settings = settings.copy(z3Ram = it) }

		when (hardware.stringOrNull("blitter")?.lowercase()) {
			"immediate" -> settings = settings.copy(immediateBlits = true)
		}

		hardware.boolOrNull("jit")?.let { jit ->
			if (jit) {
				settings = settings.copy(
					jitCacheSize = 16384,
					jitFpu = true,
					cpuCompatible = false,
					cycleExact = false,
					address24Bit = false,
					cpuSpeed = "max"
				)
			}
		}

		hardware.boolOrNull("ntsc")?.let { settings = settings.copy(ntsc = it) }
		hardware.stringOrNull("sprites")?.takeIf { !it.equals("nul", ignoreCase = true) }?.let {
			settings = settings.copy(collisionLevel = it)
		}
		hardware.boolOrNull("screen_autoheight")?.let {
			settings = settings.copy(autoCrop = it)
		}

		return settings
	}

	private fun JSONObject.stringOrNull(key: String): String? {
		if (!has(key) || isNull(key)) return null
		return when (val value = get(key)) {
			is Boolean -> value.toString()
			is Number -> value.toString()
			else -> value.toString()
		}
	}

	private fun JSONObject.boolOrNull(key: String): Boolean? =
		stringOrNull(key)?.let { value ->
			when {
				value.equals("true", ignoreCase = true) -> true
				value == "1" || value.equals("yes", ignoreCase = true) -> true
				value.equals("false", ignoreCase = true) -> false
				value == "0" || value.equals("no", ignoreCase = true) -> false
				else -> null
			}
		}

	private fun JSONObject.intOrNull(key: String): Int? =
		stringOrNull(key)?.toIntOrNull()

	private fun String?.equalsIgnoringCase(other: String): Boolean =
		this?.equals(other, ignoreCase = true) == true

	private fun sha1Of(file: File): String =
		try {
			val digest = MessageDigest.getInstance("SHA-1")
			file.inputStream().use { input ->
				val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
				while (true) {
					val read = input.read(buffer)
					if (read <= 0) break
					digest.update(buffer, 0, read)
				}
			}
			digest.digest().joinToString("") { "%02x".format(it.toInt() and 0xff) }
		} catch (_: Exception) {
			""
		}
}
