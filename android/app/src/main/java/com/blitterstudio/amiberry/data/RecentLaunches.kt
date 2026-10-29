package com.blitterstudio.amiberry.data

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaModel
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

object RecentLaunches {
	const val MAX_ITEMS = 10

	/** What kind of launch an entry repeats; [labelRes] names it for the user. */
	enum class Type(@param:StringRes val labelRes: Int) {
		QuickStart(R.string.recent_type_quick_start),
		Configuration(R.string.recent_type_configuration),
		WhdLoad(R.string.recent_type_whdload),
		Rp9(R.string.recent_type_rp9),
		Unknown(R.string.recent_type_unknown);

		companion object {
			fun of(entry: JSONObject): Type = when (entry.optString("type")) {
				"quickstart" -> QuickStart
				"config" -> Configuration
				"whdload" -> WhdLoad
				"rp9" -> Rp9
				else -> Unknown
			}
		}
	}

	/** Whether an entry can be launched again, and if not, why. */
	enum class Status {
		Available,

		/** A file the launch used (configuration, game, disk or CD image) is gone. */
		MissingFiles,

		/** The Quick Start model has no installed Kickstart ROM. */
		ModelUnavailable
	}

	/**
	 * What a Recent row shows: a title, the kind of launch, a short detail (file name or
	 * model), never a full path, and when it was last launched (null for entries stored
	 * before the launcher recorded it).
	 */
	data class Details(
		val title: String,
		val type: Type,
		val detail: String,
		val launchedAtMs: Long? = null
	)

