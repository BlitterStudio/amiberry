package com.blitterstudio.amiberry.ui.screens.appsettings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema

/**
 * One section of an App Settings page: the [group]'s settings no other
 * section takes, or exactly [ids] (a designed split of the group).
 * [collapsed] starts the section closed (the expert rows, in Apple's
 * Advanced disclosures); a search that matches inside it opens it.
 */
internal data class AppSettingsSection(
	@param:StringRes val titleRes: Int,
	val group: String? = null,
	val ids: List<String> = emptyList(),
	val collapsed: Boolean = false
) {
	/** The section's stable identity (a group has one section; an id list is its own), for remembering it open. */
	val key: String get() = group ?: ids.joinToString(",")
}

/** One page of App Settings: its sections, in order. */
internal data class AppSettingsPage(
	val id: String,
	@param:StringRes val titleRes: Int,
	val sections: List<AppSettingsSection>
)

/**
 * App Settings' pages and sections, as the Apple apps' settings group them
 * (audit §3b/§3d): Appearance (what the emulator shows — the ImGui-only
 * settings the layer once had are hidden on Android), Defaults for new
 * machines (the controllers and mouse apart from the shortcut defaults, the
 * RetroArch buttons' defaults disclosed), and Advanced (logging and
 * diagnostics). The launcher's own cards sit in the page they belong
 * (AppSettingsScreen.LauncherSettings).
 */
internal object AppSettingsSections {

	/** The path settings the Folders section offers, in the layer's order. */
	private val FOLDER_IDS = listOf(
		"config_path", "controllers_path", "retroarch_config",
		"whdboot_path", "whdload_arch_path",
		"floppy_path", "harddrive_path", "cdrom_path",
		"rom_path", "rp9_path", "floppy_sounds_dir", "saveimage_dir",
		"savestate_dir", "ripper_path", "inputrecordings_dir",
		"screenshot_dir", "nvram_dir", "plugins_dir",
		"shaders_path", "bezels_path"
	)

	val pages = listOf(
		AppSettingsPage("appearance", R.string.appsettings_group_appearance, listOf(
			AppSettingsSection(R.string.appsettings_section_visual, group = "global_visual")
		)),
		// The touch-control defaults are not among them: a launcher machine's
		// controls are its remembered ones, so they cannot reach it.
		AppSettingsPage("defaults", R.string.appsettings_group_defaults, listOf(
			AppSettingsSection(
				R.string.appsettings_section_controllers_mouse,
				ids = listOf(
					"default_controller1", "default_controller2", "default_controller3", "default_controller4",
					"default_mouse1", "default_mouse2", "input_default_mouse_speed", "default_joystick_deadzone",
					"input_keyboard_as_joystick_stop_keypresses", "rctrl_as_ramiga"
				)
			),
			// The input group's remainder: the shortcut keys a new machine starts with.
			AppSettingsSection(R.string.appsettings_section_default_shortcuts, group = "global_input"),
			AppSettingsSection(
				R.string.appsettings_section_retroarch_buttons,
				ids = listOf("default_retroarch_quit", "default_retroarch_menu", "default_retroarch_reset"),
				collapsed = true
			),
			AppSettingsSection(R.string.appsettings_section_display_defaults, group = "global_display"),
			AppSettingsSection(R.string.appsettings_section_sound_defaults, group = "global_sound"),
			AppSettingsSection(R.string.appsettings_section_whdload, group = "global_whdload")
		)),
		// The paths group as the Apple apps' settings split it (audit §3b):
		// the log switch with its log file, and every content folder the
		// Android build honours — the launcher's repositories follow them
		// (ContentFolders), so they change here, not in the classic window.
		AppSettingsPage("advanced", R.string.appsettings_group_advanced, listOf(
			AppSettingsSection(
				R.string.appsettings_section_logging,
				ids = listOf("write_logfile", "logfile_path")
			),
			AppSettingsSection(R.string.appsettings_section_folders, ids = FOLDER_IDS),
			AppSettingsSection(R.string.appsettings_section_advanced, group = "global_advanced")
		))
	)

	/**
	 * [section]'s settings: exactly its ids, or its group's settings no other
	 * section takes, in the layer's order for a group — every section the
	 * layer has something for, narrowed by [matches].
	 */
	fun settingsOf(
		schema: SettingsSchema,
		section: AppSettingsSection,
		matches: (SettingsSchema.Option) -> Boolean = { true }
	): List<SettingsSchema.Option> {
		val named = pages.flatMap { page -> page.sections }.flatMap { it.ids }.toSet()
		val settings = if (section.ids.isNotEmpty()) {
			section.ids.mapNotNull { id -> schema.hostSettings.firstOrNull { it.id == id } }
		} else {
			schema.hostSettings.filter { it.group == section.group && it.id !in named }
		}
		return settings.filter { !it.hiddenOnAndroid && matches(it) }
	}
}

/** Whether the setting's label, help or id holds every word of [query]. */
internal fun SettingsSchema.Option.matches(query: String): Boolean {
	if (query.isBlank()) return true
	val text = "$label $help $id".lowercase()
	return query.lowercase().split(Regex("\\s+")).filter { it.isNotBlank() }.all { text.contains(it) }
}
