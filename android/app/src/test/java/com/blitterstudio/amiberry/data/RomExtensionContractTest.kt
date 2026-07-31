package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class RomExtensionContractTest {
	@Test
	fun `Android ROM extensions match every general native ROM chooser`() {
		val romPanel = File("../../src/osdep/imgui/rom.cpp").readText()
		val chooserFilters = Regex(
			"""OpenFileDialogKey\("ROM", "Select (?:Main|Extended|Custom|Cartridge) ROM", "([^"]+)""""
		).findAll(romPanel)
			.map { match ->
				match.groupValues[1]
					.split(',')
					.map { it.removePrefix(".").lowercase() }
					.toSet()
			}
			.toList()

		assertEquals("Expected all four general ROM chooser filters.", 4, chooserFilters.size)
		chooserFilters.forEach { chooserExtensions ->
			assertEquals(FileCategory.ROMS.extensions, chooserExtensions)
		}
	}

	@Test
	fun `native ROM scanner matches the chooser-compatible base extensions case-insensitively`() {
		val nativeGui = File("../../src/osdep/amiberry_gui.cpp").readText()
		val scanner = Regex(
			"""static int isromext\(const std::string& path, bool deepscan\)([\s\S]*?)static bool scan_rom_hook"""
		).find(nativeGui)?.groupValues?.get(1)
			?: error("Could not find isromext() in src/osdep/amiberry_gui.cpp")
		val baseVector = Regex(
			"""static const std::vector<std::string> extensions = \{([^}]+)\};"""
		).find(scanner)?.groupValues?.get(1)
			?.let { initializer ->
				Regex(""""([^"]+)"""").findAll(initializer)
					.map { it.groupValues[1] }
					.toSet()
			}
			?: error("Could not find the base ROM extension vector")
		val expectedBaseVector = FileCategory.ROMS.extensions + "roz"

		assertEquals(expectedBaseVector, baseVector)
		assertTrue(
			"The base vector must use a case-insensitive comparison so mixed-case CdTv and cD32 filenames survive native rescan.",
			scanner.contains("strcasecmp(ext.c_str(), extension.c_str()) == 0") &&
				baseVector.any { it.equals("CdTv", ignoreCase = true) } &&
				baseVector.any { it.equals("cD32", ignoreCase = true) }
		)
		assertFalse(
			"The base-vector check must not retain exact std::find matching.",
			scanner.contains("std::find(extensions.begin(), extensions.end(), ext)")
		)

		val baseComparison = scanner.indexOf("strcasecmp(ext.c_str(), extension.c_str()) == 0")
		val numberedExtension = scanner.indexOf("std::toupper(ext[0]) == 'U' && std::isdigit(ext[1])")
		val deepScanGate = scanner.indexOf("if (!deepscan)")
		val archiveLoop = scanner.indexOf("for (auto i = 0; uae_archive_extensions[i]; i++)")
		assertTrue(
			"Base ROMs and scanner-only U-numbered suffixes must remain unconditional, while archives stay behind the deep-scan gate.",
			baseComparison >= 0 &&
				baseComparison < numberedExtension &&
				numberedExtension < deepScanGate &&
				deepScanGate < archiveLoop
		)
	}
}
