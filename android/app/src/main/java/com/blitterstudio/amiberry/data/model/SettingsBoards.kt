package com.blitterstudio.amiberry.data.model

/**
 * The expansion boards a configuration lists, as the emulator's own
 * configuration writer spells them (cfgfile_write_board_rom): a unit of a
 * board type exists while the file has its `<id>_rom_file` line, and what
 * the user configured about it follows as `<id>_rom_options` — the variant
 * (`subtype=<configname>`), the SCSI ID (`id=<n>`), the switches
 * (`autoboot_disabled=true`, `dma24bit=true`, `inserted=true`) and the
 * board's own settings, whose switches and selector positions the line
 * names and whose text settings it writes as `name=value`.
 *
 * The launcher edits these lines itself: it runs in another process than
 * the emulator, so the schema (board_types) carries the fields instead of
 * the settings session the Apple editor calls. A board type the schema
 * does not know keeps its lines untouched (ConfigParser leaves them
 * verbatim).
 */

/** One unit of an expansion board type the configuration lists. */
data class BoardUnit(
	/** The board type's configuration name (schema board_types id, for example "a2091"). */
	val typeId: String,
	/** The unit's number below the type's count; 0 is the only one of most types. */
	val unit: Int,
	/**
	 * The unit's entry-line value as the configuration keeps it: the ROM
	 * image, or the ":NOROM"/":ENABLED" marker while it has none of its
	 * own. [romFileOf] is what the editor shows.
	 */
	val romFile: String = "",
	/**
	 * Whether the file has the unit's `<id>_rom_file` line (a built-in
	 * controller the chipset option puts in the machine has none until a
	 * ROM is picked).
	 */
	val entry: Boolean = true,
	/** The unit's field values by field id (subtype, scsi_id, setting:...), in .uae spelling. */
	val fields: Map<String, String> = emptyMap(),
	/**
	 * `_rom_options` tokens the launcher does not understand (the network
	 * mode selector, whose positions name the host's drivers): kept as they
	 * came, and written back with the rest.
	 */
	val extras: List<String> = emptyList(),
	/**
	 * The board's `<id>_mem_size` value (its RAM in 256 KB units) as the
	 * configuration had it; null when it had none. Only the first unit of
	 * a type has one (cfgfile.cpp:6486-6491). Kept with the board and gone
	 * with it: a non-zero size alone puts the board back in the machine.
	 */
	val memory: String? = null
) {
	val key: String get() = "$typeId#$unit"
}

object SettingsBoards {

	/** A configuration line about an expansion board unit the schema knows. */
	data class Line(
		val typeId: String,
		val unit: Int,
		val kind: Kind,
		val value: String
	) {
		enum class Kind { Entry, Options, Memory, Identity, Drop }
	}

	/**
	 * The configuration line [key] spells about an expansion board, when
	 * the schema knows its type: the unit's entry (`<id>_rom_file`), its
	 * options (`<id>_rom_options`), the ROM identity the emulator records
	 * (`<id>_rom`, `<id>_rom_file_id`; kept for a unit the codec does not
	 * own, dropped with the layer's rom_file setter otherwise) or a line
	 * the launcher drops outright (the accepted-for-compatibility
	 * `scsi_<id>` switch, whose value the reader ignores).
	 */
	fun line(key: String, schema: SettingsSchema? = SettingsSchema.current): Line? {
		val typeId = schema?.boardTypes ?: return null
		for (type in typeId) {
			for (unit in 0 until type.units) {
				val name = if (unit == 0) type.id else "${type.id}-${unit + 1}"
				when (key) {
					"${name}_rom_file" -> return Line(type.id, unit, Line.Kind.Entry, "")
					"${name}_rom_options" -> return Line(type.id, unit, Line.Kind.Options, "")
					"${name}_rom", "${name}_rom_file_id" -> return Line(type.id, unit, Line.Kind.Identity, "")
					"${type.id}_mem_size" -> if (unit == 0) return Line(type.id, 0, Line.Kind.Memory, "")
					"scsi_${type.id}" -> return Line(type.id, 0, Line.Kind.Drop, "")
				}
			}
		}
		return null
	}

