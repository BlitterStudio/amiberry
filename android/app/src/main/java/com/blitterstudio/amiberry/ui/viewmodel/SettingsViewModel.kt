package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.FileRepository
import java.io.File
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.SettingsChange
import com.blitterstudio.amiberry.data.model.SettingsChangeSummary
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotice
import com.blitterstudio.amiberry.data.model.SettingsAdjustmentNotices
import com.blitterstudio.amiberry.data.model.SettingsIntentPreset
import com.blitterstudio.amiberry.data.model.SettingsIntentPresets
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.ConfigRepository
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.data.WhdLoadAutoConfig
import com.blitterstudio.amiberry.ui.hasTouchScreen
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class SettingsViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)
	private val configRepository = ConfigRepository.getInstance(application)
	private val appPreferences = AppPreferences.getInstance(application)

	var settings by mutableStateOf(EmulatorSettings())
		private set

	var adjustmentNotices by mutableStateOf<List<SettingsAdjustmentNotice>>(emptyList())
		private set

	var currentUnknownLines by mutableStateOf<List<String>>(emptyList())
		private set

	var currentConfigName by mutableStateOf<String?>(null)
		private set

	var currentConfigDescription by mutableStateOf("")
		private set

	var currentConfigPath by mutableStateOf<String?>(null)
		private set

	// State-backed so updating the baseline (on save/load, when `settings` itself does not
	// change) still triggers recomposition of anything observing isDirty.
	private var baselineSettings by mutableStateOf(EmulatorSettings())
	private var baselineUnknownLines by mutableStateOf<List<String>>(emptyList())

	val isDirty: Boolean
		get() = currentConfigName != null &&
			(settings != baselineSettings || currentUnknownLines != baselineUnknownLines)

	val changeSummary: List<SettingsChange>
		get() = SettingsChangeSummary.diff(baselineSettings, settings)

	val availableRoms: StateFlow<List<AmigaFile>> = repository.roms
	val availableFloppies: StateFlow<List<AmigaFile>> = repository.floppies
	val availableCds: StateFlow<List<AmigaFile>> = repository.cdImages
	val availableHardDrives: StateFlow<List<AmigaFile>> = repository.hardDrives

	init {
		if (!restoreLastSession()) {
			settings = appPreferences.applyRememberedAndroidControls(settings, emptySet())
		}
		settings = applyConstraints(settings)
		appPreferences.saveAndroidControls(settings)
		viewModelScope.launch {
			repository.rescan()
			autoSelectDefaultRomIfNeeded(availableRoms.value)
		}
	}

	private fun restoreLastSession(): Boolean {
		val context = getApplication<Application>()
		val sessionFile = ConfigGenerator.configFile(context, LAST_SESSION_FILE)
		val legacySessionFile = ConfigGenerator.legacyExternalConfigFile(context, LAST_SESSION_FILE)
		val readableSessionFile = when {
			sessionFile.exists() -> sessionFile
			legacySessionFile.exists() -> legacySessionFile
			else -> return false
		}
		try {
			val parsed = ConfigParser.parse(readableSessionFile)
			settings = appPreferences.applyRememberedAndroidControls(parsed.settings, parsed.explicitKeys)
			currentUnknownLines = parsed.unknownLines
			return true
		} catch (_: Exception) {
			// Corrupted session file — start with defaults
			return false
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
		val previousSettings = settings
		val newSettings = EmulatorSettings.fromModel(model).copy(
			romFile = selectedRoms.kick?.path.orEmpty(),
			romExtFile = selectedRoms.ext?.path.orEmpty(),
			joyport0 = previousSettings.joyport0,
			joyport1 = previousSettings.joyport1,
			onScreenJoystick = previousSettings.onScreenJoystick,
			onScreenKeyboard = previousSettings.onScreenKeyboard
		)
		applyConstrainedSettings(newSettings, publishNotices = true)
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	fun loadConfig(parsed: ConfigParser.ParsedConfig, name: String, path: String) {
		applyConstrainedSettings(
			appPreferences.applyRememberedAndroidControls(parsed.settings, parsed.explicitKeys),
			publishNotices = true
		)
		currentUnknownLines = parsed.unknownLines
		currentConfigName = name
		currentConfigDescription = parsed.description
		currentConfigPath = path
		autoSelectDefaultRomIfNeeded(availableRoms.value)
		baselineSettings = settings
		baselineUnknownLines = currentUnknownLines
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	/** Overwrite the exact config file currently being edited. Requires an open config. */
	suspend fun saveTracked(): ConfigurationSaveActions.SaveResult {
		val path = currentConfigPath
		if (path == null) {
			// No exact path tracked — fall back to name-based save.
			val name = currentConfigName ?: return ConfigurationSaveActions.SaveResult.InvalidName
			return saveAs(name, currentConfigDescription, allowOverwrite = true)
		}
		val savedSettings = settings
		val savedUnknownLines = currentUnknownLines
		val result = withContext(Dispatchers.IO) {
			configRepository.overwriteConfigAtPath(
				path = path,
				settings = savedSettings,
				unknownLines = savedUnknownLines,
				description = currentConfigDescription
			)
		}
		if (result is ConfigurationSaveActions.SaveResult.Saved) {
			currentConfigName = result.file.nameWithoutExtension
			currentConfigPath = result.file.absolutePath
			baselineSettings = savedSettings
			baselineUnknownLines = savedUnknownLines
		}
		return result
	}

	/**
	 * Save under [name]. With [allowOverwrite] false, a collision with a *different*
	 * existing config returns AlreadyExists so the UI can confirm. On success the saved
	 * config becomes the tracked config and the dirty baseline is reset.
	 */
	suspend fun saveAs(
		name: String,
		description: String,
		allowOverwrite: Boolean
	): ConfigurationSaveActions.SaveResult {
		val savedSettings = settings
		val savedUnknownLines = currentUnknownLines
		val result = withContext(Dispatchers.IO) {
			configRepository.saveResolved(
				settings = savedSettings,
				requestedName = name,
				currentConfigName = currentConfigName,
				unknownLines = savedUnknownLines,
				description = description,
				allowOverwrite = allowOverwrite
			)
		}
		if (result is ConfigurationSaveActions.SaveResult.Saved) {
			currentConfigName = result.file.nameWithoutExtension
			currentConfigPath = result.file.absolutePath
			currentConfigDescription = description
			baselineSettings = savedSettings
			baselineUnknownLines = savedUnknownLines
		}
		return result
	}

	/** Revert in-memory edits back to the last saved/loaded values of the open config. */
	fun discardChanges() {
		settings = baselineSettings
		currentUnknownLines = baselineUnknownLines
		clearAdjustmentNotices()
		appPreferences.saveAndroidControls(settings)
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
		applyConstrainedSettings(newSettings, publishNotices = true)
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	fun applyIntentPreset(preset: SettingsIntentPreset) {
		applyConstrainedSettings(
			SettingsIntentPresets.apply(settings, preset),
			publishNotices = true
		)
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	fun clearAdjustmentNotices() {
		adjustmentNotices = emptyList()
	}

	fun applyWhdLoadAutoConfig(file: AmigaFile) {
		val currentSettings = settings
		viewModelScope.launch {
			val detectedSettings = withContext(Dispatchers.IO) {
				WhdLoadAutoConfig.settingsFor(
					context = getApplication(),
					whdLoadFile = file,
					availableRoms = availableRoms.value,
					currentSettings = currentSettings
				)
			} ?: return@launch

			applyConstrainedSettings(detectedSettings, publishNotices = true)
			appPreferences.saveAndroidControls(settings)
			saveLastSession()
		}
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

	private fun applyConstrainedSettings(requested: EmulatorSettings, publishNotices: Boolean) {
		val constrained = applyConstraints(requested)
		settings = constrained
		if (publishNotices) {
			adjustmentNotices = SettingsAdjustmentNotices.fromAdjustment(requested, constrained)
		}
	}
}
