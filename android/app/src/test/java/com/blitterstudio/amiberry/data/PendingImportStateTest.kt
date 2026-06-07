package com.blitterstudio.amiberry.data

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class PendingImportStateTest {

	@Test
	fun `pending import waits for assets to be ready`() {
		assertFalse(
			PendingImportState.shouldProcess(
				hasPendingUri = true,
				assetsReady = false,
				assetExtractionFailed = false,
				importLaunchInProgress = false
			)
		)
	}

	@Test
	fun `pending import waits while asset extraction failure is visible`() {
		assertFalse(
			PendingImportState.shouldProcess(
				hasPendingUri = true,
				assetsReady = true,
				assetExtractionFailed = true,
				importLaunchInProgress = false
			)
		)
	}

	@Test
	fun `pending import waits while crash recovery is visible`() {
		assertFalse(
			PendingImportState.shouldProcess(
				hasPendingUri = true,
				assetsReady = true,
				assetExtractionFailed = false,
				crashRecoveryVisible = true,
				importLaunchInProgress = false
			)
		)
	}

	@Test
	fun `pending import waits while another direct import launch is in progress`() {
		assertFalse(
			PendingImportState.shouldProcess(
				hasPendingUri = true,
				assetsReady = true,
				assetExtractionFailed = false,
				importLaunchInProgress = true
			)
		)
	}

	@Test
	fun `pending import processes when assets are ready`() {
		assertTrue(
			PendingImportState.shouldProcess(
				hasPendingUri = true,
				assetsReady = true,
				assetExtractionFailed = false,
				importLaunchInProgress = false
			)
		)
	}

	@Test
	fun `pending import ignores empty state`() {
		assertFalse(
			PendingImportState.shouldProcess(
				hasPendingUri = false,
				assetsReady = true,
				assetExtractionFailed = false,
				importLaunchInProgress = false
			)
		)
	}
}
