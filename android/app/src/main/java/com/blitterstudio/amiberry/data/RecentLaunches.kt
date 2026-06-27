package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

object RecentLaunches {
	const val MAX_ITEMS = 10

	data class Details(
		val title: String,
		val typeLabel: String,
		val detail: String
	)

	fun parse(raw: String): List<JSONObject> {
		return try {
			val arr = JSONArray(raw)
			(0 until arr.length()).mapNotNull { index -> arr.opt(index) as? JSONObject }
		} catch (_: Exception) {
			emptyList()
		}
	}

	fun serialize(entries: List<JSONObject>, maxItems: Int = MAX_ITEMS): String {
		val arr = JSONArray()
		entries.take(maxItems).forEach { arr.put(it) }
		return arr.toString()
	}

	fun add(entries: List<JSONObject>, entry: JSONObject, maxItems: Int = MAX_ITEMS): List<JSONObject> {
		val entryString = entry.toString()
		return (listOf(entry) + entries.filterNot { it.toString() == entryString }).take(maxItems)
	}

	fun available(
		entries: List<JSONObject>,
		availableModels: Collection<AmigaModel>,
		fileExists: (String) -> Boolean = { File(it).exists() }
	): List<JSONObject> = entries.filter { isAvailable(it, availableModels, fileExists) }

	fun pruneMissingFiles(
		entries: List<JSONObject>,
		fileExists: (String) -> Boolean = { File(it).exists() }
	): List<JSONObject> = entries.filter { hasExistingFiles(it, fileExists) }

	fun isAvailable(
		entry: JSONObject,
		availableModels: Collection<AmigaModel>,
		fileExists: (String) -> Boolean = { File(it).exists() }
	): Boolean {
		return when (entry.optString("type")) {
			"config", "whdload" -> {
				val path = entry.optString("path")
				path.isNotBlank() && fileExists(path)
			}
			"quickstart" -> {
				val model = AmigaModel.entries.firstOrNull { it.cmdArg == entry.optString("model") }
				model in availableModels && MEDIA_KEYS.all { key ->
					val path = entry.optString(key)
					path.isBlank() || fileExists(path)
				}
			}
			else -> false
		}
	}

	private fun hasExistingFiles(
		entry: JSONObject,
		fileExists: (String) -> Boolean
	): Boolean {
		return when (entry.optString("type")) {
			"config", "whdload" -> {
				val path = entry.optString("path")
				path.isNotBlank() && fileExists(path)
			}
			"quickstart" -> {
				AmigaModel.entries.any { it.cmdArg == entry.optString("model") } &&
					MEDIA_KEYS.all { key ->
						val path = entry.optString(key)
						path.isBlank() || fileExists(path)
					}
			}
			else -> false
		}
	}

	fun label(entry: JSONObject): String {
		return when (entry.optString("type")) {
			"quickstart" -> {
				val modelName = entry.optString("model", "?")
				val media = MEDIA_KEYS
					.mapNotNull { key -> fileName(entry.optString(key)).takeIf { it.isNotBlank() } }
					.joinToString(", ")
					.takeIf { it.isNotBlank() }
				if (media != null) "$modelName \u2014 $media" else modelName
			}
			"config" -> fileName(entry.optString("path")).removeSuffix(".uae").ifEmpty { "Config" }
			"whdload" -> fileName(entry.optString("path"))
				.removeSuffix(".lha")
				.removeSuffix(".lzx")
				.removeSuffix(".lzh")
				.ifEmpty { "WHDLoad" }
			else -> "?"
		}
	}

	fun details(entry: JSONObject): Details {
		return when (entry.optString("type")) {
			"quickstart" -> quickStartDetails(entry)
			"config" -> Details(
				title = label(entry),
				typeLabel = "Configuration",
				detail = entry.optString("path")
			)
			"whdload" -> Details(
				title = label(entry),
				typeLabel = "WHDLoad",
				detail = entry.optString("path")
			)
			else -> Details(
				title = "?",
				typeLabel = "Unknown",
				detail = ""
			)
		}
	}

	fun sameEntries(left: List<JSONObject>, right: List<JSONObject>): Boolean =
		left.size == right.size && left.zip(right).all { (a, b) -> a.toString() == b.toString() }

	private fun quickStartDetails(entry: JSONObject): Details {
		val modelName = entry.optString("model", "?")
		val mediaDetails = MEDIA_KEYS
			.mapNotNull { key ->
				val name = fileName(entry.optString(key)).takeIf { it.isNotBlank() } ?: return@mapNotNull null
				"${key.uppercase()}: $name"
			}
		val firstMedia = mediaDetails.firstOrNull()?.substringAfter(": ") ?: modelName
		val detail = if (mediaDetails.isEmpty()) {
			modelName
		} else {
			"$modelName - ${mediaDetails.joinToString(", ")}"
		}
		return Details(
			title = firstMedia,
			typeLabel = "Quick Start",
			detail = detail
		)
	}

	private fun fileName(path: String): String =
		path.substringAfterLast('/').substringAfterLast('\\')

	private val MEDIA_KEYS = listOf("df0", "df1", "cd")
}
