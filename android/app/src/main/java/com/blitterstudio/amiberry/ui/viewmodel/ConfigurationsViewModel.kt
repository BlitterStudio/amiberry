package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigFiles
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.ConfigRepository
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext
import kotlinx.coroutines.launch

class ConfigurationsViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = ConfigRepository.getInstance(application)
	private val appPreferences = AppPreferences.getInstance(application)

	private val _configs = MutableStateFlow<List<ConfigInfo>>(emptyList())
	val configs: StateFlow<List<ConfigInfo>> = _configs.asStateFlow()

	init {
		refresh()
	}

	fun refresh() {
		viewModelScope.launch(Dispatchers.IO) {
			_configs.value = repository.listConfigs()
		}
	}

	/** Delete the file; a configuration deleted on purpose also leaves Recent. */
	suspend fun deleteConfig(path: String): Boolean {
		val deleted = withContext(Dispatchers.IO) {
			val removed = repository.deleteConfig(path)
			_configs.value = repository.listConfigs()
			removed
		}
		if (deleted) {
			appPreferences.forgetRecentConfig(path)
		}
		return deleted
	}

	/** The name a Duplicate of [config] is offered under: "X copy", or "X copy 2" and on past the names in use. */
	fun suggestedCopyName(config: ConfigInfo): String =
		ConfigFiles.copyName(config.name, _configs.value.map { it.name })

	/** Why [name] cannot be used, checked against the listed configurations; [renaming] may keep its own name. */
	fun nameProblem(name: String, renaming: ConfigInfo? = null): ConfigFiles.NameProblem? =
		ConfigFiles.nameProblem(
			name = name,
			existingNames = _configs.value.filter { it.path != renaming?.path }.map { it.name },
			currentName = renaming?.name
		)

	suspend fun duplicateConfig(path: String, newName: String): ConfigFiles.EditResult {
		return withContext(Dispatchers.IO) {
			val result = repository.duplicateConfig(path, newName)
			_configs.value = repository.listConfigs()
			result
		}
	}

	/** Rename the file; Recent follows a renamed configuration so it does not show as gone. */
	suspend fun renameConfig(path: String, newName: String): ConfigFiles.EditResult {
		val result = withContext(Dispatchers.IO) {
			val renamed = repository.renameConfig(path, newName)
			_configs.value = repository.listConfigs()
			renamed
		}
		if (result is ConfigFiles.EditResult.Done) {
			appPreferences.renameRecentConfig(path, result.file.absolutePath)
		}
		return result
	}

	suspend fun loadConfig(path: String): ConfigParser.ParsedConfig {
		return withContext(Dispatchers.IO) {
			repository.loadConfig(path)
		}
	}
}
