package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import java.io.File

object CrashRecoveryActions {

	fun summary(entry: LaunchDiagnostics.Entry?): String? {
		entry ?: return null
		val configName = entry.configPath?.let { File(it).name }
		return buildString {
			append(entry.requestType)
			append(", ")
			append(entry.args.size)
			append(" args")
			if (!configName.isNullOrBlank()) {
				append(", config ")
				append(configName)
			}
		}
	}

	fun copyPayload(
		entry: LaunchDiagnostics.Entry?,
		packageName: String,
		versionName: String,
		versionCode: Long,
		sdkInt: Int
	): String = buildString {
		appendLine("Amiberry Android crash diagnostics")
		appendLine("Package: $packageName")
		appendLine("Version: ${versionName.ifBlank { "unknown" }} ($versionCode)")
		appendLine("SDK: $sdkInt")
		appendLine()
		if (entry == null) {
			appendLine("Last launch: unavailable")
		} else {
			appendLine("Last launch: ${entry.requestType}")
			appendLine("Track session: ${entry.trackSession}")
			appendLine("Started at: ${entry.startedAtEpochMs}")
			appendLine("Config: ${entry.configPath ?: "none"}")
			appendLine("Args: ${entry.args.joinToString(" ")}")
		}
	}

	fun copyMessage(): ConfigurationActions.Message =
		ConfigurationActions.Message(R.string.msg_crash_diagnostics_copied)
}
