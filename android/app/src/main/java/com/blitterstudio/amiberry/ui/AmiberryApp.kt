package com.blitterstudio.amiberry.ui

import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.blitterstudio.amiberry.ui.navigation.Screen
import com.blitterstudio.amiberry.ui.screens.ConfigurationsScreen
import com.blitterstudio.amiberry.ui.screens.FileManagerScreen
import com.blitterstudio.amiberry.ui.screens.QuickStartScreen
import com.blitterstudio.amiberry.ui.screens.settings.SettingsScreen

@Composable
fun AmiberryApp() {
	val navController = rememberNavController()

	Scaffold(
		bottomBar = {
			NavigationBar(modifier = Modifier.focusGroup()) {
				val navBackStackEntry by navController.currentBackStackEntryAsState()
				val currentDestination = navBackStackEntry?.destination

				Screen.bottomNavItems.forEach { screen ->
					NavigationBarItem(
						icon = { Icon(screen.icon, contentDescription = screen.title) },
						label = { Text(screen.title) },
						selected = currentDestination?.hierarchy?.any { it.route == screen.route } == true,
						onClick = {
							navController.navigate(screen.route) {
								popUpTo(navController.graph.findStartDestination().id) {
									saveState = true
								}
								launchSingleTop = true
								restoreState = true
							}
						}
					)
				}
			}
		}
	) { innerPadding ->
		NavHost(
			navController = navController,
			startDestination = Screen.QuickStart.route,
			modifier = Modifier.padding(innerPadding)
		) {
			composable(Screen.QuickStart.route) {
				QuickStartScreen()
			}
			composable(Screen.Settings.route) {
				SettingsScreen()
			}
			composable(Screen.FileManager.route) {
				FileManagerScreen()
			}
			composable(Screen.Configurations.route) {
				ConfigurationsScreen()
			}
		}
	}
}