	/** The type's units the configuration lists, parsed from its lines' [values]. */
	fun parse(schema: SettingsSchema, values: Map<String, String>): List<BoardUnit> {
		val units = LinkedHashMap<String, MutableBoard>()
		for (type in schema.boardTypes) {
			for (unit in 0 until type.units) {
				val name = if (unit == 0) type.id else "${type.id}-${unit + 1}"
				val entry = values["${name}_rom_file"]
				val options = values["${name}_rom_options"]
				val memory = if (unit == 0) values["${type.id}_mem_size"] else null
				// A unit exists while the file has its entry line, or its
				// options line (a built-in controller the chipset option
				// puts in the machine, configured without a ROM of its own),
				// or a non-zero memory size (which makes the board,
				// cfgfile.cpp:6488-6490). An empty entry is none:
				// cfgfile_read_board_rom creates the board only for a
				// non-empty value (cfgfile.cpp:6381-6388).
				val hasEntry = !entry.isNullOrEmpty()
				val hasMemory = memory != null && memory.trim().toIntOrNull() != 0
				if (!hasEntry && options == null && !hasMemory) continue
				val board = units.getOrPut("${type.id}#$unit") { MutableBoard() }
				board.entry = hasEntry
				board.romFile = entry.orEmpty()
				board.memory = memory
				val tokens = splitTokens(options ?: "")
				for (field in type.fields) {
					if (field.id == "enabled" || field.id == "rom_file") continue
					val value = fieldValue(field, tokens) ?: continue
					board.fields[field.id] = value
				}
				// Tokens no field of the type takes ride along unchanged.
				board.extras = tokens.filter { token -> token.isNotEmpty() && !takenBy(type, token) }
			}
		}
		return units.map { (key, board) ->
			val (typeId, unit) = key.split('#')
			BoardUnit(typeId, unit.toInt(), board.romFile, board.entry, board.fields, board.extras, board.memory)
		}
	}

	private class MutableBoard {
		var entry: Boolean = true
		var romFile: String = ""
		var fields = LinkedHashMap<String, String>()
		var extras: List<String> = emptyList()
		var memory: String? = null
	}

	/**
	 * The unit's configuration lines, in the writer's order: the entry, the
	 * options of a unit anything is set on, and no line for a unit the
	 * schema no longer knows (a removed type keeps nothing).
	 */
	fun render(schema: SettingsSchema, units: List<BoardUnit>): List<String> {
		val lines = mutableListOf<String>()
		for (unit in units.sortedWith(compareBy({ it.typeId }, { it.unit }))) {
			val type = schema.boardType(unit.typeId) ?: continue
			val name = if (unit.unit == 0) type.id else "${type.id}-${unit.unit + 1}"
			val options = renderOptions(type, unit)
			// A configured board the chipset option puts in the machine, but
			// whose entry the reader only builds after the whole file parses
			// (cdtvcd, unlike pcmcia, which makes it while parsing the
			// option, cfgfile.cpp:6936-6940), needs the entry line first:
			// the options are read with get_device_rom, which only finds
			// what exists (cfgfile.cpp:6415). The option's own entry marker
			// (addbcromtype's ":ENABLED", cfgfile.cpp:6504) is what the
			// writer keeps for it.
			if (options.isNotEmpty() && !unit.entry && unit.romFile.isEmpty())
				lines += "${name}_rom_file=:ENABLED"
			if (unit.entry || unit.romFile.isNotEmpty())
				lines += "${name}_rom_file=${unit.romFile.ifEmpty { entryValue(type, unit.romFile) }}"
			if (options.isNotEmpty()) lines += "${name}_rom_options=$options"
			unit.memory?.let { lines += "${type.id}_mem_size=$it" }
		}
		return lines
	}

