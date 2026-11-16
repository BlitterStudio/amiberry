package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.data.WhdbootStatus
import com.blitterstudio.amiberry.data.WhdbootTools
import com.blitterstudio.amiberry.ui.components.WhdbootDownloadState
import com.blitterstudio.amiberry.ui.screens.library.LibraryLaunch
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
/** The launcher-process WHDBoot operation shared by App Settings and the Library. */
internal class WhdbootViewModel(application: Application) : AndroidViewModel(application) {
	private val app = application

	var status by mutableStateOf<WhdbootStatus?>(null)
		private set
	val download = WhdbootDownloadState()

	var pendingLaunch by mutableStateOf<LibraryLaunch.PlayResult.NeedsWhdboot?>(null)
		private set
	var launchResult by mutableStateOf<LibraryLaunch.PlayResult?>(null)
		private set
	var libraryResult by mutableStateOf<Int?>(null)
		private set

	fun reload() {
		viewModelScope.launch {
			status = withContext(Dispatchers.IO) { runCatching { WhdbootTools.status(app) }.getOrNull() }
		}
	}

	fun downloadForSettings() {
		download.download(viewModelScope, app) { finished -> status = finished.status }
	}

	fun requestLaunch(request: LibraryLaunch.PlayResult.NeedsWhdboot) {
		pendingLaunch = request
	}

	fun clearPendingLaunch() {
		pendingLaunch = null
	}

	fun downloadForPendingLaunch() {
		val request = pendingLaunch ?: return
		download.download(viewModelScope, app) { finished ->
			status = finished.status
			pendingLaunch = null
			if (finished.result == WhdbootTools.RESULT_CANCELLED) {
				libraryResult = finished.result
				return@download
			}
			val canLaunch = finished.result == WhdbootTools.RESULT_OK ||
				(finished.result == WhdbootTools.RESULT_PARTIAL && finished.status?.readyForSelectedTool == true)
			if (!canLaunch) {
				libraryResult = finished.result
				return@download
			}
			if (finished.result == WhdbootTools.RESULT_PARTIAL) libraryResult = finished.result
			viewModelScope.launch {
				launchResult = LibraryLaunch.retryWhdload(app, request.path)
			}
		}
	}

	fun consumeLaunchResult(): LibraryLaunch.PlayResult? = launchResult.also { launchResult = null }

	fun clearLibraryResult() {
		libraryResult = null
	}
}
