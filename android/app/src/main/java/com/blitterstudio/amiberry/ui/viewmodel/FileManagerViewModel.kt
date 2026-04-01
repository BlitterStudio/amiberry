package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import android.net.Uri
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
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
	val isScanning: StateFlow<Boolean> = repository.isScanning

	private val _importResult = MutableStateFlow<String?>(null)
	val importResult: StateFlow<String?> = _importResult.asStateFlow()

	private val _isImporting = MutableStateFlow(false)
	val isImporting: StateFlow<Boolean> = _isImporting.asStateFlow()

	private var importJob: Job? = null

	init {
		rescan()
	}

	fun rescan() {
		viewModelScope.launch(Dispatchers.IO) {
			try {
				repository.rescan()
			} catch (e: Exception) {
				Log.e(TAG, "Failed to rescan files", e)
			}
		}
	}

	fun importFile(uri: Uri, category: FileCategory) {
		importJob?.cancel()
		importJob = viewModelScope.launch(Dispatchers.IO) {
			_isImporting.value = true
			try {
				val app = getApplication<Application>()

				// Validate file extension before importing
				val fileName = FileManager.getDisplayName(app, uri)
				if (fileName != null) {
					val ext = fileName.substringAfterLast('.', "").lowercase()
					if (ext.isNotEmpty() && ext !in category.extensions) {
						_importResult.value = app.getString(R.string.msg_unsupported_file_type)
						return@launch
					}
				}

				val result = FileManager.importFile(app, uri, category)
				if (result != null) {
					_importResult.value = app.getString(R.string.msg_imported_successfully)
					repository.rescanCategory(category)
				} else {
					_importResult.value = app.getString(R.string.msg_import_failed)
				}
			} catch (e: CancellationException) {
				throw e
			} catch (e: Exception) {
				Log.e(TAG, "Failed to import file", e)
				_importResult.value = getApplication<Application>().getString(R.string.msg_import_failed)
			} finally {
				_isImporting.value = false
			}
		}
	}

	fun deleteFile(file: AmigaFile) {
		viewModelScope.launch(Dispatchers.IO) {
			try {
				val app = getApplication<Application>()
				val deleted = java.io.File(file.path).delete()
				if (deleted) {
					_importResult.value = app.getString(R.string.msg_deleted_file, file.name)
					repository.rescanCategory(file.category)
				} else {
					_importResult.value = app.getString(R.string.msg_failed_delete_file, file.name)
				}
			} catch (e: CancellationException) {
				throw e
			} catch (e: Exception) {
				Log.e(TAG, "Failed to delete file: ${file.path}", e)
				_importResult.value = getApplication<Application>().getString(R.string.msg_failed_delete_file, file.name)
			}
		}
	}

	fun clearImportResult() {
		_importResult.value = null
	}

	fun getStoragePath(): String {
		return FileManager.getAppStoragePath(getApplication())
	}

	companion object {
		private const val TAG = "Amiberry-FileManagerVM"
	}
}