	/**
	 * The unit's `_rom_options` value: the fields the layer's default
	 * leaves out, then the tokens the configuration had that no field of
	 * the type takes.
	 */
	private fun renderOptions(type: SettingsSchema.BoardType, unit: BoardUnit): String {
		val tokens = mutableListOf<String>()
		for (field in type.fields) {
			when (field.id) {
				"enabled", "rom_file" -> {}
				"subtype" -> unit.fields[field.id]?.let { tokens += "subtype=$it" }
				"scsi_id" -> unit.fields[field.id]?.let { tokens += "id=$it" }
				"autoboot_disabled", "dma24bit", "inserted" ->
					if (unit.fields[field.id] == "true") tokens += "${field.id}=true"
				else -> {
					val value = unit.fields[field.id] ?: field.default
					when (field.type) {
						SettingsSchema.TYPE_BOOL -> if (value == "true") tokens += settingToken(field)
						SettingsSchema.TYPE_CHOICE -> {
							// A selector's positions are bare tokens of their own, whichever is chosen
							// (cfgfile_write_rom_settings appends the position's name,
							// cfgfile.cpp:1809-1812); the first is the line's absence.
							if (value != field.default && field.choices.any { it.value == value })
								tokens += value
						}
						else -> if (value.isNotEmpty()) tokens += "${settingToken(field)}=${escapeMin(value)}"
					}
				}
			}
		}
		tokens += unit.extras
		return tokens.joinToString(",")
	}

	/**
	 * The token the field's own name spells (a switch, or a text setting's
	 * name): the configuration name, without the layer's index suffix for
	 * a setting a table names twice (the A1060 sidecar's two memory
	 * selectors), which is addressing only.
	 */
	private fun settingToken(field: SettingsSchema.BoardField): String =
		field.id.removePrefix("setting:").substringBefore('#')

	/**
	 * The field's value in [tokens], or null when the line leaves it at the
	 * default: a switch is on while its name is a bare token, a selector
	 * while one of its positions is, a text setting as `name=value`.
	 */
	private fun fieldValue(field: SettingsSchema.BoardField, tokens: List<String>): String? = when (field.id) {
		"subtype" -> tokenValue(tokens, "subtype")
		"scsi_id" -> tokenValue(tokens, "id")
		// The writer spells these three as name=true (cfgfile_option_bool),
		// a switch only while on.
		"autoboot_disabled", "dma24bit", "inserted" ->
			if (tokenValue(tokens, field.id) in setOf("true", "1")) "true"
			else if (field.default == "true") "false" else null
		else -> when (field.type) {
			SettingsSchema.TYPE_BOOL -> if (bareToken(tokens, settingToken(field))) "true"
				else if (field.default == "true") "false" else null
			SettingsSchema.TYPE_CHOICE -> {
				val position = field.choices.firstOrNull { choice -> bareToken(tokens, choice.value) }?.value
				position ?: if (field.default.isNotEmpty()) null else field.default
			}
			else -> tokenValue(tokens, settingToken(field)) ?: if (field.default.isNotEmpty()) null else field.default
		}
	}

	/**
	 * Whether [token] is one a field of [type] takes (bare or as
	 * name=value). Tokens no field takes ride along: the emulator reads a
	 * few for every board whatever the layer lists (the PCMCIA `inserted`,
	 * the custom boards' `mid` and `pid`), and what a configuration set
	 * keeps working.
	 */
	private fun takenBy(type: SettingsSchema.BoardType, token: String): Boolean {
		val name = token.substringBefore('=')
		for (field in type.fields) {
			when (field.id) {
				"subtype" -> if (name == "subtype") return true
				"scsi_id" -> if (name == "id") return true
				// A choice setting's positions are bare tokens (below); a text
			// or switch setting claims its own name.
			else -> if (name == settingToken(field) && field.type != SettingsSchema.TYPE_CHOICE) return true
			}
		}
		// A selector's positions are bare tokens of their own.
		if (!token.contains('=')) for (field in type.fields) {
			if (field.type == SettingsSchema.TYPE_CHOICE && field.choices.any { it.value == token }) return true
		}
		return false
	}

