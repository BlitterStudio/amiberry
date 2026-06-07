package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Test

class QuickStartSettingsSnapshotTest {

	@Test
	fun `recent quick start snapshot uses recent model media and current controls`() {
		val currentSettings = EmulatorSettings.fromModel(AmigaModel.A4000).copy(
			romFile = "/roms/current-a4000.rom",
			romExtFile = "/roms/current-ext.rom",
			floppy0 = "/current/df0.adf",
			floppy1 = "/current/df1.adf",
			cdImage = "/current/cd.iso",
			joyport0 = "joy0",
			joyport1 = "joy1",
			onScreenJoystick = false,
			onScreenKeyboard = false
		)

		val snapshot = buildRecentQuickStartSettingsSnapshot(
			model = AmigaModel.A500,
			roms = listOf(rom("kick13.rom", 0xc4f0f55fL)),
			currentSettings = currentSettings,
			floppy0 = "",
			floppy1 = "/recent/df1.adf",
			cdImage = ""
		)

		assertEquals(AmigaModel.A500, snapshot.baseModel)
		assertEquals(68000, snapshot.cpuModel)
		assertEquals("ocs", snapshot.chipset)
		assertEquals("/roms/kick13.rom", snapshot.romFile)
		assertEquals("", snapshot.romExtFile)
		assertEquals("", snapshot.floppy0)
		assertEquals("/recent/df1.adf", snapshot.floppy1)
		assertEquals(0, snapshot.floppy1Type)
		assertEquals("", snapshot.cdImage)
		assertEquals("joy0", snapshot.joyport0)
		assertEquals("joy1", snapshot.joyport1)
		assertEquals(false, snapshot.onScreenJoystick)
		assertEquals(false, snapshot.onScreenKeyboard)
	}

	@Test
	fun `recent quick start snapshot keeps DF1 disabled when recent DF1 is blank`() {
		val snapshot = buildRecentQuickStartSettingsSnapshot(
			model = AmigaModel.A500,
			roms = listOf(rom("kick13.rom", 0xc4f0f55fL)),
			currentSettings = EmulatorSettings.fromModel(AmigaModel.A500),
			floppy0 = "/recent/df0.adf",
			floppy1 = "",
			cdImage = ""
		)

		assertEquals("", snapshot.floppy1)
		assertEquals(-1, snapshot.floppy1Type)
	}

	private fun rom(name: String, crc32: Long, path: String = "/roms/$name") = AmigaFile(
		path = path,
		name = name,
		extension = "rom",
		size = 524288,
		lastModified = 0,
		category = FileCategory.ROMS,
		crc32 = crc32
	)
}
