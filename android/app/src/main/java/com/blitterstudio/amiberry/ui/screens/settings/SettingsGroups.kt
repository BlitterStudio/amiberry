package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema

/**
 * Where the Settings tabs show the settings layer's options, after the
 * Apple app's pages (apple/Shared/UI/SettingsModel.swift, DesignedPages.swift)
 * fitted to the launcher's tabs: each tab shows its own rows first, then a
 * section per layer group for the group's options those rows do not show,
 * in the layer's order. An option the schema lists (settings_schema_options.txt)
 * shows exactly once; one it has only because a preset sets it does not show.
 */
object SettingsGroups {

	/** A group's section on a tab; [collapsed] starts closed (options few people change). */
	data class Placement(val group: String, val collapsed: Boolean = false)

	/** The groups each tab shows, in order. */
	val byTab: Map<SettingsTab, List<Placement>> = linkedMapOf(
		SettingsTab.Cpu to listOf(Placement("cpu"), Placement("mmu"), Placement("fpu"), Placement("cpu_speed"), Placement("jit")),
		SettingsTab.Chipset to listOf(
			Placement("chipset"),
			Placement("adv_chipset", collapsed = true),
			Placement("genlock", collapsed = true)
		),
		SettingsTab.Memory to listOf(Placement("memory"), Placement("rom")),
		// The tab's own Display card comes first; the twenty more technical rows start closed.
		SettingsTab.Display to listOf(Placement("display", collapsed = true), Placement("filter")),
		SettingsTab.Sound to listOf(Placement("sound")),
		SettingsTab.Input to listOf(Placement("input"), Placement("onscreen")),
		SettingsTab.Storage to listOf(Placement("floppy"), Placement("cd"), Placement("filesystem")),
		SettingsTab.Misc to listOf(Placement("misc"), Placement("expansions"), Placement("io"))
	)

	/** Whether [group]'s section starts closed ([Placement.collapsed]). */
	fun startsCollapsed(group: String): Boolean = byTab.values.any { list -> list.any { it.group == group && it.collapsed } }

	/** The options each tab's own rows show (the launcher's own controls for them). */
	val laidOut: Map<SettingsTab, Set<String>> = mapOf(
		SettingsTab.Cpu to setOf(
			"cpu_model", "cpu_speed", "cpu_compatible", "cpu_24bit_addressing", "fpu_model", "cachesize", "compfpu"
		),
		SettingsTab.Chipset to setOf("chipset", "cycle_exact", "immediate_blits", "ntsc", "collision_level"),
		SettingsTab.Memory to setOf("chipmem_size", "bogomem_size", "fastmem_size", "z3mem_size"),
		SettingsTab.Display to setOf(
			"amiberry.gfx_correct_aspect", "amiberry.gfx_auto_crop", "amiberry.scaling_method", "gfx_autoresolution",
			"amiberry.shader"
		),
		SettingsTab.Sound to setOf(
			"sound_output", "sound_channels", "sound_frequency", "sound_max_buff", "sound_volume", "sound_volume_paula",
			"sound_volume_cd", "sound_volume_ahi", "sound_volume_midi"
		) + (0..3).flatMap { listOf("floppy${it}sound", "floppy${it}soundvolume_disk", "floppy${it}soundvolume_empty") },
		SettingsTab.Input to setOf(
			"joyport0", "joyport1", "joyport1mousemap", "input.joymouse_speed_analog",
			"amiberry.onscreen_joystick", "amiberry.vkbd_touch_button", "amiberry.vkbd_numpad", "amiberry.vkbd_toggle"
		),
		SettingsTab.Storage to setOf("kickstart_rom_file", "kickstart_ext_rom_file", "cdimage0") +
			(0..3).flatMap { listOf("floppy$it", "floppy${it}type") }
	)

	/**
	 * The options the launcher writes itself and never shows: the
	 * on-screen keyboard switch (made from the touch button and the gamepad
	 * toggle), port 0's stick mouse (port 0 holds the mouse, so the
	 * launcher keeps it off) and the emulator's GUI, which a launch always
	 * skips.
	 */
	val managed: Set<String> = setOf("amiberry.vkbd_enabled", "joyport0mousemap", "use_gui")

	/** The options no group section shows: the tabs' own rows and the [managed] ones. */
	val placed: Set<String> = laidOut.values.flatten().toSet() + managed

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
		else -> throw IllegalArgumentException("no tab places the group $group")
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
	 * [placed] leaves out, in the layer's order.
	 */
	fun options(schema: SettingsSchema, group: String, placed: Set<String> = SettingsGroups.placed): List<SettingsSchema.Option> =
		schema.options.values.filter { it.group == group && !it.presetOnly && it.id !in placed && rowKind(it) != null }

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
}
