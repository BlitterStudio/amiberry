package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.ConfigInfo
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Test

class ConfigurationsSearchStateTest {

	@Test
	fun `configuration search is shown only for larger config lists`() {
		assertEquals(false, shouldShowConfigurationSearch(configCount = 5))
		assertEquals(true, shouldShowConfigurationSearch(configCount = 6))
	}

	@Test
	fun `blank query and all filter return the original list`() {
		val configs = listOf(config("Workbench"), config("Lotus"))

		assertSame(configs, filterConfigurations(configs, "", ConfigurationListFilter.All))
	}

	@Test
	fun `configuration search matches name description and path case insensitively`() {
		val configs = listOf(
			config("Workbench", description = "Classic desktop", path = "/configs/workbench.uae"),
			config("Lotus", description = "Racing", path = "/configs/lotus.uae"),
			config("AGA Demo", description = "", path = "/configs/demos/aga.uae")
		)

		assertEquals(listOf(configs[0]), filterConfigurations(configs, "desktop", ConfigurationListFilter.All))
		assertEquals(listOf(configs[1]), filterConfigurations(configs, "LOTUS", ConfigurationListFilter.All))
		assertEquals(listOf(configs[2]), filterConfigurations(configs, "demos", ConfigurationListFilter.All))
	}

	@Test
	fun `configuration filters narrow by description and recent timestamp`() {
		val newest = 10_000_000L
		val configs = listOf(
			config("Recent described", description = "AGA", lastModified = newest),
			config("Recent plain", lastModified = newest - 1_000L),
			config("Old described", description = "OCS", lastModified = newest - EIGHT_DAYS)
		)

		assertEquals(
			listOf(configs[0], configs[2]),
			filterConfigurations(configs, "", ConfigurationListFilter.WithDescription)
		)
		assertEquals(
			listOf(configs[0], configs[1]),
			filterConfigurations(configs, "", ConfigurationListFilter.Recent)
		)
	}

	@Test
	fun `filter names round trip with a safe default`() {
		assertEquals(ConfigurationListFilter.All, ConfigurationListFilter.fromName("missing"))
		assertTrue(ConfigurationListFilter.entries.all { ConfigurationListFilter.fromName(it.name) == it })
	}

	private fun config(
		name: String,
		description: String = "",
		path: String = "/configs/$name.uae",
		lastModified: Long = 1
	): ConfigInfo = ConfigInfo(
		path = path,
		name = name,
		lastModified = lastModified,
		description = description
	)

	private companion object {
		private const val EIGHT_DAYS = 8L * 24L * 60L * 60L * 1000L
	}
}
