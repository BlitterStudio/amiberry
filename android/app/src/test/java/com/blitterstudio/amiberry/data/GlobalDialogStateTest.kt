package com.blitterstudio.amiberry.data

import org.junit.Assert.assertEquals
import org.junit.Test

class GlobalDialogStateTest {

	@Test
	fun `setup failure has priority over crash recovery`() {
		assertEquals(
			GlobalDialogState.VisibleDialog.AssetExtractionFailure,
			GlobalDialogState.visibleDialog(
				crashDetected = true,
				assetExtractionFailed = true
			)
		)
	}

	@Test
	fun `crash recovery is visible only when setup is not blocking`() {
		assertEquals(
			GlobalDialogState.VisibleDialog.CrashRecovery,
			GlobalDialogState.visibleDialog(
				crashDetected = true,
				assetExtractionFailed = false
			)
		)
	}

	@Test
	fun `no global dialog is visible for a healthy app shell`() {
		assertEquals(
			GlobalDialogState.VisibleDialog.None,
			GlobalDialogState.visibleDialog(
				crashDetected = false,
				assetExtractionFailed = false
			)
		)
	}
}
