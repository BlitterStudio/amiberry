package com.blitterstudio.amiberry.ui.navigation

private val topLevelRoutes = Screen.bottomNavItems.map { it.route }.toSet()

internal fun shouldMoveTaskToBack(
	route: String?,
	crashDialogVisible: Boolean = false,
	assetExtractionFailed: Boolean = false
): Boolean {
	if (crashDialogVisible || assetExtractionFailed) return false
	return route in topLevelRoutes
}
