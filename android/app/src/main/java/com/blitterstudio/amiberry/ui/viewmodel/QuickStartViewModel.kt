package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import kotlinx.coroutines.flow.StateFlow

class QuickStartViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)

	var selectedWhdload by mutableStateOf<AmigaFile?>(null)
		private set

	val availableFloppies: StateFlow<List<AmigaFile>> = repository.floppies
	val availableCds: StateFlow<List<AmigaFile>> = repository.cdImages
	val availableRoms: StateFlow<List<AmigaFile>> = repository.roms
	val availableWhdloadGames: StateFlow<List<AmigaFile>> = repository.whdloadGames

	init {
		repository.rescan()
	}

	fun selectWhdload(file: AmigaFile?) {
		selectedWhdload = file
	}

	fun rescanWhdload() {
		repository.rescanCategory(FileCategory.WHDLOAD_GAMES)
	}
}
