package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import java.io.File

object ConfigurationSaveActions {

	sealed interface SaveTarget {
		data class Ready(val file: File) : SaveTarget
		data object InvalidName : SaveTarget
		data object AlreadyExists : SaveTarget
	}

	sealed interface SaveResult {
		data class Saved(val file: File) : SaveResult
		data object InvalidName : SaveResult
		data object AlreadyExists : SaveResult
		data object Failed : SaveResult
	}

	fun targetForName(confDir: File, name: String): SaveTarget {
		val target = ConfigFiles.targetFileForName(confDir, name) ?: return SaveTarget.InvalidName
		return if (target.exists()) {
			SaveTarget.AlreadyExists
		} else {
			SaveTarget.Ready(target)
		}
	}

	fun messageFor(result: SaveResult): ConfigurationActions.Message =
		when (result) {
			is SaveResult.Saved -> ConfigurationActions.Message(R.string.msg_saved_configuration, result.file.name)
			SaveResult.InvalidName -> ConfigurationActions.Message(R.string.msg_invalid_config_name)
			SaveResult.AlreadyExists -> ConfigurationActions.Message(R.string.msg_config_already_exists)
			SaveResult.Failed -> ConfigurationActions.Message(R.string.msg_failed_save_config)
		}
}
