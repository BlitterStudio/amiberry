package com.blitterstudio.amiberry.ui.screens.settings

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

enum class SettingsTab(
	@param:StringRes val titleRes: Int,
	val testTag: String
) {
	Cpu(R.string.tab_cpu, "settings-tab-0"),
	Chipset(R.string.tab_chipset, "settings-tab-1"),
	Memory(R.string.tab_memory, "settings-tab-2"),
	Display(R.string.tab_display, "settings-tab-3"),
	Sound(R.string.tab_sound, "settings-tab-4"),
	Input(R.string.tab_input, "settings-tab-5"),
	Storage(R.string.tab_storage, "settings-tab-6")
}

object SettingsTabs {
	val all: List<SettingsTab> = listOf(
		SettingsTab.Cpu,
		SettingsTab.Chipset,
		SettingsTab.Memory,
		SettingsTab.Display,
		SettingsTab.Sound,
		SettingsTab.Input,
		SettingsTab.Storage
	)

	fun validSelectedIndex(index: Int): Int =
		index.coerceIn(0, all.lastIndex)

	fun tabFor(index: Int): SettingsTab =
		all[validSelectedIndex(index)]
}
