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

	private fun shaderFile(directory: File, name: String): File =
		File(directory, name).also { it.writeText("shader") }
}
