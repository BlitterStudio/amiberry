package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.FileRepository
import java.io.File
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.StoragePaths
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.ui.hasTouchScreen
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class SettingsViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)

	var settings by mutableStateOf(EmulatorSettings())
		private set

	var currentUnknownLines by mutableStateOf<List<String>>(emptyList())
		private set

	val availableRoms: StateFlow<List<AmigaFile>> = repository.roms
	val availableFloppies: StateFlow<List<AmigaFile>> = repository.floppies
	val availableCds: StateFlow<List<AmigaFile>> = repository.cdImages
	private val romIdCacheByPath = mutableMapOf<String, Int?>()

	init {
		restoreLastSession()
		settings = applyConstraints(settings)
		viewModelScope.launch {
			repository.rescan()
			autoSelectDefaultRomIfNeeded(availableRoms.value)
		}
	}

	private fun restoreLastSession() {
		val confDir = File(getApplication<Application>().getExternalFilesDir(null), StoragePaths.CONFIGURATIONS)
		val sessionFile = File(confDir, LAST_SESSION_FILE)
		if (!sessionFile.exists()) return
		try {
			val parsed = ConfigParser.parse(sessionFile)
			settings = parsed.settings
			currentUnknownLines = parsed.unknownLines
		} catch (_: Exception) {
			// Corrupted session file — start with defaults
		}
	}

	private fun saveLastSession() {
		viewModelScope.launch(kotlinx.coroutines.Dispatchers.IO) {
			try {
				ConfigGenerator.writeConfig(getApplication(), settings, LAST_SESSION_FILE)
			} catch (_: Exception) {
				// Non-critical — settings persistence is best-effort
			}
		}
	}

	fun applyModel(model: AmigaModel) {
		val selectedRoms = selectRomsForModel(model, availableRoms.value)
		val newSettings = EmulatorSettings.fromModel(model).copy(
			romFile = selectedRoms.kick?.path.orEmpty(),
			romExtFile = selectedRoms.ext?.path.orEmpty()
		)
		settings = applyConstraints(newSettings)
		saveLastSession()
	}

	fun loadConfig(parsed: ConfigParser.ParsedConfig) {
		settings = applyConstraints(parsed.settings)
		currentUnknownLines = parsed.unknownLines
		autoSelectDefaultRomIfNeeded(availableRoms.value)
		saveLastSession()
	}

	private fun autoSelectDefaultRomIfNeeded(roms: List<AmigaFile>) {
		if (roms.isEmpty()) return
		if (settings.romFile.isNotBlank()) return

		val selectedRoms = selectRomsForModel(settings.baseModel, roms)
		val kickPath = selectedRoms.kick?.path ?: return
		settings = applyConstraints(
			settings.copy(
				romFile = kickPath,
				romExtFile = selectedRoms.ext?.path.orEmpty()
			)
		)
	}

	private fun selectRomsForModel(model: AmigaModel, roms: List<AmigaFile>): SelectedRoms {
		if (roms.isEmpty()) return SelectedRoms(null, null)
		val profile = MODEL_ROM_PROFILE[model] ?: return SelectedRoms(null, null)

		val romsById = roms
			.mapNotNull { rom -> detectRomId(rom)?.let { id -> id to rom } }
			.groupBy({ it.first }, { it.second })

		val selectedKickId = profile.kickIds.firstOrNull { id -> !romsById[id].isNullOrEmpty() }
		val kick = selectedKickId?.let { id -> pickDeterministic(romsById[id].orEmpty()) }

		if (kick == null) {
			return SelectedRoms(null, null)
		}

		val ext = when (model) {
			AmigaModel.CD32 -> {
				// id 64 is combined KS+ext ROM; id 18 requires ext id 19.
				if (selectedKickId == 64) {
					null
				} else {
					val extId = profile.extIds.firstOrNull { id -> !romsById[id].isNullOrEmpty() }
					if (extId == null) return SelectedRoms(null, null)
					pickDeterministic(romsById[extId].orEmpty())
				}
			}
			AmigaModel.CDTV -> {
				// CDTV defaults require both Kickstart and extended ROM.
				val extId = profile.extIds.firstOrNull { id -> !romsById[id].isNullOrEmpty() }
				if (extId == null) return SelectedRoms(null, null)
				pickDeterministic(romsById[extId].orEmpty())
			}
			else -> null
		}

		return SelectedRoms(kick, ext)
	}

	private fun detectRomId(rom: AmigaFile): Int? {
		return romIdCacheByPath.getOrPut(rom.path) {
			val crc = rom.crc32 ?: return@getOrPut null
			ROM_CRC_TO_ID[crc and 0xffffffffL]
		}
	}

	private fun pickDeterministic(candidates: List<AmigaFile>): AmigaFile? {
		return candidates.sortedBy { it.name.lowercase() }.firstOrNull()
	}

	private data class SelectedRoms(
		val kick: AmigaFile?,
		val ext: AmigaFile?
	)

	private data class ModelRomProfile(
		val kickIds: List<Int>,
		val extIds: List<Int> = emptyList()
	)

	private companion object {
		private const val LAST_SESSION_FILE = ".last_session.uae"

		/**
		 * Exact --model ROM priority behavior from main.cpp wrappers:
		 * A500->bip_a500(130), A500P->bip_a500plus(-1), A2000->bip_a2000(130), etc.
		 */
		private val MODEL_ROM_PROFILE = mapOf(
			AmigaModel.A500 to ModelRomProfile(kickIds = listOf(6, 5, 4)),
			AmigaModel.A500_PLUS to ModelRomProfile(kickIds = listOf(7, 6, 5)),
			AmigaModel.A600 to ModelRomProfile(kickIds = listOf(10, 9, 8)),
			AmigaModel.A1000 to ModelRomProfile(kickIds = listOf(24)),
			AmigaModel.A2000 to ModelRomProfile(kickIds = listOf(6, 5, 4)),
			AmigaModel.A3000 to ModelRomProfile(kickIds = listOf(59)),
			AmigaModel.A1200 to ModelRomProfile(kickIds = listOf(11, 15, 276, 281, 286, 291, 304)),
			AmigaModel.A4000 to ModelRomProfile(kickIds = listOf(16, 31, 13, 12, 46, 278, 283, 288, 293, 306)),
			AmigaModel.CD32 to ModelRomProfile(kickIds = listOf(64, 18), extIds = listOf(19)),
			AmigaModel.CDTV to ModelRomProfile(kickIds = listOf(6, 32), extIds = listOf(20, 21, 22))
		)

		// CRC32 -> ROM ID mapping for the ids used above.
		private val ROM_CRC_TO_ID: Map<Long, Int> = mapOf(
			0x9ed783d0L to 4,
			0xa6ce1636L to 5,
			0xc4f0f55fL to 6,
			0xc3bdb240L to 7,
			0x83028fb5L to 8,
			0x64466c2aL to 9,
			0x43b0df7bL to 10,
			0x6c9b07d2L to 11,
			0x9e6ac152L to 12,
			0x2b4566f1L to 13,
			0x1483a091L to 15,
			0xd6bae334L to 16,
			0x1e62d4a5L to 18,
			0x87746be2L to 19,
			0x42baa124L to 20,
			0x30b54232L to 21,
			0xceae68d2L to 22,
			0x0b1ad2d0L to 24,
			0x43b6dd22L to 31,
			0xe0f37258L to 32,
			0xbc0ec13fL to 59,
			0xf5d4f3c8L to 64,
			0xf17fa97fL to 276,
			0xd47e18fdL to 278,
			0xb87506a7L to 281,
			0x1b84cb33L to 283,
			0xbd1ff75eL to 286,
			0x9bb8fc93L to 288,
			0x2b653371L to 291,
			0xf3ced3b8L to 293,
			0x5c40328aL to 304,
			0x4bea9798L to 306,
		)
	}

	fun updateSettings(transform: (EmulatorSettings) -> EmulatorSettings) {
		val newSettings = transform(settings)
		settings = applyConstraints(newSettings)
		saveLastSession()
	}

	fun generateLaunchArgs(): Array<String> {
		val configFile = ConfigGenerator.writeConfig(getApplication(), settings, ".current_settings.uae")
		if (currentUnknownLines.isNotEmpty()) {
			configFile.appendText("\n; Preserved settings from original config\n")
			currentUnknownLines.forEach { configFile.appendText("$it\n") }
		}
		return arrayOf(
			"--rescan-roms",
			"--model", settings.baseModel.cmdArg,
			"--config", configFile.absolutePath,
			"-G"
		)
	}

	/**
	 * Enforce hardware constraints between interdependent settings.
	 */
	private fun applyConstraints(s: EmulatorSettings): EmulatorSettings {
		var result = s

		// 68000/68010 must use 24-bit addressing
		if (result.cpuModel <= 68010) {
			result = result.copy(address24Bit = true, z3Ram = 0, jitCacheSize = 0)
		}

		// Cycle-exact forces real speed and disables JIT
		if (result.cycleExact) {
			result = result.copy(cpuSpeed = "real", jitCacheSize = 0)
		}

		// JIT requires 68020+
		if (result.jitCacheSize > 0) {
			if (result.cpuModel < 68020) {
				result = result.copy(jitCacheSize = 0, jitFpu = false)
			} else {
				// JIT forces fastest possible speed and JIT FPU
				result = result.copy(cpuSpeed = "max", jitFpu = true)
			}
		} else {
			result = result.copy(jitFpu = false)
		}

		// Z3 RAM requires 32-bit addressing
		if (result.address24Bit && result.z3Ram > 0) {
			result = result.copy(z3Ram = 0)
		}

		// 24-bit addressing disables JIT
		if (result.address24Bit) {
			result = result.copy(jitCacheSize = 0)
		}

		// FPU: only internal for 68040/68060
		if (result.fpuModel == 68040 && result.cpuModel < 68040) {
			result = result.copy(fpuModel = 0)
		}

		// On-screen joystick requires a touchscreen
		if (result.joyport1 == "onscreen_joy" && !hasTouchScreen(getApplication())) {
			result = result.copy(joyport1 = "joy0", onScreenJoystick = false)
		}

		return result
	}
}
