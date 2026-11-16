package com.blitterstudio.amiberry.data.model

/** The RTG editor's split controls packed into cfgfile's two RTG entries. */
object RtgConfig {
	private val modeFormats = mapOf(
		"rtg_modes:8bit" to mapOf("CLUT" to 0x2),
		"rtg_modes:16bit" to mapOf(
			"R5G6B5PC" to 0x10, "R5G5B5PC" to 0x20, "R5G6B5" to 0x400,
			"R5G5B5" to 0x800, "B5G6R5PC" to 0x1000, "B5G5R5PC" to 0x2000
		),
		"rtg_modes:24bit" to mapOf("R8G8B8" to 0x4, "B8G8R8" to 0x8),
		"rtg_modes:32bit" to mapOf("A8R8G8B8" to 0x40, "A8B8G8R8" to 0x80, "R8G8B8A8" to 0x100, "B8G8R8A8" to 0x200)
	)

	fun parent(id: String): String? = when {
		id in modeFormats -> "rtg_modes"
		id in setOf("gfxcard_options:monitor", "gfxcard_options:autoswitch", "gfxcard_options:initial") -> "gfxcard_options"
		else -> null
	}

	fun pack(parent: String, existing: String, id: String, value: String): String = when (parent) {
		"rtg_modes" -> packModes(existing, id, value)
		"gfxcard_options" -> packBoardOptions(existing, id, value)
		else -> existing
	}

	fun unpack(options: Map<String, String>): Map<String, String> {
		val result = LinkedHashMap(options)
		options["rtg_modes"]?.let { modes ->
			modeFormats.keys.forEach { id -> result[id] = unpackMode(modes, id) }
		}
		options["gfxcard_options"]?.let { packed ->
			val values = packed.split(',').filter { it.isNotEmpty() }.toSet()
			result["gfxcard_options:monitor"] = values.firstOrNull { it.startsWith("monitor=") }
				?.substringAfter('=') ?: "0"
			result["gfxcard_options:autoswitch"] = (!"noautoswitch".let(values::contains)).toString()
			result["gfxcard_options:initial"] = values.contains("initial").toString()
		}
		return result
	}

	fun value(parent: String, packed: String, id: String): String? = when (parent) {
		"rtg_modes" -> unpackMode(packed, id)
		"gfxcard_options" -> {
			val values = packed.split(',').filter { it.isNotEmpty() }.toSet()
			when (id) {
				"gfxcard_options:monitor" -> values.firstOrNull { it.startsWith("monitor=") }
					?.substringAfter('=') ?: "0"
				"gfxcard_options:autoswitch" -> (!values.contains("noautoswitch")).toString()
				"gfxcard_options:initial" -> values.contains("initial").toString()
				else -> null
			}
		}
		else -> null
	}

	private fun modeValue(text: String): Int =
		text.toIntOrNull() ?: text.removePrefix("0x").removePrefix("0X").toIntOrNull(16) ?: 0

	private fun packModes(existing: String, id: String, value: String): String {
		val formats = modeFormats.getValue(id)
		val mask = formats.values.fold(0) { result, bit -> result or bit }
		val current = modeValue(existing)
		val selected = when (value) {
			"none" -> 0
			"all" -> mask
			else -> formats[value] ?: 0
		}
		return "0x${((current and mask.inv()) or selected).toString(16)}"
	}

	private fun unpackMode(existing: String, id: String): String {
		val formats = modeFormats.getValue(id)
		val current = modeValue(existing)
		val selected = formats.filterValues { current and it != 0 }.keys
		return when {
			selected.isEmpty() -> "none"
			selected.size == formats.size && formats.size > 1 -> "all"
			else -> selected.first()
		}
	}

	private fun packBoardOptions(existing: String, id: String, value: String): String {
		val values = existing.split(',').filter { it.isNotEmpty() }.toMutableSet()
		when (id) {
			"gfxcard_options:monitor" -> {
				values.removeAll { it.startsWith("monitor=") }
				if (value != "0") values += "monitor=$value"
			}
			"gfxcard_options:autoswitch" -> {
				values.remove("noautoswitch")
				if (value != "true") values += "noautoswitch"
			}
			"gfxcard_options:initial" -> {
				values.remove("initial")
				if (value == "true") values += "initial"
			}
		}
		return values.sortedWith(compareBy({ when {
			it.startsWith("order=") -> 0
			it.startsWith("monitor=") -> 1
			it == "noautoswitch" -> 2
			it == "initial" -> 3
			else -> 4
		} }, { it })).joinToString(",")
	}
}
