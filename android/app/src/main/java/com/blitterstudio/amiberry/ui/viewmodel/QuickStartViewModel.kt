package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.ui.screens.QuickStartLaunchMode
import com.blitterstudio.amiberry.ui.screens.QuickStartUiState
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class QuickStartViewModel(
	application: Application,
	private val savedStateHandle: SavedStateHandle
) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)
	private val prefs = AppPreferences.getInstance(application)

	var selectedWhdload by mutableStateOf<AmigaFile?>(null)
		private set

	/** The mode the user picked (kept across recreation); until then, [defaultLaunchMode]. */
	var selectedLaunchMode by mutableStateOf(
		savedStateHandle.get<String>(SELECTED_LAUNCH_MODE_KEY)?.let(QuickStartLaunchMode::fromName)
			?: defaultLaunchMode()
	)
		private set

	private val modeChosen: Boolean
		get() = savedStateHandle.contains(SELECTED_LAUNCH_MODE_KEY)

	val availableFloppies: StateFlow<List<AmigaFile>> = repository.floppies
	val availableCds: StateFlow<List<AmigaFile>> = repository.cdImages
	val availableRoms: StateFlow<List<AmigaFile>> = repository.roms
	val availableWhdloadGames: StateFlow<List<AmigaFile>> = repository.whdloadGames

	init {
		viewModelScope.launch {
			repository.rescan()
			restoreWhdloadSelection()
			// The scan may have found the first WHDLoad games.
			if (!modeChosen) selectedLaunchMode = defaultLaunchMode()
		}
		// Clear stale WHDLoad selection when the file list changes
		// (e.g., after a file is deleted in FileManager while this screen is live)
		viewModelScope.launch {
			availableWhdloadGames.collect { games ->
				val current = selectedWhdload ?: return@collect
				if (games.none { it.path == current.path }) {
					selectedWhdload = null
					prefs.lastWhdloadPath = ""
				}
			}
		}
	}

	private fun defaultLaunchMode(): QuickStartLaunchMode = QuickStartUiState.defaultLaunchMode(
		hasRecent = prefs.recentLaunches.value.any { RecentLaunches.Type.of(it) != RecentLaunches.Type.Unknown },
		hasWhdloadGames = repository.whdloadGames.value.isNotEmpty()
	)

	private fun restoreWhdloadSelection() {
		val savedPath = prefs.lastWhdloadPath
		if (savedPath.isBlank()) return
		val match = availableWhdloadGames.value.firstOrNull { it.path == savedPath }
		if (match != null) {
			selectedWhdload = match
		} else {
			// File was deleted or moved — clear the stale preference
			prefs.lastWhdloadPath = ""
		}
	}

	fun selectWhdload(file: AmigaFile?) {
		selectedWhdload = file
		prefs.lastWhdloadPath = file?.path ?: ""
	}

	fun selectLaunchMode(mode: QuickStartLaunchMode) {
		selectedLaunchMode = mode
		savedStateHandle[SELECTED_LAUNCH_MODE_KEY] = mode.name
	}

	fun rescanWhdload() {
		viewModelScope.launch {
			repository.rescanCategory(FileCategory.WHDLOAD_GAMES)
		}
	}

	fun rescanRoms() {
		viewModelScope.launch {
			repository.rescanCategory(FileCategory.ROMS)
		}
	}

	private companion object {
		const val SELECTED_LAUNCH_MODE_KEY = "quick_start_launch_mode"
	}
}
