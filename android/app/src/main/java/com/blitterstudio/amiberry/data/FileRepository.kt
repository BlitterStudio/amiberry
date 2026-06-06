package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext

class FileRepository internal constructor(
	private val scanCategory: (FileCategory) -> List<AmigaFile>
) {

	constructor(context: Context) : this({ category -> FileManager.scanForCategory(context, category) })

	private val scanMutex = Mutex()
	private val _isScanning = MutableStateFlow(false)
	val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()

	private val _roms = MutableStateFlow<List<AmigaFile>>(emptyList())
	val roms: StateFlow<List<AmigaFile>> = _roms.asStateFlow()

	private val _floppies = MutableStateFlow<List<AmigaFile>>(emptyList())
	val floppies: StateFlow<List<AmigaFile>> = _floppies.asStateFlow()

	private val _hardDrives = MutableStateFlow<List<AmigaFile>>(emptyList())
	val hardDrives: StateFlow<List<AmigaFile>> = _hardDrives.asStateFlow()

	private val _cdImages = MutableStateFlow<List<AmigaFile>>(emptyList())
	val cdImages: StateFlow<List<AmigaFile>> = _cdImages.asStateFlow()

	private val _whdloadGames = MutableStateFlow<List<AmigaFile>>(emptyList())
	val whdloadGames: StateFlow<List<AmigaFile>> = _whdloadGames.asStateFlow()

	suspend fun rescan() = withContext(Dispatchers.IO) {
		scanMutex.withLock {
			_isScanning.value = true
			try {
				_roms.value = scanCategory(FileCategory.ROMS)
				_floppies.value = scanCategory(FileCategory.FLOPPIES)
				_hardDrives.value = scanCategory(FileCategory.HARD_DRIVES)
				_cdImages.value = scanCategory(FileCategory.CD_IMAGES)
				_whdloadGames.value = scanCategory(FileCategory.WHDLOAD_GAMES)
			} finally {
				_isScanning.value = false
			}
		}
	}

	suspend fun rescanCategory(category: FileCategory) = withContext(Dispatchers.IO) {
		scanMutex.withLock {
			_isScanning.value = true
			try {
				val files = scanCategory(category)
				when (category) {
					FileCategory.ROMS -> _roms.value = files
					FileCategory.FLOPPIES -> _floppies.value = files
					FileCategory.HARD_DRIVES -> _hardDrives.value = files
					FileCategory.CD_IMAGES -> _cdImages.value = files
					FileCategory.WHDLOAD_GAMES -> _whdloadGames.value = files
				}
			} finally {
				_isScanning.value = false
			}
		}
	}

	fun getFilesForCategory(category: FileCategory): StateFlow<List<AmigaFile>> {
		return when (category) {
			FileCategory.ROMS -> roms
			FileCategory.FLOPPIES -> floppies
			FileCategory.HARD_DRIVES -> hardDrives
			FileCategory.CD_IMAGES -> cdImages
			FileCategory.WHDLOAD_GAMES -> whdloadGames
		}
	}

	companion object {
		@Volatile
		private var instance: FileRepository? = null

		fun getInstance(context: Context): FileRepository {
			return instance ?: synchronized(this) {
				instance ?: FileRepository(context.applicationContext).also { instance = it }
			}
		}
	}
}
