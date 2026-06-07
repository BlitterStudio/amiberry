package com.blitterstudio.amiberry.data

object PendingImportState {
	fun shouldProcess(
		hasPendingUri: Boolean,
		assetsReady: Boolean,
		assetExtractionFailed: Boolean,
		crashRecoveryVisible: Boolean = false,
		importLaunchInProgress: Boolean
	): Boolean = hasPendingUri &&
		assetsReady &&
		!assetExtractionFailed &&
		!crashRecoveryVisible &&
		!importLaunchInProgress
}
