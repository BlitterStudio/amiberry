package com.blitterstudio.amiberry.ui.components

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * When a folder pick — a destination the launcher writes into — needs the
 * write storage permission: below Android 11 the picker asks for it beside
 * the read one (the app's minSdk is 29, with requestLegacyExternalStorage,
 * so the permissions reach the whole shared storage); Android 11 and up
 * have All Files Access instead.
 */
class FolderPickWritePermissionTest {

	@Test
	fun `a folder pick needs the write permission below Android 11`() {
		assertTrue(legacyFolderPickNeedsWrite(sdkInt = 29))
		assertTrue(legacyFolderPickNeedsWrite(sdkInt = 30 - 1))
	}

	@Test
	fun `Android 11 and up write through All Files Access instead`() {
		assertFalse(legacyFolderPickNeedsWrite(sdkInt = 30))
		assertFalse(legacyFolderPickNeedsWrite(sdkInt = 36))
	}
}
