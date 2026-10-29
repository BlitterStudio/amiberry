package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Test

class WhdLoadTitlesTest {

	@Test
	fun `archive names read as titles without the installer tag`() {
		assertEquals("The Lost Pixel v1.3", WhdLoadTitles.clean("TheLostPixelv1.3_WHD4.lha"))
		assertEquals("Castlevania AGA", WhdLoadTitles.clean("CastlevaniaAGA_WHDLoad.lha"))
		assertEquals("Lotus", WhdLoadTitles.clean("Lotus.LHA"))
		assertEquals("Alien Breed v2.1", WhdLoadTitles.clean("AlienBreed_v2.1.lzx"))
		assertEquals("Lemmings 2", WhdLoadTitles.clean("Lemmings2.lha"))
		assertEquals("Robocop 3D", WhdLoadTitles.clean("Robocop3D.lha"))
	}

	@Test
	fun `a name that is only the tag keeps something readable`() {
		assertEquals("WHDLoad", WhdLoadTitles.clean("WHDLoad.lha"))
	}
}
