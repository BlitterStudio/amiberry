package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.ConfigRepository
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext
import kotlinx.coroutines.launch
import java.io.File

class ConfigurationsViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = ConfigRepository.getInstance(application)

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

	suspend fun deleteConfig(path: String): Boolean {
		return withContext(Dispatchers.IO) {
			val deleted = repository.deleteConfig(path)
			_configs.value = repository.listConfigs()
			deleted
		}
	}

	suspend fun duplicateConfig(path: String): Result<File> {
		return withContext(Dispatchers.IO) {
			val sourceName = File(path).nameWithoutExtension
			val existingNames = repository.listConfigs().map { it.name }.toSet()
			var copyName = "${sourceName}_copy"
			var suffix = 2
			while (copyName in existingNames) {
				copyName = "${sourceName}_copy$suffix"
				suffix++
			}
			val duplicated = repository.duplicateConfig(path, copyName)
			_configs.value = repository.listConfigs()
			if (duplicated != null) {
				Result.success(duplicated)
			} else {
				Result.failure(IllegalStateException("Duplicate failed"))
			}
		}
	}

	suspend fun renameConfig(path: String, newName: String): Boolean {
		return withContext(Dispatchers.IO) {
			val renamed = repository.renameConfig(path, newName)
			_configs.value = repository.listConfigs()
			renamed != null
		}
	}

	suspend fun loadConfig(path: String): ConfigParser.ParsedConfig {
		return withContext(Dispatchers.IO) {
			repository.loadConfig(path)
		}
	}
}