	/** One row of the Recent list. */
	data class Item(
		val entry: JSONObject,
		val details: Details,
		val status: Status
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

	/** [entry] first, launched at [nowMs], without its earlier launch. */
	fun add(
		entries: List<JSONObject>,
		entry: JSONObject,
		maxItems: Int = MAX_ITEMS,
		nowMs: Long = System.currentTimeMillis()
	): List<JSONObject> {
		val stamped = JSONObject(entry.toString()).put(LAUNCHED_AT, nowMs)
		val identity = identity(stamped)
		return (listOf(stamped) + entries.filterNot { identity(it) == identity }).take(maxItems)
	}

	/** [entries] without [entry] (in any model ID spelling it was stored with). */
	fun remove(entries: List<JSONObject>, entry: JSONObject): List<JSONObject> {
		val identity = identity(entry)
		return entries.filterNot { identity(it) == identity }
	}

	/**
	 * [entries] with configuration launches of [oldPath] pointing at [newPath], so a renamed
	 * configuration stays in Recent instead of showing as gone.
	 */
	fun replaceConfigPath(entries: List<JSONObject>, oldPath: String, newPath: String): List<JSONObject> =
		entries
			.map { entry ->
				if (Type.of(entry) == Type.Configuration && ConfigFiles.sameConfigPath(entry.optString("path"), oldPath)) {
					JSONObject(entry.toString()).put("path", newPath)
				} else {
					entry
				}
			}
			.distinctBy { identity(it) }

	/**
	 * [entries] without launches of the configuration at [path]: one the user deleted in the app
	 * leaves Recent, while files that vanish outside the app stay listed as missing.
	 */
	fun removeConfigPath(entries: List<JSONObject>, path: String): List<JSONObject> =
		entries.filterNot { entry ->
			Type.of(entry) == Type.Configuration && ConfigFiles.sameConfigPath(entry.optString("path"), path)
		}

	// The same launch whatever model ID spelling it was stored with ("A500"
	// before the launcher used the emulator's presets, "a500" since) and
	// whenever it was launched.
	private fun identity(entry: JSONObject): String {
		val launch = JSONObject(entry.toString()).apply { remove(LAUNCHED_AT) }
		if (launch.optString("type") != "quickstart") return launch.toString()
		val model = AmigaModel.find(launch.optString("model")) ?: return launch.toString()
		return launch.put("model", model.cmdArg).toString()
	}

	/**
	 * The rows to show, most recent first, up to [limit]. Entries that can no longer be
	 * launched stay in the list with their [Status], so the user sees and removes them;
	 * only entries of an unknown kind are left out.
	 */
	fun items(
		entries: List<JSONObject>,
		availableModels: Collection<AmigaModel>,
		fileExists: (String) -> Boolean = { File(it).exists() },
		limit: Int = MAX_ITEMS
	): List<Item> =
		entries
			.filter { Type.of(it) != Type.Unknown }
			.take(limit)
			.map { entry -> Item(entry, details(entry), status(entry, availableModels, fileExists)) }

	fun status(
		entry: JSONObject,
		availableModels: Collection<AmigaModel>,
		fileExists: (String) -> Boolean = { File(it).exists() }
	): Status {
		return when (Type.of(entry)) {
			Type.Configuration, Type.WhdLoad, Type.Rp9 -> {
				val path = entry.optString("path")
				if (path.isNotBlank() && fileExists(path)) Status.Available else Status.MissingFiles
			}
			Type.QuickStart -> {
				val mediaPresent = MEDIA_KEYS.all { key ->
					val path = entry.optString(key)
					path.isBlank() || fileExists(path)
				}
				val model = AmigaModel.find(entry.optString("model"))
				when {
					!mediaPresent -> Status.MissingFiles
					model == null || model !in availableModels -> Status.ModelUnavailable
					else -> Status.Available
				}
			}
			Type.Unknown -> Status.MissingFiles
		}
	}

	fun label(entry: JSONObject): String {
		return when (Type.of(entry)) {
			Type.QuickStart -> {
				val modelName = modelName(entry)
				val media = MEDIA_KEYS
					.mapNotNull { key -> fileName(entry.optString(key)).takeIf { it.isNotBlank() } }
					.joinToString(", ")
					.takeIf { it.isNotBlank() }
				if (media != null) "$modelName \u2014 $media" else modelName
			}
			Type.Configuration -> stripSuffixCaseInsensitive(fileName(entry.optString("path")), ".uae")
				.ifEmpty { "Config" }
			Type.WhdLoad -> ARCHIVE_SUFFIXES
				.fold(fileName(entry.optString("path"))) { name, suffix -> stripSuffixCaseInsensitive(name, suffix) }
				.ifEmpty { "WHDLoad" }
			Type.Rp9 -> stripSuffixCaseInsensitive(fileName(entry.optString("path")), ".rp9")
				.ifEmpty { "RP9" }
			Type.Unknown -> "?"
		}
	}

	fun details(entry: JSONObject): Details {
		val type = Type.of(entry)
		val details = when (type) {
			Type.QuickStart -> quickStartDetails(entry)
			Type.Configuration, Type.Rp9 -> Details(
				title = label(entry),
				type = type,
				detail = fileName(entry.optString("path"))
			)
			Type.WhdLoad -> Details(
				title = WhdLoadTitles.clean(fileName(entry.optString("path"))).ifEmpty { "WHDLoad" },
				type = type,
				detail = fileName(entry.optString("path"))
			)
			Type.Unknown -> Details(title = "?", type = type, detail = "")
		}
		return details.copy(launchedAtMs = entry.optLong(LAUNCHED_AT, 0L).takeIf { it > 0L })
	}

	private fun quickStartDetails(entry: JSONObject): Details {
		val modelName = modelName(entry)
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
			type = Type.QuickStart,
			detail = detail
		)
	}

	private fun modelName(entry: JSONObject): String {
		val storedModel = entry.optString("model", "?")
		return AmigaModel.find(storedModel)?.displayName ?: storedModel
	}

	private fun fileName(path: String): String =
		path.substringAfterLast('/').substringAfterLast('\\')

	private fun stripSuffixCaseInsensitive(value: String, suffix: String): String =
		if (value.endsWith(suffix, ignoreCase = true)) value.dropLast(suffix.length) else value

	private val MEDIA_KEYS = listOf("df0", "df1", "cd")
	private val ARCHIVE_SUFFIXES = listOf(".lha", ".lzx", ".lzh")

	/** When the entry was last launched, epoch milliseconds. */
	private const val LAUNCHED_AT = "time"
}
