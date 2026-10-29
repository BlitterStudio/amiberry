package com.blitterstudio.amiberry.overlay

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class OverlayMenuTest {

	@Test
	fun `the menu opens on Home with Resume focused`() {
		val menu = OverlayMenu.HOME
		assertEquals(OverlayPage.Home, menu.page)
		assertEquals(OverlayKeys.RESUME, menu.focus)
		assertEquals(1, menu.depth)
	}

	@Test
	fun `Back on Home closes the menu`() {
		assertNull(OverlayMenu.HOME.back())
	}

	@Test
	fun `a page opens on its first item and Back returns focus to the item that opened it`() {
		val drives = OverlayMenu.HOME.open(OverlayPage.Media, OverlayKeys.MEDIA)
		assertEquals(OverlayPage.Media, drives.page)
		assertNull(drives.focus)

		val home = drives.back()!!
		assertEquals(OverlayPage.Home, home.page)
		assertEquals(OverlayKeys.MEDIA, home.focus)
		assertNull(home.back())
	}

	@Test
	fun `the file picker returns to the drive whose Insert opened it`() {
		val target = MediaTarget.Floppy(1)
		val picker = OverlayMenu.HOME
			.open(OverlayPage.Media, OverlayKeys.MEDIA)
			.open(OverlayPage.Picker(target), OverlayKeys.insert(target))
		assertEquals(OverlayPage.Picker(target), picker.page)
		assertEquals(3, picker.depth)

		val drives = picker.back()!!
		assertEquals(OverlayPage.Media, drives.page)
		assertEquals("insert_df1", drives.focus)
		assertEquals(OverlayKeys.MEDIA, drives.back()!!.focus)
	}

	@Test
	fun `cancelling a confirmation goes back to the item that asked`() {
		val confirm = OverlayMenu.HOME.open(OverlayPage.Confirm(ConfirmAction.Quit), OverlayKeys.QUIT)
		assertEquals(OverlayPage.Confirm(ConfirmAction.Quit), confirm.page)
		val home = confirm.back()!!
		assertEquals(OverlayPage.Home, home.page)
		assertEquals(OverlayKeys.QUIT, home.focus)
	}

	@Test
	fun `reopening a page from another item remembers the newer item`() {
		val home = OverlayMenu.HOME
			.open(OverlayPage.States, OverlayKeys.STATES).back()!!
			.open(OverlayPage.Settings, OverlayKeys.SETTINGS).back()!!
		assertEquals(OverlayKeys.SETTINGS, home.focus)
		assertEquals(1, home.depth)
	}

	@Test
	fun `insert keys name the drive`() {
		assertEquals("insert_df0", OverlayKeys.insert(MediaTarget.Floppy(0)))
		assertEquals("insert_cd", OverlayKeys.insert(MediaTarget.Cd))
	}
}
