package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import kotlinx.coroutines.flow.StateFlow

class QuickStartViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)

	var selectedModel by mutableStateOf(AmigaModel.A500)
		private set

	var selectedFloppy by mutableStateOf<AmigaFile?>(null)
		private set

	var selectedCd by mutableStateOf<AmigaFile?>(null)
		private set

	var selectedWhdload by mutableStateOf<AmigaFile?>(null)
		private set

	val availableFloppies: StateFlow<List<AmigaFile>> = repository.floppies
	val availableCds: StateFlow<List<AmigaFile>> = repository.cdImages
	val availableRoms: StateFlow<List<AmigaFile>> = repository.roms
	val availableWhdloadGames: StateFlow<List<AmigaFile>> = repository.whdloadGames

	init {
		repository.rescan()
	}

	fun selectModel(model: AmigaModel) {
		selectedModel = model
		// Clear media selection when switching between floppy/CD models
		if (!model.hasFloppy) selectedFloppy = null
		if (!model.hasCd) selectedCd = null
	}

	fun selectFloppy(file: AmigaFile?) {
		selectedFloppy = file
	}

	fun selectCd(file: AmigaFile?) {
		selectedCd = file
	}

	fun selectWhdload(file: AmigaFile?) {
		selectedWhdload = file
	}

	fun rescanWhdload() {
		repository.rescanCategory(FileCategory.WHDLOAD_GAMES)
	}

	fun hasRoms(): Boolean {
		return availableRoms.value.isNotEmpty()
	}
}
