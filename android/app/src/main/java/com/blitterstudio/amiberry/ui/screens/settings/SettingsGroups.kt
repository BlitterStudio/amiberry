package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema

/**
 * Where the editor's pages (SettingsPage) show the settings layer's options,
 * after the Apple app's pages (apple/Shared/UI/SettingsModel.swift,
 * DesignedPages.swift): each page shows its own rows first, then a section
 * per layer group for the group's options those rows do not show, in the
 * layer's order. An option the schema lists (settings_schema_options.txt)
 * shows exactly once — on the page that owns it (SettingsGroups.pageFor) —
 * and All Settings lists them all again; one the schema has only because a
 * preset sets it does not show, nor one hidden on Android (the schema's
 * hidden_on: settings that do nothing here). The Overview re-shows a few of the Storage
 * page's rows (the first drives) at a glance, as the Apple Overview does.
 */
object SettingsGroups {

	/** A group's section on a page; [collapsed] starts closed (options few people change). */
	data class Placement(val group: String, val collapsed: Boolean = false)

	/** The groups each page shows, in order. The Overview's rows are the launcher's own. */
	val byPage: Map<SettingsPage, List<Placement>> = linkedMapOf(
		SettingsPage.Overview to emptyList(),
		// Apple's SettingsPage.groups for hardware, without x86 (the launcher's
		// layer has none): the empty sections (ppc, accelerator, rtg) show nothing.
		SettingsPage.Hardware to listOf(
			Placement("cpu"), Placement("mmu"), Placement("fpu"), Placement("cpu_speed"), Placement("jit"),
			Placement("chipset"), Placement("adv_chipset", collapsed = true), Placement("memory"), Placement("rom"),
			Placement("accelerator"), Placement("rtg"), Placement("expansions"), Placement("ppc"),
			Placement("genlock", collapsed = true)
		),
		SettingsPage.DisplaySound to listOf(
			// The page's own Display card comes first; the technical rows start closed.
			Placement("display", collapsed = true),
			Placement("filter"),
			Placement("sound")
		),
		SettingsPage.Input to listOf(Placement("input"), Placement("onscreen")),
		SettingsPage.Storage to listOf(Placement("floppy"), Placement("cd"), Placement("filesystem")),
		SettingsPage.PortsMisc to listOf(Placement("misc"), Placement("io"))
	)

	/** Every group a page shows, in order; [page] AllSettings names them all, in the layer's order. */
	fun groupsOf(page: SettingsPage, schema: SettingsSchema): List<String> =
		if (page == SettingsPage.AllSettings) schema.options.values.map { it.group }.distinct()
		else byPage.getValue(page).map { it.group }

	/** Whether [group]'s section starts closed ([Placement.collapsed]). */
	fun startsCollapsed(group: String): Boolean = byPage.values.any { list -> list.any { it.group == group && it.collapsed } }

	/** The options each page's own rows show (the launcher's own controls for them). */
	val laidOut: Map<SettingsPage, Set<String>> = mapOf(
		// The Kickstart picker; withKickstartRom keeps the extended ROM beside it.
		SettingsPage.Overview to setOf("kickstart_rom_file", "kickstart_ext_rom_file"),
		SettingsPage.Hardware to setOf(
			"cpu_model", "cpu_speed", "cpu_compatible", "cpu_24bit_addressing", "fpu_model",
			"cachesize", "compfpu",
			"chipset", "cycle_exact", "immediate_blits", "ntsc", "collision_level",
			"chipmem_size", "bogomem_size", "fastmem_size", "z3mem_size"
		),
		SettingsPage.DisplaySound to setOf(
			"amiberry.gfx_correct_aspect", "amiberry.gfx_auto_crop", "amiberry.scaling_method", "gfx_autoresolution",
			"amiberry.shader",
			"sound_output", "sound_channels", "sound_frequency", "sound_max_buff", "sound_volume", "sound_volume_paula",
			"sound_volume_cd", "sound_volume_ahi", "sound_volume_midi"
		) + (0..3).flatMap { listOf("floppy${it}sound", "floppy${it}soundvolume_disk", "floppy${it}soundvolume_empty") },
		SettingsPage.Input to setOf(
			"joyport0", "joyport1", "joyport1mousemap", "input.joymouse_speed_analog",
			"amiberry.onscreen_joystick", "amiberry.vkbd_touch_button", "amiberry.vkbd_numpad", "amiberry.vkbd_toggle"
		),
		SettingsPage.Storage to setOf("cdimage0") +
			(0..3).flatMap { listOf("floppy$it", "floppy${it}type") },
		SettingsPage.PortsMisc to emptySet()
	)

