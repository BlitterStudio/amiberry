package com.blitterstudio.amiberry.overlay

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class StateSlotsTest {

	private val base = StateNameSources(
		stateDir = "/data/Savestates/",
		screenshotDir = "/data/Screenshots/",
		floppy0 = "",
		cd = "",
		whdload = "",
		lastConfig = "",
		description = ""
	)

	@Test
	fun `slots are the built-in GUI's 1 to 10`() {
		assertEquals((1..10).toList(), StateSlots.slots.toList())
	}

	@Test
	fun `DF0's image names the states as the built-in GUI does`() {
		val sources = base.copy(floppy0 = "/data/Floppies/Turrican II.adf", cd = "/data/CD/game.iso", lastConfig = "A500")
		val paths = StateSlots.paths(sources, 1)
		assertEquals("/data/Savestates/Turrican II-1.uss", paths.state)
		assertEquals("/data/Screenshots/Turrican II-1.png", paths.thumbnail)
		assertEquals(1, paths.slot)
	}

	@Test
	fun `without a floppy the CD, then the WHDLoad game, then the configuration name the states`() {
		assertEquals(
			"/data/Savestates/Fire and Ice-3.uss",
			StateSlots.paths(base.copy(cd = "/cd/Fire and Ice.cue", whdload = "/w/Other.lha"), 3).state
		)
		assertEquals(
			"/data/Savestates/Lotus2-10.uss",
			StateSlots.paths(base.copy(whdload = "/w/Lotus2.lha", lastConfig = "A1200"), 10).state
		)
		assertEquals(
			"/data/Savestates/A1200-2.uss",
			StateSlots.paths(base.copy(lastConfig = "A1200"), 2).state
		)
	}

	@Test
	fun `a dot in a configuration name survives as in the built-in GUI`() {
		assertEquals("/data/Savestates/Workbench 3.1-4.uss", StateSlots.paths(base.copy(lastConfig = "Workbench 3.1"), 4).state)
	}

	@Test
	fun `a session with no media or configuration uses its description as a file name`() {
		val sources = base.copy(description = "Amiga 500: KS 1.3/OCS")
		assertEquals("/data/Savestates/Amiga 500_ KS 1_3_OCS-1.uss", StateSlots.paths(sources, 1).state)
		assertEquals("/data/Screenshots/Amiga 500_ KS 1_3_OCS-1.png", StateSlots.paths(sources, 1).thumbnail)
	}

	@Test
	fun `no description at all falls back to a fixed name`() {
		assertEquals("/data/Savestates/default-5.uss", StateSlots.paths(base, 5).state)
		assertEquals("default", StateSlots.safeName("   "))
	}

	@Test
	fun `folders without a trailing separator get one`() {
		val sources = base.copy(stateDir = "/s", screenshotDir = "/p", floppy0 = "/f/disk.adz")
		assertEquals("/s/disk-1.uss", StateSlots.paths(sources, 1).state)
		assertEquals("/p/disk-1.png", StateSlots.paths(sources, 1).thumbnail)
	}

	@Test
	fun `the native sources parse in order and short answers are rejected`() {
		val parsed = StateNameSources.parse(arrayOf("/s/", "/p/", "/f/a.adf", "", "", "cfg", "Desc"))
		assertEquals(StateNameSources("/s/", "/p/", "/f/a.adf", "", "", "cfg", "Desc"), parsed)
		assertNull(StateNameSources.parse(arrayOf("/s/")))
		assertNull(StateNameSources.parse(null))
	}
}
