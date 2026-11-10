package com.blitterstudio.amiberry.overlay

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class NativeDialogsTest {

	@Test
	fun `a media hotkey opens the drive's or the CD picker, a state hotkey the states list`() {
		assertEquals(OverlayPage.Picker(MediaTarget.Floppy(0)), OverlayMenu.pageFor(0))
		assertEquals(OverlayPage.Picker(MediaTarget.Floppy(3)), OverlayMenu.pageFor(3))
		assertEquals(OverlayPage.States, OverlayMenu.pageFor(4))
		assertEquals(OverlayPage.States, OverlayMenu.pageFor(5))
		assertEquals(OverlayPage.Picker(MediaTarget.Cd), OverlayMenu.pageFor(6))
	}

	@Test
	fun `the menu's own open and unknown keys stay Home`() {
		assertNull(OverlayMenu.pageFor(-1))
		assertNull(OverlayMenu.pageFor(7))
	}

	@Test
	fun `the recovery request reads drive and missing path`() {
		assertEquals(FloppyRecoveryRequest(2, "/storage/Game.adf"), FloppyRecoveryRequest.parse(arrayOf("2", "/storage/Game.adf")))
	}

	@Test
	fun `a malformed recovery request is refused`() {
		assertNull(FloppyRecoveryRequest.parse(arrayOf("2")))
		assertNull(FloppyRecoveryRequest.parse(arrayOf("x", "/storage/Game.adf")))
		assertNull(FloppyRecoveryRequest.parse(arrayOf("4", "/storage/Game.adf")))
		assertNull(FloppyRecoveryRequest.parse(arrayOf("-1", "/storage/Game.adf")))
	}
}
