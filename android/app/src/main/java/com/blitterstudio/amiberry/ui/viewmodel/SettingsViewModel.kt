package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigFiles
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigSettingsResolver
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ShaderCatalog
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
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.data.model.SettingsIntentPresets
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.ConfigRepository
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.data.OpenConfigTracking
import com.blitterstudio.amiberry.ui.hasTouchScreen
import com.blitterstudio.amiberry.ui.screens.MediaDrive
import com.blitterstudio.amiberry.ui.screens.QuickStartMediaState
import java.io.File
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class SettingsViewModel(application: Application) : AndroidViewModel(application) {
	enum class ShaderCatalogStatus {
		NOT_LOADED,
		LOADING,
		LOADED
	}

	private val repository = FileRepository.getInstance(application)
	private val configRepository = ConfigRepository.getInstance(application)
	private val appPreferences = AppPreferences.getInstance(application)

	var settings by mutableStateOf(EmulatorSettings())
		private set

	var shaderCatalogEntries by mutableStateOf(ShaderCatalog.BUILT_INS)
		private set

	var shaderCatalogStatus by mutableStateOf(ShaderCatalogStatus.NOT_LOADED)
		private set

	private var shaderCatalogRefreshGeneration = 0

	var adjustmentNotices by mutableStateOf<List<SettingsAdjustmentNotice>>(emptyList())
		private set

	var selectedIntentPreset by mutableStateOf<SettingsIntentPreset?>(null)
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

	/**
	 * What Review changes compares with: the open configuration as last saved or
	 * loaded, or for a new configuration its model's preset (EmulatorSettings.presetMachine).
	 */
	val reviewBaseline: EmulatorSettings
		get() = if (currentConfigName != null) baselineSettings else settings.presetMachine()

	val changeSummary: List<SettingsChange>
		get() = SettingsChangeSummary.diff(reviewBaseline, settings)

	/**
	 * Whether Save would keep something the editor would otherwise lose: edits to the
	 * open configuration, or a new configuration that differs from its model's preset.
	 * Unlike [isDirty] (the shell's leave and Start questions, which save to the open
	 * file) it covers new configurations too.
	 */
	val hasUnsavedChanges: Boolean
		get() = if (currentConfigName != null) isDirty else changeSummary.isNotEmpty()

	/**
	 * Set before another screen switches to Settings with a button (Configurations'
	 * Edit and New): Settings then puts D-pad focus on its first control, as the
	 * button that had it is gone. Settings clears it.
	 */
	var focusOnEntry by mutableStateOf(false)

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
			// A Kickstart imported anywhere (the Files tab too) fills settings that name none.
			availableRoms.collect { roms -> autoSelectDefaultRomIfNeeded(roms) }
		}
	}

	fun refreshShaderCatalog() {
		val refreshGeneration = ++shaderCatalogRefreshGeneration
		shaderCatalogStatus = ShaderCatalogStatus.LOADING

		viewModelScope.launch {
			val catalogEntries = withContext(Dispatchers.IO) {
				val application = getApplication<Application>()
				val externalFilesDir = application.getExternalFilesDir(null)
				val settingsFile = ShaderCatalog.findSettingsFile(
					application.filesDir,
					externalFilesDir
				)
				val roots = ShaderCatalog.resolveScanRoots(
					settingsFile,
					externalFilesDir
				)
				ShaderCatalog.scan(roots)
			}

			if (refreshGeneration != shaderCatalogRefreshGeneration) return@launch
			shaderCatalogEntries = catalogEntries
			shaderCatalogStatus = ShaderCatalogStatus.LOADED
		}
	}

	private fun restoreLastSession(): Boolean {
		val context = getApplication<Application>()
		val globalSettingsFile = ShaderCatalog.findSettingsFile(
			context.filesDir,
			context.getExternalFilesDir(null)
		)
		val sessionFile = ConfigGenerator.configFile(context, LAST_SESSION_FILE)
		val legacySessionFile = ConfigGenerator.legacyExternalConfigFile(context, LAST_SESSION_FILE)
		val readableSessionFile = when {
			sessionFile.exists() -> sessionFile
			legacySessionFile.exists() -> legacySessionFile
			else -> {
				settings = ConfigSettingsResolver.defaults(globalSettingsFile)
				return false
			}
		}
		try {
			val parsed = ConfigSettingsResolver.parse(readableSessionFile, globalSettingsFile)
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

	/** What a model change did: the settings before it (for Undo) and the media it had to eject. */
	data class ModelChange(
		val previous: EmulatorSettings,
		val previousUnknownLines: List<String>,
		val droppedMedia: List<MediaDrive>
	)

	/**
	 * Switch to [model]'s preset at its default compatibility level, keeping
	 * the Android controls, shader and inserted media (a disk the new model
	 * has no drive for goes: see the returned [ModelChange]). The previous
	 * configuration's machine goes, its preserved lines with it: they
	 * described that machine.
	 */
	fun applyModel(model: AmigaModel): ModelChange {
		val previous = settings
		val previousUnknownLines = currentUnknownLines
		selectedIntentPreset = null
		val selectedRoms = ModelRomAvailability.selectRomsForModel(model, availableRoms.value)
		val machine = settings.withModel(model, selectedRoms.config).copy(
			romFile = selectedRoms.romFile,
			romExtFile = selectedRoms.romExtFile
		)
		val media = QuickStartMediaState.keepMedia(previous, machine)
		currentUnknownLines = emptyList()
		applyConstrainedSettings(media.settings, publishNotices = true)
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
		return ModelChange(previous, previousUnknownLines, media.dropped)
	}

	/** Undo [change]: the settings as they were before the model change. */
	fun undoModelChange(change: ModelChange) {
		selectedIntentPreset = null
		currentUnknownLines = change.previousUnknownLines
		applyConstrainedSettings(change.previous, publishNotices = false)
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	/** Put the model's preset at compatibility [level] (see EmulatorSettings.withCompatibilityLevel). */
	fun applyCompatibilityLevel(level: Int) {
		updateSettings { it.withCompatibilityLevel(level) }
	}

	fun loadConfig(parsed: ConfigParser.ParsedConfig, name: String, path: String) {
		selectedIntentPreset = null
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

	/**
	 * Put the editor back to [reviewBaseline]: the open configuration's last saved or
	 * loaded values, or a new configuration's model preset.
	 */
	fun discardChanges() {
		selectedIntentPreset = null
		if (currentConfigName != null) {
			settings = baselineSettings
			currentUnknownLines = baselineUnknownLines
		} else {
			settings = applyConstraints(settings.presetMachine())
		}
		clearAdjustmentNotices()
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	/**
	 * Set [change]'s option back to its [reviewBaseline] value, with what the layer's
	 * setter changes along with it (reverting the CPU can drop the JIT again). The
	 * launcher's own rows (no option) cannot be reverted one by one.
	 */
	fun revertChange(change: SettingsChange) {
		val id = change.optionId ?: return
		val value = reviewBaseline.optionValue(id) ?: SettingsSchema.current?.option(id)?.defaultOnAndroid ?: return
		updateSettings { it.withOption(id, value) }
	}

	/**
	 * Start a new, unnamed configuration: the default model's preset with the
	 * remembered Android controls, as on a first start.
	 */
	fun newConfiguration() {
		val context = getApplication<Application>()
		val globalSettingsFile = ShaderCatalog.findSettingsFile(context.filesDir, context.getExternalFilesDir(null))
		selectedIntentPreset = null
		currentConfigName = null
		currentConfigPath = null
		currentConfigDescription = ""
		currentUnknownLines = emptyList()
		settings = applyConstraints(
			appPreferences.applyRememberedAndroidControls(ConfigSettingsResolver.defaults(globalSettingsFile), emptySet())
		)
		autoSelectDefaultRomIfNeeded(availableRoms.value)
		baselineSettings = settings
		baselineUnknownLines = currentUnknownLines
		clearAdjustmentNotices()
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	/** The name Save offers a new configuration: its model's, numbered past the saved ones. */
	suspend fun suggestedConfigName(): String {
		val names = withContext(Dispatchers.IO) { configRepository.listConfigs().map { it.name } }
		return ConfigFiles.suggestedName(settings.baseModel.displayName, names)
	}

	private val tracked: OpenConfigTracking.Tracked
		get() = OpenConfigTracking.Tracked(currentConfigName, currentConfigPath)

	/** Whether the configuration file at [path] is the one open here. */
	fun isConfigOpen(path: String): Boolean = OpenConfigTracking.isOpen(tracked, path)

	/** The file at [oldPath] was renamed to [renamed]: when it is the open one, Save now writes the new file. */
	fun onConfigRenamed(oldPath: String, renamed: File) {
		val updated = OpenConfigTracking.afterRename(tracked, oldPath, renamed)
		currentConfigName = updated.name
		currentConfigPath = updated.path
	}

	/**
	 * The file at [path] was deleted. When it is the open one, its values stay open as an
	 * unnamed configuration (no file to save back to). Returns whether it was the open one.
	 */
	fun onConfigDeleted(path: String): Boolean {
		val before = tracked
		val updated = OpenConfigTracking.afterDelete(before, path)
		if (updated == before) return false
		currentConfigName = updated.name
		currentConfigPath = updated.path
		return true
	}

	private fun autoSelectDefaultRomIfNeeded(roms: List<AmigaFile>) {
		if (roms.isEmpty()) return
		if (settings.romFile.isNotBlank()) return

		val selectedRoms = ModelRomAvailability.selectRomsForModel(settings.baseModel, roms)
		val kickPath = selectedRoms.kick?.path ?: return
		settings = applyConstraints(
			settings.copy(
				baseConfig = selectedRoms.config,
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
		selectedIntentPreset = preset.takeIf { SettingsIntentPresets.matches(settings, it) }
		appPreferences.saveAndroidControls(settings)
		saveLastSession()
	}

	fun clearAdjustmentNotices() {
		adjustmentNotices = emptyList()
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
		selectedIntentPreset = selectedIntentPreset?.takeIf {
			SettingsIntentPresets.matches(constrained, it)
		}
		if (publishNotices) {
			adjustmentNotices = SettingsAdjustmentNotices.fromAdjustment(requested, constrained)
		}
	}
}
