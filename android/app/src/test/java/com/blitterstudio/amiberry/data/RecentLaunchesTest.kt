package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class RecentLaunchesTest {

	@Test
	fun `items keep entries that cannot launch and mark why while preserving order`() {
		val quickstart = quickstart(model = "A500", df0 = "/media/disk.adf")
		val staleConfig = entry("config", path = "/configs/missing.uae")
		val whdload = entry("whdload", path = "/whdload/Lotus.lha")
		val rp9 = entry("rp9", path = "/rp9/Asteroids.rp9")
		val unsupportedModel = quickstart(model = "CD32", cd = "/cds/game.cue")
		val unknownType = JSONObject().put("type", "mystery")

		val items = RecentLaunches.items(
			entries = listOf(quickstart, staleConfig, whdload, rp9, unsupportedModel, unknownType),
			availableModels = setOf(AmigaModel.A500),
			fileExists = { it in setOf("/media/disk.adf", "/whdload/Lotus.lha", "/rp9/Asteroids.rp9", "/cds/game.cue") }
		)

		assertEquals(listOf(quickstart, staleConfig, whdload, rp9, unsupportedModel), items.map { it.entry })
		assertEquals(
			listOf(
				RecentLaunches.Status.Available,
				RecentLaunches.Status.MissingFiles,
				RecentLaunches.Status.Available,
				RecentLaunches.Status.Available,
				RecentLaunches.Status.ModelUnavailable
			),
			items.map { it.status }
		)
	}

	@Test
	fun `quickstart media paths are optional but must exist when present`() {
		val noMedia = quickstart(model = "A1200")
		val missingFloppy = quickstart(model = "A1200", df1 = "/media/df1.adf")
		val unknownModel = quickstart(model = "UNKNOWN")
		val models = setOf(AmigaModel.A1200)

		assertEquals(RecentLaunches.Status.Available, RecentLaunches.status(noMedia, models) { false })
		assertEquals(RecentLaunches.Status.MissingFiles, RecentLaunches.status(missingFloppy, models) { false })
		assertEquals(RecentLaunches.Status.ModelUnavailable, RecentLaunches.status(unknownModel, models) { false })
	}

	@Test
	fun `items show up to ten entries`() {
		val entries = (1..12).map { entry("config", path = "/configs/$it.uae") }

		val items = RecentLaunches.items(entries, availableModels = emptySet(), fileExists = { true })

		assertEquals(RecentLaunches.MAX_ITEMS, items.size)
		assertEquals("/configs/1.uae", items.first().entry.optString("path"))
	}

	@Test
	fun `removing an entry removes it in any model spelling and keeps the others`() {
		val workbench = entry("config", path = "/configs/Workbench.uae")
		val storedOldSpelling = quickstart(model = "A500", df0 = "/media/disk.adf")
		val lotus = entry("whdload", path = "/whdload/Lotus.lha")
		val entries = listOf(workbench, storedOldSpelling, lotus)

		assertEquals(listOf(storedOldSpelling, lotus), RecentLaunches.remove(entries, workbench).toList())
		assertEquals(
			listOf(workbench, lotus),
			RecentLaunches.remove(entries, quickstart(model = AmigaModel.A500.cmdArg, df0 = "/media/disk.adf"))
		)
	}

	@Test
	fun `a renamed configuration keeps its place in Recent`() {
		val renamed = entry("config", path = "/configs/Workbench.uae")
		val other = entry("config", path = "/configs/Games.uae")
		val whdload = entry("whdload", path = "/configs/Workbench.uae")

		val updated = RecentLaunches.replaceConfigPath(
			listOf(renamed, other, whdload),
			oldPath = "/configs/Workbench.uae",
			newPath = "/configs/Workbench 3.1.uae"
		)

		assertEquals(
			listOf("/configs/Workbench 3.1.uae", "/configs/Games.uae", "/configs/Workbench.uae"),
			updated.map { it.optString("path") }
		)
		assertEquals("whdload", updated.last().optString("type"))
	}

	@Test
	fun `a configuration deleted in the app leaves Recent while other entries stay`() {
		val deleted = entry("config", path = "/configs/Workbench.uae")
		val other = entry("config", path = "/configs/Games.uae")
		val sameFileWhdload = entry("whdload", path = "/configs/Workbench.uae")
		val vanishedOutside = entry("config", path = "/configs/Gone.uae")

		val updated = RecentLaunches.removeConfigPath(
			listOf(deleted, other, sameFileWhdload, vanishedOutside),
			path = "/configs/./Workbench.uae"
		)

		assertEquals(listOf(other, sameFileWhdload, vanishedOutside), updated)
		// A file that vanished outside the app is still listed, marked missing.
		assertEquals(
			RecentLaunches.Status.MissingFiles,
			RecentLaunches.items(updated, emptySet(), fileExists = { it == "/configs/Games.uae" })
				.single { it.entry === vanishedOutside }.status
		)
	}

	@Test
	fun `labels match launch type and strip common file suffixes`() {
		assertEquals(
			"Amiga 500 \u2014 Workbench.adf, Extras.adf",
			RecentLaunches.label(quickstart(model = "A500", df0 = "/media/Workbench.adf", df1 = "/media/Extras.adf"))
		)
		assertEquals("Workbench", RecentLaunches.label(entry("config", path = "/configs/Workbench.uae")))
		assertEquals("Lotus", RecentLaunches.label(entry("whdload", path = "/whdload/Lotus.lha")))
		assertEquals("Asteroids", RecentLaunches.label(entry("rp9", path = "/rp9/Asteroids.Rp9")))
		assertEquals("?", RecentLaunches.label(JSONObject().put("type", "mystery")))
	}

	@Test
	fun `details provide type and media context for richer recent cards`() {
		val quickstart = RecentLaunches.details(
			quickstart(model = "A500", df0 = "/media/Workbench.adf", df1 = "/media/Extras.adf")
		)
		val config = RecentLaunches.details(entry("config", path = "/configs/Workbench.uae"))
		val whdload = RecentLaunches.details(entry("whdload", path = "/whdload/Lotus.lha"))
		val rp9 = RecentLaunches.details(entry("rp9", path = "/rp9/Asteroids.rp9"))

		assertEquals("Workbench.adf", quickstart.title)
		assertEquals(RecentLaunches.Type.QuickStart, quickstart.type)
		assertEquals("Amiga 500 - DF0: Workbench.adf, DF1: Extras.adf", quickstart.detail)
		assertEquals("Workbench", config.title)
		assertEquals(RecentLaunches.Type.Configuration, config.type)
		assertEquals("Workbench.uae", config.detail)
		assertEquals(RecentLaunches.Type.WhdLoad, whdload.type)
		assertEquals("Lotus.lha", whdload.detail)
		assertEquals(RecentLaunches.Type.Rp9, rp9.type)
		assertEquals("Asteroids.rp9", rp9.detail)
	}

	@Test
	fun `adding a recent launch moves duplicates to the front and caps the list`() {
		val existing = (1..12).map { entry("config", path = "/configs/$it.uae") }
		val duplicate = entry("config", path = "/configs/5.uae")

		val added = RecentLaunches.add(existing, duplicate, maxItems = 10)

		assertEquals(10, added.size)
		assertEquals("/configs/5.uae", added.first().optString("path"))
		assertEquals(1, added.count { it.optString("path") == "/configs/5.uae" })
		assertTrue(added.any { it.optString("path") == "/configs/10.uae" })
		assertTrue(added.none { it.optString("path") == "/configs/11.uae" })
		assertTrue(added.none { it.optString("path") == "/configs/12.uae" })
	}

	@Test
	fun `a launch is stamped with its time and replaces the earlier launch of the same thing`() {
		val first = RecentLaunches.add(emptyList(), entry("whdload", path = "/whdload/Lotus.lha"), nowMs = 1_000L)
		val again = RecentLaunches.add(first, entry("whdload", path = "/whdload/Lotus.lha"), nowMs = 5_000L)

		assertEquals(1, again.size)
		assertEquals(5_000L, RecentLaunches.details(again.single()).launchedAtMs)
	}

	@Test
	fun `entries stored before launch times have none and still remove by identity`() {
		val old = entry("config", path = "/configs/A1200.uae")
		val stamped = RecentLaunches.add(emptyList(), entry("config", path = "/configs/A1200.uae"), nowMs = 42L).single()

		assertEquals(null, RecentLaunches.details(old).launchedAtMs)
		assertTrue(RecentLaunches.remove(listOf(stamped), old).isEmpty())
	}

	@Test
	fun `a WHDLoad entry is titled by its readable game name`() {
		val details = RecentLaunches.details(entry("whdload", path = "/whdload/TheLostPixelv1.3_WHD4.lha"))

		assertEquals("The Lost Pixel v1.3", details.title)
		assertEquals("TheLostPixelv1.3_WHD4.lha", details.detail)
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
