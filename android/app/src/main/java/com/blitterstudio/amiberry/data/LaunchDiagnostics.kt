package com.blitterstudio.amiberry.data

import org.json.JSONObject
import java.io.File

object LaunchDiagnostics {
	const val LAST_LAUNCH_FILE = ".last_launch.json"

	data class Entry(
		val schemaVersion: Int = 1,
		val requestType: String,
		val args: List<String>,
		val configPath: String?,
		val trackSession: Boolean,
		val startedAtEpochMs: Long
	)

	fun fromRequest(
		request: LaunchRequest,
		args: List<String>,
		trackSession: Boolean,
		startedAtEpochMs: Long
	): Entry = Entry(
		requestType = request.typeName(),
		args = args,
		configPath = request.configPathOrNull(),
		trackSession = trackSession,
		startedAtEpochMs = startedAtEpochMs
	)

	fun write(baseDir: File, entry: Entry): File {
		if (!baseDir.exists()) baseDir.mkdirs()
		return File(baseDir, LAST_LAUNCH_FILE).also { file ->
			file.writeText(entry.toJson())
		}
	}

	fun read(file: File): Entry {
		val json = JSONObject(file.readText())
		val args = json.getJSONArray("args")
		return Entry(
			schemaVersion = json.getInt("schemaVersion"),
			requestType = json.getString("requestType"),
			args = List(args.length()) { index -> args.getString(index) },
			configPath = if (json.isNull("configPath")) null else json.getString("configPath"),
			trackSession = json.getBoolean("trackSession"),
			startedAtEpochMs = json.getLong("startedAtEpochMs")
		)
	}

	private fun Entry.toJson(): String = buildString {
		appendLine("{")
		appendLine("  \"schemaVersion\": $schemaVersion,")
		appendLine("  \"requestType\": ${requestType.toJsonString()},")
		appendLine("  \"args\": [${args.joinToString(", ") { it.toJsonString() }}],")
		appendLine("  \"configPath\": ${configPath?.toJsonString() ?: "null"},")
		appendLine("  \"trackSession\": $trackSession,")
		appendLine("  \"startedAtEpochMs\": $startedAtEpochMs")
		appendLine("}")
	}

	private fun LaunchRequest.typeName(): String = when (this) {
		is LaunchRequest.AdvancedGui -> "AdvancedGui"
		is LaunchRequest.QuickStart -> "QuickStart"
		is LaunchRequest.SavedConfig -> "SavedConfig"
		is LaunchRequest.SettingsConfig -> "SettingsConfig"
		is LaunchRequest.WhdLoad -> "WhdLoad"
	}

	private fun LaunchRequest.configPathOrNull(): String? = when (this) {
		is LaunchRequest.AdvancedGui -> configPath
		is LaunchRequest.QuickStart -> configPath
		is LaunchRequest.SavedConfig -> configPath
		is LaunchRequest.SettingsConfig -> configPath
		is LaunchRequest.WhdLoad -> configPath
	}

	private fun String.toJsonString(): String = buildString {
		append('"')
		this@toJsonString.forEach { char ->
			when (char) {
				'\\' -> append("\\\\")
				'"' -> append("\\\"")
				'\n' -> append("\\n")
				'\r' -> append("\\r")
				'\t' -> append("\\t")
				else -> append(char)
			}
		}
		append('"')
	}

}
