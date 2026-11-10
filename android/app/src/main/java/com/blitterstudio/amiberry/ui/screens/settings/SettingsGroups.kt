package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema

/**
 * Where the editor's pages (SettingsPage) show the settings layer's options,
 * after the Apple app's pages (apple/Shared/UI/SettingsModel.swift,
 * DesignedPages.swift): each page shows designed sections, and a section is
 * either a layer group's options no other section takes or exactly the
 * options it names, so a group can split the way the Apple app splits it
 * (Hardware's CPU rows and its CPU Advanced disclosure). An option the
 * schema lists (settings_schema_options.txt) shows exactly once — in one
 * section of the page that owns it (SettingsGroups.pageFor) — and All
 * Settings lists them all again, by layer group; one the schema has only
 * because a preset sets it does not show, nor one hidden on Android (the
 * schema's hidden_on: settings that do nothing here). The Overview re-shows
 * a few of the Storage page's rows (the first drives) at a glance and
 * Hardware re-shows the Overview's Kickstart picker, as the Apple pages do.
 */
object SettingsGroups {

	/**
	 * A section of a page: the [group]'s listed options no other section or
	 * row takes, or exactly [ids] (a designed split, of one group or more).
	 * [collapsed] starts the section closed — the expert rows, in Apple's
	 * Advanced disclosures; the editor remembers a section opened
	 * (SettingsViewModel.expandedSections), whatever page it is on.
	 */
	data class Placement(
		@param:StringRes val titleRes: Int,
		val group: String? = null,
		val ids: List<String> = emptyList(),
		val collapsed: Boolean = false
	) {
		/** The section's stable identity (a group has one section; an id list is its own), for remembering it open. */
		val key: String get() = group ?: ids.joinToString(",")
	}

	/** The sections each page shows, in order. The Overview's rows are the launcher's own. */
	val byPage: Map<SettingsPage, List<Placement>> = linkedMapOf(
		SettingsPage.Overview to emptyList(),
		// Apple's designed Hardware page (audit §3a): CPU, Chipset, Memory,
		// ROMs and Expansions, the on-board interfaces the layer still lists,
		// then the expert rows in collapsed Advanced sections at the bottom.
		// The accelerator/RTG/PPC groups are preset_only here, so they have no
		// section (their options show nowhere).
		SettingsPage.Hardware to listOf(
			Placement(R.string.settings_group_cpu, group = "cpu"),
			Placement(R.string.settings_group_chipset, group = "chipset"),
			Placement(R.string.settings_group_memory, group = "memory"),
			Placement(R.string.settings_group_rom, group = "rom"),
			Placement(R.string.settings_group_expansions, group = "expansions"),
			Placement(
				R.string.settings_section_on_board_interfaces,
				ids = listOf("pcmcia", "ide", "scsi_a3000", "scsi_a4000t"),
				collapsed = true
			),
			Placement(
				R.string.settings_section_cpu_advanced,
				ids = listOf(
					"cpu_throttle", "cpu_multiplier", "amiberry.cpu_idle", "cpu_threaded",
					"cachesize", "compfpu", "comp_constjump", "comp_flushmode", "comp_nf",
					"comp_catchfault", "comp_trustbyte",
					"cpu_no_unimplemented", "cpu_data_cache", "mmu_model",
					"fpu_strict", "fpu_softfloat", "fpu_no_unimplemented"
				),
				collapsed = true
			),
			Placement(
				R.string.settings_section_chipset_advanced,
				ids = listOf("waiting_blits", "keyboard_type", "keyboard_nkro", "display_optimizations", "hvcsync"),
				collapsed = true
			),
			// The adv_chipset group minus its on-board interfaces: the
			// compatibility internals the presets and chipset_compatible set.
			Placement(R.string.settings_section_chipset_compatibility, group = "adv_chipset", collapsed = true),
			Placement(
				R.string.settings_section_memory_advanced,
				ids = listOf("z3mapping", "a3000mem_size", "mbresmem_size", "megachipmem_size"),
				collapsed = true
			),
			Placement(
				R.string.settings_section_rom_advanced,
				ids = listOf("boot_rom_uae", "uaeboard", "kickshifter"),
				collapsed = true
			),
			Placement(R.string.settings_group_genlock, group = "genlock", collapsed = true)
		),
		SettingsPage.DisplaySound to listOf(
			// The page's own Display card comes first; the technical rows start closed.
			Placement(R.string.settings_section_display_advanced, group = "display", collapsed = true),
			Placement(R.string.settings_group_filter, group = "filter"),
			Placement(R.string.settings_group_sound, group = "sound")
		),
		SettingsPage.Input to listOf(
			Placement(R.string.settings_input_controller_options_title, group = "input"),
			Placement(R.string.settings_input_on_screen_controls_title, group = "onscreen")
		),
		SettingsPage.Storage to listOf(
			// The floppy and cd groups are the Storage page's own drive cards
			// (their rows show inside them); the placements name the page that
			// owns the groups.
			Placement(R.string.settings_group_floppy, group = "floppy"),
			Placement(R.string.settings_group_cd, group = "cd"),
			Placement(R.string.settings_group_filesystem, group = "filesystem")
		),
		// Apple's Ports & Misc sections (audit §3a) on the options the layer
		// offers here: the misc group split by purpose, the io group's dongle
		// its own section.
		SettingsPage.PortsMisc to listOf(
			Placement(R.string.settings_section_status_line, ids = listOf("show_leds", "show_leds_rtg", "power_led_dim")),
			Placement(R.string.settings_section_copy_protection, group = "io"),
			Placement(R.string.settings_section_startup, ids = listOf("cpu_reset_pause", "warpboot")),
			Placement(R.string.settings_section_host_integration, ids = listOf("clipboard_sharing", "synchronize_clock")),
			Placement(R.string.settings_section_keyboard, ids = listOf("amiberry.right_control_is_right_win")),
			Placement(
				R.string.settings_section_retroarch_buttons,
				ids = listOf("amiberry.use_retroarch_quit", "amiberry.use_retroarch_menu", "amiberry.use_retroarch_reset")
			),
			Placement(
				R.string.settings_section_protection,
				ids = listOf("floppy_write_protect", "harddrive_write_protect")
			),
			Placement(
				R.string.settings_section_compatibility,
				ids = listOf("uae_hide_autoconfig", "scsidevice_disable"),
				collapsed = true
			),
			Placement(
				R.string.settings_section_debug,
				ids = listOf("log_illegal_mem", "debug_mem", "cpu_halt_auto_reset"),
				collapsed = true
			)
		)
	)

