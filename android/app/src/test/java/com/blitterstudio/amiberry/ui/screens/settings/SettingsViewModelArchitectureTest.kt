package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class SettingsViewModelArchitectureTest {

	private val source =
		File("src/main/java/com/blitterstudio/amiberry/ui/viewmodel/SettingsViewModel.kt").readText()

	@Test
	fun `tracks the open config name and description`() {
		assertTrue(source.contains("var currentConfigName"))
		assertTrue(source.contains("var currentConfigDescription"))
	}

	@Test
	fun `exposes dirty state derived from a baseline snapshot`() {
		assertTrue(source.contains("baselineSettings"))
		assertTrue(source.contains("baselineUnknownLines"))
		assertTrue(
			Regex("""val isDirty[\s\S]*currentConfigName != null[\s\S]*settings != baselineSettings""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `loadConfig takes the config name`() {
		assertTrue(
			Regex("""fun loadConfig\(\s*parsed: ConfigParser\.ParsedConfig,\s*name: String""")
				.containsMatchIn(source)
		)
	}

	@Test
	fun `exposes save and discard operations`() {
		assertTrue(source.contains("suspend fun saveTracked()"))
		assertTrue(source.contains("suspend fun saveAs("))
		assertTrue(source.contains("fun discardChanges()"))
		assertTrue(source.contains("configRepository.saveResolved("))
	}

	@Test
	fun `tracks the exact config path and overwrites it on save`() {
		assertTrue(source.contains("var currentConfigPath"))
		assertTrue(source.contains("configRepository.overwriteConfigAtPath("))
		assertTrue(Regex("""fun loadConfig\([\s\S]*name: String,\s*path: String""").containsMatchIn(source))
	}

	@Test
	fun `shader catalog starts with built ins and explicit loading state`() {
		assertTrue(
			source.contains(
				"var shaderCatalogEntries by mutableStateOf(ShaderCatalog.BUILT_INS)"
			)
		)
		assertTrue(source.contains("enum class ShaderCatalogStatus"))
		assertTrue(
			source.contains(
				"var shaderCatalogStatus by mutableStateOf(ShaderCatalogStatus.NOT_LOADED)"
			)
		)
	}

	@Test
	fun `shader catalog refresh resolves and scans away from the main thread`() {
		assertTrue(source.contains("fun refreshShaderCatalog()"))
		assertTrue(source.contains("shaderCatalogStatus = ShaderCatalogStatus.LOADING"))
		assertTrue(
			Regex(
				"""withContext\(Dispatchers\.IO\)\s*\{[\s\S]*ShaderCatalog\.resolveRoot\([\s\S]*ShaderCatalog\.scan\("""
			).containsMatchIn(source)
		)
		assertTrue(source.contains("File(application.filesDir, \"amiberry.conf\")"))
		assertTrue(source.contains("application.getExternalFilesDir(null)"))
	}

	@Test
	fun `only the latest shader catalog refresh publishes its result`() {
		assertTrue(source.contains("private var shaderCatalogRefreshGeneration = 0"))
		assertTrue(source.contains("val refreshGeneration = ++shaderCatalogRefreshGeneration"))
		assertTrue(
			source.contains(
				"if (refreshGeneration != shaderCatalogRefreshGeneration) return@launch"
			)
		)
		val staleResultGuard = source.indexOf(
			"if (refreshGeneration != shaderCatalogRefreshGeneration) return@launch"
		)
		assertTrue(source.indexOf("shaderCatalogEntries = catalogEntries") > staleResultGuard)
		assertTrue(
			source.indexOf("shaderCatalogStatus = ShaderCatalogStatus.LOADED") > staleResultGuard
		)
	}

	@Test
	fun `shader catalog refresh does not rewrite configured settings`() {
		val refreshMethod = Regex(
			"""fun refreshShaderCatalog\(\)\s*\{([\s\S]*?)\n\t\}"""
		).find(source)?.groupValues?.get(1).orEmpty()

		assertTrue(refreshMethod.isNotEmpty())
		assertTrue(!refreshMethod.contains("settings ="))
		assertTrue(!refreshMethod.contains("copy(shader"))
	}
}
