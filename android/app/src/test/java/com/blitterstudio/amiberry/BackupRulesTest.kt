package com.blitterstudio.amiberry

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.w3c.dom.Element
import java.io.File
import javax.xml.parsers.DocumentBuilderFactory

class BackupRulesTest {

	private val userExternalIncludes = setOf(
		Entry("include", "external", "Configurations/"),
		Entry("include", "external", "Controllers/")
	)

	private val transientConfigExcludes = setOf(
		Entry("exclude", "external", "Configurations/.android_controls.uae"),
		Entry("exclude", "external", "Configurations/.current_settings.uae"),
		Entry("exclude", "external", "Configurations/.intent_quickstart_settings.uae"),
		Entry("exclude", "external", "Configurations/.last_session.uae"),
		Entry("exclude", "external", "Configurations/.quickstart_settings.uae"),
		Entry("exclude", "external", "Configurations/.temp_native_ui.uae")
	)

	private val mediaDirectories = setOf(
		"CDROMs/",
		"Floppies/",
		"HardDrives/",
		"LHA/",
		"ROMs/",
		"SaveStates/",
		"WHDBoot/"
	)

	@Test
	fun `legacy full backup includes user metadata and excludes transient emulator state`() {
		val entries = entriesFrom("backup_rules.xml")

		assertTrue(entries.containsAll(userExternalIncludes))
		assertTrue(entries.contains(Entry("include", "sharedpref", "amiberry_prefs.xml")))
		assertTrue(entries.containsAll(transientConfigExcludes))
		assertNoRootMarkersIncluded(entries)
		assertNoMediaDirectoriesIncluded(entries)
	}

	@Test
	fun `android twelve cloud backup includes user metadata and excludes transient emulator state`() {
		val entries = entriesFrom("data_extraction_rules.xml", sectionName = "cloud-backup")

		assertTrue(entries.containsAll(userExternalIncludes))
		assertTrue(entries.contains(Entry("include", "sharedpref", "amiberry_prefs.xml")))
		assertTrue(entries.containsAll(transientConfigExcludes))
		assertNoRootMarkersIncluded(entries)
		assertNoMediaDirectoriesIncluded(entries)
	}

	@Test
	fun `android twelve device transfer includes user metadata and excludes transient emulator state`() {
		val entries = entriesFrom("data_extraction_rules.xml", sectionName = "device-transfer")

		assertTrue(entries.containsAll(userExternalIncludes))
		assertTrue(entries.contains(Entry("include", "sharedpref", "amiberry_prefs.xml")))
		assertTrue(entries.containsAll(transientConfigExcludes))
		assertNoRootMarkersIncluded(entries)
		assertNoMediaDirectoriesIncluded(entries)
	}

	private fun assertNoRootMarkersIncluded(entries: Set<Entry>) {
		val includedExternalPaths = entries
			.filter { it.tag == "include" && it.domain == "external" }
			.map { it.path }
			.toSet()

		setOf(".clean_exit", ".emulator_session", ".last_launch.json").forEach { path ->
			assertFalse("Root marker should not be backed up: $path", path in includedExternalPaths)
		}
	}

	private fun assertNoMediaDirectoriesIncluded(entries: Set<Entry>) {
		val includedExternalPaths = entries
			.filter { it.tag == "include" && it.domain == "external" }
			.map { it.path }
			.toSet()

		mediaDirectories.forEach { path ->
			assertFalse("Media directory should not be backed up: $path", path in includedExternalPaths)
		}
	}

	private fun entriesFrom(fileName: String, sectionName: String? = null): Set<Entry> {
		val document = DocumentBuilderFactory.newInstance()
			.newDocumentBuilder()
			.parse(File("src/main/res/xml/$fileName"))
		val parent = if (sectionName == null) {
			document.documentElement
		} else {
			document.documentElement.directChildren(sectionName).single()
		}

		return parent.directChildren("include").map { it.toEntry("include") }.toSet() +
			parent.directChildren("exclude").map { it.toEntry("exclude") }
	}

	private fun Element.directChildren(tagName: String): List<Element> =
		(0 until childNodes.length)
			.map { childNodes.item(it) }
			.filterIsInstance<Element>()
			.filter { it.tagName == tagName }

	private fun Element.toEntry(tag: String): Entry =
		Entry(
			tag = tag,
			domain = getAttribute("domain"),
			path = getAttribute("path")
		)

	private data class Entry(
		val tag: String,
		val domain: String,
		val path: String
	)
}
