package com.blitterstudio.amiberry.ui.screens.settings

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class DisplayShaderSelectorArchitectureTest {

	private val source =
		File("src/main/java/com/blitterstudio/amiberry/ui/screens/settings/DisplayTab.kt").readText()

	@Test
	fun `refreshes the shader catalog on display entry and lifecycle resume`() {
		assertTrue(
			Regex(
				"""LaunchedEffect\(viewModel\)\s*\{\s*viewModel\.refreshShaderCatalog\(\)\s*\}"""
			).containsMatchIn(source)
		)
		assertTrue(source.contains("LocalLifecycleOwner.current"))
		assertTrue(source.contains("LifecycleEventObserver"))
		assertTrue(
			Regex(
				"""Lifecycle\.Event\.ON_RESUME[\s\S]*viewModel\.refreshShaderCatalog\(\)"""
			).containsMatchIn(source)
		)
		assertTrue(source.contains("lifecycleOwner.lifecycle.removeObserver(observer)"))
	}

	@Test
	fun `places native shader selector after integer scaling in the first display card`() {
		val integerScaling = source.indexOf("R.string.settings_display_integer_scaling")
		val shaderLabel = source.indexOf("R.string.settings_display_shader_label")
		val appTheme = source.indexOf("R.string.settings_display_app_theme")

		assertTrue(integerScaling >= 0)
		assertTrue(shaderLabel > integerScaling)
		assertTrue(appTheme > shaderLabel)
	}

	@Test
	fun `shader selector updates only the configured shader`() {
		assertTrue(
			source.contains(
				"viewModel.updateSettings { s -> s.copy(shader = shader) }"
			)
		)
	}

	@Test
	fun `marks unavailable selections only after loading completes`() {
		assertTrue(
			Regex(
				"""val shaderUnavailable =\s*viewModel\.isShaderCatalogLoaded\s*&&\s*!viewModel\.isShaderCatalogLoading\s*&&[\s\S]*settings\.shader !in viewModel\.shaderCatalogEntries"""
			).containsMatchIn(source)
		)
		assertTrue(source.contains("R.string.settings_display_shader_unavailable_value"))
		assertTrue(source.contains("R.string.settings_display_shader_unavailable_help"))
	}

	@Test
	fun `keeps missing shader as a disabled presentation row after catalog entries`() {
		val catalogEntries = source.indexOf("viewModel.shaderCatalogEntries.forEach { shader ->")
		val unavailableBranch = source.indexOf("if (shaderUnavailable)", catalogEntries)
		val disabledRow = source.indexOf("enabled = false", unavailableBranch)

		assertTrue(catalogEntries >= 0)
		assertTrue(unavailableBranch > catalogEntries)
		assertTrue(disabledRow > unavailableBranch)
	}

	@Test
	fun `shader field remains responsive and exposes label and supporting text`() {
		assertTrue(source.contains("R.string.settings_display_shader_label"))
		assertTrue(source.contains("R.string.settings_display_shader_help"))
		assertTrue(source.contains("supportingText ="))
		assertTrue(
			Regex(
				"""settings_display_shader_label[\s\S]*menuAnchor\([\s\S]*fillMaxWidth\(\)"""
			).containsMatchIn(source)
		)
	}
}