	/** The line's tokens: comma-separated, a quoted part one token. */
	internal fun splitTokens(options: String): List<String> {
		val tokens = mutableListOf<String>()
		val current = StringBuilder()
		var quoted = false
		var any = false
		for (char in options) {
			when {
				char == '"' && current.isNotEmpty() && current.last() == '\\' -> {
					current.setLength(current.length - 1)
					current.append('"')
				}
				char == '"' -> quoted = !quoted
				char == ',' && !quoted -> {
					tokens += current.toString()
					current.setLength(0)
					any = false
				}
				else -> {
					current.append(char)
					any = true
				}
			}
		}
		if (any || current.isNotEmpty()) tokens += current.toString()
		return tokens
	}

	private fun bareToken(tokens: List<String>, name: String): Boolean = name in tokens

	private fun tokenValue(tokens: List<String>, name: String): String? =
		tokens.firstOrNull { it.startsWith("$name=") }?.substringAfter('=')

	/**
	 * A text value as the configuration line writes it (cfgfile_escape_min):
	 * quoted when it has a comma or a quote, or starts or ends with a space.
	 */
	internal fun escapeMin(text: String): String {
		val quote = '"'
		val mustQuote = text.any { it == ',' || it == quote } || text.first() == ' ' || text.last() == ' '
		if (!mustQuote) return text
		// As cfgfile_escape with min: quoted, a quote escaped as backslash-quote,
		// a backslash literal (a file name cannot end with one).
		val escapedQuote = "\\" + quote
		return quote + text.replace(quote.toString(), escapedQuote) + quote
	}

	/**
	 * A ROM-less board type's entry marker: the type without a rom_file
	 * field exists as `<id>_rom_file=:ENABLED`, one with a ROM as :NOROM
	 * while the user has picked none (the emulator loads the scanned ROM of
	 * the type, which the configuration then keeps).
	 */
	fun entryMarker(type: SettingsSchema.BoardType): String =
		if (type.field("rom_file") == null) ":ENABLED" else ":NOROM"

	/**
	 * The entry line's value of a unit's [romFile]: the marker while no ROM
	 * is picked, the file as it is.
	 */
	fun entryValue(type: SettingsSchema.BoardType, romFile: String): String =
		if (romFile.isEmpty()) entryMarker(type) else romFile

	/** A parsed entry value as the editor's rom_file field holds it. */
	fun romFileOf(entry: String): String = when (entry) {
		":NOROM", ":ENABLED" -> ""
		else -> entry
	}
}

/**
 * The accelerator board's jumpers, as the one configuration line that
 * keeps them spells it: `cpuboard_settings=` names every switch that is on
 * and every selector's position, a text setting as `name=value`
 * (cfgfile_read/write_rom_settings). One jumper is no configuration key of
 * its own, so the launcher holds each as its own value and packs the line
 * when it writes the file.
 */
object AcceleratorJumpers {

	/**
	 * The jumpers of [board] the [values] set, as the native writer spells
	 * the cpuboard_settings line: none while every jumper is at its default
	 * (the settings word is 0, cfgfile.cpp:3404), else every selector's
	 * position, the first one too (cfgfile.cpp:1785-1814), each switch that
	 * is on and each text setting with a value.
	 */
	fun pack(board: SettingsSchema.AcceleratorBoard, values: Map<String, String>): String {
		val tokens = mutableListOf<String>()
		var set = false
		for (jumper in board.settings) {
			when {
				jumper.selector -> {
					val value = values[jumper.name]?.takeIf { it in jumper.positions } ?: boardDefault(jumper)
					if (value.isNotEmpty()) tokens += value
					if (value != boardDefault(jumper)) set = true
				}
				jumper.type == "text" -> values[jumper.name]?.takeIf { it.isNotEmpty() }?.let {
					tokens += "${jumper.name}=${SettingsBoards.escapeMin(it)}"
					set = true
				}
				else -> if (values[jumper.name] == "true") {
					tokens += jumper.name
					set = true
				}
			}
		}
		return if (set) tokens.joinToString(",") else ""
	}

