package com.blitterstudio.amiberry.ui

import android.content.res.Configuration
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalConfiguration

/**
 * The launcher window's width by the Material 3 window size class breakpoints
 * (600 dp and 840 dp), which decides where the screens put panes side by side.
 */
enum class WindowWidthClass {
	Compact,
	Medium,
	Expanded;

	/** Room for a narrow list beside the content: Settings' tabs, the Files categories. */
	val showsSideList: Boolean get() = this != Compact

	/** Room for two full panes: list and detail (Configurations), two columns (Quick Start). */
	val showsTwoPanes: Boolean get() = this == Expanded

	companion object {
		private const val MEDIUM_WIDTH_DP = 600
		private const val EXPANDED_WIDTH_DP = 840
		private const val MEDIUM_HEIGHT_DP = 480

		/**
		 * A window less than 480 dp tall is a phone held sideways: it keeps the phone
		 * layout, as panes side by side would leave each too short to use.
		 */
		fun of(widthDp: Int, heightDp: Int): WindowWidthClass = when {
			heightDp < MEDIUM_HEIGHT_DP || widthDp < MEDIUM_WIDTH_DP -> Compact
			widthDp < EXPANDED_WIDTH_DP -> Medium
			else -> Expanded
		}

		/**
		 * The navigation rail replaces the bottom bar in landscape (as before) and in any
		 * window of medium width or more, such as a tablet held upright.
		 */
		fun usesNavigationRail(widthDp: Int, landscape: Boolean): Boolean =
			landscape || widthDp >= MEDIUM_WIDTH_DP
	}
}

/** The current window's width class; recomposes when the window is resized or rotated. */
@Composable
fun windowWidthClass(): WindowWidthClass {
	val configuration = LocalConfiguration.current
	return WindowWidthClass.of(configuration.screenWidthDp, configuration.screenHeightDp)
}

/** Whether the app shell shows the navigation rail instead of the bottom bar. */
@Composable
fun usesNavigationRail(): Boolean {
	val configuration = LocalConfiguration.current
	return WindowWidthClass.usesNavigationRail(
		widthDp = configuration.screenWidthDp,
		landscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
	)
}
