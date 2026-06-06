package com.blitterstudio.amiberry.data

object GlobalDialogState {

	enum class VisibleDialog {
		None,
		AssetExtractionFailure,
		CrashRecovery
	}

	fun visibleDialog(
		crashDetected: Boolean,
		assetExtractionFailed: Boolean
	): VisibleDialog =
		when {
			assetExtractionFailed -> VisibleDialog.AssetExtractionFailure
			crashDetected -> VisibleDialog.CrashRecovery
			else -> VisibleDialog.None
		}
}
