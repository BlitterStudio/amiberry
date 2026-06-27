package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class RecentLaunchesTest {

	@Test
	fun `available launches prune stale paths and unsupported models while preserving order`() {
		val quickstart = quickstart(model = "A500", df0 = "/media/disk.adf")
		val staleConfig = entry("config", path = "/configs/missing.uae")
		val whdload = entry("whdload", path = "/whdload/Lotus.lha")
		val unsupportedModel = quickstart(model = "CD32", cd = "/cds/game.cue")
		val unknownType = JSONObject().put("type", "mystery")

		val available = RecentLaunches.available(
			entries = listOf(quickstart, staleConfig, whdload, unsupportedModel, unknownType),
			availableModels = setOf(AmigaModel.A500),
			fileExists = { it in setOf("/media/disk.adf", "/whdload/Lotus.lha", "/cds/game.cue") }
		)

		assertEquals(listOf(quickstart, whdload), available)
	}

	@Test
	fun `quickstart media paths are optional but must exist when present`() {
		val noMedia = quickstart(model = "A1200")
		val missingFloppy = quickstart(model = "A1200", df1 = "/media/df1.adf")

		val available = RecentLaunches.available(
			entries = listOf(noMedia, missingFloppy),
			availableModels = setOf(AmigaModel.A1200),
			fileExists = { false }
		)

		assertEquals(listOf(noMedia), available)
	}

	@Test
	fun `stored launches prune missing files but keep currently unsupported models`() {
		val unsupportedModel = quickstart(model = "CD32", cd = "/cds/game.cue")
		val staleConfig = entry("config", path = "/configs/missing.uae")
		val invalidModel = quickstart(model = "UNKNOWN", df0 = "/media/disk.adf")

		val stored = RecentLaunches.pruneMissingFiles(
			entries = listOf(unsupportedModel, staleConfig, invalidModel),
			fileExists = { it in setOf("/cds/game.cue", "/media/disk.adf") }
		)

		assertEquals(listOf(unsupportedModel), stored)
	}

	@Test
	fun `labels match launch type and strip common file suffixes`() {
		assertEquals(
			"A500 \u2014 Workbench.adf, Extras.adf",
			RecentLaunches.label(quickstart(model = "A500", df0 = "/media/Workbench.adf", df1 = "/media/Extras.adf"))
		)
		assertEquals("Workbench", RecentLaunches.label(entry("config", path = "/configs/Workbench.uae")))
		assertEquals("Lotus", RecentLaunches.label(entry("whdload", path = "/whdload/Lotus.lha")))
		assertEquals("?", RecentLaunches.label(JSONObject().put("type", "mystery")))
	}

	@Test
	fun `details provide type and media context for richer recent cards`() {
		val quickstart = RecentLaunches.details(
			quickstart(model = "A500", df0 = "/media/Workbench.adf", df1 = "/media/Extras.adf")
		)
		val config = RecentLaunches.details(entry("config", path = "/configs/Workbench.uae"))
		val whdload = RecentLaunches.details(entry("whdload", path = "/whdload/Lotus.lha"))

		assertEquals("Workbench.adf", quickstart.title)
		assertEquals("Quick Start", quickstart.typeLabel)
		assertEquals("A500 - DF0: Workbench.adf, DF1: Extras.adf", quickstart.detail)
		assertEquals("Configuration", config.typeLabel)
		assertEquals("/configs/Workbench.uae", config.detail)
		assertEquals("WHDLoad", whdload.typeLabel)
		assertEquals("/whdload/Lotus.lha", whdload.detail)
	}

	@Test
	fun `adding a recent launch moves duplicates to the front and caps the list`() {
		val existing = (1..12).map { entry("config", path = "/configs/$it.uae") }
		val duplicate = entry("config", path = "/configs/5.uae")

		val added = RecentLaunches.add(existing, duplicate, maxItems = 10)

		assertEquals(10, added.size)
		assertEquals(duplicate.toString(), added.first().toString())
		assertEquals(1, added.count { it.toString() == duplicate.toString() })
		assertTrue(added.any { it.optString("path") == "/configs/10.uae" })
		assertTrue(added.none { it.optString("path") == "/configs/11.uae" })
		assertTrue(added.none { it.optString("path") == "/configs/12.uae" })
	}

	@Test
	fun `parse returns no recent launches for malformed stored json`() {
		assertTrue(RecentLaunches.parse("not-json").isEmpty())
	}

	private fun quickstart(
		model: String,
		df0: String = "",
		df1: String = "",
		cd: String = ""
	): JSONObject = JSONObject()
		.put("type", "quickstart")
		.put("model", model)
		.put("df0", df0)
		.put("df1", df1)
		.put("cd", cd)

	private fun entry(type: String, path: String): JSONObject = JSONObject()
		.put("type", type)
		.put("path", path)
}
