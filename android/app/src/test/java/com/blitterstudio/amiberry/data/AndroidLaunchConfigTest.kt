package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Test

class AndroidLaunchConfigTest {

	@Test
	fun `quick start snapshot uses imported floppy selected rom and current controls`() {
		val currentSettings = EmulatorSettings.fromModel(AmigaModel.A4000).copy(
			joyport0 = "joy0",
			joyport1 = "joy1",
			onScreenJoystick = false,
			onScreenKeyboard = false,
			onScreenKeyboardNumpad = true
		)

		val snapshot = AndroidLaunchConfig.buildQuickStartSettingsSnapshot(
			model = AmigaModel.A500,
			roms = listOf(rom("kick13.rom", 0xc4f0f55fL)),
			currentSettings = currentSettings,
			floppy0 = "/import/game.adf",
			floppy1 = "",
			cdImage = ""
		)

		assertEquals(AmigaModel.A500, snapshot.baseModel)
		assertEquals("/roms/kick13.rom", snapshot.romFile)
		assertEquals("/import/game.adf", snapshot.floppy0)
		assertEquals("", snapshot.floppy1)
		assertEquals(-1, snapshot.floppy1Type)
		assertEquals("joy0", snapshot.joyport0)
		assertEquals("joy1", snapshot.joyport1)
		assertEquals(false, snapshot.onScreenJoystick)
		assertEquals(false, snapshot.onScreenKeyboard)
		assertEquals(true, snapshot.onScreenKeyboardNumpad)
	}

	@Test
	fun `quick start snapshot keeps CD model floppy drives disabled`() {
		val snapshot = AndroidLaunchConfig.buildQuickStartSettingsSnapshot(
			model = AmigaModel.CD32,
			roms = emptyList(),
			currentSettings = EmulatorSettings.fromModel(AmigaModel.A500),
			floppy0 = "",
			floppy1 = "",
			cdImage = "/import/game.iso"
		)

		assertEquals(AmigaModel.CD32, snapshot.baseModel)
		assertEquals(-1, snapshot.floppy0Type)
		assertEquals(-1, snapshot.floppy1Type)
		assertEquals("/import/game.iso", snapshot.cdImage)
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
