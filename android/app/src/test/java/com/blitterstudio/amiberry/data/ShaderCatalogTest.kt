package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeNoException
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.nio.file.Files

class ShaderCatalogTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	@Test
	fun catalogStartsWithNativeBuiltInsInExactOrder() {
		assertEquals(
			listOf("none", "tv", "pc", "lite", "1084"),
			ShaderCatalog.scan(null)
		)
	}

	@Test
	fun catalogDiscoversNativeExtensionsAndSortsExternalEntries() {
		val root = tempDir.newFolder("Shaders")
		shaderFile(root, "z.glslp")
		shaderFile(root, "root.glsl")
		shaderFile(root, "UPPER.GLSLP")
		shaderFile(root, "notes.txt")
		val nested = File(root, "crt/deep").also { assertTrue(it.mkdirs()) }
		shaderFile(nested, "nested.glslp")
		shaderFile(nested, "nested.glsl")

		assertEquals(
			listOf(
				"none", "tv", "pc", "lite", "1084",
				"crt/deep/nested.glslp", "root.glsl", "z.glslp"
			),
			ShaderCatalog.scan(root)
		)
	}

	@Test
	fun catalogEmitsInvariantSeparatorsAndRemovesDuplicateDiscoveries() {
		val root = tempDir.newFolder("DuplicateShaders")
		val nested = File(root, "presets/crt").also { assertTrue(it.mkdirs()) }
		val preset = shaderFile(nested, "soft.glslp")

		val catalog = ShaderCatalog.scan(root) { directory ->
			if (directory == nested) listOf(preset, preset) else directory.listFiles()?.toList()
		}

		assertEquals(1, catalog.count { it == "presets/crt/soft.glslp" })
		assertFalse(catalog.any { it.contains('\\') })
	}

	@Test
	fun catalogReturnsBuiltInsForUnavailableRootsWithoutThrowing() {
		val missing = File(tempDir.root, "missing")
		val regularFile = tempDir.newFile("not-a-directory")
		val builtIns = listOf("none", "tv", "pc", "lite", "1084")

		assertEquals(builtIns, ShaderCatalog.scan(null))
		assertEquals(builtIns, ShaderCatalog.scan(missing))
		assertEquals(builtIns, ShaderCatalog.scan(regularFile))
		assertEquals(builtIns, ShaderCatalog.scan(tempDir.root) { null })
	}

	@Test
	fun catalogPreservesDiscoveriesMadeBeforeNestedTraversalFailure() {
		val root = tempDir.newFolder("PartialShaders")
		val topLevel = shaderFile(root, "usable.glsl")
		val nested = File(root, "blocked").also { assertTrue(it.mkdir()) }

		val catalog = ShaderCatalog.scan(root) { directory ->
			when (directory) {
				root -> listOf(topLevel, nested)
				nested -> throw SecurityException("blocked")
				else -> emptyList()
			}
		}

		assertTrue("usable.glsl" in catalog)
	}

	@Test
	fun catalogDoesNotFollowDirectorySymlinks() {
		val root = tempDir.newFolder("SymlinkShaders")
		val outside = tempDir.newFolder("OutsideShaders")
		shaderFile(outside, "outside.glslp")
		val link = File(root, "linked")
		try {
			Files.createSymbolicLink(link.toPath(), outside.toPath())
		} catch (exception: Exception) {
			assumeNoException("Directory symlinks are unavailable on this host", exception)
			return
		}

		assertFalse("linked/outside.glslp" in ShaderCatalog.scan(root))
	}

	@Test
	fun settingsFileResolverUsesNativeAndroidPrecedence() {
		val internalRoot = tempDir.newFolder("internal")
		val externalRoot = tempDir.newFolder("external")
		val legacyConfRoot = File(externalRoot, "conf").also { assertTrue(it.mkdir()) }
		val internalSettings = File(internalRoot, "amiberry.conf").also { it.writeText("internal") }
		val externalSettings = File(externalRoot, "amiberry.conf").also { it.writeText("external") }
		val legacyConfSettings = File(legacyConfRoot, "amiberry.conf").also { it.writeText("legacy") }

		assertEquals(
			internalSettings,
			ShaderCatalog.findSettingsFile(internalRoot, externalRoot)
		)
		assertTrue(internalSettings.delete())
		assertEquals(
			externalSettings,
			ShaderCatalog.findSettingsFile(internalRoot, externalRoot)
		)
		assertTrue(externalSettings.delete())
		assertEquals(
			legacyConfSettings,
			ShaderCatalog.findSettingsFile(internalRoot, externalRoot)
		)
	}

	@Test
	fun settingsFileResolverIgnoresNonFilesAndMissingExternalStorage() {
		val internalRoot = tempDir.newFolder("internal-non-file")
		val externalRoot = tempDir.newFolder("external-fallback")
		val externalSettings = File(externalRoot, "amiberry.conf").also { it.writeText("external") }
		assertTrue(File(internalRoot, "amiberry.conf").mkdir())

		assertEquals(
			externalSettings,
			ShaderCatalog.findSettingsFile(internalRoot, externalRoot)
		)
		assertNull(ShaderCatalog.findSettingsFile(internalRoot, null))
	}

	@Test
	fun resolverPrefersExplicitShaderPathAndUsesLastValue() {
		val settings = tempDir.newFile("amiberry.conf")
		settings.writeText("""
			shaders_path=/old/Shaders
			base_content_path=/content
			shaders_path = /new/Shaders
		""".trimIndent())

		assertEquals(File("/new/Shaders"), ShaderCatalog.resolveRoot(settings, tempDir.root))
	}

	@Test
	fun resolverSkipsLegacyDefaultShaderPathLikeNativeBootstrap() {
		val settings = tempDir.newFile("legacy-default.conf")
		val legacyRoot = File(
			File(tempDir.root, "Configurations"),
			"Shaders"
		)
		settings.writeText("shaders_path=${legacyRoot.path}")

		assertEquals(
			File(tempDir.root, "Visuals/Shaders"),
			ShaderCatalog.resolveRoot(settings, tempDir.root)
		)
	}

	@Test
	fun resolverSkipsLegacyBaseContentShaderPathLikeNativeBootstrap() {
		val settings = tempDir.newFile("legacy-base.conf")
		val contentRoot = File(tempDir.root, "content")
		settings.writeText("""
			base_content_path=${contentRoot.path}
			shaders_path=${File(contentRoot, "Configurations/Shaders").path}
		""".trimIndent())

		assertEquals(
			File(contentRoot, "Visuals/Shaders"),
			ShaderCatalog.resolveRoot(settings, tempDir.root)
		)
	}

	@Test
	fun resolverMigratesLegacyVisualPathFromInferredContentRoot() {
		val settings = tempDir.newFile("legacy-inferred.conf")
		val inferredRoot = File(tempDir.root, "inferred")
		settings.writeText("""
			config_path=${File(inferredRoot, "Configurations").path}
			rom_path=${File(inferredRoot, "ROMs").path}
			floppy_path=${File(inferredRoot, "Floppies").path}
			shaders_path=${File(inferredRoot, "Configurations/Shaders").path}
		""".trimIndent())

		assertEquals(
			File(inferredRoot, "Visuals/Shaders"),
			ShaderCatalog.resolveRoot(settings, tempDir.root)
		)
	}

	@Test
	fun resolverSkipsSerializedShaderPathFromSupersededContentRoot() {
		val settings = tempDir.newFile("superseded-base.conf")
		val activeRoot = File(tempDir.root, "active")
		val serializedRoot = File(tempDir.root, "serialized")
		settings.writeText("""
			base_content_path=${activeRoot.path}
			config_path=${File(serializedRoot, "Configurations").path}
			rom_path=${File(serializedRoot, "ROMs").path}
			floppy_path=${File(serializedRoot, "Floppies").path}
			shaders_path=${File(serializedRoot, "Visuals/Shaders").path}
		""".trimIndent())

		assertEquals(
			File(activeRoot, "Visuals/Shaders"),
			ShaderCatalog.resolveRoot(settings, tempDir.root)
		)
	}

	@Test
	fun resolverKeepsEarlierCustomOverrideWhenLaterLegacyLineIsSkipped() {
		val settings = tempDir.newFile("custom-then-legacy.conf")
		val customRoot = File(tempDir.root, "custom-shaders")
		val legacyRoot = File(tempDir.root, "Configurations/Shaders")
		settings.writeText("""
			shaders_path=${customRoot.path}
			shaders_path=${legacyRoot.path}
		""".trimIndent())

		assertEquals(customRoot, ShaderCatalog.resolveRoot(settings, tempDir.root))
	}

	@Test
	fun resolverDerivesShaderPathFromLastNonEmptyBaseContentPath() {
		val settings = tempDir.newFile("base.conf")
		settings.writeText("""
			base_content_path=/old
			base_content_path=/active
		""".trimIndent())

		assertEquals(
			File(File("/active"), "Visuals/Shaders"),
			ShaderCatalog.resolveRoot(settings, tempDir.root)
		)
	}

	@Test
	fun resolverTreatsPresentEmptyShaderPathAsBuiltInsOnly() {
		val settings = tempDir.newFile("empty-shader.conf")
		settings.writeText("""
			base_content_path=/content
			shaders_path=
		""".trimIndent())

		assertNull(ShaderCatalog.resolveRoot(settings, tempDir.root))
	}

	@Test
	fun resolverFallsBackToExternalVisualsShadersForAbsentOrEmptyBasePath() {
		val emptyBase = tempDir.newFile("empty-base.conf")
		emptyBase.writeText("base_content_path=")
		val expected = File(tempDir.root, "Visuals/Shaders")

		assertEquals(expected, ShaderCatalog.resolveRoot(null, tempDir.root))
		assertEquals(expected, ShaderCatalog.resolveRoot(File(tempDir.root, "missing.conf"), tempDir.root))
		assertEquals(expected, ShaderCatalog.resolveRoot(emptyBase, tempDir.root))
	}

	@Test
	fun resolverHandlesMalformedAndUnreadableSettingsSafely() {
		val malformed = tempDir.newFile("malformed.conf")
		malformed.writeText("""
			this is not an option
			; shaders_path=/commented
			SHADERS_PATH=/wrong-case
		""".trimIndent())
		val unreadable = tempDir.newFolder("settings-directory")
		val expected = File(tempDir.root, "Visuals/Shaders")

		assertEquals(expected, ShaderCatalog.resolveRoot(malformed, tempDir.root))
		assertEquals(expected, ShaderCatalog.resolveRoot(unreadable, tempDir.root))
	}

	@Test
	fun resolverDoesNotCreateRelativeFallbackWithoutExternalStorage() {
		assertNull(ShaderCatalog.resolveRoot(null, null))
	}

	@Test
	fun kotlinCatalogContractIsPinnedToNativeSource() {
		val nativeSource = File("../../src/osdep/imgui/shader_catalog.cpp").readText()
		val nativeBuiltIns = Regex(
			"""builtin_shaders\[\]\s*=\s*\{([^}]+)}"""
		).find(nativeSource)?.groupValues?.get(1)
			?.let { initializer ->
				Regex(""""([^"]+)"""").findAll(initializer).map { it.groupValues[1] }.toList()
			}
			?: error("Could not find native built-in shader catalog")

		assertEquals(ShaderCatalog.BUILT_INS, nativeBuiltIns)
		assertTrue(nativeSource.contains("recursive_directory_iterator"))
		assertTrue(nativeSource.contains("filename.substr(filename.size() - 6) == \".glslp\""))
		assertTrue(nativeSource.contains("filename.substr(filename.size() - 5) == \".glsl\""))
		assertTrue(nativeSource.contains("is_glsl && !relative_path.parent_path().empty()"))
		assertTrue(nativeSource.contains("relative_path.generic_string()"))
		assertTrue(nativeSource.contains("std::sort(shader_names.begin() + std::size(builtin_shaders)"))
		assertTrue(nativeSource.contains("std::unique(shader_names.begin(), shader_names.end())"))
	}

	@Test
	fun kotlinResolverContractIsPinnedToNativeBootstrapSource() {
		val nativeSource = File("../../src/osdep/amiberry.cpp").readText()

		assertTrue(nativeSource.contains("infer_serialized_base_content_path(managed_path_lines)"))
		assertTrue(nativeSource.contains("get_legacy_default_visual_asset_paths(g_portable_mode)"))
		assertTrue(nativeSource.contains("get_legacy_base_content_path_set(configured_base_path)"))
		assertTrue(nativeSource.contains("get_legacy_base_content_path_set(inferred_serialized_base_path)"))
		assertTrue(nativeSource.contains("managed_path_line_matches_visual_paths"))
		assertTrue(nativeSource.contains("skip_legacy_visual_line"))
	}

	private fun shaderFile(directory: File, name: String): File =
		File(directory, name).also { it.writeText("shader") }
}
