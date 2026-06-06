package com.blitterstudio.amiberry.ui.screens.settings

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Test

class SettingsTabsTest {

	@Test
	fun `settings tabs keep the expected order and stable numeric test tags`() {
		assertEquals(
			listOf(
				R.string.tab_cpu to "settings-tab-0",
				R.string.tab_chipset to "settings-tab-1",
				R.string.tab_memory to "settings-tab-2",
				R.string.tab_display to "settings-tab-3",
				R.string.tab_sound to "settings-tab-4",
				R.string.tab_input to "settings-tab-5",
				R.string.tab_storage to "settings-tab-6"
			),
			SettingsTabs.all.map { it.titleRes to it.testTag }
		)
	}

	@Test
	fun `selected index is clamped to a valid tab`() {
		assertEquals(0, SettingsTabs.validSelectedIndex(-1))
		assertEquals(0, SettingsTabs.validSelectedIndex(0))
		assertEquals(6, SettingsTabs.validSelectedIndex(6))
		assertEquals(6, SettingsTabs.validSelectedIndex(99))
	}

	@Test
	fun `tab for index uses the clamped selection`() {
		assertEquals(SettingsTab.Cpu, SettingsTabs.tabFor(-10))
		assertEquals(SettingsTab.Input, SettingsTabs.tabFor(5))
		assertEquals(SettingsTab.Storage, SettingsTabs.tabFor(99))
	}
}