	/** Every section a page shows; the AllSettings page lists the layer's groups instead, in their order. */
	fun sectionsOf(page: SettingsPage): List<Placement> =
		if (page == SettingsPage.AllSettings) emptyList() else byPage.getValue(page)

	/** The [page]'s section that shows the layer's [group], if it has one. */
	fun placementOf(page: SettingsPage, group: String): Placement? =
		byPage[page]?.firstOrNull { it.group == group }

	/** Every group a page's sections take, in order; the AllSettings page names every group the layer has, in the layer's order. */
	fun groupsOf(page: SettingsPage, schema: SettingsSchema): List<String> =
		if (page == SettingsPage.AllSettings) schema.options.values.map { it.group }.distinct()
		else byPage.getValue(page).mapNotNull { it.group }

	/** The options each page's own rows show (the launcher's own controls for them). */
	val laidOut: Map<SettingsPage, Set<String>> = mapOf(
		// The Kickstart picker; withKickstartRom keeps the extended ROM beside it.
		SettingsPage.Overview to setOf("kickstart_rom_file", "kickstart_ext_rom_file"),
		SettingsPage.Hardware to setOf(
			"cpu_model", "cpu_compatible", "cpu_24bit_addressing", "cpu_speed", "fpu_model",
			"chipset", "chipset_compatible", "cycle_exact", "immediate_blits", "ntsc", "collision_level",
			"chipmem_size", "bogomem_size", "fastmem_size", "z3mem_size",
			// The accelerator's own rows (AcceleratorBoardSection): the
			// board list and the jumpers come from the schema's catalogs.
			"cpuboard_type", "cpuboard_settings", "cpuboardmem1_size", "cpuboard_rom_file"
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
			(0..3).flatMap { listOf("floppy$it", "floppy${it}type") } +
			// The disk swapper's entries (DiskSwapperSection).
			(0 until 20).map { "diskimage$it" },
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

	/**
	 * The options no section's rows show: the pages' own rows, the [managed]
	 * ones and the options a section names by id (its own rows).
	 */
	val placed: Set<String> = laidOut.values.flatten().toSet() + managed +
		byPage.values.flatten().flatMap { it.ids }

	private val idPages: Map<String, SettingsPage> = byPage.entries.flatMap { (page, list) ->
		list.flatMap { placement -> placement.ids.map { it to page } }
	}.toMap()

	private val groupPages: Map<String, SettingsPage> = byPage.entries.flatMap { (page, list) ->
		list.mapNotNull { placement -> placement.group?.let { it to page } }
	}.toMap()

	/**
	 * The page that owns [optionId]: the page whose own rows lay it out,
	 * else the section that names it, else its group's page; null when the
	 * schema does not list the option.
	 */
	fun pageFor(optionId: String): SettingsPage? {
		laidOut.forEach { (page, ids) -> if (optionId in ids) return page }
		return idPages[optionId] ?: groupPages[schemaGroupOf(optionId)]
	}

	private fun schemaGroupOf(optionId: String): String? = SettingsSchema.current?.option(optionId)?.group

	/** The title of a layer group, as All Settings heads the group's rows. */
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
		"diskswapper" -> R.string.settings_group_diskswapper
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
	 * The options [placement]'s section shows: exactly its ids, or its
	 * group's listed options [placed] leaves out, in the layer's order.
	 */
	fun options(schema: SettingsSchema, placement: Placement, placed: Set<String> = SettingsGroups.placed): List<SettingsSchema.Option> =
		if (placement.ids.isNotEmpty()) {
			placement.ids.mapNotNull { schema.option(it) }
				.filter { !it.presetOnly && !it.hiddenOnAndroid && it.id !in managed && rowKind(it) != null }
		} else {
			options(schema, placement.group.orEmpty(), placed)
		}

	/**
	 * The options [group]'s own rows show: the group's listed options
	 * [placed] leaves out, in the layer's order (a page's drive cards, the
	 * All Settings list). All Settings passes no [placed], so it lists every
	 * listed option its row can edit.
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