	/**
	 * The options the launcher writes itself and never shows: the
	 * on-screen keyboard switch (made from the touch button and the gamepad
	 * toggle), port 0's stick mouse (port 0 holds the mouse, so the
	 * launcher keeps it off) and the emulator's GUI, which a launch always
	 * skips.
	 */
	val managed: Set<String> = setOf("amiberry.vkbd_enabled", "joyport0mousemap", "use_gui")

	/** The options no group section shows: the pages' own rows and the [managed] ones. */
	val placed: Set<String> = laidOut.values.flatten().toSet() + managed

	private val groupPages: Map<String, SettingsPage> = byPage.entries.flatMap { (page, list) ->
		list.map { it.group to page }
	}.toMap()

	/**
	 * The page that owns [optionId]: the page whose own rows lay it out,
	 * else its group's page; null when the schema does not list the option.
	 */
	fun pageFor(optionId: String): SettingsPage? {
		laidOut.forEach { (page, ids) -> if (optionId in ids) return page }
		return groupPages[schemaGroupOf(optionId)]
	}

	private fun schemaGroupOf(optionId: String): String? = SettingsSchema.current?.option(optionId)?.group

	/** The title of a group's section, worded as the Apple app titles it. */
	@StringRes
	fun titleRes(group: String): Int = when (group) {
		"cpu" -> R.string.settings_group_cpu
		"mmu" -> R.string.settings_group_mmu
		"fpu" -> R.string.settings_group_fpu
		"cpu_speed" -> R.string.settings_group_cpu_speed
		"jit" -> R.string.settings_group_jit
		"chipset" -> R.string.settings_group_chipset
		"adv_chipset" -> R.string.settings_group_adv_chipset
		"genlock" -> R.string.settings_group_genlock
		"memory" -> R.string.settings_group_memory
		"rom" -> R.string.settings_group_rom
		"display" -> R.string.settings_group_display
		"filter" -> R.string.settings_group_filter
		"sound" -> R.string.settings_group_sound
		"input" -> R.string.settings_group_input
		"onscreen" -> R.string.settings_group_onscreen
		"floppy" -> R.string.settings_group_floppy
		"cd" -> R.string.settings_group_cd
		"filesystem" -> R.string.settings_group_filesystem
		"misc" -> R.string.settings_group_misc
		"expansions" -> R.string.settings_group_expansions
		"io" -> R.string.settings_group_io
		"ppc" -> R.string.settings_group_ppc
		"rtg" -> R.string.settings_group_rtg
		"accelerator" -> R.string.settings_group_accelerator
		else -> throw IllegalArgumentException("no page places the group $group")
	}

	/** How a group section shows an option. */
	enum class RowKind { Switch, Choice, Level, Number }

	/**
	 * The row for [option]: a switch for a bool, a slider for a level (a
	 * number or choice the layer marks as set by feel), a dropdown for
	 * another choice, a field for another number; null for a type no
	 * group row edits (paths, text, hotkeys).
	 */
	fun rowKind(option: SettingsSchema.Option): RowKind? = when (option.type) {
		SettingsSchema.TYPE_BOOL -> RowKind.Switch
		SettingsSchema.TYPE_CHOICE -> if (option.level) RowKind.Level else RowKind.Choice
		SettingsSchema.TYPE_INT, SettingsSchema.TYPE_REAL -> if (option.level) RowKind.Level else RowKind.Number
		else -> null
	}

	/**
	 * The options [group]'s section shows: the group's listed options
	 * [placed] leaves out, in the layer's order. All Settings passes no
	 * [placed], so it lists every listed option its row can edit.
	 */
	fun options(schema: SettingsSchema, group: String, placed: Set<String> = SettingsGroups.placed): List<SettingsSchema.Option> =
		schema.options.values.filter { it.group == group && !it.presetOnly && !it.hiddenOnAndroid && it.id !in placed && rowKind(it) != null }

	/**
	 * The options the launcher's constraints set together, so no page may
	 * edit one half alone — the constraints would restore it from the other
	 * (the integer scaling pair: EmulatorSettingsConstraints keeps the
	 * scaling method and the resolution autoswitch in step). The first of a
	 * pair is where a page shows its one row for it; the rest follow it.
	 */
	val coupledPairs: List<List<String>> = listOf(
		listOf("amiberry.scaling_method", "gfx_autoresolution")
	)

	/** The coupled pair [id] belongs to, else null. */
	fun coupledPairOf(id: String): List<String>? = coupledPairs.firstOrNull { id in it }

