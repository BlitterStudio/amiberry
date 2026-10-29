package com.blitterstudio.amiberry.data

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import java.io.File

object ConfigurationActions {

	data class Message(
		@param:StringRes val stringRes: Int,
		val argument: String? = null
	)

	sealed interface LoadResult<out T> {
		data class Loaded<T>(val value: T) : LoadResult<T>
		data class Failed(val message: Message) : LoadResult<Nothing>
	}

	sealed interface ShareTarget {
		data class Ready(val file: File) : ShareTarget
		data class Failed(val message: Message) : ShareTarget
	}

	sealed interface LaunchTarget {
		data class Ready(val path: String) : LaunchTarget
		data class Failed(val message: Message) : LaunchTarget
	}

	fun <T> load(result: Result<T>): LoadResult<T> =
		result.fold(
			onSuccess = { LoadResult.Loaded(it) },
			onFailure = { LoadResult.Failed(Message(R.string.msg_failed_load_config)) }
		)

	/** What the user is told after deleting [name]; [wasOpen] says Settings had it open and keeps its values unnamed. */
	fun deleteMessage(name: String, deleted: Boolean, wasOpen: Boolean): Message =
		when {
			!deleted -> Message(R.string.msg_failed_delete_config)
			wasOpen -> Message(R.string.msg_config_deleted_was_open, name)
			else -> Message(R.string.msg_config_deleted_named, name)
		}

	fun duplicateMessage(result: ConfigFiles.EditResult): Message =
		when (result) {
			is ConfigFiles.EditResult.Done -> Message(R.string.msg_config_duplicated_as, result.file.nameWithoutExtension)
			is ConfigFiles.EditResult.Refused -> nameProblemMessage(result.problem)
			ConfigFiles.EditResult.Failed -> Message(R.string.msg_failed_duplicate_config)
		}

	fun renameMessage(result: ConfigFiles.EditResult): Message =
		when (result) {
			is ConfigFiles.EditResult.Done -> Message(R.string.msg_config_renamed_as, result.file.nameWithoutExtension)
			is ConfigFiles.EditResult.Refused -> nameProblemMessage(result.problem)
			ConfigFiles.EditResult.Failed -> Message(R.string.msg_failed_rename_config)
		}

	fun nameProblemMessage(problem: ConfigFiles.NameProblem): Message =
		when (problem) {
			ConfigFiles.NameProblem.Empty -> Message(R.string.config_name_problem_empty)
			ConfigFiles.NameProblem.InvalidCharacters -> Message(R.string.config_name_problem_invalid)
			is ConfigFiles.NameProblem.Taken -> Message(R.string.config_name_problem_taken, problem.name)
		}

	fun shareTarget(path: String): ShareTarget {
		val file = savedConfigFile(path)
		return if (file != null) {
			ShareTarget.Ready(file)
		} else {
			ShareTarget.Failed(Message(R.string.msg_failed_share_config))
		}
	}

	fun shareFailedMessage(): Message = Message(R.string.msg_failed_share_config)

	fun launchTarget(path: String): LaunchTarget {
		val file = savedConfigFile(path)
		return if (file != null) {
			LaunchTarget.Ready(file.path)
		} else {
			LaunchTarget.Failed(Message(R.string.msg_failed_launch_config))
		}
	}

	private fun savedConfigFile(path: String): File? {
		val file = File(path)
		return file.takeIf { it.isFile && ConfigFiles.isUserConfigFileName(it.name) }
	}
}
