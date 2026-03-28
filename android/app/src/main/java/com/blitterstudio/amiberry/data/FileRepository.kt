package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext

class FileRepository(private val context: Context) {

	private val scanLock = AtomicBoolean(false)
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
		if (!scanLock.compareAndSet(false, true)) return@withContext
		_isScanning.value = true
		try {
			_roms.value = FileManager.scanForCategory(context, FileCategory.ROMS)
			_floppies.value = FileManager.scanForCategory(context, FileCategory.FLOPPIES)
			_hardDrives.value = FileManager.scanForCategory(context, FileCategory.HARD_DRIVES)
			_cdImages.value = FileManager.scanForCategory(context, FileCategory.CD_IMAGES)
			_whdloadGames.value = FileManager.scanForCategory(context, FileCategory.WHDLOAD_GAMES)
		} finally {
			_isScanning.value = false
			scanLock.set(false)
		}
	}

	suspend fun rescanCategory(category: FileCategory) = withContext(Dispatchers.IO) {
		val files = FileManager.scanForCategory(context, category)
		when (category) {
			FileCategory.ROMS -> _roms.value = files
			FileCategory.FLOPPIES -> _floppies.value = files
			FileCategory.HARD_DRIVES -> _hardDrives.value = files
			FileCategory.CD_IMAGES -> _cdImages.value = files
			FileCategory.WHDLOAD_GAMES -> _whdloadGames.value = files
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
