package com.blitterstudio.amiberry.data

import android.content.Context
import androidx.annotation.Keep
import com.blitterstudio.amiberry.data.model.StoragePaths
import com.blitterstudio.amiberry.data.model.SettingsSchema
import java.io.File

/** The WHDBoot downloader in libamiberry, run by the launcher process, never the SDL process. */
data class WhdbootStatus(
	val whdloadInstalled: Boolean,
	val jstInstalled: Boolean,
	val rtbFilesInstalled: Boolean,
	val librariesInstalled: Boolean,
	val downloadsAvailable: Boolean,
	val useJst: Boolean = false
) {
	val readyForSelectedTool: Boolean get() = if (useJst) jstInstalled else whdloadInstalled
}

data class WhdbootProgress(
	val step: Int,
	val totalSteps: Int,
	val label: String,
	val bytesDownloaded: Long,
	val bytesTotal: Long
)

internal fun parseWhdbootStatus(fields: Array<String>): WhdbootStatus {
	fun field(index: Int) = fields.getOrNull(index) == "1"
	return WhdbootStatus(
		whdloadInstalled = field(0),
		jstInstalled = field(1),
		rtbFilesInstalled = field(2),
		librariesInstalled = field(3),
		downloadsAvailable = field(4)
	)
}

object WhdbootTools {
	const val RESULT_OK = 0
	const val RESULT_PARTIAL = 1
	const val RESULT_FAILED = 2
	const val RESULT_CANCELLED = 3
	const val RESULT_UNAVAILABLE = 4
	const val RESULT_BUSY = 5
	const val RESULT_EMULATION_RUNNING = 6

	init {
		System.loadLibrary("amiberry")
	}

	private external fun nativeStatus(path: String, useJst: Boolean): Array<String>
	private external fun nativeDownload(path: String, useJst: Boolean, listener: NativeProgressListener): Int

	fun status(context: Context): WhdbootStatus {
		val input = input(context)
		return parseStatus(nativeStatus(input.path, input.useJst)).copy(useJst = input.useJst)
	}

	fun download(context: Context, onProgress: (WhdbootProgress) -> Boolean): Int {
		if (EmulatorLauncher.hasSessionMarker(context))
			return RESULT_EMULATION_RUNNING
		val input = input(context)
		return nativeDownload(input.path, input.useJst, NativeProgressListener(onProgress))
	}

	internal fun parseStatus(fields: Array<String>): WhdbootStatus = parseWhdbootStatus(fields)

	private data class Input(val path: String, val useJst: Boolean)

	private fun input(context: Context): Input {
		val external = context.getExternalFilesDir(null)
		val settingsFile = HostSettingsStore.settingsFile(context.filesDir, external)
		val path = ContentFolders.effective("whdboot_path", settingsFile, external)
			?: File(external ?: context.filesDir, StoragePaths.WHDBOOT)
		val useJst = SettingsSchema.bundled(context)
			?.let { HostSettingsStore(settingsFile, it).read()["use_jst_instead_of_whd"].toBoolean() }
			?: false
		return Input(path.absolutePath, useJst)
	}

	@Keep
	class NativeProgressListener(private val onProgress: (WhdbootProgress) -> Boolean) {
		@Keep
		fun onProgress(step: Int, totalSteps: Int, label: String, bytesDownloaded: Long, bytesTotal: Long): Boolean =
			onProgress(WhdbootProgress(step, totalSteps, label, bytesDownloaded, bytesTotal))
	}
}
