package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class IntentImportTest {

	@Test
	fun `classify detects config imports after filename sanitizing`() {
		val import = IntentImport.classify("../configs/Workbench:Default.UAE")

		assertEquals(IntentImport.Classification.Config("Workbench_Default.UAE"), import)
	}

	@Test
	fun `classify detects RP9 packages case insensitively`() {
		assertEquals(IntentImport.Classification.Rp9("Game.RP9"), IntentImport.classify("Game.RP9"))
	}

	@Test
	fun `classify detects media categories case insensitively`() {
		assertEquals(
			IntentImport.Classification.Media("Lotus.ADF", FileCategory.FLOPPIES),
			IntentImport.classify("Lotus.ADF")
		)
		assertEquals(
			IntentImport.Classification.Media("Kick31.ROM", FileCategory.ROMS),
			IntentImport.classify("Kick31.ROM")
		)
		assertEquals(
			IntentImport.Classification.Media("Game.CHD", FileCategory.CD_IMAGES),
			IntentImport.classify("Game.CHD")
		)
	}

	@Test
	fun `classify rejects unsupported and extensionless names`() {
		assertEquals(
			IntentImport.Classification.Unsupported("readme.txt", "txt"),
			IntentImport.classify("readme.txt")
		)
		assertEquals(
			IntentImport.Classification.Unsupported("imported_file", ""),
			IntentImport.classify("..")
		)
	}

	@Test
	fun `sanitizedName returns the classified safe name for every result`() {
		val values = listOf(
			IntentImport.classify("Workbench.uae"),
			IntentImport.classify("Disk.adf"),
			IntentImport.classify("notes.txt")
		)

		assertTrue(values.all { it.safeName.isNotBlank() })
		assertEquals(listOf("Workbench.uae", "Disk.adf", "notes.txt"), values.map { it.safeName })
	}
}
