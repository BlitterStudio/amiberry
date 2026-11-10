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

	@Test
	fun `message boxes alone keep the view up`() {
		val presence = OverlayViewPresence(messages = 1)
		assertEquals(true, presence.dialogs)
		assertEquals(true, presence.view)
	}

	@Test
	fun `the recovery requester alone keeps the view up`() {
		val presence = OverlayViewPresence(recovery = true)
		assertEquals(true, presence.dialogs)
		assertEquals(true, presence.view)
	}

	@Test
	fun `messages over the recovery requester keep the view up`() {
		val presence = OverlayViewPresence(messages = 3, recovery = true)
		assertEquals(true, presence.dialogs)
		assertEquals(true, presence.view)
	}

	@Test
	fun `the menu keeps the view up with and without dialogs`() {
		assertEquals(true, OverlayViewPresence(menu = true).view)
		assertEquals(true, OverlayViewPresence(menu = true, messages = 1).view)
	}

	@Test
	fun `nothing showing hides the view, so the emulation gets its input back`() {
		val presence = OverlayViewPresence()
		assertEquals(false, presence.dialogs)
		assertEquals(false, presence.view)
	}

	@Test
	fun `dismissing the last message of many hides the view only then`() {
		val queued = OverlayViewPresence(messages = 2)
		val dismissed = queued.copy(messages = 1)
		val lastDismissed = dismissed.copy(messages = 0)
		assertEquals(true, queued.view)
		assertEquals(true, dismissed.view)
		assertEquals(false, lastDismissed.view)
	}

}
