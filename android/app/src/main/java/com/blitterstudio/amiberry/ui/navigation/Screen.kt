package com.blitterstudio.amiberry.ui.navigation

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

/**
 * The launcher's destinations: the Library the app opens to, the machine editor it opens
 * from the Library, and App Settings (with About & Licences) from the Library's gear.
 */
sealed class Screen(
	val route: String,
	@param:StringRes val titleRes: Int
) {
	data object Library : Screen("library", R.string.library_title)
	data object Settings : Screen("settings", R.string.nav_settings)
	data object AppSettings : Screen("app-settings", R.string.appsettings_title)

	companion object {
		/** The destinations the shell owns: Back on one moves the task to the back. */
		val topLevel = listOf(Library)
	}
}