	/**
	 * The options the All Settings page lists for [group] given which option
	 * ids [matches]: the group's rows, a coupled pair as its first member
	 * alone, shown when it or any of its mates matches.
	 */
	fun allSettingsRows(
		schema: SettingsSchema,
		group: String,
		matches: (String) -> Boolean
	): List<SettingsSchema.Option> = options(schema, group, placed = managed).filter { option ->
		val pair = coupledPairOf(option.id)
		if (pair == null) {
			matches(option.id)
		} else {
			option.id == pair.first() && pair.any(matches)
		}
	}


	/**
	 * The memory boards that show once the board before them is fitted
	 * (Apple's HiddenWhenUnavailable.after): most machines have one Zorro
	 * board of a kind or none, so a second to fourth appears only when used.
	 * Board ID to the options of which one must be in use.
	 */
	private val after: Map<String, List<String>> = buildMap {
		for (board in 2..4) {
			val previous = if (board == 2) "" else "${board - 1}"
			val zorro2 = listOf("fastmem${previous}_size", "fastmem${previous}_size_k")
			put("fastmem${board}_size", zorro2)
			put("fastmem${board}_size_k", zorro2)
			put("z3mem${board}_size", listOf("z3mem${previous}_size"))
		}
	}

	/** The JIT cache sizes offered in the cachesize field's menu (stored KB; 0 turns the JIT off). */
	val jitCacheSuggestions = listOf(0, 1024, 2048, 4096, 8192, 16384)

	/** The numbers a number field's menu offers (the JIT cache's sizes). */
	fun numberSuggestions(option: SettingsSchema.Option): List<Int> =
		if (option.id == "cachesize") jitCacheSuggestions else emptyList()

	/**
	 * The numbers a field accepts, shared by every row that edits [id]: the
	 * JIT cache takes 0 (off) or at least 128 KB (cachesize_accepts in
	 * settings_cpu.cpp) — the core silently drops the JIT for anything
	 * between, so no field may store it.
	 */
	fun numberAccepts(id: String): (Int) -> Boolean = when (id) {
		"cachesize" -> { size -> size == 0 || size >= 128 }
		else -> { _ -> true }
	}

	/** [numberAccepts] for an option. */
	fun numberAccepts(option: SettingsSchema.Option): (Int) -> Boolean = numberAccepts(option.id)

	/**
	 * Whether [id] shows, given each option's value ([value]): a board of
	 * [after] shows while it or one of the options before it is in use.
	 */
	fun shows(id: String, value: (String) -> String?): Boolean {
		val before = after[id] ?: return true
		return inUse(value(id)) || before.any { inUse(value(it)) }
	}

	/** Whether a value sets something: not empty, 0 or none. */
	private fun inUse(value: String?): Boolean =
		value?.trim()?.lowercase()?.let { it !in setOf("", "0", "none") } ?: false

	/**
	 * An option's label without the section's name it starts with (the
	 * Apple app's OptionSection.label(_:in:)): "Chipset extra" under
	 * "Chipset" is "Extra". A label that is the section's name, or only
	 * starts with the same letters ("Floppy drive…" under "Floppy
	 * Drives"), stays. TalkBack keeps the whole name (the rows take it as
	 * their semanticsLabel).
	 */
	fun labelIn(label: String, section: String): String {
		val title = if (section.endsWith(":")) section.dropLast(1) else section
		if (title.isEmpty() || label.length <= title.length) return label
		if (!label.lowercase().startsWith(title.lowercase())) return label
		val after = label.substring(title.length)
		if (after.firstOrNull() != ' ' && after.firstOrNull() != ':') return label
		val rest = after.dropWhile { it == ' ' || it == ':' }
		if (rest.isEmpty()) return label
		return rest.replaceFirstChar { it.uppercase() }
	}

	/**
	 * Why the unavailable options are, each reason once in the order the
	 * options come (the Apple app's OptionSection.constraintNotes): a
	 * reason only one option has names it — "DF1 image: Needs…" — unless
	 * it starts with the option's name already.
	 */
	fun constraintNotes(options: List<SettingsSchema.Option>, available: (String) -> Boolean): List<String> {
		val reasons = mutableListOf<String>()
		val labels = HashMap<String, MutableList<String>>()
		for (option in options) {
			if (available(option.id)) continue
			val reason = option.constraint?.takeIf { it.isNotBlank() } ?: continue
			if (reason !in labels) reasons += reason
			labels.getOrPut(reason) { mutableListOf() }.add(option.label)
		}
		return reasons.map { reason ->
			val named = labels[reason].orEmpty()
			if (named.size != 1 || reason.lowercase().startsWith(named[0].lowercase())) reason
			else "${named[0].replace(":", "")}: $reason"
		}
	}
}
