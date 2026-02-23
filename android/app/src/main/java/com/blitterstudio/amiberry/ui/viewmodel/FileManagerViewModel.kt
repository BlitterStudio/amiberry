package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import android.net.Uri
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class FileManagerViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)

	val roms: StateFlow<List<AmigaFile>> = repository.roms
	val floppies: StateFlow<List<AmigaFile>> = repository.floppies
	val hardDrives: StateFlow<List<AmigaFile>> = repository.hardDrives
	val cdImages: StateFlow<List<AmigaFile>> = repository.cdImages
	val whdloadGames: StateFlow<List<AmigaFile>> = repository.whdloadGames

	private val _importResult = MutableStateFlow<String?>(null)
	val importResult: StateFlow<String?> = _importResult.asStateFlow()

	init {
		rescan()
	}

	fun rescan() {
		viewModelScope.launch(Dispatchers.IO) {
			repository.rescan()
		}
	}

	fun importFile(uri: Uri, category: FileCategory) {
		viewModelScope.launch(Dispatchers.IO) {
			val result = FileManager.importFile(getApplication(), uri, category)
			if (result != null) {
				_importResult.value = "Imported successfully"
				repository.rescanCategory(category)
			} else {
				_importResult.value = "Import failed"
			}
		}
	}

	fun clearImportResult() {
		_importResult.value = null
	}

	fun getStoragePath(): String {
		return FileManager.getAppStoragePath(getApplication())
	}
}