	/** The jumpers of [board] as the packed [line] sets them: the line's tokens by display value. */
	fun unpack(board: SettingsSchema.AcceleratorBoard, line: String): Map<String, String> {
		if (line.isEmpty()) return emptyMap()
		val tokens = SettingsBoards.splitTokens(line)
		val values = LinkedHashMap<String, String>()
		for (jumper in board.settings) {
			when {
				jumper.selector -> values[jumper.name] =
					jumper.positions.firstOrNull { it in tokens } ?: boardDefault(jumper)
				jumper.type == "text" -> values[jumper.name] =
					tokens.firstOrNull { it.startsWith("${jumper.name}=") }?.substringAfter('=') ?: ""
				else -> values[jumper.name] = if (jumper.name in tokens) "true" else "false"
			}
		}
		return values
	}

	/** A selector's first position, the line's absence. */
	private fun boardDefault(jumper: SettingsSchema.AcceleratorJumper): String =
		jumper.positions.firstOrNull() ?: ""
}

/** The editor's board units: the configuration's and the built-in ones the machine's chipset options put in. */
object BoardEditor {

	/** The units the editor lists: [boards]' own, then the internal boards whose option is on and no unit holds them. */
	fun units(schema: SettingsSchema, settings: EmulatorSettings): List<BoardUnit> {
		val units = settings.boards.toMutableList()
		for (type in schema.boardTypes) {
			if (type.option == null) continue
			val on = settings.optionValue(type.option) == "true"
			if (on && units.none { it.typeId == type.id }) units += BoardUnit(type.id, 0, "", entry = false)
		}
		return units.sortedWith(compareBy({ it.typeId }, { it.unit }))
	}

	/** The unit's title: the type's label, with its number when the type takes several. */
	fun title(schema: SettingsSchema, unit: BoardUnit): String {
		val type = schema.boardType(unit.typeId)
		val label = type?.label ?: unit.typeId
		return if ((type?.units ?: 1) > 1) "$label #${unit.unit + 1}" else label
	}

	/** The type's first unit the configuration does not list; null when all are in use or the schema does not know it. */
	fun freeUnit(schema: SettingsSchema, settings: EmulatorSettings, typeId: String): Int? {
		val type = schema.boardType(typeId) ?: return null
		return (0 until type.units).firstOrNull { unit -> settings.boards.none { it.typeId == typeId && it.unit == unit } }
	}

	/**
	 * The types the Add menu offers, in the layer's order: every type but
	 * the built-in controllers a machine's chipset puts in. An optional
	 * board of the internal category (the CDTV's SRAM card and SCSI
	 * expansion) is added by its entry line like any other.
	 */
	fun addable(schema: SettingsSchema): List<SettingsSchema.BoardType> =
		schema.boardTypes.filter { !it.builtIn }

	/** The settings with a unit of [typeId] added (its entry line marks it in the machine). */
	fun add(settings: EmulatorSettings, schema: SettingsSchema, typeId: String): EmulatorSettings {
		val unit = freeUnit(schema, settings, typeId) ?: return settings
		return settings.copy(boards = settings.boards + BoardUnit(typeId, unit, ""))
	}

