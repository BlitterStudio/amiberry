package com.blitterstudio.amiberry.ui.navigation

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.compose.ui.graphics.vector.ImageVector

sealed class Screen(
	val route: String,
	val title: String,
	val icon: ImageVector
) {
	data object Home : Screen("home", "Home", Icons.Default.Home)
	data object QuickStart : Screen("quickstart", "Quick Start", Icons.Default.PlayArrow)
	data object Settings : Screen("settings", "Settings", Icons.Default.Settings)
	data object FileManager : Screen("files", "Files", Icons.Default.Folder)
	data object Configurations : Screen("configs", "Configs", Icons.Default.Save)

	companion object {
		val bottomNavItems = listOf(Home, QuickStart, Settings, FileManager, Configurations)
	}
}
