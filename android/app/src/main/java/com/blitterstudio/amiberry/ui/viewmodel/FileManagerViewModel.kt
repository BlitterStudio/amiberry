package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import android.net.Uri
import android.util.Log
import androidx.annotation.StringRes
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.FileListing
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.ImportBatchFeedback
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.components.PickedFolder
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class FileManagerViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)

	/** The ROM rows: one AROS row, no WHDBooter copies of a listed Kickstart. */
	val roms: StateFlow<List<AmigaFile>> = repository.roms
		.map { FileListing.visibleRoms(it) }
		.stateIn(viewModelScope, SharingStarted.Eagerly, FileListing.visibleRoms(repository.roms.value))
	val floppies: StateFlow<List<AmigaFile>> = repository.floppies
	val hardDrives: StateFlow<List<AmigaFile>> = repository.hardDrives
	val cdImages: StateFlow<List<AmigaFile>> = repository.cdImages
	val whdloadGames: StateFlow<List<AmigaFile>> = repository.whdloadGames
	val isScanning: StateFlow<Boolean> = repository.isScanning

	private val _importResult = MutableStateFlow<String?>(null)
	val importResult: StateFlow<String?> = _importResult.asStateFlow()

	private val _isImporting = MutableStateFlow(false)
	val isImporting: StateFlow<Boolean> = _isImporting.asStateFlow()

	/** A category whose tab should open: every file of the last import went there. */
	private val _revealCategory = MutableStateFlow<FileCategory?>(null)
	val revealCategory: StateFlow<FileCategory?> = _revealCategory.asStateFlow()

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

	/** Each file goes to the category of its extension, whichever tab ([openCategory]) is open. */
	fun importFiles(uris: List<Uri>, openCategory: FileCategory) {
		if (uris.isEmpty()) return
		startImport(openCategory) { app ->
			FileManager.importFilesSortedWithResults(app, uris)
		}
	}

	fun importFolder(folder: PickedFolder, openCategory: FileCategory) {
		startImport(openCategory) { app ->
			when (folder) {
				is PickedFolder.Tree -> FileManager.importFolderWithResults(app, folder.uri)
				is PickedFolder.Local -> FileManager.importDirectoryWithResults(app, folder.dir)
			}
		}
	}

	private fun startImport(
		openCategory: FileCategory,
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
				val summary = FileListing.importSummary(results, openCategory)

				val outcome = if (results.size == 1) {
					val message = ImportFeedback.fromImportResult(results.first())
					app.getString(message.stringRes, message.argument)
				} else {
					val message = ImportBatchFeedback.messageFor(results)
					app.getString(message.stringRes, *message.args.map { it as Any }.toTypedArray())
				}
				val notes = buildList {
					// Say where files went when it is not (only) the open tab.
					if (summary.categories.any { it != openCategory }) {
						add(
							app.getString(
								R.string.file_manager_import_sorted_into,
								summary.categories.joinToString(", ") { app.getString(categoryTitle(it)) }
							)
						)
					}
					// A single archive already says so in its own message.
					if (results.size > 1 && summary.notWhdloadGames > 0) {
						add(
							app.resources.getQuantityString(
								R.plurals.file_manager_import_not_whdload,
								summary.notWhdloadGames,
								summary.notWhdloadGames
							)
						)
					}
				}
				_importResult.value = (listOf(outcome) + notes).joinToString("\n")

				summary.categories.forEach { repository.rescanCategory(it) }
				summary.revealCategory?.let { _revealCategory.value = it }
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

	fun clearRevealCategory() {
		_revealCategory.value = null
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

		/** The tab title of each category. */
		@StringRes
		fun categoryTitle(category: FileCategory): Int = when (category) {
			FileCategory.ROMS -> R.string.file_manager_tab_roms
			FileCategory.WHDLOAD_GAMES -> R.string.file_manager_tab_whdload
			FileCategory.FLOPPIES -> R.string.file_manager_tab_floppies
			FileCategory.HARD_DRIVES -> R.string.file_manager_tab_hard_drives
			FileCategory.CD_IMAGES -> R.string.file_manager_tab_cds
		}
	}
}
