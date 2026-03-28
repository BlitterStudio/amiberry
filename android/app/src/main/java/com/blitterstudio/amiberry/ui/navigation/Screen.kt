package com.blitterstudio.amiberry.ui.navigation

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.annotation.StringRes
import androidx.compose.ui.graphics.vector.ImageVector
import com.blitterstudio.amiberry.R

sealed class Screen(
	val route: String,
	@param:StringRes val titleRes: Int,
	val icon: ImageVector
) {
	data object QuickStart : Screen("quickstart", R.string.nav_home, Icons.Default.Home)
	data object Settings : Screen("settings", R.string.nav_settings, Icons.Default.Settings)
	data object FileManager : Screen("files", R.string.nav_files, Icons.Default.Folder)
	data object Configurations : Screen("configs", R.string.nav_configs, Icons.Default.Save)
	data object About : Screen("about", R.string.nav_about, Icons.Default.Info)

	companion object {
		val bottomNavItems = listOf(QuickStart, Settings, FileManager, Configurations)
	}
}
