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
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
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
	val availableHardDrives: StateFlow<List<AmigaFile>> = repository.hardDrives

	init {
		restoreLastSession()
		settings = applyConstraints(settings)
		viewModelScope.launch {
			repository.rescan()
			autoSelectDefaultRomIfNeeded(availableRoms.value)
		}
	}

	private fun restoreLastSession() {
		val context = getApplication<Application>()
		val sessionFile = ConfigGenerator.configFile(context, LAST_SESSION_FILE)
		val legacySessionFile = ConfigGenerator.legacyExternalConfigFile(context, LAST_SESSION_FILE)
		val readableSessionFile = when {
			sessionFile.exists() -> sessionFile
			legacySessionFile.exists() -> legacySessionFile
			else -> return
		}
		try {
			val parsed = ConfigParser.parse(readableSessionFile)
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
		val selectedRoms = ModelRomAvailability.selectRomsForModel(model, availableRoms.value)
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

		val selectedRoms = ModelRomAvailability.selectRomsForModel(settings.baseModel, roms)
		val kickPath = selectedRoms.kick?.path ?: return
		settings = applyConstraints(
			settings.copy(
				romFile = kickPath,
				romExtFile = selectedRoms.ext?.path.orEmpty()
			)
		)
	}

	private companion object {
		private const val LAST_SESSION_FILE = ".last_session.uae"
	}

	fun updateSettings(transform: (EmulatorSettings) -> EmulatorSettings) {
		val newSettings = transform(settings)
		settings = applyConstraints(newSettings)
		saveLastSession()
	}

	fun writeSettingsConfig(
		settingsSnapshot: EmulatorSettings,
		filename: String,
		unknownLines: List<String> = emptyList()
	): File {
		val configFile = ConfigGenerator.writeConfig(getApplication(), settingsSnapshot, filename)
		if (unknownLines.isNotEmpty()) {
			configFile.appendText("\n; Preserved settings from original config\n")
			unknownLines.forEach { configFile.appendText("$it\n") }
		}
		return configFile
	}

	fun writeCurrentSettingsConfig(filename: String = ".current_settings.uae"): File =
		writeSettingsConfig(settings, filename, currentUnknownLines)

	/**
	 * Enforce hardware constraints between interdependent settings.
	 */
	private fun applyConstraints(s: EmulatorSettings): EmulatorSettings =
		EmulatorSettingsConstraints.apply(s, hasTouchScreen = hasTouchScreen(getApplication()))
}
