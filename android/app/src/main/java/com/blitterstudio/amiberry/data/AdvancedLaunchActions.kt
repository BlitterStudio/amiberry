package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import java.io.File

object AdvancedLaunchActions {
	const val SETTINGS_ADVANCED_CONFIG = ".temp_native_ui.uae"

	sealed interface ConfigTarget {
		data class Ready(val path: String) : ConfigTarget
		data class Failed(val message: ConfigurationActions.Message) : ConfigTarget
	}

	fun savedConfigTarget(path: String): ConfigTarget {
		val file = File(path)
		return if (file.isFile && ConfigFiles.isUserConfigFileName(file.name)) {
			ConfigTarget.Ready(file.path)
		} else {
			ConfigTarget.Failed(launchFailedMessage())
		}
	}

	fun launchFailedMessage(): ConfigurationActions.Message =
		ConfigurationActions.Message(R.string.msg_failed_launch_advanced)
}
