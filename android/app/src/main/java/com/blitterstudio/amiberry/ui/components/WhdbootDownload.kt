package com.blitterstudio.amiberry.ui.components

import android.content.Context
import android.text.format.Formatter
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.WhdbootProgress
import com.blitterstudio.amiberry.data.WhdbootStatus
import com.blitterstudio.amiberry.data.WhdbootTools
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.concurrent.atomic.AtomicBoolean

class WhdbootDownloadState {
	var progress by mutableStateOf<WhdbootProgress?>(null)
		private set
	var result by mutableStateOf<Int?>(null)
		private set
	var downloading by mutableStateOf(false)
		private set

	private val cancelled = AtomicBoolean(false)

	fun download(
		scope: CoroutineScope,
		context: Context,
		onFinished: (WhdbootDownloadResult) -> Unit
	) {
		if (downloading) return
		downloading = true
		cancelled.set(false)
		result = null
		scope.launch {
			val finished = withContext(Dispatchers.IO) {
				val result = runCatching {
					WhdbootTools.download(context) { next ->
						scope.launch {
							if (downloading) progress = next
						}
						cancelled.get()
					}
				}.getOrDefault(WhdbootTools.RESULT_FAILED)
				WhdbootDownloadResult(
					result = result,
					status = runCatching { WhdbootTools.status(context) }.getOrNull()
				)
			}
			downloading = false
			progress = null
			result = finished.result
			onFinished(finished)
		}
	}

	fun cancel() {
		cancelled.set(true)
	}
}

data class WhdbootDownloadResult(val result: Int, val status: WhdbootStatus?)

@Composable
fun WhdbootDownloadProgress(state: WhdbootDownloadState, cancelModifier: Modifier = Modifier) {
	val progress = state.progress
	Column {
		if (progress != null) {
			Text(stringResource(R.string.whdboot_progress, progress.step, progress.totalSteps, progress.label))
			if (progress.bytesTotal > 0) {
				LinearProgressIndicator(
					progress = { progress.bytesDownloaded.toFloat() / progress.bytesTotal.toFloat() },
					modifier = Modifier.fillMaxWidth()
				)
				Text(
					stringResource(
						R.string.whdboot_progress_bytes,
						Formatter.formatFileSize(LocalContext.current, progress.bytesDownloaded),
						Formatter.formatFileSize(LocalContext.current, progress.bytesTotal)
					)
				)
			} else {
				LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
			}
		} else {
			LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
		}
		TextButton(onClick = state::cancel, modifier = cancelModifier) {
			Text(stringResource(R.string.whdboot_cancel))
		}
	}
}

fun whdbootResultString(result: Int): Int = when (result) {
	WhdbootTools.RESULT_OK -> R.string.whdboot_result_ok
	WhdbootTools.RESULT_PARTIAL -> R.string.whdboot_result_partial
	WhdbootTools.RESULT_FAILED -> R.string.whdboot_result_failed
	WhdbootTools.RESULT_CANCELLED -> R.string.whdboot_result_cancelled
	WhdbootTools.RESULT_UNAVAILABLE -> R.string.whdboot_result_unavailable
	WhdbootTools.RESULT_EMULATION_RUNNING -> R.string.whdboot_result_emulation_running
	else -> R.string.whdboot_result_busy
}
