package com.blitterstudio.amiberry.data

object PendingImportState {
	fun shouldProcess(
		hasPendingUri: Boolean,
		assetsReady: Boolean,
		assetExtractionFailed: Boolean,
		importLaunchInProgress: Boolean
	): Boolean = hasPendingUri && assetsReady && !assetExtractionFailed && !importLaunchInProgress
}
