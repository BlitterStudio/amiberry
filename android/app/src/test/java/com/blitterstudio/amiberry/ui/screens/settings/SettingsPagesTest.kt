package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingText
import com.blitterstudio.amiberry.data.model.SettingsChange
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class SettingsPagesTest {

	@Test
	fun `settings pages keep the Apple app's order and stable numeric test tags`() {
		assertEquals(
			listOf(
				R.string.settings_page_overview to "settings-page-0",
				R.string.settings_page_hardware to "settings-page-1",
				R.string.settings_page_display_sound to "settings-page-2",
				R.string.settings_page_input to "settings-page-3",
				R.string.settings_page_storage to "settings-page-4",
				R.string.settings_page_ports_misc to "settings-page-5",
				R.string.settings_page_all_settings to "settings-page-6"
			),
			SettingsPages.all.map { it.titleRes to it.testTag }
		)
	}

	@Test
	fun `selected index is clamped to a valid page`() {
		assertEquals(0, SettingsPages.validSelectedIndex(-1))
		assertEquals(0, SettingsPages.validSelectedIndex(0))
		assertEquals(6, SettingsPages.validSelectedIndex(6))
		assertEquals(6, SettingsPages.validSelectedIndex(99))
	}

	@Test
	fun `page for index uses the clamped selection`() {
		assertEquals(SettingsPage.Overview, SettingsPages.pageFor(-10))
		assertEquals(SettingsPage.Input, SettingsPages.pageFor(3))
		assertEquals(SettingsPage.AllSettings, SettingsPages.pageFor(99))
	}

	private fun optionChange(id: String) =
		SettingsChange(SettingText.Plain(id), SettingText.Plain("before"), SettingText.Plain("after"), id)

	private fun launcherChange(res: Int) =
		SettingsChange(SettingText.Resource(res), SettingText.Plain("before"), SettingText.Plain("after"))

	@Test
	fun `an option's edits mark the page that owns it`() {
		assertEquals(setOf(SettingsPage.Hardware), SettingsPages.editedPages(listOf(optionChange("cpu_model"), optionChange("mmu_model"))))
		assertEquals(setOf(SettingsPage.Storage), SettingsPages.editedPages(listOf(optionChange("cdimage0"), optionChange("floppy1type"))))
		// The Kickstart picker is the Overview's own row.
		assertEquals(setOf(SettingsPage.Overview), SettingsPages.editedPages(listOf(optionChange("kickstart_rom_file"))))
	}

	@Test
	fun `the launcher's own rows mark their pages`() {
		assertEquals(
			setOf(SettingsPage.Overview),
			SettingsPages.editedPages(
				listOf(
					launcherChange(R.string.settings_cpu_model_preset),
					launcherChange(R.string.settings_compatibility_level)
				)
			)
		)
		assertEquals(setOf(SettingsPage.DisplaySound), SettingsPages.editedPages(listOf(launcherChange(R.string.settings_display_resolution_label))))
		assertEquals(
			setOf(SettingsPage.Storage),
			SettingsPages.editedPages(listOf(launcherChange(R.string.settings_storage_hard_drive_label)))
		)
	}

	@Test
	fun `an option the schema does not list marks nothing`() {
		assertTrue(SettingsPages.editedPages(listOf(optionChange("not_an_option"))).isEmpty())
	}
}
