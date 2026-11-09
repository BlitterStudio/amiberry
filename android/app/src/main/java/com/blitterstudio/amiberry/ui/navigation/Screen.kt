package com.blitterstudio.amiberry.ui.navigation

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

/**
 * The launcher's destinations: the Library the app opens to, the machine editor it opens
 * from the Library, and About from the Library's overflow menu.
 */
sealed class Screen(
	val route: String,
	@param:StringRes val titleRes: Int
) {
	data object Library : Screen("library", R.string.library_title)
	data object Settings : Screen("settings", R.string.nav_settings)
	data object About : Screen("about", R.string.nav_about)

	companion object {
		/** The destinations the shell owns: Back on one moves the task to the back. */
		val topLevel = listOf(Library)
	}
}
