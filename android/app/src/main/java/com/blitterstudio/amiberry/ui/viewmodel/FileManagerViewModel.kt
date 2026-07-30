package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import android.net.Uri
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ImportBatchFeedback
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import kotlinx.coroutines.CancellationException
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
	val isScanning: StateFlow<Boolean> = repository.isScanning

	private val _importResult = MutableStateFlow<String?>(null)
	val importResult: StateFlow<String?> = _importResult.asStateFlow()

	private val _isImporting = MutableStateFlow(false)
	val isImporting: StateFlow<Boolean> = _isImporting.asStateFlow()

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

	fun importFiles(uris: List<Uri>, category: FileCategory) {
		if (uris.isEmpty()) return
		startImport(category) { app ->
			uris.map { uri -> FileManager.importFileWithResult(app, uri, category) }
		}
	}

	fun importFolder(treeUri: Uri, category: FileCategory) {
		startImport(category) { app ->
			FileManager.importFolderWithResults(app, treeUri, category)
		}
	}

	private fun startImport(
		category: FileCategory,
		importer: (Application) -> List<FileManager.ImportResult>
	) {
		if (_isImporting.value) {
			return
		}
		_isImporting.value = true
		viewModelScope.launch(Dispatchers.IO) {
			try {
				val app = getApplication<Application>()
				val results = importer(app)
				val successCount = results.count { it is FileManager.ImportResult.Imported }

				_importResult.value = if (results.size == 1) {
					val message = ImportFeedback.fromImportResult(results.first())
					app.getString(message.stringRes, message.argument)
				} else {
					val message = ImportBatchFeedback.messageFor(results)
					app.getString(message.stringRes, *message.args.map { it as Any }.toTypedArray())
				}

				if (successCount > 0) {
					repository.rescanCategory(category)
				}
			} catch (e: CancellationException) {
				throw e
			} catch (e: Exception) {
				Log.e(TAG, "Failed to import files", e)
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
				val baseDir = app.getExternalFilesDir(null)
				val deleted = baseDir != null && FileManager.deleteManagedFile(baseDir, file)
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