	/**
	 * The settings with [unit] removed: a board of the configuration loses
	 * its lines; a built-in controller switches off through the chipset
	 * option the layer owns (a machine that still has it stays with it).
	 */
	fun remove(settings: EmulatorSettings, schema: SettingsSchema, unit: BoardUnit): EmulatorSettings {
		val type = schema.boardType(unit.typeId)
		val own = settings.boards.firstOrNull { it.typeId == unit.typeId && it.unit == unit.unit }
		if (type?.option != null) {
			// The option carries the board; its lines go with it.
			var next = if (own != null) settings.copy(boards = settings.boards - own) else settings
			if (settings.optionValue(type.option) == "true")
				next = next.withOption(type.option, "false")
			return next
		}
		if (own == null) return settings
		return settings.copy(boards = settings.boards - own)
	}

	/** Whether the unit can be removed: a built-in controller only through a machine that no longer has it. */
	fun removable(schema: SettingsSchema, settings: EmulatorSettings, unit: BoardUnit): Boolean {
		val type = schema.boardType(unit.typeId) ?: return false
		if (type.option == null) return true
		// The option switches it; the layer's rules say whether it can go off.
		return settings.optionValue(type.option) == "true"
	}

	/**
	 * The settings with the unit's [fieldId] set to [value] (.uae spelling;
	 * a field's default leaves it out). The unit joins the configuration
	 * when it was only the chipset option's (editing it writes its lines).
	 */
	fun setField(
		settings: EmulatorSettings,
		schema: SettingsSchema,
		unit: BoardUnit,
		fieldId: String,
		value: String
	): EmulatorSettings {
		val type = schema.boardType(unit.typeId) ?: return settings
		val field = type.field(fieldId) ?: return settings
		val default = field.default
		val own = settings.boards.firstOrNull { it.typeId == unit.typeId && it.unit == unit.unit }
			?: unit // the internal board's synthetic unit: materialize it
		val nextFields = if (value == default) own.fields - fieldId else own.fields + (fieldId to value)
		if (field.type == "string" && !textsFit(type, nextFields)) return settings
		val next = if (fieldId == "rom_file") own.copy(romFile = value) else own.copy(fields = nextFields)
		return settings.copy(boards = (settings.boards - own) + next)
	}

	/** The size of a board's text settings buffer, NUL terminators included (ROMCONFIG_CONFIGTEXT_LEN). */
	private const val CONFIG_TEXT_LEN = 256

	/**
	 * Whether a unit's text settings fit the one buffer the emulator keeps
	 * them in, each with its terminator: the reader copies them without a
	 * bound (cfgfile_read_rom_settings), so the editor takes no text that
	 * would not, as the layer's put_texts refuses one.
	 */
	fun textsFit(type: SettingsSchema.BoardType, fields: Map<String, String>): Boolean =
		type.fields.filter { it.type == "string" && it.id.startsWith("setting:") }
			.sumOf { (fields[it.id] ?: it.default).toByteArray(Charsets.UTF_8).size + 1 } <= CONFIG_TEXT_LEN

	/**
	 * The units as the change summary shows them, joined: each with what is
	 * configured about it (its ROM and the fields away from their
	 * defaults), so an edit the board's own rows made reads as a change
	 * and Review changes and Revert stay available.
	 */
	fun summary(schema: SettingsSchema, settings: EmulatorSettings): String =
		units(schema, settings).joinToString(" · ") { unit -> describe(schema, unit) }

	/** One unit with what is configured about it. */
	private fun describe(schema: SettingsSchema, unit: BoardUnit): String {
		val type = schema.boardType(unit.typeId) ?: return unit.typeId
		val configured = type.fields.mapNotNull { field ->
			when (field.id) {
				"enabled" -> null
				"rom_file" -> SettingsBoards.romFileOf(unit.romFile).takeIf { it.isNotEmpty() }
					?.let { it.substringAfterLast('/') }
				else -> unit.fields[field.id]?.takeIf { it != field.default }
					?.let { "${field.label} ${field.valueLabel(it)}" }
			}
		}
		val title = title(schema, unit)
		return if (configured.isEmpty()) title else "$title (${configured.joinToString(", ")})"
	}
}
