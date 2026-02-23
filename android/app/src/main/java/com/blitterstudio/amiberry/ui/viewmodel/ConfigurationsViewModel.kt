package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.ConfigRepository
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

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

	fun deleteConfig(path: String) {
		viewModelScope.launch(Dispatchers.IO) {
			repository.deleteConfig(path)
			_configs.value = repository.listConfigs()
		}
	}

	fun duplicateConfig(path: String, newName: String) {
		viewModelScope.launch(Dispatchers.IO) {
			repository.duplicateConfig(path, newName)
			_configs.value = repository.listConfigs()
		}
	}

	fun renameConfig(path: String, newName: String) {
		viewModelScope.launch(Dispatchers.IO) {
			repository.renameConfig(path, newName)
			_configs.value = repository.listConfigs()
		}
	}
}
