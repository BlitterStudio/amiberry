package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingText
import com.blitterstudio.amiberry.data.model.SettingsChange

/**
 * A page of the machine editor, after the Apple app's pages
 * (apple/Shared/UI/SettingsModel.swift, SettingsPage): each page lays out
 * the option groups it covers (SettingsGroups.byPage), and All Settings
 * lists every option the launcher shows. The Overview additionally re-shows
 * a few of the Storage page's rows (the first drives) at a glance.
 */
enum class SettingsPage(
	@param:StringRes val titleRes: Int,
	val testTag: String
) {
	/** The model, compatibility, Kickstart, memory summary and first drives. */
	Overview(R.string.settings_page_overview, "settings-page-0"),
	Hardware(R.string.settings_page_hardware, "settings-page-1"),
	DisplaySound(R.string.settings_page_display_sound, "settings-page-2"),
	Input(R.string.settings_page_input, "settings-page-3"),
	/** The floppy and CD drives and the mounted hard drives. */
	Storage(R.string.settings_page_storage, "settings-page-4"),
	PortsMisc(R.string.settings_page_ports_misc, "settings-page-5"),
	/** Every option, searchable. */
	AllSettings(R.string.settings_page_all_settings, "settings-page-6")
}

object SettingsPages {
	/** The editor's pages, in the sidebar's order. */
	val all: List<SettingsPage> = SettingsPage.entries.toList()

	/** A remembered selection index clamped to the pages there are. */
	fun validSelectedIndex(index: Int): Int = index.coerceIn(0, all.lastIndex)

	fun pageFor(index: Int): SettingsPage = all[validSelectedIndex(index)]

	/**
	 * The pages the editor's unsaved edits are on, for the page list's mark:
	 * each changed option's page (SettingsGroups.pageFor), and the
	 * launcher's own rows (model, compatibility, resolution, hard drives),
	 * which no layer option stands for.
	 */
	fun editedPages(changes: List<SettingsChange>): Set<SettingsPage> = buildSet {
		for (change in changes) {
			val id = change.optionId
			if (id != null) {
				SettingsGroups.pageFor(id)?.let(::add)
			} else if (change.label is SettingText.Resource) {
				when (change.label.id) {
					R.string.settings_cpu_model_preset,
					R.string.settings_change_model_configuration,
					R.string.settings_compatibility_level -> add(SettingsPage.Overview)
					R.string.settings_display_resolution_label -> add(SettingsPage.DisplaySound)
					R.string.settings_storage_hard_drive_label,
					R.string.settings_change_hard_drive_read_only,
					R.string.settings_change_hard_drive_boot_priority -> add(SettingsPage.Storage)
				}
			}
		}
	}
}
