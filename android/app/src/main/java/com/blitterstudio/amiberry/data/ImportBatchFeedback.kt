package com.blitterstudio.amiberry.data

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

object ImportBatchFeedback {

	data class Message(
		@param:StringRes val stringRes: Int,
		val args: List<Int>
	)

	fun messageFor(results: List<FileManager.ImportResult>): Message {
		val imported = results.count { it is FileManager.ImportResult.Imported }
		val unsupported = results.count { it is FileManager.ImportResult.Unsupported }
		val failed = results.count { it is FileManager.ImportResult.Failed }
		val total = results.size

		return when {
			imported == total -> Message(R.string.msg_imported_multiple, listOf(imported, total))
			imported == 0 -> Message(R.string.msg_imported_none, listOf(unsupported, failed))
			else -> Message(R.string.msg_imported_multiple_with_failures, listOf(imported, total, unsupported, failed))
		}
	}
}
